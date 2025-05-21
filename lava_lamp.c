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
#define MAX_CIRCLES 2048


/* * * * * * * * * * *
 * structs
 */

typedef struct Circle {
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
  b32 quit;


  Shader blob_shader;
  Circle circles_buf[MAX_CIRCLES];
  GPU_circle gpu_circles_buf[MAX_CIRCLES];
  Texture2D circles_tex;

  int circles_count;
  int circles_tex_loc;
  int circles_count_loc;

} Game;

/* * * * * * * * * * *
 * globals
 */


/* * * * * * * * * * *
 * function headers
 */

Game *game_init(void);
void game_close(Game *gp);
void game_update_and_draw(Game* gp);
void game_load_assets(Game* gp);
void game_unload_assets(Game* gp);


/* * * * * * * * * * *
 * function bodies
 */

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

  Game *gp = os_alloc(sizeof(Game));
  memory_set(gp, 0, sizeof(Game));

  game_load_assets(gp);

  Circle circles[] = {
    { .color = ORANGE, .center = { 300, 500, }, .radius = 190, .softness = 10.f, },
    { .color = BLUE, .center = { 380, 770, }, .radius = 130, .softness = 10.f, },
    //{ .color = RED   , .center = {0 }, },
    //{ .color = GREEN , .center = {0 }, },
    //{ .color = YELLOW, .center = {0 }, },
  };

  memory_copy(gp->circles_buf, circles, sizeof(circles));

  gp->circles_count = ARRLEN(circles);
  //int circles_count = 1;

  for(int i = 0; i < gp->circles_count; i++) {
    Circle c = gp->circles_buf[i];
    Vector4 color = ColorNormalize(c.color);
    gp->gpu_circles_buf[i].center_x = (u16)FloatToHalf(c.center.x);
    gp->gpu_circles_buf[i].center_y = (u16)FloatToHalf(c.center.y);
    gp->gpu_circles_buf[i].radius   = (u16)FloatToHalf(c.radius);
    gp->gpu_circles_buf[i].softness = (u16)FloatToHalf(c.softness);
    gp->gpu_circles_buf[i].color_x  = (u16)FloatToHalf(color.x);
    gp->gpu_circles_buf[i].color_y  = (u16)FloatToHalf(color.y);
    gp->gpu_circles_buf[i].color_z  = (u16)FloatToHalf(color.z);
    gp->gpu_circles_buf[i].color_w  = (u16)FloatToHalf(color.w);
  }

  Image circles_tex_img =
  {
    .data = gp->gpu_circles_buf,
    .width = ARRLEN(gp->gpu_circles_buf),
    .height = 1,
    .mipmaps = 1,
    .format = PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,
  };

  gp->circles_tex = LoadTextureFromImage(circles_tex_img);

  //int screen_rect_loc = GetShaderLocation(blob_shader, "screen_rect");

  gp->circles_tex_loc = GetShaderLocation(gp->blob_shader, "circles_tex");
  gp->circles_count_loc = GetShaderLocation(gp->blob_shader, "circles_count");

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

void game_update_and_draw(Game* gp) {

  if(WindowShouldClose()) {
    gp->quit = 1;
    return;
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

      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
      //{
      //  Rectangle src = { 0, 0, 1, 1 }; // use full texture
      //  Rectangle dst = { 0, 0, GetScreenWidth(), GetScreenHeight() };
      //  Vector2 origin = { 0, 0 };

      //  DrawTexturePro(white_tex, src, dst, origin, 0.0f, WHITE);
      //}

    }

  }


}
