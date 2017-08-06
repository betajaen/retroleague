#include "funk.h"
#include "float.h"

bool Object_IsAlive(Object* obj)
{
  return obj->type != OT_NONE;
}

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

void Player_TickPhysics(Player* player)
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

	//  finally we can update position
	player->obj.position.FORWARD += player->obj.velocity.FORWARD * DELTA;
	player->obj.position.RIGHT += player->obj.velocity.RIGHT * DELTA;

}

void Player_TickBallCollision(Player* player, Ball* ball)
{
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
#define AI_STATE_SHOOT             2

#define PID_THROTTLE_BRAKE    a
#define PID_STEERING b

#if 1
#define PID_DEBUG(T, ...) printf(T, __VA_ARGS__)
#else
#define PID_DEBUG(T, ...)
#endif

void Player_AI_StartMoveTowardsBall(Player* player, Ball* ball)
{
  Pid* throttle = &player->ai.PID_THROTTLE_BRAKE;
  Pid* steering = &player->ai.PID_STEERING;

  MakePidDefaults1(throttle);
  MakePidDefaults1(steering);
  steering->min = -$Deg2Rad(80.0f);
  steering->max = $Deg2Rad(80.0f);
}

void Player_AI_TickMoveTowardsBall(Player* player, Ball* ball)
{
  Pid* throttle = &player->ai.PID_THROTTLE_BRAKE;
  Pid* steering = &player->ai.PID_STEERING;
  
  Vec3f playerPos     = player->obj.position;
  f32   playerHeading = player->heading;

  Vec3f ballPosWorld  = BALL.obj.position;
  Vec3f ballPosLocal  = TransformWorldPointToLocalSpaceXZRad(playerPos, playerHeading, ballPosWorld);

  Vec3f ballDirection = $Vec3_Normalise(ballPosLocal);
  Vec3f forward       = $Vec3_Xyz(0,0,1);

  f32   dot           = $Vec3_DotXZ(ballDirection, forward);

  f32   angle          = acosf(dot);

  f32   atanf2_angle   = atan2f((forward.z - ballDirection.z), (forward.x - ballDirection.x));

  f32   sign = $SignF(($Rad2Deg(atanf2_angle) - 90.0f));

  f32   signedAngle = angle * sign * -1.0f;

  f32 error = PidError(0.0f, signedAngle);
  f32 newSteering = UpdatePid(steering, error, $.fixedDeltaTime);
  player->steering += newSteering * $.fixedDeltaTime;
  PID_DEBUG("Dot-%.2f, Error= %.2f, Steering=%.2f, Angle=%.2f Signed=%.2f  ATan2=%.2f", dot, error, newSteering, $Rad2Deg(angle), $Rad2Deg(signedAngle), $Rad2Deg(atanf2_angle) - 90.0f);
}

void Player_AI_StartShootState(Player* player, Ball* ball)
{
}

void Player_AI_TickShootState(Player* player, Ball* ball)
{
}

void Player_TickAI(Player* player, Ball* ball)
{
  PID_DEBUG("[%i]", player->ai.state);

  switch(player->ai.state)
  {
    default:
    case AI_STATE_NONE:
    {
      Vec3f direction = $Vec3_Sub(ball->obj.position, player->obj.position);
      f32 distance  = $Vec3_Length(direction);
      direction = $Vec3_Normalise(direction);

      if (distance < 3.0f)
      {
        player->ai.state = AI_STATE_SHOOT;
        Player_AI_StartShootState(player, ball);
      }
      else
      {
        player->ai.state = AI_STATE_MOVE_TOWARDS_BALL;
        Player_AI_StartMoveTowardsBall(player, ball);
      }
    }
    break;
    case AI_STATE_MOVE_TOWARDS_BALL:
    {
      Player_AI_TickMoveTowardsBall(player, ball);
    }
    break;
    case AI_STATE_SHOOT:
    {
    
    }
    break;
  }

  #if (1)
    printf("\n");
  #endif
}

void Player_Tick(Player* player)
{
  
  if (player->autopilot)
  {
    Player_TickAI(player, &BALL);
  }
  else if (player == ME)
  {
      Vec3f playerPos     = player->obj.position;
      f32   playerHeading = player->heading;

      Vec3f ballPosWorld  = BALL.obj.position;
      Vec3f ballPosLocal  = TransformWorldPointToLocalSpaceXZRad(playerPos, playerHeading, ballPosWorld);

      Vec3f ballDirection = $Vec3_Normalise(ballPosLocal);
      Vec3f forward       = $Vec3_Xyz(0,0,1);

      f32   dot           = $Vec3_DotXZ(ballDirection, forward);

      printf("[-] %.1f %.1f  %.1f %.1f = %.2f\n", ballDirection.x, ballDirection.z, forward.x, forward.z, dot);













  #if 0
//      Vec3f car = player->obj.position;
      Vec3f ball = BALL.obj.position;

      Vec3f fwd;
      fwd.z = 1.0;
      fwd.x = 0.0;
      fwd.y = 0.0;
      
      Mat44 rotMatrix;
      $Mat44_RotMatrixY(&rotMatrix, $Rad2Deg(player->heading));

      Vec4f fwd4;
      fwd4.x = fwd.x;
      fwd4.y = fwd.y;
      fwd4.z = fwd.z;
      fwd4.w = 1.0f;

      Vec4f changedFwd;
      $Mat44_MultiplyVec4(&changedFwd, &rotMatrix, &fwd4);
      
      fwd.x = changedFwd.x;
      fwd.z = changedFwd.z;

      Vec3f c = $Vec3_Cross(fwd, ball);

      f32 angle = c.y;

      if (angle > 0)
        printf("[-] LEFT=%.1f HED = %.1f  FWD %.1f %.1f\n", angle, $Rad2Deg(player->heading), fwd.x, fwd.z);
      else
        printf("[-] RIGH=%.1f HED = %.1f  FWD %.1f %.1f\n", angle, $Rad2Deg(player->heading), fwd.x, fwd.z);
        #endif
    }
  
    Player_TickBallCollision(player, &BALL);
    Player_TickPhysics(player);
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
  }
  else if (ball->obj.position.x > BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.x = BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.x = -ball->obj.velocity.x;
    ball->obj.acceleration.x = -ball->obj.acceleration.x;
  }
  
  if (inGoalArea == false && ball->obj.position.z < -BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.z = -BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.z = -ball->obj.velocity.z;
    ball->obj.acceleration.z = -ball->obj.acceleration.z;
  }
  else if (inGoalArea == false && ball->obj.position.z > BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.z = BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.z = -ball->obj.velocity.z;
    ball->obj.acceleration.z = -ball->obj.acceleration.z;
  }

  if (inGoalArea && ball->obj.position.z < -BOUNDS_SIZE_HALF_F)
  {
    printf("Blue Scored\n");
    ball->blue++;
    ball->obj.position = $Vec3_Xyz(0,0,0);
    ball->obj.velocity = $Vec3_Xyz(0,0,0);
    ball->obj.acceleration = $Vec3_Xyz(0,0,0);
  }
  else if (inGoalArea && ball->obj.position.z > +BOUNDS_SIZE_HALF_F)
  {
    printf("Red Scored\n");
    ball->red++;
    ball->obj.position = $Vec3_Xyz(0,0,0);
    ball->obj.velocity = $Vec3_Xyz(0,0,0);
    ball->obj.acceleration = $Vec3_Xyz(0,0,0);
  }

  ball->obj.velocity.FORWARD    += ball->obj.acceleration.FORWARD * DELTA;
  ball->obj.velocity.RIGHT      += ball->obj.acceleration.RIGHT * DELTA;
  
  ball->obj.position.FORWARD    += ball->obj.velocity.FORWARD * DELTA;
  ball->obj.position.RIGHT      += ball->obj.velocity.RIGHT * DELTA;
  
  ball->obj.velocity.FORWARD    *= 0.95f;
  ball->obj.velocity.RIGHT      *= 0.95f;
  

  ball->obj.acceleration.FORWARD = 0;
  ball->obj.acceleration.RIGHT   = 0;
}
