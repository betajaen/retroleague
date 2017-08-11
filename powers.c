#include "funk.h"

const char* POWERS_NAME[MAX_POWERS] = {
  "KICK",
  "MAGNET",
  "SPIN"
};

bool Can_Power(Player* player, u32 power)
{
  bool canPower = (player->powerAvailable & (1 << power)) != 0;

  if (power == POWER_MAGNET)
    return BALL.magnet == 255 && canPower;

  return canPower;
}

void Activate_Power(Player* player, u32 power)
{
  player->powerControls |= (1 << power);
}

void Power_Activate_Punt(Player* player, Ball* ball)
{
  
  Vec3f ballPos = ball->obj.position;
  f32 distance = $Vec3_Length($Vec3_Sub(player->obj.position, ballPos));
  
  if (distance > 6.0f)
  {
    player->powerAvailable |= POWER_BIT_KICK;
    player->powerCooldown[POWER_KICK] = 0.0f;
    return;
  }
  
  player->powerCooldown[POWER_KICK] = 1.0f;

  Vec3f tgt;
  tgt.x = 0.0f;
  tgt.y = 0.0f;

  if (player->team == 0)
    tgt.z = BOUNDS_SIZE_HALF_F;
  else if (player->team == 1)
    tgt.z = -BOUNDS_SIZE_HALF_F;

  Vec3f direction = $Vec3_Sub(tgt, ballPos);
  direction = $Vec3_Normalise(direction);

  f32 force = 80.0f;
  ball->obj.velocity = $Vec3_Add(ball->obj.velocity, $Vec3_MulS(direction, force));

  #if 0
  AnimateMoveXZ(&player->anim,
    player->obj.position,
    $Vec3_Add(ballPos, $Vec3_MulS(direction, 10.0f)),
    player->heading,
    player->heading,
    8.0f,
    1.0f
  );
  #endif

}

void Power_Activate_Magnet(Player* player, Ball* ball)
{
  player->powerCooldown[POWER_MAGNET] = 5.0f;

  Vec3f ballPos = ball->obj.position;
  f32 distance = $Vec3_Length($Vec3_Sub(player->obj.position, ballPos));
  
  if (distance > 6.0f)
  {
    player->powerAvailable |= POWER_BIT_MAGNET;
    player->powerCooldown[POWER_MAGNET] = 0.0f;
    return;
  }
  
  player->powerCooldown[POWER_MAGNET] = 10.0f;

  ball->magnet     = FindPlayerIndex(player);
  ball->magnetTime = 4.0f;
}

void Power_Activate_Spin(Player* player, Ball* ball)
{
  player->powerCooldown[POWER_SPIN] = 2.0f;
  
  Vec3f ballPos = ball->obj.position;
  f32 distance = $Vec3_Length($Vec3_Sub(player->obj.position, ballPos));
  
  if (distance > 6.0f)
  {
    player->powerAvailable |= POWER_BIT_SPIN;
    player->powerCooldown[POWER_SPIN] = 0.0f;
    return;
  }
  
  player->powerCooldown[POWER_SPIN] = 1.0f;

  Vec3f tgt;
  tgt.x = 0.0f;
  tgt.y = 0.0f;

  if (player->team == 0)
    tgt.z = BOUNDS_SIZE_HALF_F;
  else if (player->team == 1)
    tgt.z = -BOUNDS_SIZE_HALF_F;

  Vec3f direction = player->obj.velocity;
  direction = $Vec3_Normalise(direction);

  f32 force = 400.0f;
  ball->obj.velocity = $Vec3_Add(ball->obj.velocity, $Vec3_MulS(direction, force));
  /*
  player->carVelocity = $Vec3_Xyz(0,0,0);
  player->obj.velocity = $Vec3_Xyz(0,0,0);
  player->absVelocity = 0;
  player->carAcceleration = $Vec3_Xyz(0,0,0);
  player->obj.acceleration = $Vec3_Xyz(0,0,0);
  */
  AnimateSpin(&player->anim,
    $Deg2Rad(25),
    player->heading,
    1.5f 
  );


}
