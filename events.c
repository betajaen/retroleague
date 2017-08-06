#include "funk.h"
#include <string.h>

void $Setup()
{
#if IS_STREAMING
  $.screenX = -1600;
  $.screenY = 120;
#endif

  DELTA = 1.0f / 60.0f;

  $.title = "Retro League ///";
  $.displayScale = 4;

  Mesh_MakePlayer(&MESH_PLAYER);
  Mesh_MakeBall(&MESH_BALL);

  $.Input.BindControl(CONTROL_FORWARD,            $KEY_W);
  $.Input.BindControl(CONTROL_BACKWARD,           $KEY_S);
  $.Input.BindControl(CONTROL_LEFT,               $KEY_A);
  $.Input.BindControl(CONTROL_RIGHT,              $KEY_D);
  $.Input.BindControl(CONTROL_CAMERA_LEFT,        $KEY_Q);
  $.Input.BindControl(CONTROL_CAMERA_RIGHT,       $KEY_E);
  $.Input.BindControl(CONTROL_HANDBRAKE,          $KEY_SPACE);
  $.Input.BindControl(CONTROL_AUTOPILOT,          $KEY_TAB);
  $.Input.BindControl(CONTROL_CHEAT_FLIP_180,     $KEY_X);
  $.Input.BindControl(CONTROL_POWER_PUNT,         $KEY_1);
  $.Input.BindControl(CONTROL_POWER_MAGNET,       $KEY_2);
  $.Input.BindControl(CONTROL_POWER_SPIN,         $KEY_3);
  $.Input.BindControl(SOUND_TEST,                 $KEY_C);
  
  $.Input.BindControl(CONTROL_START_SINGLE,       $KEY_A);
  $.Input.BindControl(CONTROL_START_MULTI,        $KEY_B);
}

void $Start()
{
  FRAME_COUNT = 0;

  $.Bitmap.Load(&ART, "art.png");
  $.Bitmap.Load(&TITLE, "title.png");
  $.Font.New(&FONT, "font.png", $Rgb(0,0,255), $Rgb(255, 0, 255));

  // $.Music.Play("dragon2.mod");

  $.Surface.New(&SURFACE);
  $.Scene.New(&SCENE);
  $.Canvas.New(&CANVAS);
  $Vec3_Set(&CAMERA_POSITION, 2,0,-5);

  memset(PLAYER, 0, sizeof(PLAYER));
  memset(&BALL, 0, sizeof(BALL));
  BALL.obj.type = OT_BALL;
  BALL.obj.position.FORWARD = 0;
  BALL.magnet = 255;

  $.Scene.SetPovLookAtXyz(&SCENE, 10, 10, 10, 0,0,0);
  
  GAME_STATE = GAME_STATE_TITLE;
}

void $Update()
{
  FRAME_COUNT++;

  if (GAME_STATE == GAME_STATE_SINGLE)
    Tick_Singleplayer();
  else if (GAME_STATE == GAME_STATE_MULTI)
    Tick_Multiplayer();
}

void $Draw()
{
  if (GAME_STATE == GAME_STATE_SINGLE)
    Draw_Singleplayer();
  else if (GAME_STATE == GAME_STATE_MULTI)
    Draw_Multiplayer();
  else if (GAME_STATE == GAME_STATE_TITLE)
    Draw_Title();
}
