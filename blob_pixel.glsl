#version 330 core

in vec2 fragTexCoord;

out vec4 finalColor;

//uniform vec4 screen_rect;
uniform float dt;
uniform sampler2D circles_tex;
uniform int circles_count;

struct Circle {
  vec2 center;
  float radius;
  float softness;
  vec4 color;
};

Circle get_circle(int i) {
  vec4 a = texelFetch(circles_tex, ivec2(i * 2, 0), 0);
  vec4 b = texelFetch(circles_tex, ivec2(i * 2 + 1, 0), 0);

  Circle c;
  //c.center = (a.xy - screen_rect.xy) / screen_rect.zw;
  //c.radius = a.z / screen_rect.z;
  //c.softness = a.w / screen_rect.z;
  //c.color = b;

  c.center = a.xy;
  c.radius = a.z;
  c.softness = a.w;
  c.color = b;

  return c;
}

//vec4 sdf_s_min(vec4 a, vec4 b, float k) {
//    k *= 8.0;
//    float h = max( k-abs(a.x-b.x), 0.0 )/(2.0*k);
//    return vec4( min(a.x,b.x)-h*h*k,
//                 mix(a.yz,b.yz,(a.x<b.x)?h:1.0-h), min(a.w, b.w) );
///*
//    k *= 4.0;
//    float h = max( k-abs(a.x-b.x), 0.0 )/k;
//    float m = h*h*k*(1.0/4.0);
//    float n = h*(1.0/2.0);
//    return (a.x<b.x) ? vec3(a.x-m, mix(a.yz, b.yz, n) ): 
//                       vec3(b.x-m, mix(a.yz, b.yz, 1.0-n) );
//*/                       
//}

// cubic polynomial
//float smin( float a, float b, float k )
//{
//    k *= 6.0;
//    float h = max( k-abs(a-b), 0.0 )/k;
//    return min(a,b) - h*h*h*k*(1.0/6.0);
//}

// cubic polynomial
vec2 smin(float a, float b, float k) {
    float h = 1.0 - min( abs(a-b)/(6.0*k), 1.0 );
    float w = h*h*h;
    float m = w*0.5;
    float s = w*k; 
    return (a<b) ? vec2(a-s,m) : vec2(b-s,1.0-m);
}

float circle_sdf(vec2 p, Circle circle) {

  vec2 center = circle.center;
  float radius = circle.radius;
  float softness = circle.softness;

  float dist = distance(p, center) - radius;

  float t = smoothstep(radius + softness, radius - softness, dist);

  return t;
}

void main() {
  vec2 frag_coord = gl_FragCoord.xy;

  Circle c0 = get_circle(0);
  float softness = 1.8;
  //softness += (softness*0.4)*exp(sin(dt*1.6)+1);
  float k = 65.6;
  vec4 color = c0.color;
  float d = 1e9;

  for(int i = 0; i < circles_count; i++) {
    Circle c = get_circle(i);

    //float radius = c.radius + (c.radius*0.1)*(sin(dt*3.0)+1);
    float radius = c.radius;
    float di = length(frag_coord - c.center) - radius;
    vec2 result = smin(d, di, k);
    d = result.x;
    float blend = result.y;

    color  = mix(color, c.color, blend);

  }

  float alpha = 1.0 - smoothstep(0.0, softness, d);
  finalColor = vec4(color.rgb, alpha);

  //Circle c1 = get_circle(0);
  //Circle c2 = get_circle(1);

  //float d1 = length(frag_coord - c1.center) - c1.radius;
  //float d2 = length(frag_coord - c2.center) - c2.radius;

  //float d = smin(d1, d2, 13.3);
  //float softness = 15.0;
  //float alpha = 1.0 - smoothstep(0.0, softness, d);

  //vec2 dir = c2.center - c1.center;
  //vec3 color = mix(c1.color.rgb, c2.color.rgb, 0.7);

  //finalColor = vec4(vec3(0.4, 0.8, 0.2), alpha);
  //finalColor = vec4(color, alpha);

  //vec4 result = vec4(1.0);

  //for(int i = 0; i < circles_count; i++) {
  //  Circle c = get_circle(i);

  //  float a = circle_sdf(gl_FragCoord.xy, c.center, c.radius, c.softness);

  //  vec4 color = vec4(c.color.rgb * a * c.color.a, a * c.color.a);
  //  result = sdf_s_min(result, color, 0.05);

  //}

  //finalColor = result;

  /*
  vec4 result = vec4(0.0);
  float field = 0.0;

  for(int i = 0; i < circles_count; i++) {
    Circle c = get_circle(i);
    float dist = distance(gl_FragCoord.xy, c.center);
    float influence;
    //influence = c.radius / dist;
    influence = pow(c.radius, 2.0) / (dist*dist);
    field += influence;
    result += c.color * influence;
  }

  float threshold = 0.8;
  float mask = smoothstep(threshold - 0.04, threshold + 0.04, field);
  finalColor = vec4(result.rgb / field, 1.0) * mask;
  */

  //vec2 center = vec2(0.5, 0.5);
  //float radius = 0.25;
  //float softness = 0.01;
  //float alpha = circle_sdf(fragTexCoord, center, radius, softness);
  //finalColor = vec4(1.0, 0.0, 0.0, alpha);  // Red circle with alpha blend

  /*
  vec4 result = vec4(0.0);

  Circle c = get_circle(0);

  float a = circle_sdf(gl_FragCoord.xy, c.center, c.radius, c.softness);

  result.rgb += c.color.rgb * a * c.color.a;
  result.a += a * c.color.a;

  finalColor = result;
  */
  /*
     vec4 result = vec4(0.0);

     for(int i = 0; i < circles_count; i++) {
     Circle c = get_circle(i);

     float a = circle_sdf(fragTexCoord, c.center, c.radius, c.softness);

     result.rgb += c.color.rgb * a * c.color.a;
     result.a += a * c.color.a;

     }

     finalColor = result;
   */

}
