#include "funk.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>

void $Setup()
{
  srand((u32) time(NULL));

#if IS_STREAMING
  $.screenX = -1600;
  $.screenY = 120;
#endif

  $.title = "Retro League ///";
  $.width = 320;
  $.height = 200;
  $.displayScale = 3;
  
  Mesh_MakeRedPlayer(&MESH_PLAYER[0]);
  Mesh_MakeBluePlayer(&MESH_PLAYER[1]);
  Mesh_MakeBall(&MESH_BALL);
  Mesh_MakeWheel(&MESH_WHEEL);

  $.Input.BindControl(CONTROL_P1_FORWARD,            $KEY_W);
  $.Input.BindControl(CONTROL_P1_BACKWARD,           $KEY_S);
  $.Input.BindControl(CONTROL_P1_LEFT,               $KEY_A);
  $.Input.BindControl(CONTROL_P1_RIGHT,              $KEY_D);
  $.Input.BindControl(CONTROL_P1_CAMERA_LEFT,        $KEY_Q);
  $.Input.BindControl(CONTROL_P1_CAMERA_RIGHT,       $KEY_E);
  $.Input.BindControl(CONTROL_P1_HANDBRAKE,          $KEY_X);
  $.Input.BindControl(CONTROL_P1_POWER_KICK,         $KEY_1);
  $.Input.BindControl(CONTROL_P1_POWER_MAGNET,       $KEY_2);
  $.Input.BindControl(CONTROL_P1_POWER_SPIN,         $KEY_3);

  $.Input.BindControl(CONTROL_P2_FORWARD,            $KEY_I);
  $.Input.BindControl(CONTROL_P2_BACKWARD,           $KEY_K);
  $.Input.BindControl(CONTROL_P2_LEFT,               $KEY_J);
  $.Input.BindControl(CONTROL_P2_RIGHT,              $KEY_L);
  $.Input.BindControl(CONTROL_P2_CAMERA_LEFT,        $KEY_U);
  $.Input.BindControl(CONTROL_P2_CAMERA_RIGHT,       $KEY_O);
  $.Input.BindControl(CONTROL_P2_HANDBRAKE,          $KEY_COMMA);
  $.Input.BindControl(CONTROL_P2_AUTOPILOT,          $KEY_M);
  $.Input.BindControl(CONTROL_P2_POWER_KICK,         $KEY_7);
  $.Input.BindControl(CONTROL_P2_POWER_MAGNET,       $KEY_8);
  $.Input.BindControl(CONTROL_P2_POWER_SPIN,         $KEY_9);

  $.Input.BindControl(CONTROL_START_SINGLE,          $KEY_A);
  $.Input.BindControl(CONTROL_START_MULTI,           $KEY_B);
  $.Input.BindControl(CONTROL_CONFIRM,               $KEY_SPACE);
  $.Input.BindControl(CONTROL_CANCEL,                $KEY_TAB);
  $.Input.BindControl(CONTROL_SECRET,                $KEY_P);
  $.Input.BindControl(CONTROL_OPTION_1,              $KEY_1);
  $.Input.BindControl(CONTROL_OPTION_2,              $KEY_2);

}

void $Start()
{
  FRAME_COUNT = 0;

  $.Bitmap.Load(&ART, "art.png");
  $.Bitmap.Load(&TITLE, "title.png");
  $.Font.New(&FONT, "font.png", $Rgb(0,0,255), $Rgb(255, 0, 255));

  $.Music.Play("speed_rules.mod");

  $.Surface.New(&SURFACE);
  $.Canvas.New(&HUD);

  memset(PLAYER, 0, sizeof(PLAYER));
  memset(&BALL, 0, sizeof(BALL));
  memset(&LOCAL, 0, sizeof(LOCAL));
  memset(&SCENES, 0, sizeof(SCENES));

  BALL.obj.type = OT_BALL;
  BALL.obj.position.FORWARD = 0;
  BALL.magnet = 255;

  GAME_STATE = GAME_STATE_TITLE;
}

void $Update()
{
  FRAME_COUNT++;
  
  if (GAME_STATE == GAME_STATE_SINGLE)
    FixedUpdate_Singleplayer();
  else if (GAME_STATE == GAME_STATE_MULTI)
    FixedUpdate_Multiplayer();
}

void Update_SinglePlayerSetup();
void Update_MultiPlayerSetup();
void Update_MultiPlayer();

void Update_Scores();

void $Draw()
{
  if (GAME_STATE == GAME_STATE_SINGLE)
    Update_SinglePlayer();
  else if (GAME_STATE == GAME_STATE_MULTI)
    Update_MultiPlayer();
  else if (GAME_STATE == GAME_STATE_TITLE)
    Update_Title();
  else if (GAME_STATE == GAME_STATE_SINGLE_SETUP)
    Update_SinglePlayerSetup();
  else if (GAME_STATE == GAME_STATE_MULTI_SETUP)
    Update_MultiPlayerSetup();
  else if (GAME_STATE == GAME_STATE_NETWORK_CONNECT)
    Update_NetworkConnect();
  else if (GAME_STATE == GAME_STATE_SCORES)
    Update_Scores();
}
