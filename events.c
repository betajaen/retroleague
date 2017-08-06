#include "funk.h"
#include <string.h>

#define CAMERA_THETA_DEFAULT 90.0f

f32 CAMERA_THETA;
f32 CAMERA_THETA_TIME;
f32 CAMERA_THETA_PID_DELTA;

bool CAMERA_THETA_PID_ENABLED;
Pid  CAMERA_THETA_PID;

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
  $.Input.BindControl(CONTROL_CAMERA_LEFT,  $KEY_Q);
  $.Input.BindControl(CONTROL_CAMERA_RIGHT, $KEY_E);
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
  BALL.obj.position.FORWARD = 30;

  $.Net.Connect("localhost", 5000);
  char* n = "HELLO";
  $.Net.SendMessage(5, n);

  $.Scene.SetPovLookAtXyz(&SCENE, 10, 10, 10, 0,0,0);

  CAMERA_THETA      = CAMERA_THETA_DEFAULT;
  CAMERA_THETA_TIME = 0.0f;
  CAMERA_THETA_PID_ENABLED = false;
  CAMERA_THETA_PID_DELTA = 0.0f;
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

  if (ME != NULL)
  {
    if (CAMERA_THETA_TIME >= 0.0f)
    {
      CAMERA_THETA_TIME -= $.fixedDeltaTime;
      if (CAMERA_THETA_TIME <= 0.0f)
      {
        CAMERA_THETA_PID_ENABLED = true;
        CAMERA_THETA_PID_DELTA = 0.0f;
        MakePidDefaults1(&CAMERA_THETA_PID);
      }
    }

    if ($.Input.ControlDown(CONTROL_LEFT))
    {
      ME->steering = -10;
    }
    else if ($.Input.ControlDown(CONTROL_RIGHT))
    {
      ME->steering = +10;
    }
    else
    {
      ME->steering = 0;
    }

    if ($.Input.ControlDown(CONTROL_FORWARD))
      ME->acceleratorBrake = 100;
    else if ($.Input.ControlDown(CONTROL_BACKWARD))
      ME->acceleratorBrake = -100;
    else
      ME->acceleratorBrake = 0;

    if ($.Input.ControlDown(CONTROL_CAMERA_LEFT))
    {
      CAMERA_THETA -= 70.0f * $.fixedDeltaTime;
      CAMERA_THETA_TIME = 2.0f;
      CAMERA_THETA_PID_ENABLED = false;
    }
    else if ($.Input.ControlDown(CONTROL_CAMERA_RIGHT))
    {
      CAMERA_THETA += 70.0f * $.fixedDeltaTime;
      CAMERA_THETA_TIME = 2.0f;
      CAMERA_THETA_PID_ENABLED = false;
    }
    CAMERA_THETA = ConstrainAngle(CAMERA_THETA);
  }

  if (CAMERA_THETA_PID_ENABLED)
  {
    f32 error = CAMERA_THETA_DEFAULT - CAMERA_THETA;
    //f32 before = CAMERA_THETA;
    CAMERA_THETA_PID_DELTA = UpdatePid(&CAMERA_THETA_PID, error, $.fixedDeltaTime);
    CAMERA_THETA += CAMERA_THETA_PID_DELTA * $.fixedDeltaTime * 4.0f;
    //printf("BEFORE=%.1f, AFTER=%.1f, ERROR=%.1f, DELTA=%.1f\n", before, CAMERA_THETA, error, CAMERA_THETA_PID_DELTA);

    if (AbsDifference(CAMERA_THETA, CAMERA_THETA_DEFAULT) < 1.0f)
    {
      CAMERA_THETA = CAMERA_THETA_DEFAULT;
      CAMERA_THETA_PID_ENABLED = false;
    }
  }

  Game_Tick();

}

float rotationTimer = 0.0f;

void $Draw()
{
  rotationTimer += (1.0f / 60.0f) * 10.0f;
  rotationTimer = ConstrainAngle(rotationTimer);

  $.Scene.DrawSkybox(&SCENE, DB16_CADET_BLUE, DB16_LEAF);

  const u32 dotDistance = 4;
  u32 nbDots = BOUNDS_SIZE_I / 4;

  for(u32 ii=0;ii < nbDots;ii++)
  {
    for(u32 jj=0;jj < nbDots;jj++)
    {
      f32 x = -(BOUNDS_SIZE_F * 0.5f) + (ii * dotDistance);
      f32 z = -(BOUNDS_SIZE_F * 0.5f) + (jj * dotDistance);
      $.Scene.DrawGroundDot(&SCENE, DB16_PEPPERMINT, x, z);
    }
  }

  nbDots = GOAL_SIZE_X_I / dotDistance;
  f32 goalCenter = BOUNDS_SIZE_HALF_F - GOAL_SIZE_X_F * 0.5f;
  
  for(u32 ii=0;ii < nbDots;ii++)
  {
    for(u32 jj=0;jj < nbDots;jj++)
    {
      f32 x = -(GOAL_SIZE_X_F * 0.5f) + (ii * dotDistance);
      f32 z = BOUNDS_SIZE_HALF_F + (jj * dotDistance);
      $.Scene.DrawGroundDot(&SCENE, DB16_FADED_RED, x, z);
      z = -BOUNDS_SIZE_HALF_F - GOAL_SIZE_X_F + (jj * dotDistance);
      $.Scene.DrawGroundDot(&SCENE, DB16_CADET_BLUE, x, z);
    }
  }

  if (ME != NULL)
  {
    const f32 cameraDistance = 6.0f;
    f32 c = cosf($Deg2Rad(CAMERA_THETA)) * 6.0f;
    f32 s = sinf($Deg2Rad(CAMERA_THETA)) * 6.0f;

    Vec3f cameraTarget = RotatePointXZ($Vec3_Xyz(c,0,s), ME->obj.yaw);
    
    Vec3f cameraPos    = RotatePointXZ($Vec3_Xyz(-c,4,-s), ME->obj.yaw);

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
  
  /*
  for(u32 i=0;i < 40;i++)
  {
    $.Scene.DrawMeshXyz(&SCENE, &MESH_BALL, 0, 0,i * 2.0f,  0,0,0);
  }*/

  $.Scene.Render(&SCENE, &SURFACE);
  $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_BANANA, 0, 200 - 9, "FPS %.1f, Triangles: %i Yaw: %i", $.Stats.fps, $.Stats.nbTriangles, ME->obj.yaw);
  if (ME != NULL)
  {
    $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_BANANA, 0, 200 - 18, "XYZ: %.1f %.1f VEL: %.1f %.1f ACC: %.1f %.1f A: %i S: %i",
     ME->obj.position.x, ME->obj.position.z, 
     ME->obj.velocity.x, ME->obj.velocity.z, 
     ME->obj.acceleration.x, ME->obj.acceleration.z, 
     ME->acceleratorBrake, ME->steering);
  }
   $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_BANANA, 0, 200 - 18 - 9, "XYZ: %.1f %.1f VEL: %.1f %.1f ACC: %.1f %.1f",
     BALL.obj.position.x, BALL.obj.position.z, 
     BALL.obj.velocity.x, BALL.obj.velocity.z, 
     BALL.obj.acceleration.x, BALL.obj.acceleration.z);


  
   $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_FADED_RED, 0, 0, "R: %i B: %i", BALL.red, BALL.blue);

  $.Canvas.Render(&CANVAS, &SURFACE);

  $.Surface.Render(&SURFACE);
}
