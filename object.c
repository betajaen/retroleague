#include "funk.h"
#include "float.h"

#define PLAYER_gravity 9.81f  // m/s^2
#define PLAYER_mass  1200.0f  // kg
#define PLAYER_inertiaScale  1.0f  // Multiply by mass for inertia
#define PLAYER_halfWidth  0.8f // Centre to side of chassis (metres)
#define PLAYER_cgToFront 2.0f // Centre of gravity to front of chassis (metres)
#define PLAYER_cgToRear  2.0f   // Centre of gravity to rear of chassis
#define PLAYER_cgToFrontAxle 1.25f  // Centre gravity to front axle
#define PLAYER_cgToRearAxle  1.25f  // Centre gravity to rear axle
#define PLAYER_cgHeight 0.55f  // Centre gravity height
#define PLAYER_wheelRadius  0.3f  // Includes tire (also represents height of axle)
#define PLAYER_wheelWidth  0.2f  // Used for render only
#define PLAYER_tireGrip 2.0f  // How much grip tires have
#define PLAYER_lockGrip 0.7f  // % of grip available when wheel is locked
#define PLAYER_engineForce 8000.0f
#define PLAYER_brakeForce 12000.0f
#define PLAYER_eBrakeForce PLAYER_brakeForce / 2.5f
#define PLAYER_weightTransfer 0.2f  // How much weight is transferred during acceleration/braking
#define PLAYER_maxSteer 0.6f  // Maximum steering angle in radians
#define PLAYER_cornerStiffnessFront 5.0f
#define PLAYER_cornerStiffnessRear 5.2f
#define PLAYER_airResist 2.5f	// air resistance (* vel)
#define PLAYER_rollResist 8.0f	// rolling resistance force (* vel)


#define PLAYER_inertia PLAYER_mass * PLAYER_inertiaScale
#define PLAYER_wheelBase PLAYER_cgToFrontAxle + PLAYER_cgToRearAxle
#define PLAYER_axleWeightRatioFront  PLAYER_cgToRearAxle / PLAYER_wheelBase // % car weight on the front axle
#define PLAYER_axleWeightRatioRear PLAYER_cgToFrontAxle / PLAYER_wheelBase // % car weight on the rear axle


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
  {
    return (f32) -v;
  }
  return 0;
}

inline f32 Player_GetHandBrake(Player* player)
{
  i8 v = player->handBrake;
  if (v)
    return 1.0f;
  return 0.0f;
}

void Player_TickPhysics(Player* player, bool isAnimating)
{
  //  Vec3f velocity, acceleration; //, lateralFrontForce, lateralRearForce, traction, drag, force;

  //f32 rotAngle, sideslip, slipAngleFront, slipAngleRear, angularAcceleration, torque,
  f32 sn = sinf(player->heading),
  cs = cosf(player->heading),
  steerAngle = (f32) player->steering;

  steerAngle = $Clamp(steerAngle, -PLAYER_maxSteer, PLAYER_maxSteer);
  player->steering = steerAngle;

	// Get velocity in local car coordinates
	player->carVelocity.FORWARD = cs * player->obj.velocity.FORWARD + sn * player->obj.velocity.RIGHT;
	player->carVelocity.RIGHT   = cs * player->obj.velocity.RIGHT   - sn * player->obj.velocity.FORWARD;

	// Weight on axles based on centre of gravity and weight shift due to forward/reverse acceleration
	f32 axleWeightFront = PLAYER_mass * (PLAYER_axleWeightRatioFront * PLAYER_gravity - PLAYER_weightTransfer * player->carAcceleration.FORWARD * PLAYER_cgHeight / PLAYER_wheelBase);
	f32 axleWeightRear = PLAYER_mass * (PLAYER_axleWeightRatioRear * PLAYER_gravity + PLAYER_weightTransfer * player->carAcceleration.FORWARD * PLAYER_cgHeight / PLAYER_wheelBase);

	// Resulting velocity of the wheels as result of the yaw rate of the car body.
	// v = yawrate * r where r is distance from axle to CG and yawRate (angular velocity) in rad/s.
	f32 yawSpeedFront = PLAYER_cgToFrontAxle * player->yawRate;
	f32 yawSpeedRear = -PLAYER_cgToRearAxle * player->yawRate;

	// Calculate slip angles for front and rear wheels (a.k.a. alpha)
	f32 slipAngleFront = atan2f(player->carVelocity.RIGHT + yawSpeedFront, fabsf(player->carVelocity.FORWARD)) - $Sign(player->carVelocity.FORWARD) * steerAngle;
	f32 slipAngleRear  = atan2f(player->carVelocity.RIGHT + yawSpeedRear,  fabsf(player->carVelocity.FORWARD));

	f32 tireGripFront = PLAYER_tireGrip;
	f32 tireGripRear = PLAYER_tireGrip * (1.0f - Player_GetHandBrake(player)); // * (1.0 - this.inputs.ebrake * (1.0 - PLAYER_lockGrip)); // reduce rear grip when ebrake is on

	f32 frictionForceFront_cy = $Clamp(-PLAYER_cornerStiffnessFront * slipAngleFront, -tireGripFront, tireGripFront) * axleWeightFront;
	f32 frictionForceRear_cy = $Clamp(-PLAYER_cornerStiffnessRear * slipAngleRear, -tireGripRear, tireGripRear) * axleWeightRear;

	//  Get amount of brake/throttle from our inputs
	f32 brake = $Max(Player_GetBrake(player), Player_GetHandBrake(player)) * 100.0f; // $Min(this.inputs.brake * PLAYER_brakeForce + this.inputs.ebrake * PLAYER_eBrakeForce, PLAYER_brakeForce);
	f32 throttle = Player_GetThrottle(player) * 100.0f; //this.inputs.throttle * PLAYER_engineForce;


	//  Resulting force in local car coordinates.
	//  This is implemented as a RWD car only.
	f32 tractionForce_cx = throttle - brake * $Sign(player->carVelocity.FORWARD);
	f32 tractionForce_cy = 0;
  
	f32 dragForce_cx = -PLAYER_rollResist * player->carVelocity.FORWARD - PLAYER_airResist * player->carVelocity.FORWARD * fabsf(player->carVelocity.FORWARD);
	f32 dragForce_cy = -PLAYER_rollResist * player->carVelocity.RIGHT - PLAYER_airResist * player->carVelocity.RIGHT * fabsf(player->carVelocity.RIGHT);
  
	// total force in car coordinates
	f32 totalForce_cx = dragForce_cx + tractionForce_cx;
  f32 css_sin = sinf(steerAngle);
  f32 css_cos = cosf(steerAngle);

  f32 css = sinf(steerAngle) * frictionForceFront_cy;
	f32 totalForce_cy = dragForce_cy + tractionForce_cy + css + frictionForceRear_cy;

	// acceleration along car axes
	player->carAcceleration.FORWARD = totalForce_cx / PLAYER_mass;  // forward/reverse accel
	player->carAcceleration.RIGHT = totalForce_cy / PLAYER_mass;  // sideways accel

	// acceleration in world coordinates
	player->obj.acceleration.FORWARD = cs * player->carAcceleration.FORWARD - sn * player->carAcceleration.RIGHT;
	player->obj.acceleration.RIGHT = sn * player->carAcceleration.FORWARD + cs * player->carAcceleration.RIGHT;

	// update velocity
	player->obj.velocity.FORWARD += player->obj.acceleration.FORWARD * DELTA;
	player->obj.velocity.RIGHT += player->obj.acceleration.RIGHT * DELTA;

	player->absVelocity = $Vec3_Length(player->obj.velocity); //.len();

	// calculate rotational forces
	f32 angularTorque = (frictionForceFront_cy + tractionForce_cy) * PLAYER_cgToFrontAxle - frictionForceRear_cy * PLAYER_cgToRearAxle;

	//  Sim gets unstable at very slow speeds, so just stop the car
	if( fabsf(player->absVelocity) < 0.5 && !throttle )
	{
		player->obj.velocity.FORWARD = player->obj.velocity.RIGHT = player->absVelocity = 0;
		angularTorque = player->yawRate = 0;
	}

	f32 angularAccel = angularTorque / PLAYER_inertia;

	player->yawRate += angularAccel * DELTA;
	player->heading += player->yawRate * DELTA;

  player->obj.yaw = (i16) ($Rad2Deg(-player->heading));

  if (isAnimating == false)
  {
	  //  finally we can update position
	  player->obj.position.FORWARD += player->obj.velocity.FORWARD * DELTA;
	  player->obj.position.RIGHT += player->obj.velocity.RIGHT * DELTA;
  }
}

void Player_TickBallCollision(Player* player, Ball* ball)
{
  if (ball->magnet != 255)
    return;

  Vec3f localPoint = TransformWorldPointToLocalSpaceXZ(player->obj.position, player->obj.yaw, ball->obj.position);

  /*
    printf("%.1f %.1f | %.1f %.1f => %.1f/%.1f\n", 
    ball->obj.position.x, ball->obj.position.z,
    localPoint.x, localPoint.z, 
    $Vec3_Length(localPoint),
    $Vec3_Length($Vec3_Sub(player->obj.position, ball->obj.position)) 
    );
  */

  // @TODO

  IntersectPointResult result = { 0 };

  if (IntersectPointXZRadius($Vec3_Xyz(0,0,0), MESH_PLAYER.halfSize, MESH_PLAYER.min, MESH_PLAYER.max, localPoint, 2.56f, &result))
  {
    
    #if 0
    Vec3f worldNormal = TransformWorldPointToLocalSpaceXZ($Vec3_Xyz(0,0,0), player->obj.yaw, result.normal);

    BALL.obj.acceleration.x += worldNormal.x * 3000.0f;
    BALL.obj.acceleration.z += worldNormal.z * 3000.0f;
    #else
    Vec3f normalisedVelocity = $Vec3_Normalise(player->obj.velocity); // $Vec3_Mul(player->obj.velocity, worldNormal));
    BALL.obj.acceleration.x += normalisedVelocity.x * 4000.0f;
    BALL.obj.acceleration.z += normalisedVelocity.z * 4000.0f;
    #endif

  }
}

#define AI_STATE_NONE              0
#define AI_STATE_MOVE_TOWARDS_BALL 1
#define AI_STATE_POWER_MAGNET      2
#define AI_STATE_POWER_PUNT        3
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
  i8 throttleAbs = $Clamp((i8) newThrottle, 0, 100);
  player->acceleratorBrake = throttleAbs;
}

void Player_AI_StartMoveTowardsBall(Player* player, Ball* ball)
{
  player->ai.state = AI_STATE_MOVE_TOWARDS_BALL;

  Pid* throttle = &player->ai.PID_THROTTLE_BRAKE;
  Pid* steering = &player->ai.PID_STEERING;

  MakePidDefaults1(throttle);
  throttle->min = 0;
  throttle->max = 100.0f;
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
  player->ai.state = AI_STATE_POWER_PUNT;
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
  Player_AI_UpdateThrottle(player, 25.0f);
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
    
    if (Can_Power(player, POWER_MAGNET))
    {
      Activate_Power(player, POWER_MAGNET);
      Player_AI_StartMagnetState(player, ball);
    }
    else if (Can_Power(player, POWER_SPIN))
    {
      Activate_Power(player, POWER_SPIN);
    }
    else if (Can_Power(player, POWER_PUNT))
    {
      Activate_Power(player, POWER_PUNT);
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
    case AI_STATE_POWER_PUNT:
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

const char* POWERS_NAME[MAX_POWERS] = {
  "PUNT",
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
    player->powerAvailable |= POWER_BIT_PUNT;
    player->powerCooldown[POWER_PUNT] = 0.0f;
    return;
  }
  
  player->powerCooldown[POWER_PUNT] = 1.0f;

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
  printf("** Magnet\n");
  
  Vec3f ballPos = ball->obj.position;
  f32 distance = $Vec3_Length($Vec3_Sub(player->obj.position, ballPos));
  
  if (distance > 6.0f)
  {
    player->powerAvailable |= POWER_BIT_MAGNET;
    player->powerCooldown[POWER_MAGNET] = 0.0f;
    return;
  }
  
  player->powerCooldown[POWER_MAGNET] = 30.0f;

  ball->magnet     = FindPlayerIndex(player);
  ball->magnetTime = 15.0f;
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

  player->carVelocity = $Vec3_Xyz(0,0,0);
  player->obj.velocity = $Vec3_Xyz(0,0,0);
  player->absVelocity = 0;
  player->carAcceleration = $Vec3_Xyz(0,0,0);
  player->obj.acceleration = $Vec3_Xyz(0,0,0);

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

void Player_Tick(Player* player)
{
  
  for(u32 ii=0;ii < MAX_POWERS;ii++)
  {
    player->powerCooldown[ii] -= $.fixedDeltaTime;
    if (player->powerCooldown[ii] <= 0.0f && Can_Power(player, ii) == false)
    {
      printf("** Power Available: %s\n", POWERS_NAME[ii]);
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
        case POWER_PUNT:   Power_Activate_Punt(player, &BALL); break;
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
  
}

#define BALL_MASS 1200

void Ball_Tick(Ball* ball)
{

  bool inGoalArea = (ball->obj.position.x >= -GOAL_SIZE_XH_F && ball->obj.position.x <= GOAL_SIZE_XH_F);


  if (ball->obj.position.x < -BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.x = -BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.x = -ball->obj.velocity.x;
    ball->obj.acceleration.x = -ball->obj.acceleration.x;
    ball->magnet = 255;
  }
  else if (ball->obj.position.x > BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.x = BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.x = -ball->obj.velocity.x;
    ball->obj.acceleration.x = -ball->obj.acceleration.x;
    ball->magnet = 255;
  }
  
  if (inGoalArea == false && ball->obj.position.z < -BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.z = -BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.z = -ball->obj.velocity.z;
    ball->obj.acceleration.z = -ball->obj.acceleration.z;
    ball->magnet = 255;
  }
  else if (inGoalArea == false && ball->obj.position.z > BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.z = BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.z = -ball->obj.velocity.z;
    ball->obj.acceleration.z = -ball->obj.acceleration.z;
    ball->magnet = 255;
  }

  if (inGoalArea && ball->obj.position.z < -BOUNDS_SIZE_HALF_F)
  {
    printf("Blue Scored\n");
    ball->blue++;
    ball->obj.position = $Vec3_Xyz(0,0,0);
    ball->obj.velocity = $Vec3_Xyz(0,0,0);
    ball->obj.acceleration = $Vec3_Xyz(0,0,0);
    ball->magnet = 255;
  }
  else if (inGoalArea && ball->obj.position.z > +BOUNDS_SIZE_HALF_F)
  {
    printf("Red Scored\n");
    ball->red++;
    ball->obj.position = $Vec3_Xyz(0,0,0);
    ball->obj.velocity = $Vec3_Xyz(0,0,0);
    ball->obj.acceleration = $Vec3_Xyz(0,0,0);
    ball->magnet = 255;
  }

  if (ball->magnet == 255)
  {
    ball->obj.velocity.FORWARD    += ball->obj.acceleration.FORWARD * DELTA;
    ball->obj.velocity.RIGHT      += ball->obj.acceleration.RIGHT * DELTA;
  
    ball->obj.position.FORWARD    += ball->obj.velocity.FORWARD * DELTA;
    ball->obj.position.y           = 0;
    ball->obj.position.RIGHT      += ball->obj.velocity.RIGHT * DELTA;
  
    ball->obj.velocity.FORWARD    *= 0.95f;
    ball->obj.velocity.RIGHT      *= 0.95f;
  

    ball->obj.acceleration.FORWARD = 0;
    ball->obj.acceleration.RIGHT   = 0;
  }
  else
  {
    Player* player = &PLAYER[ball->magnet];
    
    Vec3f fwd;
    fwd.FORWARD = 3.5f;
    fwd.RIGHT = 0;
    fwd.UP = 0;

    fwd = TransformLocalPointToWorldSpaceXZRad(player->obj.position, player->heading, fwd);

    ball->obj.velocity.FORWARD    = 0;
    ball->obj.velocity.RIGHT      = 0;
    ball->obj.position.FORWARD    = fwd.FORWARD;
    ball->obj.position.y          = 0;
    ball->obj.position.RIGHT      = fwd.RIGHT;
    ball->obj.yaw                 = (ball->obj.yaw + 3) % 360;

    ball->magnetTime -= $.fixedDeltaTime;
    if (ball->magnetTime <= 0.0f)
    {
      ball->magnet = 255;
    }

  }
}
