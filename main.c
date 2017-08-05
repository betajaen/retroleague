#include "synthwave.h"
#include <math.h>
#include <stdio.h>

// #define IS_STREAMING 1

#define DB16_TRANSPARENT 0
#define DB16_VERY_DARK_VIOLET 1
#define DB16_SHADOWY_LAVENDER 2
#define DB16_FLINT 3
#define DB16_REGENT_GREY 4
#define DB16_PEPPERMINT 5
#define DB16_RED_EARTH 6
#define DB16_ROOT_BEER 7
#define DB16_FADED_RED 8
#define DB16_BRONZE 9
#define DB16_BIRTHDAY_SUIT 10
#define DB16_BANANA 11
#define DB16_DEEP_KOAMARU 12
#define DB16_INDIGO 13
#define DB16_CADET_BLUE 14
#define DB16_HUNTER_GREEN 15
#define DB16_LEAF 16

Surface SURFACE;
Scene   SCENE;
Canvas  CANVAS;
Bitmap  BITMAP;
Font    NEOSANS;
Sound   COIN;

#define TRI(V0_X, V0_Y, V0_Z, V1_X, V1_Y, V1_Z, V2_X, V2_Y, V2_Z, COLOUR) { .v[0] = {V0_X,V0_Y,V0_Z}, .v[1] = {V1_X,V1_Y,V1_Z}, .v[2] = {V2_X,V2_Y,V2_Z}, .shader = 0, .colour = COLOUR, .p0 = 0, .p1 = 0}
#define S 5

Triangle plane[2] = {
  TRI(-S,0,S,   S,0, S,   S,0,-S,   2)
};

#define B 0.5

Triangle box[12] = {
    TRI(
      -B,-B,-B,
      -B,-B, B,
      -B, B, B,
    1),
    
    TRI(
       B, B,-B, 
      -B,-B,-B,
      -B, B,-B, 
    2),

    TRI(
       B,-B, B,
      -B,-B,-B,
       B,-B,-B,
    3),

    TRI(
       B, B,-B,
       B,-B,-B,
      -B,-B,-B,
    4),

    TRI(
      -B,-B,-B,
      -B, B, B,
      -B, B,-B,
    5),

    TRI(
       B,-B, B,
      -B,-B, B,
      -B,-B,-B,
    6),

    TRI(
      -B, B, B,
      -B,-B, B,
       B,-B, B,
    7),
    
    TRI(
       B, B, B,
       B,-B,-B,
       B, B,-B,
    8),
    
    TRI(
       B,-B,-B,
       B, B, B,
       B,-B, B,
    9),
    TRI(
       B, B, B,
       B, B,-B,
      -B, B,-B,
    10),
    TRI(
       B, B, B,
      -B, B,-B,
      -B, B, B,
    11),
    TRI(
       B, B, B,
      -B, B, B,
       B,-B, B,
    12)
};

Mesh  planeMesh, boxMesh;
Vec3f camPos;
f32   camRot;
Bitmap TILESET;

#define CONTROL_FORWARD    1
#define CONTROL_BACKWARD   2
#define CONTROL_LEFT       3
#define CONTROL_RIGHT      4
#define CONTROL_TURN_LEFT  5
#define CONTROL_TURN_RIGHT 6
#define CONTROL_UP         7
#define CONTROL_DOWN       8
#define SOUND_TEST         9

void $Setup()
{

#if IS_STREAMING
  $.screenX = -1600;
  $.screenY = 120;
#endif

  $.displayScale = 3;

  planeMesh.triangles = &plane[0];
  planeMesh.nbTriangles = 2;
  boxMesh.triangles = &box[0];
  boxMesh.nbTriangles = 12;

  $.Mesh.Finalise(&planeMesh);
  $.Mesh.Finalise(&boxMesh);

  $.Input.BindControl(CONTROL_FORWARD,    $KEY_W);
  $.Input.BindControl(CONTROL_BACKWARD,   $KEY_S);
  $.Input.BindControl(CONTROL_LEFT,       $KEY_A);
  $.Input.BindControl(CONTROL_RIGHT,      $KEY_D);
  $.Input.BindControl(CONTROL_TURN_LEFT,  $KEY_Q);
  $.Input.BindControl(CONTROL_TURN_RIGHT, $KEY_E);
  $.Input.BindControl(CONTROL_UP,         $KEY_SPACE);
  $.Input.BindControl(CONTROL_DOWN,       $KEY_LCTRL);
  $.Input.BindControl(SOUND_TEST,         $KEY_C);
  
}

void $Start()
{
  $.Bitmap.Load(&TILESET, "tileset.png");
  $.Font.New(&NEOSANS, "NeoSans.png", $Rgb(0,0,255), $Rgb(255, 0, 255));
  $.Sound.New(&COIN, "coin.wav");
  $.Music.Play("dragon2.mod");

  $.Surface.New(&SURFACE);
  $.Scene.New(&SCENE);
  $.Canvas.New(&CANVAS);
  $Vec3_Set(&camPos, 2,0,-5);

  $.Net.Connect("localhost", 5000);
  char* n = "HELLO";
  $.Net.SendMessage(5, n);
}

const float delta = 1.0f / 60.0f;
const float moveSpeed = 10.0f;
const float turnSpeed = 45.0f;
f32 cubeRot = 0;
f32 height = 1.85f;

f32 constrainAngle(f32 x){
    x = fmodf(x, 360.0f);
    if (x < 0.0f)
        x += 360.0f;
    return x;
}

void $Update()
{
  
  float x = 0;
  float z = 0;


  if ($.Input.ControlDown(CONTROL_FORWARD))
    z += 4;
  else if ($.Input.ControlDown(CONTROL_BACKWARD))
    z -= 4;
  if ($.Input.ControlDown(CONTROL_LEFT))
    x -= 4;
  else if ($.Input.ControlDown(CONTROL_RIGHT))
    x += 4;

  if ($.Input.ControlDown(CONTROL_TURN_LEFT))
  {
    camRot -= turnSpeed * delta;
    camRot = constrainAngle(camRot);
  }
  else if ($.Input.ControlDown(CONTROL_TURN_RIGHT))
  {
    camRot += turnSpeed * delta;
    camRot = constrainAngle(camRot);
  }

  if ($.Input.ControlDown(CONTROL_UP))
    height += 1.0f * delta;
  else if ($.Input.ControlDown(CONTROL_DOWN))
    height -= 1.0f * delta;

  f32 d = 1.0f, c = cosf($Deg2Rad(camRot)), s = sinf($Deg2Rad(camRot));

  x *= delta;
  z *= delta;

  f32 mx = x * c - z * s;
  f32 mz = z * c + x * s;

  camPos.x += mx;
  camPos.z += mz;
  
  if ($.Net.PeekMessage())
  {
    u8* data = $TempNewA(u8, 10255);
    for(u32 ii=0;ii < 8;ii++)
    {
      u32 length = 0;
      printf("%i\n", ii);
      if ($.Net.RecvMessage(&length, 1025, data) == false)
        break;
      data[length] = '\0';
      printf("%s\n", data);
      if ($.Net.PeekMessage() == false)
        break;
    }
  }

}

void $Draw()
{
  f32 d = 1.0f, c = cosf($Deg2Rad(camRot)), s = sinf($Deg2Rad(camRot));
  
  f32 x = -s;
  f32 z = +c;
  
  if ($.Input.ControlReleased(SOUND_TEST))
  {
    $.Sound.Play(&COIN);
  }

  $.Scene.SetPovLookAtXyz(&SCENE, 
    camPos.x, height, camPos.z,  
    camPos.x + x, 1.85f, camPos.z + z);
  
  cubeRot = constrainAngle(cubeRot + 10.0f * delta);
  
  $.Scene.DrawSkybox(&SCENE, DB16_CADET_BLUE, DB16_LEAF);
  $.Scene.DrawMeshXyz(&SCENE, &boxMesh, cubeRot * 0.1f ,0.5,0,  (i32) cubeRot,(i32) cubeRot, (i32) cubeRot);
  $.Scene.DrawMeshXyz(&SCENE, &boxMesh, 1,0.5,0,  0,(i32) cubeRot, 0);
  $.Scene.DrawCustomShaderMeshXyz(&SCENE, &boxMesh, 1, 0,0.5,1,  0,(i32) cubeRot, 0);
  $.Scene.DrawCustomShaderMeshXyz(&SCENE, &boxMesh, 1, 0,1.5,0,  0,(i32) cubeRot, 0);
  $.Scene.Render(&SCENE, &SURFACE);

  $.Canvas.DrawTextF(&CANVAS, &NEOSANS, DB16_BANANA, 0, 200 - 9, "FPS %.1f", $.Stats.fps);
  $.Canvas.Render(&CANVAS, &SURFACE);

  $.Surface.Render(&SURFACE);
}
