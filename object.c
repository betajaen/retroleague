#include "funk.h"
#include "float.h"

bool Object_IsAlive(Object* obj)
{
  return obj->type != OT_NONE;
}


#define PLAYER_B              1.0f
#define PLAYER_C              1.0f
#define PLAYER_WHEELBASE      (PLAYER_B + PLAYER_C)
#define PLAYER_H              1.0f
#define PLAYER_MASS           1500.0f
#define PLAYER_INERTIA        1500.0f
#define PLAYER_LENGTH         3.0f
#define PLAYER_WIDTH          1.5f
#define PLAYER_WHEEL_LENGTH   0.7f
#define PLAYER_WHEEL_WIDTH    0.3f
#define GRAVITY               9.8f
#define PLAYER_WEIGHT         (PLAYER_MASS * GRAVITY * 0.5f)
#define PLAYER_DRAG           5.0f
#define PLAYER_RESISTANCE     30.0f
#define PLAYER_CA_R           -5.20f
#define PLAYER_CA_F           -5.0f
#define PLAYER_MAX_GRIP       2.0f

inline f32 Player_GetThrottle(Player* player)
{
  i8 v = player->acceleratorBrake;
  if (v > 0)
    return v;
  return 0;
}

inline f32 Player_GetBrake(Player* player)
{
  i8 v = player->acceleratorBrake;
  if (v < 0)
    return (f32) -v;
  return 0;
}

#define FORWARD z
#define RIGHT   x

void Player_Tick(Player* player)
{
  Vec3f velocity, acceleration, lateralFrontForce, lateralRearForce, tractionForce, resistance, force;

  f32 rotAngle, sideslip, slipAngleFront, slipAngleRear, angularAcceleration, torque,
  sn = sinf(player->angle),
  cs = cosf(player->angle),
  steering = $Deg2Rad((f32) player->steering);


  velocity.FORWARD = cs * player->obj.velocity.RIGHT + sn * player->obj.velocity.FORWARD;
  velocity.RIGHT =-sn * player->obj.velocity.RIGHT + cs * player->obj.velocity.FORWARD;

  f32 yawSpeed = PLAYER_WHEELBASE * 0.5f * player->angularVelocity;

  if (ApproxZero(velocity.FORWARD))
  {
    rotAngle = 0.0f;
    sideslip = 0.0f;
  }
  else
  {
    rotAngle = atan2f(yawSpeed, velocity.FORWARD);
    sideslip = atan2f(velocity.RIGHT, velocity.FORWARD);
  }

  slipAngleFront = sideslip + rotAngle - steering;
  slipAngleRear  = sideslip - rotAngle;

  lateralFrontForce.FORWARD = 0.0f;
  lateralFrontForce.RIGHT = PLAYER_CA_F * slipAngleFront;
  lateralFrontForce.RIGHT = $Min(PLAYER_MAX_GRIP, lateralFrontForce.RIGHT);
  lateralFrontForce.RIGHT = $Max(-PLAYER_MAX_GRIP, lateralFrontForce.RIGHT);
  lateralFrontForce.RIGHT *= PLAYER_WEIGHT;
  // if (HANDBRAKE)
  //  lateralFrontForce.RIGHT *= 0.5;
  
  lateralRearForce.FORWARD = 0.0f;
  lateralRearForce.RIGHT = PLAYER_CA_R * slipAngleRear;
  lateralRearForce.RIGHT = $Min(PLAYER_MAX_GRIP, lateralRearForce.RIGHT);
  lateralRearForce.RIGHT = $Max(-PLAYER_MAX_GRIP, lateralRearForce.RIGHT);
  lateralRearForce.RIGHT *= PLAYER_WEIGHT;
  
  // if (HANDBRAKE)
  //  lateralRearForce.RIGHT *= 0.5;

  tractionForce.FORWARD = 100.0f * (Player_GetThrottle(player) - Player_GetBrake(player) * $Sign(velocity.FORWARD));
  tractionForce.RIGHT = 0;
  
  // if (HANDBRAKE)
  //  tractionForce.RIGHT *= 0.5;

  resistance.FORWARD = -( PLAYER_RESISTANCE * velocity.FORWARD + PLAYER_DRAG * velocity.FORWARD * fabsf(velocity.FORWARD));
  resistance.RIGHT = -( PLAYER_RESISTANCE * velocity.RIGHT + PLAYER_DRAG * velocity.RIGHT * fabsf(velocity.RIGHT));
  
  force.FORWARD = tractionForce.FORWARD + sinf(steering) * lateralFrontForce.FORWARD + lateralRearForce.FORWARD + resistance.FORWARD;
  force.RIGHT = tractionForce.RIGHT + cosf(steering) * lateralFrontForce.RIGHT + lateralRearForce.RIGHT + resistance.RIGHT;

  torque = PLAYER_B * lateralFrontForce.RIGHT - PLAYER_C * lateralRearForce.RIGHT;

  acceleration.FORWARD = force.FORWARD / PLAYER_MASS;
  acceleration.RIGHT = force.RIGHT / PLAYER_MASS;

  angularAcceleration = torque / PLAYER_INERTIA;

  player->obj.acceleration.FORWARD =  cs * acceleration.RIGHT + sn * acceleration.FORWARD;
  player->obj.acceleration.RIGHT = -sn * acceleration.RIGHT + cs * acceleration.FORWARD;

  player->obj.velocity.FORWARD += DELTA * acceleration.FORWARD;
  player->obj.velocity.RIGHT += DELTA * acceleration.RIGHT;

  player->obj.position.FORWARD += DELTA * player->obj.velocity.FORWARD;
  player->obj.position.RIGHT += DELTA * player->obj.velocity.RIGHT;

  player->angularVelocity += DELTA * angularAcceleration;

  player->angle += DELTA * player->angularVelocity;

  player->obj.yaw = (i16) $Rad2Deg(player->angle);

}
