#include "funk.h"
#include <string.h>

f32     DELTA;
Surface SURFACE;
Scene   SCENE;
Canvas  CANVAS;
Player  PLAYER[MAX_PLAYERS];
Player* ME;
u32     ME_INDEX;
Ball    BALL;
Font    FONT;
Bitmap  ART;
Mesh    MESH_PLAYER;
Mesh    MESH_BALL;
Vec3f   CAMERA_POSITION;
Vec3f   CAMERA_ROTATION;
u32     FRAME_COUNT;
Bitmap  TITLE;
i32     GAME_STATE;

void Game_Tick()
{
  for(u32 ii=0;ii < MAX_PLAYERS;ii++)
  {
    Player* player = &PLAYER[ii];
    if (Object_IsAlive($Cast(Object*) player) == false)
      continue;

    Player_Tick(player);
  }

  Ball_Tick(&BALL);
}

void Draw_Title()
{
  $.Canvas.DrawBitmap(&CANVAS, &TITLE, 0, 0);
  $.Canvas.Render(&CANVAS, &SURFACE);
  $.Surface.Render(&SURFACE);

  if ($.Input.ControlReleased(CONTROL_START_SINGLE))
  {
    GAME_STATE = GAME_STATE_SINGLE;
    Start_SinglePlayer();
    return;
  }
  else if ($.Input.ControlReleased(CONTROL_START_MULTI))
  {
    GAME_STATE = GAME_STATE_MULTI;
    Start_Multiplayer();
    return;
  } 

}

static void Draw_Scene()
{
  
  if ($.Input.ControlReleased(CONTROL_AUTOPILOT))
  {
    ME->autopilot = !ME->autopilot;
  }
  
  if ($.Input.ControlReleased(CONTROL_CHEAT_FLIP_180))
  {
    ME->heading -= $Deg2Rad(180.0f);
  }

  if ($.Input.ControlReleased(CONTROL_POWER_PUNT) && Can_Power(ME, POWER_PUNT))
  {
    Activate_Power(ME, POWER_PUNT);
  }
  
  if ($.Input.ControlReleased(CONTROL_POWER_MAGNET) && Can_Power(ME, POWER_MAGNET))
  {
    Activate_Power(ME, POWER_MAGNET);
  }
  
  if ($.Input.ControlReleased(CONTROL_POWER_SPIN) && Can_Power(ME, POWER_SPIN))
  {
    Activate_Power(ME, POWER_SPIN);
  }

  $.Scene.DrawSkybox(&SCENE, DB16_VERY_DARK_VIOLET, DB16_SHADOWY_LAVENDER);

  const u32 dotDistance = 8;
  u32 nbDots = BOUNDS_SIZE_I / 8;

  for(u32 ii=0;ii < nbDots;ii++)
  {
    for(u32 jj=0;jj < nbDots;jj++)
    {
      f32 x = -(BOUNDS_SIZE_F * 0.5f) + (ii * dotDistance);
      f32 z = -(BOUNDS_SIZE_F * 0.5f) + (jj * dotDistance);
      $.Scene.DrawGroundDot(&SCENE, DB16_FLINT, x, z);
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

    if (ME->anim.state != ANIMATION_STATE_SPIN)
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
    $.Scene.DrawCustomShaderMesh(&SCENE, &MESH_PLAYER, player->team == 0 ? 2 : 3, player->obj.position, rot);
  }

  Rot3i ballRot;
  ballRot.pitch = 0;
  ballRot.yaw   = BALL.obj.yaw;
  ballRot.roll  = 0;
  $.Scene.DrawMesh(&SCENE, &MESH_BALL, BALL.obj.position, ballRot);
  
  /*
  for(u32 i=0;i < 40;i++)
  {
    $.Scene.DrawMeshXyz(&SCENE, &MESH_BALL, 0, 0,i * 2.0f,  0,0,0);
  }*/

  $.Scene.Render(&SCENE, &SURFACE);
  $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_BANANA, 0, 200 - 9, "FPS %.1f, Triangles: %i Yaw: %i", $.Stats.fps, $.Stats.nbTriangles, ME->obj.yaw);
  
  for(u32 ii=0;ii < MAX_PLAYERS;ii++)
  {
    Player* player = &PLAYER[ii];
    if (player->obj.type != OT_PLAYER)
      continue;
    $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_BANANA, 0, 200 - 18 - 9 - (ii * 9), "[%i] %i %.1f %.1f",
      ii, player->timestamp,
      player->obj.position.x, player->obj.position.z);
  }
  
  if (ME != NULL)
  {
   if (ME->autopilot)
   {  
    $.Canvas.DrawText(&CANVAS, &FONT, DB16_INDIGO, 0, 9, "<<<AUTO-PILOT>>>");
   }

   for(u32 ii=0;ii < MAX_POWERS;ii++)
   {
      if (Can_Power(ME, ii))
      {
        $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_INDIGO, 0, 18 + (ii * 9), "%s RDY", POWERS_NAME[ii]);
      }
      else
      {
        $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_INDIGO, 0, 18 + (ii * 9), "%s %.1f",  POWERS_NAME[ii], ME->powerCooldown[ii]);
      }
   }

  }
  
    $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_BANANA, 0, 200 - 18, "BALL: %.1f %.1f",
     BALL.obj.position.x, BALL.obj.position.z, 
     BALL.obj.velocity.x, BALL.obj.velocity.z);

  
   $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_FADED_RED, 0, 0, "R: %i B: %i", BALL.red, BALL.blue);


  $.Canvas.Render(&CANVAS, &SURFACE);

  $.Surface.Render(&SURFACE);
}

#define CAMERA_THETA_DEFAULT 90.0f

f32  CAMERA_THETA;
f32  CAMERA_THETA_TIME;
f32  CAMERA_THETA_PID_DELTA;
bool CAMERA_THETA_PID_ENABLED;
Pid  CAMERA_THETA_PID;

void Update_Scene()
{
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
      ME->steering = -$Deg2Rad(40);
    }
    else if ($.Input.ControlDown(CONTROL_RIGHT))
    {
      ME->steering = +$Deg2Rad(40);
    }
    else
    {
      ME->steering = 0;
    }


    ME->handBrake = $.Input.ControlDown(CONTROL_HANDBRAKE);

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
    f32 error = PidError(CAMERA_THETA_DEFAULT, CAMERA_THETA);
    CAMERA_THETA_PID_DELTA = UpdatePid(&CAMERA_THETA_PID, error, $.fixedDeltaTime);
    CAMERA_THETA += CAMERA_THETA_PID_DELTA * $.fixedDeltaTime * 4.0f;
    
    if (AbsDifference(CAMERA_THETA, CAMERA_THETA_DEFAULT) < 1.0f)
    {
      CAMERA_THETA = CAMERA_THETA_DEFAULT;
      CAMERA_THETA_PID_ENABLED = false;
    }
  }

  Game_Tick();
}

void Start_SinglePlayer()
{
  for(u32 i=0;i < MAX_PLAYERS;i++)
  {
    PLAYER[i].obj.type = OT_PLAYER;
    PLAYER[i].obj.position.x = -BOUNDS_SIZE_HALF_F + 10.0f * i;
    PLAYER[i].autopilot = true;
  }

  ME = &PLAYER[0];
  ME->autopilot = false;
  
  PLAYER[1].team = 0;
  PLAYER[1].team = 1;
  PLAYER[2].team = 0;
  PLAYER[3].team = 1;
  
  CAMERA_THETA      = CAMERA_THETA_DEFAULT;
  CAMERA_THETA_TIME = 0.0f;
  CAMERA_THETA_PID_ENABLED = false;
  CAMERA_THETA_PID_DELTA = 0.0f;
}

void Tick_Singleplayer()
{
  Update_Scene();
}

void Draw_Singleplayer()
{
  Draw_Scene();
}

void Start_Multiplayer()
{
  $.Net.Connect("localhost", 5000);
  $.Net.SendMessage(1, "H");
  
  CAMERA_THETA      = CAMERA_THETA_DEFAULT;
  CAMERA_THETA_TIME = 0.0f;
  CAMERA_THETA_PID_ENABLED = false;
  CAMERA_THETA_PID_DELTA = 0.0f;
}

void Tick_Multiplayer()
{
  if ($.Net.PeekMessage())
  {
    u8* data = $TempNewA(u8, 1025);
    for(u32 ii=0;ii < 8;ii++)
    {
      u32 length = 0;
      if ($.Net.RecvMessage(&length, 1025, data) == false)
        break;
      data[length] = '\0';
      //printf("> %s\n", data);
      
      char *line = strtok(data, "\n");
      while(line)
      {
          ReceiveMessage(line);
          line = strtok(NULL, "\n");
      }

      if ($.Net.PeekMessage() == false)
        break;
    }
  }
  Update_Scene();

}

i32 syncTimer = 0;

void Draw_Multiplayer()
{
  if (ME == NULL)
  {
    $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_PEPPERMINT, 0, 0, "Waiting for Mainframe response...");
    $.Canvas.Render(&CANVAS, &SURFACE);
    $.Surface.Render(&SURFACE);
    return;
  }
  else
  {
    if (syncTimer++ > 20)
    {
      Player_SendFullUpdate();
      syncTimer = 0;
    }

    Draw_Scene();
  }
}
