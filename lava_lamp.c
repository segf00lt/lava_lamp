#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "basic.h"
#include "arena.h"
#include "str.h"
#include "context.h"
#include "os.h"


/* * * * * * * * * * *
 * macros
 */

#define TARGET_FPS 60
#define MIN_FPS 10
#define TARGET_DT ((float)1.0f/(float)TARGET_FPS)
#define MIN_DT ((float)1.0f/(float)MIN_FPS)
#define MAX_CIRCLES 2048
#define SCREEN_RECT ((Rectangle){ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() })
#define SCREEN_SIZE ((Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() })
#define SCREEN_TOP_LEFT ((Vector2){ 0, 0, })
#define SCREEN_TOP_RIGHT ((Vector2){ (float)GetScreenWidth(), 0, })
#define SCREEN_BOTTOM_RIGHT ((Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight(), })
#define SCREEN_BOTTOM_LEFT ((Vector2){ 0, (float)GetScreenHeight(), })

#define G ((float)50.0)
#define MASS_TO_RADIUS ((float)300.23)
#define FRICTION_TO_RADIUS ((float)2e-3)


/* * * * * * * * * * *
 * structs
 */

typedef struct Circle {
  Vector2 accel;
  Vector2 vel;
  f32     friction;
  f32     mass;

  Vector2 center;
  f32     radius;
  f32     softness;
  Color   color;

} Circle;

typedef struct GPU_circle {
  u16 center_x;
  u16 center_y;
  u16 radius;
  u16 softness;
  u16 color_x;
  u16 color_y;
  u16 color_z;
  u16 color_w;
} GPU_circle;

typedef struct Game {
  f32 dt;
  f32 shader_dt;
  b32 quit;

  Shader blob_shader;
  Circle circles_buf[MAX_CIRCLES];
  GPU_circle gpu_circles_buf[MAX_CIRCLES];
  Texture2D circles_tex;
  Texture2D white_tex;


  int shader_dt_loc;
  int circles_count;
  int circles_tex_loc;
  int circles_count_loc;

  b32 created_balls;

  bool paused;

  Arena *main_arena;
  Arena *frame_arena;

} Game;

STATIC_ASSERT(MB(1) >= sizeof(Game), game_state_struct_is_less_than_1_megabyte);

u64 game_state_size = MAX(MB(1), sizeof(Game));

/* * * * * * * * * * *
 * globals
 */

Matrix mat_rotate_pi_over_4;
b32 cached_mat_rotate_pi_over_4;


/* * * * * * * * * * *
 * function headers
 */

Game *game_init(void);
void game_close(Game *gp);
void game_update_and_draw(Game* gp);
void game_load_assets(Game* gp);
void game_unload_assets(Game* gp);

float get_random_float(float min, float max, int steps);


/* * * * * * * * * * *
 * function bodies
 */

force_inline float get_random_float(float min, float max, int steps) {
  int val = GetRandomValue(0, steps);
  float result = Remap((float)val, 0.0f, (float)steps, min, max);
  return result;
}

Game *game_init(void) {

  // raylib initialization
  SetTargetFPS(TARGET_FPS);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1000, 800, "Lava Lamp");
  InitAudioDevice();

  //SetMasterVolume(GetMasterVolume() * 0.5);

  SetTextLineSpacing(10);
  SetTraceLogLevel(LOG_DEBUG);
  SetExitKey(0);

  Game *gp = os_alloc(game_state_size);
  memory_set(gp, 0, game_state_size);

  game_load_assets(gp);

  Image circles_tex_img =
  {
    .data = gp->gpu_circles_buf,
    .width = ARRLEN(gp->gpu_circles_buf),
    .height = 1,
    .mipmaps = 1,
    .format = PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,
  };

  gp->circles_tex = LoadTextureFromImage(circles_tex_img);

  Image white_tex_img = GenImageColor(1, 1, WHITE);
  gp->white_tex = LoadTextureFromImage(white_tex_img);
  UnloadImage(white_tex_img);

  gp->main_arena = arena_alloc(.size = KB(20));
  gp->frame_arena = arena_alloc(.size = KB(4));

  //int screen_rect_loc = GetShaderLocation(blob_shader, "screen_rect");

  gp->circles_tex_loc = GetShaderLocation(gp->blob_shader, "circles_tex");
  gp->circles_count_loc = GetShaderLocation(gp->blob_shader, "circles_count");
  gp->shader_dt_loc = GetShaderLocation(gp->blob_shader, "dt");

  //Image white_img = GenImageColor(1, 1, WHITE);
  //Texture2D white_tex = LoadTextureFromImage(white_img);

  return gp;
}

void game_close(Game *gp) {
  game_unload_assets(gp);

  CloseWindow();
  CloseAudioDevice();

}

void game_load_assets(Game* gp) {
  gp->blob_shader = LoadShader("blob_vert.glsl", "blob_pixel.glsl");

}

void game_unload_assets(Game* gp) {

  UnloadShader(gp->blob_shader);
  //UnloadTexture(circles_tex);

}

Color color_from_hexcode(Str8 hexcode) {

  u8 components[4] = { 0, 0, 0, 0xff, };

  int i = 0; 
  if(hexcode.s[0] == '#') {
    i++;
  }
  for(int c = 0; i < hexcode.len; i += 2, c++) {
    u8 a = hexcode.s[i];
    u8 b = hexcode.s[i+1];

    components[c] = (hexdigit_to_int(a) << 4) + hexdigit_to_int(b);
  }

  Color result = { .r = components[0], .g = components[1], .b = components[2], .a = components[3] };

  return result;
}

void game_update_and_draw(Game* gp) {
  gp->dt = Clamp(GetFrameTime(), MIN_DT, TARGET_DT);
  gp->shader_dt += gp->dt;

  if(WindowShouldClose()) {
    gp->quit = 1;
    return;
  }

  if(!gp->created_balls) {
    gp->created_balls = 1;
    Circle circles[] = {
      { .color = color_from_hexcode(str8_lit("#f700ce")),
        .center = { SCREEN_SIZE.x*0.5, SCREEN_SIZE.y*0.4, }, .radius = 110, .softness = 10.f, },
      { .color = color_from_hexcode(str8_lit("#a400f7")),
        .center = { SCREEN_SIZE.x*0.654, SCREEN_SIZE.y*0.66, }, .radius = 150, .softness = 10.f, },
      { .color = color_from_hexcode(str8_lit("#f70052")),
        .center = { SCREEN_SIZE.x*0.83, SCREEN_SIZE.y*0.5, }, .radius = 85,  .softness = 10.f,  },
      //{ .color = GREEN , .center = {0 }, },
      //{ .color = YELLOW, .center = {0 }, },
    };

    gp->circles_count = ARRLEN(circles);

    Vector2 dir = {0, 1};

    for(int i = 0; i < gp->circles_count; i++) {
      Circle *c = &circles[i];

      c->vel = Vector2Scale(
          Vector2Rotate(dir, get_random_float(0, 2*PI, 30)),
          get_random_float(300, 600, 15) );

      c->friction = c->radius*FRICTION_TO_RADIUS;

      c->mass = c->radius*MASS_TO_RADIUS;

    }

    memory_copy(gp->circles_buf, circles, sizeof(circles));

  }

  // TODO make the balls get attracted towards the top and bottom of the screen, that way they'll keep moving
  { /* update balls */

    if(IsKeyPressed(KEY_F5)) {
      gp->created_balls = 0;
    }

    if(IsKeyPressed(KEY_ESCAPE)) {
      gp->paused = !gp->paused;
    }

    if(gp->paused) {
      goto update_end;
    }

    for(int i = 0; i < gp->circles_count; i++) {

      Circle *c = &gp->circles_buf[i];

      c->accel = (Vector2){0};
      for(int j = 0; j < gp->circles_count; j++) {
        if(j == i) continue;

        Circle other_c = gp->circles_buf[j];

        float r_sqr = fmaxf(1e-3, Vector2DistanceSqr(c->center, other_c.center));
        float inv_r_sqr = 1.0f/r_sqr;
        float inv_r = sqrtf(inv_r_sqr);
        Vector2 dir = Vector2Scale(Vector2Subtract(other_c.center, c->center), inv_r);
        Vector2 neg_dir = Vector2Negate(dir);
        float g = 2.2*log2(G*other_c.mass*inv_r_sqr*70.0);
        float neg_g = 11.4*log2(G*other_c.mass*inv_r_sqr*3e-1);
        c->accel = Vector2Add(c->accel, Vector2Scale(dir, g));
        c->accel = Vector2Add(c->accel, Vector2Scale(neg_dir, neg_g));

      }

      Vector2 a_X_dt = Vector2Scale(c->accel, gp->dt);
      c->vel = Vector2Add(c->vel, a_X_dt);

      c->vel = Vector2ClampValue(c->vel, 80, 1400);

      if(Vector2LengthSqr(c->vel) > SQUARE(27.0)) {
        c->vel = Vector2Subtract(c->vel, Vector2Scale(c->vel, c->friction*gp->dt));
      }
      Vector2 new_p = c->center;
      new_p = Vector2Add(new_p, Vector2Scale(c->vel, gp->dt));
      new_p = Vector2Add(new_p, Vector2Scale(a_X_dt, gp->dt*0.5));

      {

        float r = c->radius;
        new_p.x = fminf((float)GetScreenWidth() - r, fmaxf(r, new_p.x));
        new_p.y = fminf((float)GetScreenHeight() - r, fmaxf(r, new_p.y));

      }

      c->center = new_p;

      {
        Vector2 p = c->center;
        float r = c->radius;
        if(!(p.x > r && p.y > r && p.x < (float)GetScreenWidth() - r && p.y < (float)GetScreenHeight() - r)) {

          if(p.x == r || p.x == (float)GetScreenWidth() - r) {
            c->vel.x *= -1;
          }

          if(p.y == r || p.y == (float)GetScreenHeight() - r) {
            c->vel.y *= -1;
          }

          //c->vel = Vector2Negate(c->vel);
          //c->vel = Vector2Rotate(c->vel, get_random_float(-PI*0.05, PI*0.05, 10));
          //c->vel = Vector2Scale(c->vel, get_random_float(1.02, 1.1, 10));
        }
      }


    }

update_end:;
  } /* update balls */

  for(int i = 0; i < gp->circles_count; i++) {
    Circle c = gp->circles_buf[i];
    Vector4 color = ColorNormalize(c.color);
    gp->gpu_circles_buf[i].center_x = (u16)FloatToHalf(c.center.x);
    gp->gpu_circles_buf[i].center_y = (u16)FloatToHalf((float)GetScreenHeight()-c.center.y);
    gp->gpu_circles_buf[i].radius   = (u16)FloatToHalf(c.radius);
    gp->gpu_circles_buf[i].softness = (u16)FloatToHalf(c.softness);
    gp->gpu_circles_buf[i].color_x  = (u16)FloatToHalf(color.x);
    gp->gpu_circles_buf[i].color_y  = (u16)FloatToHalf(color.y);
    gp->gpu_circles_buf[i].color_z  = (u16)FloatToHalf(color.z);
    gp->gpu_circles_buf[i].color_w  = (u16)FloatToHalf(color.w);
  }

  UpdateTexture(gp->circles_tex, gp->gpu_circles_buf);

  deferloop((BeginDrawing(), ClearBackground(BLACK)), EndDrawing()) {

    deferloop(BeginShaderMode(gp->blob_shader), EndShaderMode()) {

      //Vector4 screen_rect =
      //{
      //  0, 0,
      //  GetScreenWidth(), GetScreenHeight(),
      //};

      //SetShaderValue(blob_shader, screen_rect_loc, &screen_rect, SHADER_UNIFORM_VEC4);
      SetShaderValueTexture(gp->blob_shader, gp->circles_tex_loc, gp->circles_tex);
      SetShaderValue(gp->blob_shader, gp->circles_count_loc, &(gp->circles_count), SHADER_UNIFORM_INT);
      SetShaderValue(gp->blob_shader, gp->shader_dt_loc, &(gp->shader_dt), SHADER_UNIFORM_FLOAT);

      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
      //{
      //  Rectangle src = { 0, 0, 1, 1 }; // use full texture
      //  Rectangle dst = { 0, 0, GetScreenWidth(), -(float)GetScreenHeight() };
      //  Vector2 origin = { 0, 0 };

      //  DrawTexturePro(gp->white_tex, src, dst, origin, 0.0f, WHITE);
      //}

    }

#if 0
    for(int i = 0; i < gp->circles_count; i++) {
      Circle *c = &gp->circles_buf[i];

      Str8 circle_info_text = push_str8f(gp->frame_arena,
          "scalar vel: %f\nmass: %f\nfriction: %f\n",
          Vector2Length(c->vel), c->mass, c->friction);
      Vector2 circle_info_text_offset =
      {
        .x = 0, .y = -1,
      };
      if(!cached_mat_rotate_pi_over_4) {
        cached_mat_rotate_pi_over_4 = true;
        mat_rotate_pi_over_4 = MatrixRotate((Vector3){.z = 1}, -PI*.25);
      }
      circle_info_text_offset = Vector2Transform(circle_info_text_offset, mat_rotate_pi_over_4);
      Vector2 circle_info_text_pos = Vector2Add(c->center, Vector2Scale(circle_info_text_offset, c->radius*1.14));

      DrawTextEx(GetFontDefault(), (char*)circle_info_text.s, circle_info_text_pos, 20, 2, WHITE);

      DrawCircleLinesV(c->center, c->radius, GREEN);

      Vector2 accel_line_end = Vector2Add(c->center,
          Vector2Scale(Vector2Normalize(c->accel), 100.0));

      DrawLineEx(c->center, accel_line_end, 2.0, WHITE);

    }
    DrawFPS(10, 10);
#endif

    arena_clear(gp->frame_arena);

  }


}
