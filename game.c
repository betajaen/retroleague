#include "funk.h"
#include <string.h>
#include <stdlib.h>

Surface SURFACE;
Scene   SCENES[4];
Canvas  HUD;
Player  PLAYER[MAX_PLAYERS];
Player* LOCAL[2];
Ball    BALL;
Font    FONT;
Bitmap  ART;
Mesh    MESH_PLAYER[2];
Mesh    MESH_BALL;
Mesh    MESH_WHEEL;
Vec3f   CAMERA_POSITION;
Vec3f   CAMERA_ROTATION;
u32     FRAME_COUNT;
Bitmap  TITLE;
i32     GAME_STATE;

#define CAMERA_THETA_DEFAULT 90.0f

f32  CAMERA_THETA;
f32  CAMERA_THETA_TIME;
f32  CAMERA_THETA_PID_DELTA;
bool CAMERA_THETA_PID_ENABLED;
Pid  CAMERA_THETA_PID;

static void Game_Tick()
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

static void Setup_Scenes()
{
  bool isTwoLocalPlayer = LOCAL[0] != NULL && LOCAL[1] != NULL;

  $.Scene.Delete(&SCENES[0]);
  $.Scene.Delete(&SCENES[1]);
  $.Scene.Delete(&SCENES[2]);
  $.Scene.Delete(&SCENES[3]);

  i32 x = 0, y = 0;
  u32 w = isTwoLocalPlayer ? $.width / 2 : $.width;
  u32 h = $.height;

  if (LOCAL[0] != NULL)
  {
    $.Scene.NewXywh(&SCENES[0], x, y, w, h);
    x += w;
  }

  if (LOCAL[1] != NULL)
  {
    $.Scene.NewXywh(&SCENES[1], x, y, w, h);
    x += w;
  }
}

static void Draw_HUD(Canvas* canvas)
{
  $.Canvas.Clear(canvas, DB16_TRANSPARENT);
  $.Canvas.DrawTextF(canvas, &FONT, DB16_REGENT_GREY, 0, 200 - 9, "D%03i:%.4f F%03i:%.4f T%03i", 
      (i32) $.Stats.fps,
      (f32) $.deltaTime,
      (i32) $.Stats.fixedFps,
      (f32) $.fixedDeltaTime,
      $.Stats.nbTriangles
      );
  
  #if 1
  for(u32 ii=0;ii < MAX_PLAYERS;ii++)
  {
    Player* player = &PLAYER[ii];
    if (player->obj.type != OT_PLAYER)
      continue;
    $.Canvas.DrawTextF(canvas, &FONT, DB16_BANANA, 0, 200 - 18 - 9 - (ii * 9), "[%04X] %.1f %.1f %.1f %.1f %i",
      player->multiplayerReference,
      player->obj.position.x, player->obj.position.z,
      $Rad2Deg(player->heading), player->absVelocity, player->acceleratorBrake);
  }
  #endif
  
  for(u32 ii=0;ii < 2;ii++)
  {
    Player* player = LOCAL[0];

    if (player != NULL)
    {
     if (player->autopilot)
     {  
      $.Canvas.DrawText(canvas, &FONT, DB16_BANANA, 320 / 2 - 42, 24, "<<AUTO PILOT>>");
     }

     u32 power_width = MAX_POWERS * 9;

     for(u32 ii=0;ii < MAX_POWERS;ii++)
     {
        if (Can_Power(player, ii))
        {
          $.Canvas.DrawTextF(canvas, &FONT, DB16_INDIGO + ii, (320 / 2 - power_width) + ii * 9, 200 - 11, "%c", POWERS_NAME[ii][0]);
        }
        else
        {
          $.Canvas.DrawTextF(canvas, &FONT, DB16_REGENT_GREY, (320 / 2 - power_width) + ii * 9, 200 - 11, "-");
        }
     }

    }
  }

  #if 0
    $.Canvas.DrawTextF(&CANVAS, &FONT, DB16_BANANA, 0, 200 - 18, "BALL: %.1f %.1f",
     BALL.obj.position.x, BALL.obj.position.z, 
     BALL.obj.velocity.x, BALL.obj.velocity.z);
  #endif
  
  $.Canvas.DrawTextF(canvas, &FONT, DB16_FADED_RED,  $.width  / 2 - 20, 2, "%i", BALL.red);
  $.Canvas.DrawTextF(canvas, &FONT, DB16_CADET_BLUE, $.height / 2 + 20, 2, "%i", BALL.blue);

  $.Canvas.Render(canvas, &SURFACE);

}

static void Draw_Car(Scene* scene, Vec3f worldPos, f32 yawDeg, f32 steering, u8 team)
{
  Rot3i rot;
  rot.pitch = 0;
  rot.yaw   = (i16) yawDeg;
  rot.roll  = 0;
  $.Scene.DrawMesh(scene, &MESH_PLAYER[team], worldPos, rot);

  Vec3f wheel[4];

  wheel[0] = TransformLocalPointToWorldSpaceXZ(worldPos, -rot.yaw, $Vec3_Xyz(0.9f,  0, 1.0f));   wheel[0].y = -0.62f;
  wheel[1] = TransformLocalPointToWorldSpaceXZ(worldPos, -rot.yaw, $Vec3_Xyz(-0.9f, 0, 1.0f));   wheel[1].y = -0.62f;
  wheel[2] = TransformLocalPointToWorldSpaceXZ(worldPos, -rot.yaw, $Vec3_Xyz( 0.9f, 0,-1.32f));  wheel[2].y = -0.62f;
  wheel[3] = TransformLocalPointToWorldSpaceXZ(worldPos, -rot.yaw, $Vec3_Xyz(-0.9f, 0,-1.32f));  wheel[3].y = -0.62f;

  Rot3i steeringRot = rot;
  steeringRot.yaw -= (i16) $Rad2Deg(steering);

  $.Scene.DrawMesh(scene, &MESH_WHEEL, wheel[0], steeringRot);
  $.Scene.DrawMesh(scene, &MESH_WHEEL, wheel[1], steeringRot);
  $.Scene.DrawMesh(scene, &MESH_WHEEL, wheel[2], rot);
  $.Scene.DrawMesh(scene, &MESH_WHEEL, wheel[3], rot);
}

static void Draw_Player(Player* localPlayer, Scene* scene, u32 localIndex)
{
  u32 controlOffset = (localIndex + 1) * 40;

  if ($.Input.ControlReleased(controlOffset + CONTROL_P0_AUTOPILOT))
  {
    localPlayer->autopilot = !localPlayer->autopilot;
  }
  
  if ($.Input.ControlReleased(controlOffset + CONTROL_P0_POWER_KICK) && Can_Power(localPlayer, POWER_KICK))
  {
    Activate_Power(localPlayer, POWER_KICK);
  }
  
  if ($.Input.ControlReleased(controlOffset + CONTROL_P0_POWER_MAGNET) && Can_Power(localPlayer, POWER_MAGNET))
  {
    Activate_Power(localPlayer, POWER_MAGNET);
  }
  
  if ($.Input.ControlReleased(controlOffset + CONTROL_P0_POWER_SPIN) && Can_Power(localPlayer, POWER_SPIN))
  {
    Activate_Power(localPlayer, POWER_SPIN);
  }

  $.Scene.DrawSkybox(scene, DB16_VERY_DARK_VIOLET, DB16_SHADOWY_LAVENDER);

  const f32 dotDistance = 4;
  u32 nbDots = BOUNDS_SIZE_I / 4;
  u32 middle = (nbDots / 2) + 1;

  for(u32 ii=0;ii < nbDots;ii++)
  {
    for(u32 jj=0;jj < nbDots;jj++)
    {
      u8 colour = DB16_FLINT;

      if (ii == 0 || ii == nbDots - 1 || jj == 0 || jj == nbDots - 1 || ii == middle)
      {
        if (jj < middle)
          colour = DB16_CADET_BLUE;
        else
          colour = DB16_FADED_RED;
      }

      if (jj == middle)
        colour = DB16_PEPPERMINT;

      f32 x = -(BOUNDS_SIZE_F * 0.5f) + (ii * dotDistance);
      f32 z = -(BOUNDS_SIZE_F * 0.5f) + (jj * dotDistance);
      $.Scene.DrawGroundDot(scene, colour, x, z);
    }
  }

  nbDots = GOAL_SIZE_X_I / 4;
  f32 goalCenter = BOUNDS_SIZE_HALF_F - GOAL_SIZE_X_F * 0.5f;
  
  for(u32 ii=0;ii < nbDots;ii++)
  {
    for(u32 jj=0;jj < nbDots / 4;jj++)
    {
      f32 x = -(GOAL_SIZE_X_F * 0.5f) + (ii * dotDistance);
      f32 z = dotDistance + BOUNDS_SIZE_HALF_F + (jj * dotDistance);
      $.Scene.DrawGroundDot(scene, DB16_FADED_RED, x, z);
      z = -BOUNDS_SIZE_HALF_F - GOAL_SIZE_X_F + (jj * dotDistance);
      $.Scene.DrawGroundDot(scene, DB16_CADET_BLUE, x, z);
    }
  }

  if (localPlayer->anim.state != ANIMATION_STATE_SPIN)
  {
    const f32 cameraDistance = 6.0f;
    f32 c = cosf($Deg2Rad(CAMERA_THETA)) * 6.0f;
    f32 s = sinf($Deg2Rad(CAMERA_THETA)) * 6.0f;

    Vec3f cameraTarget = RotatePointXZ($Vec3_Xyz(c,0,s), localPlayer->obj.yaw);
    
    Vec3f cameraPos    = RotatePointXZ($Vec3_Xyz(-c,4,-s), localPlayer->obj.yaw);

    cameraPos.x += localPlayer->obj.position.x;
    cameraPos.y += localPlayer->obj.position.y;
    cameraPos.z += localPlayer->obj.position.z;
    
    cameraTarget.x += localPlayer->obj.position.x;
    cameraTarget.y += localPlayer->obj.position.y;
    cameraTarget.z += localPlayer->obj.position.z;

    $.Scene.SetPovLookAt(scene, cameraPos, cameraTarget);
  }
  
  for(u32 ii=0;ii < MAX_PLAYERS;ii++)
  {
    Player* player = &PLAYER[ii];
    if (Object_IsAlive($Cast(Object*) player) == false)
      continue;
    #if 0
    Rot3i rot;
    rot.pitch = 0;
    rot.yaw   = player->obj.yaw;
    rot.roll  = 0;
    $.Scene.DrawMesh(scene, &MESH_PLAYER[player->team], player->obj.position, rot);

    Vec3f wheel[4];

    wheel[0] = TransformLocalPointToWorldSpaceXZ(player->obj.position, -rot.yaw, $Vec3_Xyz(0.9f,  0, 1.0f));   wheel[0].y = -0.62f;
    wheel[1] = TransformLocalPointToWorldSpaceXZ(player->obj.position, -rot.yaw, $Vec3_Xyz(-0.9f, 0, 1.0f));   wheel[1].y = -0.62f;
    wheel[2] = TransformLocalPointToWorldSpaceXZ(player->obj.position, -rot.yaw, $Vec3_Xyz( 0.9f, 0,-1.32f));  wheel[2].y = -0.62f;
    wheel[3] = TransformLocalPointToWorldSpaceXZ(player->obj.position, -rot.yaw, $Vec3_Xyz(-0.9f, 0,-1.32f));  wheel[3].y = -0.62f;

    Rot3i steeringRot = rot;
    steeringRot.yaw -= (i16) $Rad2Deg(player->steering);

    $.Scene.DrawMesh(scene, &MESH_WHEEL, wheel[0], steeringRot);
    $.Scene.DrawMesh(scene, &MESH_WHEEL, wheel[1], steeringRot);
    $.Scene.DrawMesh(scene, &MESH_WHEEL, wheel[2], rot);
    $.Scene.DrawMesh(scene, &MESH_WHEEL, wheel[3], rot);
    #endif
    Draw_Car(scene, player->obj.position, player->obj.yaw, player->steering, player->team);

  }

  Rot3i ballRot;
  ballRot.pitch = 0;
  ballRot.yaw   = BALL.obj.yaw;
  ballRot.roll  = 0;
  $.Scene.DrawCustomShaderMesh(scene, &MESH_BALL, 4, BALL.obj.position, ballRot);
  
  $.Scene.Render(scene, &SURFACE);
}

void Draw_Scene()
{
  if (LOCAL[0] != NULL)
    Draw_Player(LOCAL[0], &SCENES[0], 0);
  if (LOCAL[1] != NULL)
    Draw_Player(LOCAL[1], &SCENES[1], 1);
  Draw_HUD(&HUD);
  $.Surface.Render(&SURFACE);
}

void Update_Scene(Player* player, int playerId)
{
  u32 controlOffset = (playerId + 1) * 40;

  if (player != NULL)
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

    if ($.Input.ControlDown(controlOffset + CONTROL_P0_LEFT))
    {
      player->steering = -$Deg2Rad(40);
    }
    else if ($.Input.ControlDown(controlOffset + CONTROL_P0_RIGHT))
    {
      player->steering = +$Deg2Rad(40);
    }
    else
    {
      player->steering = 0;
    }


    player->handBrake = $.Input.ControlDown(controlOffset + CONTROL_P0_HANDBRAKE);

    if ($.Input.ControlDown(controlOffset + CONTROL_P0_FORWARD))
      player->acceleratorBrake = 100;
    else if ($.Input.ControlDown(controlOffset + CONTROL_P0_BACKWARD))
      player->acceleratorBrake = -100;
    else
      player->acceleratorBrake = 0;

    if ($.Input.ControlDown(controlOffset + CONTROL_P0_CAMERA_LEFT))
    {
      CAMERA_THETA -= 70.0f * $.fixedDeltaTime;
      CAMERA_THETA_TIME = 2.0f;
      CAMERA_THETA_PID_ENABLED = false;
    }
    else if ($.Input.ControlDown(controlOffset + CONTROL_P0_CAMERA_RIGHT))
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

}

void Start_SinglePlayer(u8 p0, u8 p1, u8 p2, u8 p3, bool p2IsPlayer)
{
  f32 x = GOAL_SIZE_X_F;
  f32 z = BOUNDS_SIZE_HALF_F - BOUNDS_SIZE_HALF_F * 0.5f;
  
  for(u32 ii=0;ii < MAX_PLAYERS;ii++)
  {
    PLAYER[ii].obj.type  = OT_NONE;
    PLAYER[ii].autopilot = false;
  }

#if 1
//  ME = &PLAYER[0];
  
  LOCAL[0] = &PLAYER[0];
  LOCAL[1] = p2IsPlayer ? &PLAYER[1] : NULL;

  PLAYER[0].obj.type = OT_PLAYER;
  PLAYER[0].autopilot = false;
  PLAYER[0].team = p0;
  PLAYER[0].obj.position = $Vec3_Xyz(x, 0, z);
  PLAYER[0].obj.yaw = -120;
  PLAYER[0].heading = $Deg2Rad(PLAYER[0].obj.yaw);
  
  PLAYER[1].obj.type = OT_PLAYER;
  PLAYER[1].autopilot = !p2IsPlayer;
  PLAYER[1].team = p1;
  PLAYER[1].obj.position = $Vec3_Xyz(-x, 0, -z);
  PLAYER[1].obj.yaw = 60;
  PLAYER[1].heading = $Deg2Rad(PLAYER[1].obj.yaw);
  
  PLAYER[2].obj.type = OT_PLAYER;
  PLAYER[2].autopilot = true;
  PLAYER[2].team = p2;
  PLAYER[2].obj.position = $Vec3_Xyz(-x, 0, z);
  PLAYER[2].obj.yaw = 120;
  PLAYER[2].heading = $Deg2Rad(PLAYER[2].obj.yaw);
  
  PLAYER[3].obj.type = OT_PLAYER;
  PLAYER[3].autopilot = true;
  PLAYER[3].team = p3;
  PLAYER[3].obj.position = $Vec3_Xyz(x, 0, -z);
  PLAYER[3].obj.yaw = -60;
  PLAYER[3].heading = $Deg2Rad(PLAYER[3].obj.yaw);
#else
  LOCAL[0] = &PLAYER[0];
  LOCAL[1] = &PLAYER[1];
  
  PLAYER[0].obj.type = OT_PLAYER;
  PLAYER[0].autopilot = false;
  PLAYER[0].team = 0;
  PLAYER[0].obj.position = $Vec3_Xyz(10, 0, 10);
  PLAYER[0].obj.yaw = -180;
  PLAYER[0].heading = $Deg2Rad(PLAYER[0].obj.yaw);
  
  PLAYER[1].obj.type = OT_PLAYER;
  PLAYER[1].autopilot = false;
  PLAYER[1].team = 1;
  PLAYER[1].obj.position = $Vec3_Xyz(10, 0, -10);
  PLAYER[1].obj.yaw = 0;
  PLAYER[1].heading = $Deg2Rad(PLAYER[1].obj.yaw);
#endif

  CAMERA_THETA      = CAMERA_THETA_DEFAULT;
  CAMERA_THETA_TIME = 0.0f;
  CAMERA_THETA_PID_ENABLED = false;
  CAMERA_THETA_PID_DELTA = 0.0f;

  Setup_Scenes();
}

void FixedUpdate_Singleplayer()
{
  if (LOCAL[0] != NULL)
  {
    Update_Scene(LOCAL[0], 0);
  }
  if (LOCAL[0] != NULL)
  {
    Update_Scene(LOCAL[1], 1);
  }
  Game_Tick();
}

void Update_SinglePlayer()
{
  Draw_Scene();
}

void Start_Multiplayer()
{
  CAMERA_THETA      = CAMERA_THETA_DEFAULT;
  CAMERA_THETA_TIME = 0.0f;
  CAMERA_THETA_PID_ENABLED = false;
  CAMERA_THETA_PID_DELTA = 0.0f;

  Setup_Scenes();
}

void FixedUpdate_Multiplayer()
{
  Net_Update();

  if (LOCAL[0] != NULL)
  {
    Update_Scene(LOCAL[0], 0);
  }
  
  if (LOCAL[1] != NULL)
  {
    Update_Scene(LOCAL[1], 1);
  }

  Game_Tick();
}

i32 syncTimer = 0;

void Update_MultiPlayer(Player* ME)
{
  if (ME == NULL)
  {
    $.Canvas.DrawTextF(&HUD, &FONT, DB16_PEPPERMINT, 0, 0, "Waiting for Mainframe response...");
    $.Canvas.Render(&HUD, &SURFACE);
    $.Surface.Render(&SURFACE);
    return;
  }
  else
  {
    if (syncTimer++ > 10)
    {
      for(u32 ii=0;ii < MAX_PLAYERS;ii++)
      {
        if (PLAYER[ii].multiplayerIsControlled)
        {
          Player_Send_Update(ii);
        }
      }

      if (Net_IsHosting())
      {
        Player_Send_Ball_Update(&BALL);
      }

      syncTimer = 0;
    }

    Draw_Scene();
  }
}

// 0 - Player Red
// 1 - Player Blue
// 2 - CPU Red
// 3 - CPU Blue
static i32 setupSinglePlayerMode[4];
static i32 setupSingleP2Mode;
static f32 setupSingleCarRot;

static const char* MODE_TEAMS[4] = {
  "RED",
  "BLUE",
  "RED",
  "BLUE"
};

static u32 MODE_TEAMS_HALF_LEN[4] = {
  19 / 2, 26 / 2, 19 / 2,  26 / 2,
};

static u8 MODE_TEAMS_COLOUR[4] = {
  DB16_FADED_RED, DB16_CADET_BLUE, DB16_FADED_RED, DB16_CADET_BLUE
};

static const char* MODE_NAMES[4] = {
  "< P1 >",
  "< P2 >",
  "< CPU >",
  "CPU"
};

static u32 MODE_NAMES_HALF_LEN[4] = {
  34 / 2, 34 / 2, 42 / 2, 19 / 2,
};

static u8 MODE_NAMES_COLOUR[4] = {
  DB16_BANANA, DB16_BANANA, DB16_BIRTHDAY_SUIT, DB16_FLINT
};

void Update_SinglePlayerSetup()
{
  $.Canvas.Clear(&HUD, DB16_VERY_DARK_VIOLET);

  if ($.Input.ControlPressed(CONTROL_P1_LEFT))
    setupSinglePlayerMode[0]--;
  else if ($.Input.ControlPressed(CONTROL_P1_RIGHT))
    setupSinglePlayerMode[0]++;
  
  setupSinglePlayerMode[0] = $WrapMinMax(setupSinglePlayerMode[0], 0, 2);

  if ($.Input.ControlPressed(CONTROL_P2_LEFT))
    setupSingleP2Mode--;
  else if ($.Input.ControlPressed(CONTROL_P2_RIGHT))
    setupSingleP2Mode++;
  
  setupSingleP2Mode = $WrapMinMax(setupSingleP2Mode, 0, 3);

  setupSinglePlayerMode[0] = $WrapMinMax(setupSinglePlayerMode[0], 0, 2);
  setupSinglePlayerMode[1] = setupSingleP2Mode;

  if (setupSinglePlayerMode[1] >= 2)  // Single Player
  {
    if (setupSinglePlayerMode[0] == 0)
    {
      setupSinglePlayerMode[1] = 3;
      setupSinglePlayerMode[2] = 2;
      setupSinglePlayerMode[3] = 3;
    }
    else
    {
      setupSinglePlayerMode[1] = 2;
      setupSinglePlayerMode[2] = 3;
      setupSinglePlayerMode[3] = 2;
    }
  }
  else // Two Player
  {
    if (setupSinglePlayerMode[0] == setupSinglePlayerMode[1])
    {
      if (setupSinglePlayerMode[0] == 0)
      {
        setupSinglePlayerMode[2] = 3;
        setupSinglePlayerMode[3] = 3;
      }
      else
      {
        setupSinglePlayerMode[2] = 2;
        setupSinglePlayerMode[3] = 2;
      }
    }
    else
    {
      if (setupSinglePlayerMode[0] == 0)
      {
        setupSinglePlayerMode[2] = 2;
        setupSinglePlayerMode[3] = 3;
      }
      else
      {
        setupSinglePlayerMode[2] = 3;
        setupSinglePlayerMode[3] = 2;
      }
    }
  }
  
  u32 s = $.width / 4;
  u32 x = $.width / s;
  u32 y = s / 2 + s + 10;

  for(u32 ii=0;ii < 4;ii++)
  {
    i32 mode = setupSinglePlayerMode[ii];
    $.Canvas.DrawTextF(&HUD, &FONT, MODE_TEAMS_COLOUR[mode], x + s / 2 - MODE_TEAMS_HALF_LEN[mode], y, "%s", MODE_TEAMS[mode]);

    i32 ni = 0;
    if (ii == 0)
      ni = 0;
    else if (ii == 1 && mode < 2)
      ni = 1;
    else if (ii == 1 && mode >= 2)
      ni = 2;
    else if (mode >= 2)
      ni = 3;
      
    $.Canvas.DrawTextF(&HUD, &FONT, MODE_NAMES_COLOUR[ni], x + s / 2 - MODE_NAMES_HALF_LEN[ni], y + 9, "%s", MODE_NAMES[ni]);

    x += s;
  }
  
  $.Canvas.DrawTextF(&HUD, &FONT, DB16_BIRTHDAY_SUIT, $.width / 2 - 33, 12, "GAME SETUP");

  $.Canvas.DrawTextF(&HUD, &FONT, DB16_FLINT, 12, $.height - 12, "[TAB] Menu");
  $.Canvas.DrawTextF(&HUD, &FONT, DB16_BANANA, $.width - 12 - 70, $.height - 12, "[SPACE] Play");

  $.Canvas.Render(&HUD, &SURFACE);
  
  for(u32 ii=0;ii < 4;ii++)
  {
    u8 team = 0;

    if (setupSinglePlayerMode[ii] == 0 || setupSinglePlayerMode[ii] == 2)
      team = 0;
    else
      team = 1;
    
    $.Scene.Clear(&SCENES[ii], DB16_VERY_DARK_VIOLET);
    Draw_Car(&SCENES[ii], $Vec3_Xyz(0,0,0), setupSingleCarRot, 0.0f, team);
    $.Scene.Render(&SCENES[ii], &SURFACE);
  }

  setupSingleCarRot += 90.0f * $.deltaTime;

  $.Surface.Render(&SURFACE);

  if ($.Input.ControlReleased(CONTROL_P1_HANDBRAKE))
  {
    u8 p0 = (setupSinglePlayerMode[0] == 0 || setupSinglePlayerMode[0] == 2) ? 0 : 1;
    u8 p1 = (setupSinglePlayerMode[1] == 0 || setupSinglePlayerMode[1] == 2) ? 0 : 1;
    u8 p2 = (setupSinglePlayerMode[2] == 0 || setupSinglePlayerMode[2] == 2) ? 0 : 1;
    u8 p3 = (setupSinglePlayerMode[3] == 0 || setupSinglePlayerMode[3] == 2) ? 0 : 1;
    bool p2IsPlayer = (setupSinglePlayerMode[1] < 2);


    Start_SinglePlayer(p0, p1, p2, p3, p2IsPlayer);
    GAME_STATE = GAME_STATE_SINGLE;
    return;
  }

  if ($.Input.ControlReleased(CONTROL_P1_AUTOPILOT))
  {
    GAME_STATE = GAME_STATE_TITLE;
    return;
  }
}

static int   setupMultiState;
static char* setupMultiAddress[256];
static char* setupMultiPort[256];
static int setupMultiConnectState;

void Update_Title()
{
  $.Canvas.DrawBitmap(&HUD, &TITLE, 0, 0);
  $.Canvas.Render(&HUD, &SURFACE);
  $.Surface.Render(&SURFACE);

  if ($.Input.ControlReleased(CONTROL_START_SINGLE))
  {
    GAME_STATE = GAME_STATE_SINGLE_SETUP;

    $.Scene.Delete(&SCENES[0]);
    $.Scene.Delete(&SCENES[1]);
    $.Scene.Delete(&SCENES[2]);
    $.Scene.Delete(&SCENES[3]);

    i32 s = $.width / 4;
    i32 x = 0, y = s / 2;

    for (u32 ii=0;ii < 4;ii++, x += s)
    {
      $.Scene.NewXywh(&SCENES[ii], x, y, s, s);
      $.Scene.SetPovLookAtXyz(&SCENES[ii], 0, 2.0, 2.5, 0, 0, 0);
    }
    
    setupSingleCarRot = 0;
    setupSingleP2Mode = 2;
    setupSinglePlayerMode[0] = 0;
    setupSinglePlayerMode[1] = 2;
    setupSinglePlayerMode[2] = 3;
    setupSinglePlayerMode[3] = 2;
    return;
  }
  else if ($.Input.ControlReleased(CONTROL_START_MULTI))
  {
    GAME_STATE = GAME_STATE_MULTI_SETUP;
    setupMultiState = 0;
    setupMultiAddress[0] = '\0';
    strcat((char*) &setupMultiAddress[0], "127.0.0.1");
    setupMultiPort[0] = '\0';
    strcat((char*) &setupMultiPort[0], "27960");

    GAME_STATE = GAME_STATE_MULTI_SETUP;

    return;
  } 

}
void Update_MultiPlayerSetup()
{
  $.Canvas.Clear(&HUD, DB16_VERY_DARK_VIOLET);

  
  $.Canvas.DrawTextF(&HUD, &FONT, DB16_BIRTHDAY_SUIT, $.width / 2 - 66, 12, "MAINFRAME CONNECTION");

  $.Canvas.DrawTextF(&HUD, &FONT, DB16_BANANA, 12, 48, "[1] Host");

  $.Canvas.DrawTextF(&HUD, &FONT, DB16_BIRTHDAY_SUIT, 70, 48, (char*) &setupMultiAddress[0]);

  $.Canvas.DrawTextF(&HUD, &FONT, DB16_BANANA, 12, 64, "[2] Port");
  
  $.Canvas.DrawTextF(&HUD, &FONT, DB16_BIRTHDAY_SUIT, 70, 64, (char*) &setupMultiPort[0]);

  $.Canvas.DrawTextF(&HUD, &FONT, DB16_FLINT, 12, $.height - 12, "[TAB] Menu");
  $.Canvas.DrawTextF(&HUD, &FONT, DB16_BANANA, $.width - 12 - 89, $.height - 12, "[SPACE] Connect");

  $.Canvas.Render(&HUD, &SURFACE);
  $.Surface.Render(&SURFACE);
  
  if ($.Input.ControlReleased(CONTROL_P1_HANDBRAKE))
  {
    setupMultiState = 1;

    Net_Start((char*) setupMultiAddress, (char*) setupMultiPort);
    GAME_STATE = GAME_STATE_NETWORK_CONNECT;
    return;
  }

  if ($.Input.ControlReleased(CONTROL_P1_AUTOPILOT))
  {
    GAME_STATE = GAME_STATE_TITLE;
    return;
  }
}

void Update_NetworkConnect()
{
  Net_Update();

  $.Canvas.Clear(&HUD, DB16_VERY_DARK_VIOLET);
  $.Canvas.DrawTextF(&HUD, &FONT, DB16_BIRTHDAY_SUIT, $.width / 2 - 66, 12, "CONNECTING TO MAINFRAME");

  $.Canvas.Render(&HUD, &SURFACE);
  $.Surface.Render(&SURFACE);
  
  if (Net_IsConnected())
  {
    switch(setupMultiState)
    {
      case 1:
      {
        u16 reference = rand();
        u8 playerIndex = Player_New(reference);
    
        if (Net_GetNumClients() >= 4)
        {
          GAME_STATE = GAME_STATE_TITLE;
          return;
        }
        
        printf("Creating Player...\n");
        setupMultiState = 2;
        Net_Send_CreatePlayer(reference, 0);
      }
      break;
      case 2:
      {
        Player* player = Player_GetFirstControlled();
        if (player != NULL)
        {
          printf("Starting Game...\n");
          
          LOCAL[0] = player;
          LOCAL[1] = NULL;

          GAME_STATE = GAME_STATE_MULTI;
          Start_Multiplayer();
        }
      }
      break;
    }
  }
}
