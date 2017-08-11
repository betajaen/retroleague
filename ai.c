#include "funk.h"


#define AI_STATE_NONE              0
#define AI_STATE_MOVE_TOWARDS_BALL 1
#define AI_STATE_POWER_MAGNET      2
#define AI_STATE_POWER_KICK        3
#define AI_STATE_POWER_SPIN        4

#define PID_THROTTLE_BRAKE    a
#define PID_STEERING b

#if 0
#define PID_DEBUG(T, ...) printf(T, __VA_ARGS__)
#else
#define PID_DEBUG(T, ...)
#endif

void Player_AI_UpdateSteering(Player* player, Vec3f target)
{
  Pid* steering = &player->ai.PID_STEERING;

  Vec3f playerPos     = player->obj.position;
  f32   playerHeading = player->heading;

  Vec3f targetLocal   = TransformWorldPointToLocalSpaceXZRad(playerPos, playerHeading, target);

  Vec3f targetDirection = $Vec3_Normalise(targetLocal );
  Vec3f forward         = $Vec3_Xyz(0,0,1);

  f32   dot           = $Vec3_DotXZ(targetDirection, forward);

  f32   angle          = acosf(dot);

  f32   atanf2_angle   = atan2f((forward.z - targetDirection.z), (forward.x - targetDirection.x));

  f32   sign = $SignF(($Rad2Deg(atanf2_angle) - 90.0f));

  f32   signedAngle = angle * sign * -1.0f;

  f32 steeringError = PidError(0.0f, signedAngle);
  f32 newSteering   = UpdatePid(steering, steeringError, $.fixedDeltaTime);
  player->steering += newSteering;
}

void Player_AI_UpdateThrottle(Player* player, f32 targetSpeed)
{

  Pid* throttle = &player->ai.PID_THROTTLE_BRAKE;
  f32 speed = player->absVelocity;

  f32 speedError  = PidError(targetSpeed, speed);
  f32 newThrottle = UpdatePid(throttle, speedError, $.fixedDeltaTime);
  i8 throttleAbs = $Clamp((i8) newThrottle, 0, 50) * 2;
  player->acceleratorBrake = throttleAbs;
}

void Player_AI_StartMoveTowardsBall(Player* player, Ball* ball)
{
  player->ai.state = AI_STATE_MOVE_TOWARDS_BALL;

  Pid* throttle = &player->ai.PID_THROTTLE_BRAKE;
  Pid* steering = &player->ai.PID_STEERING;

  MakePid(throttle, 1.0f, 0.01f, 0.001f); // 1.0f, 0.01f, 0.001f
  throttle->min = 25.0f;
  throttle->max = 50.0f;
  MakePidDefaults1(steering);
  steering->min = -$Deg2Rad(80.0f);
  steering->max = $Deg2Rad(80.0f);
}

void Player_AI_TickMoveTowardsBall(Player* player, Ball* ball)
{
  Player_AI_UpdateSteering(player, BALL.obj.position);
  Player_AI_UpdateThrottle(player, 25.0f);
}

void Player_AI_StartPuntState(Player* player, Ball* ball)
{
  player->ai.state = AI_STATE_POWER_KICK;
}

void Player_AI_TickPuntState(Player* player, Ball* ball)
{
}

void Player_AI_StartMagnetState(Player* player, Ball* ball)
{
  player->ai.state = AI_STATE_POWER_MAGNET;
  
  Pid* throttle = &player->ai.PID_THROTTLE_BRAKE;
  Pid* steering = &player->ai.PID_STEERING;

  MakePidDefaults1(throttle);
  throttle->min = 0;
  throttle->max = 100.0f;
  MakePidDefaults1(steering);
  steering->min = -$Deg2Rad(80.0f);
  steering->max = $Deg2Rad(80.0f);
}

void Player_AI_TickMagnetState(Player* player, Ball* ball)
{
  Vec3f tgt;
  tgt.x = 0.0f;
  tgt.y = 0.0f;

  if (player->team == 0)
    tgt.z = BOUNDS_SIZE_HALF_F;
  else if (player->team == 1)
    tgt.z = -BOUNDS_SIZE_HALF_F;

  Player_AI_UpdateSteering(player, tgt);
  Player_AI_UpdateThrottle(player, 40.0f);
}

void Player_AI_StartSpinState(Player* player, Ball* ball)
{
  player->ai.state = AI_STATE_POWER_SPIN;
}

void Player_AI_TickSpinState(Player* player, Ball* ball)
{
}

void Player_AI_Resolve(Player* player, Ball* ball)
{
  if (player->ai.state == AI_STATE_POWER_MAGNET)
  {
    if (ball->magnet == 255)
    {
      Player_AI_StartMoveTowardsBall(player, ball);
    }
  }

  f32 ballDistance = $Vec3_Length($Vec3_Sub(player->obj.position, ball->obj.position));

  if (ballDistance <= 6)
  {
    // @TODO power here.
    //  Dice rolls for powers.
    //  Magnetic and spin are more difficult to roll for.
    
    // int playerIndex = FindPlayerIndex(player);

    if (ball->magnet == 255)
    {
      if (Can_Power(player, POWER_MAGNET))
      {
        Activate_Power(player, POWER_MAGNET);
        Player_AI_StartMagnetState(player, ball);
      }
      else if (Can_Power(player, POWER_SPIN))
      {
        Activate_Power(player, POWER_SPIN);
      }
      else if (Can_Power(player, POWER_KICK))
      {
        Activate_Power(player, POWER_KICK);
      }
    }

  }
}

void Player_TickAI(Player* player, Ball* ball)
{
  PID_DEBUG("[%i]", player->ai.state);

  switch(player->ai.state)
  {
    default:
    case AI_STATE_NONE:
    {
      Player_AI_StartMoveTowardsBall(player, ball);
    }
    break;
    case AI_STATE_MOVE_TOWARDS_BALL:
    {
      Player_AI_TickMoveTowardsBall(player, ball);
    }
    break;
    case AI_STATE_POWER_KICK:
    {
      Player_AI_TickPuntState(player, &BALL);
    }
    break;
    case AI_STATE_POWER_MAGNET:
    {
      Player_AI_TickMagnetState(player, &BALL);
    }
    break;
    case AI_STATE_POWER_SPIN:
    {
      Player_AI_TickSpinState(player, &BALL);
    }
    break;
  }

  Player_AI_Resolve(player, &BALL);

  #if (0)
    printf("\n");
  #endif
}