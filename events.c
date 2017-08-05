#include "funk.h"
#include <string.h>

void $Setup()
{
#if IS_STREAMING
  $.screenX = -1600;
  $.screenY = 120;
#endif

  DELTA = 1.0f / 60.0f;

  $.displayScale = 3;

  Mesh_MakePlayer(&MESH_PLAYER);
  Mesh_MakeBall(&MESH_BALL);

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
  $.Bitmap.Load(&ART, "art.png");
  $.Font.New(&FONT, "font.png", $Rgb(0,0,255), $Rgb(255, 0, 255));
  // $.Music.Play("dragon2.mod");

  $.Surface.New(&SURFACE);
  $.Scene.New(&SCENE);
  $.Canvas.New(&CANVAS);
  $Vec3_Set(&CAMERA_POSITION, 2,0,-5);

  memset(PLAYER, 0, sizeof(PLAYER));
  
  // Absolutely temporary.
  PLAYER[0].obj.type = OT_PLAYER;
  ME = &PLAYER[0];

  memset(&BALL, 0, sizeof(BALL));
  BALL.obj.type = OT_BALL;

  $.Net.Connect("localhost", 5000);
  char* n = "HELLO";
  $.Net.SendMessage(5, n);

  $.Scene.SetPovLookAtXyz(&SCENE, 10, 10, 10, 0,0,0);
}

void $Update()
{
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

  Game_Tick();

}

float rotationTimer = 0.0f;

void $Draw()
{
  rotationTimer += (1.0f / 60.0f) * 10.0f;
  rotationTimer = ConstrainAngle(rotationTimer);
  printf("%f\n", rotationTimer);

  $.Scene.DrawSkybox(&SCENE, DB16_CADET_BLUE, DB16_LEAF); 
  
  if (ME != NULL)
  {
    Vec3f cameraTarget = RotatePointXZ($Vec3_Xyz(0,0,-6), ME->obj.yaw);
    Vec3f cameraPos    = RotatePointXZ($Vec3_Xyz(0,4,6), ME->obj.yaw);

    cameraPos.x += ME->obj.position.x;
    cameraPos.y += ME->obj.position.y;
    cameraPos.z += ME->obj.position.z;
    
    cameraTarget.x += ME->obj.position.x;
    cameraTarget.y += ME->obj.position.y;
    cameraTarget.z += ME->obj.position.z;

    $.Scene.SetPovLookAt(&SCENE, cameraPos, cameraTarget);
  }
  
  for(u32 ii=0;ii < MAX_PLAYERS;ii++)
  {
    Player* player = &PLAYER[ii];
    if (Object_IsAlive($Cast(Object*) player) == false)
      continue;
    Rot3i rot;
    rot.pitch = 0;
    rot.yaw   = player->obj.yaw;
    rot.roll  = 0;
    $.Scene.DrawMesh(&SCENE, &MESH_PLAYER, player->obj.position, rot);
  }

  Rot3i ballRot = { 0 };
  $.Scene.DrawMesh(&SCENE, &MESH_BALL, BALL.obj.position, ballRot);
  
  $.Scene.Render(&SCENE, &SURFACE);
  $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_BANANA, 0, 200 - 9, "FPS %.1f, Triangles: %i", $.Stats.fps, $.Stats.nbTriangles);
  $.Canvas.Render(&CANVAS, &SURFACE);

  $.Surface.Render(&SURFACE);
}
