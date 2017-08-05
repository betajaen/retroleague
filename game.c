#include "funk.h"

f32     DELTA;
Surface SURFACE;
Scene   SCENE;
Canvas  CANVAS;
Player  PLAYER[MAX_PLAYERS];
Player* ME;
Ball    BALL;
Font    FONT;
Bitmap  ART;
Mesh    MESH_PLAYER;
Mesh    MESH_BALL;
Vec3f   CAMERA_POSITION;
Vec3f   CAMERA_ROTATION;

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
