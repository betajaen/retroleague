#include "funk.h"
#include "float.h"
#include <string.h>
#include <stdlib.h>

void Player_TickBallCollision(Player* player, Ball* ball);
void Player_TickPhysics(Player* player, bool is_animating);
void Tick_PhysicsCollisions(Player* player);
void Power_Activate_Punt(Player* player, Ball* ball);
void Power_Activate_Magnet(Player* player, Ball* ball);
void Power_Activate_Spin(Player* player, Ball* ball);
void Player_TickAI(Player* player, Ball* ball);


bool Object_IsAlive(Object* obj)
{
  return obj->type != OT_NONE;
}

u8  FindPlayerIndex(Player* player)
{
  for(u32 ii=0;ii < MAX_PLAYERS;ii++)
  {
    if (player == &PLAYER[ii])
      return ii;
  }
  
  return 255;
}

void Player_Tick(Player* player)
{
  
  for(u32 ii=0;ii < MAX_POWERS;ii++)
  {
    player->powerCooldown[ii] -= $.fixedDeltaTime;
    if (player->powerCooldown[ii] <= 0.0f && Can_Power(player, ii) == false)
    {
    //  printf("** Power Available: %s\n", POWERS_NAME[ii]);
      player->powerAvailable |= 1 << ii;
    }
  }

  for(u32 ii=0;ii < MAX_POWERS;ii++)
  {
    if ((player->powerControls & (1 << ii)) != 0)
    {
      player->powerControls  &= ~(1 << ii);
      player->powerAvailable &= ~(1 << ii);
      switch(ii)
      {
        case POWER_KICK:   Power_Activate_Punt(player, &BALL); break;
        case POWER_MAGNET: Power_Activate_Magnet(player, &BALL); break;
        case POWER_SPIN:   Power_Activate_Spin(player, &BALL); break;
      }
    }
  }
  bool isAnimating = IsAnimating(&player->anim);
  
  if (isAnimating == false)
  {
    if (player->autopilot)
    {
      Player_TickAI(player, &BALL);
    }
  
    Player_TickBallCollision(player, &BALL);
  }
  else
  {
    Animate_Tick(&player->anim, &player->obj.position, &player->heading);
    player->obj.yaw = (i16) player->heading;
  }

  Player_TickPhysics(player, isAnimating);
  Tick_PhysicsCollisions(player);
  
}

#define BALL_MASS 1200
