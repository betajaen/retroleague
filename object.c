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

#define FORWARD z
#define RIGHT   x

void Player_Tick(Player* player)
{
  //  Vec3f velocity, acceleration; //, lateralFrontForce, lateralRearForce, traction, drag, force;

  //f32 rotAngle, sideslip, slipAngleFront, slipAngleRear, angularAcceleration, torque,
  f32 sn = sinf(player->heading),
  cs = cosf(player->heading),
  steerAngle = $Deg2Rad((f32) player->steering * 4.0f);

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
	f32 tireGripRear = PLAYER_tireGrip; // * (1.0 - this.inputs.ebrake * (1.0 - PLAYER_lockGrip)); // reduce rear grip when ebrake is on

	f32 frictionForceFront_cy = $Clamp(-PLAYER_cornerStiffnessFront * slipAngleFront, -tireGripFront, tireGripFront) * axleWeightFront;
	f32 frictionForceRear_cy = $Clamp(-PLAYER_cornerStiffnessRear * slipAngleRear, -tireGripRear, tireGripRear) * axleWeightRear;

	//  Get amount of brake/throttle from our inputs
	f32 brake = Player_GetBrake(player) * 100.0f; // $Min(this.inputs.brake * PLAYER_brakeForce + this.inputs.ebrake * PLAYER_eBrakeForce, PLAYER_brakeForce);
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
