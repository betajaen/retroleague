#include "funk.h"

#define PLAYER_gravity 9.81f  // m/s^2
#define PLAYER_mass  1200.0f  // kg
#define PLAYER_invMass  (1.0f / PLAYER_mass)  // kg
#define PLAYER_inertiaScale  1.0f  // Multiply by mass for inertia
#define PLAYER_halfWidth  0.8f // Centre to side of chassis (metres)
#define PLAYER_cgToFront 2.0f // Centre of gravity to front of chassis (metres)
#define PLAYER_cgToRear  2.0f   // Centre of gravity to rear of chassis
#define PLAYER_cgToFrontAxle 1.25f  // Centre gravity to front axle
#define PLAYER_cgToRearAxle  1.25f  // Centre gravity to rear axle
#define PLAYER_cgHeight 0.55f  // Centre gravity height
#define PLAYER_wheelRadius  0.3f  // Includes tire (also represents height of axle)
#define PLAYER_wheelWidth  0.2f  // Used for render only
#define PLAYER_tireGrip 4.0f // 2.0f  // How much grip tires have
#define PLAYER_lockGrip 0.7f  // % of grip available when wheel is locked
#define PLAYER_engineForce 9000.0f
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

void Player_TickPhysicsSimple(Player* player)
{
  f32 sn = sinf(player->heading),
      cs = cosf(player->heading);
  
  player->extraVelocity.x *= 0.45f;
  player->extraVelocity.z *= 0.45f;
  
  player->carVelocity.FORWARD = cs * (player->extraVelocity.FORWARD + player->obj.velocity.FORWARD) + sn * (player->obj.velocity.RIGHT + player->extraVelocity.RIGHT);
  player->carVelocity.RIGHT   = cs * (player->extraVelocity.RIGHT   + player->obj.velocity.RIGHT)   - sn * (player->obj.velocity.FORWARD + player->extraVelocity.FORWARD);
    
  player->obj.velocity.RIGHT   += player->obj.acceleration.RIGHT * $.fixedDeltaTime;
  player->obj.velocity.FORWARD += player->obj.acceleration.FORWARD * $.fixedDeltaTime;
  
  player->heading += player->yawRate * $.fixedDeltaTime;
  player->obj.yaw = (i16) ($Rad2Deg(-player->heading));
}

void Player_TickPhysics(Player* player, bool isAnimating)
{
  f32 sn = sinf(player->heading),
  cs = cosf(player->heading),
  steerAngle = (f32) player->steering;

  steerAngle = $Clamp(steerAngle, -PLAYER_maxSteer, PLAYER_maxSteer);
  player->steering = steerAngle;

  player->extraVelocity.x *= 0.45f;
  player->extraVelocity.z *= 0.45f;
  
	// Get velocity in local car coordinates
	player->carVelocity.FORWARD = cs * (player->extraVelocity.FORWARD + player->obj.velocity.FORWARD) + sn * (player->obj.velocity.RIGHT + player->extraVelocity.RIGHT);
	player->carVelocity.RIGHT   = cs * (player->extraVelocity.RIGHT   + player->obj.velocity.RIGHT)   - sn * (player->obj.velocity.FORWARD + player->extraVelocity.FORWARD);

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
  player->carAcceleration.RIGHT   = totalForce_cy / PLAYER_mass;  // sideways accel

  // acceleration in world coordinates
  player->obj.acceleration.FORWARD = cs * player->carAcceleration.FORWARD - sn * player->carAcceleration.RIGHT;
  player->obj.acceleration.RIGHT   = sn * player->carAcceleration.FORWARD + cs * player->carAcceleration.RIGHT;
  
  player->obj.velocity.RIGHT   += player->obj.acceleration.RIGHT * $.fixedDeltaTime;
  player->obj.velocity.FORWARD += player->obj.acceleration.FORWARD * $.fixedDeltaTime;
      
  // calculate rotational forces
  f32 angularTorque = (frictionForceFront_cy + tractionForce_cy) * PLAYER_cgToFrontAxle - frictionForceRear_cy * PLAYER_cgToRearAxle;
    
  //  Sim gets unstable at very slow speeds, so just stop the car
  if( fabsf(player->absVelocity) < 0.5 && !throttle )
  {
    player->obj.velocity.FORWARD = player->obj.velocity.RIGHT = player->absVelocity = 0;
    angularTorque = player->yawRate = 0;
  }

  f32 angularAccel = angularTorque / PLAYER_inertia;

  player->yawRate += angularAccel * $.fixedDeltaTime;
  player->heading += player->yawRate * $.fixedDeltaTime;
  
  player->obj.yaw = (i16) ($Rad2Deg(-player->heading));

}

void Player_TickBallCollision(Player* player, Ball* ball)
{
  if (ball->magnet != 255)
    return;

  Vec3f localPoint = TransformWorldPointToLocalSpaceXZ(player->obj.position, player->obj.yaw, ball->obj.position);

  IntersectPointResult result = { 0 };

  if (IntersectPointXZRadius($Vec3_Xyz(0,0,0), MESH_PLAYER[0].halfSize, MESH_PLAYER[0].min, MESH_PLAYER[0].max, localPoint, 2.56f, &result))
  {
    const f32 ballMod = 2500.0f;

    Vec3f normalisedVelocity = $Vec3_Normalise(player->obj.velocity); // $Vec3_Mul(player->obj.velocity, worldNormal));
    ball->obj.acceleration.x += normalisedVelocity.x * ballMod;
    ball->obj.acceleration.z += normalisedVelocity.z * ballMod;
    ball->lastTouch = FindPlayerIndex(player);
  }
}

void Tick_PhysicsCollisions(Player* self)
{

    Vec3f nextPos;
    nextPos.RIGHT        = self->obj.position.RIGHT   + self->obj.velocity.RIGHT   * $.fixedDeltaTime;
    nextPos.FORWARD      = self->obj.position.FORWARD + self->obj.velocity.FORWARD * $.fixedDeltaTime;
    nextPos.UP           = 0.0f;

    RecalculateOBB(&nextPos, &MESH_PLAYER[0].halfSize, self->heading, &self->obb);

    for(u32 ii=0;ii < MAX_PLAYERS;ii++)
    {
      if (PLAYER[ii].obj.type == OT_NONE)
        continue;
      
      if (&PLAYER[ii] == self)
        continue;
        
      Vec3f mtv;
        
      if (OBBvsOBB(&self->obb, &PLAYER[ii].obb, &mtv))
      {
        Player* other = &PLAYER[ii];
        
        Vec3f contactPoint = OBBGetClosestPoint(&other->obb, self->obj.position);
        Vec3f contactNormal = $Vec3_NormaliseXZ($Vec3_Sub(contactPoint, self->obj.position));
        Vec3f relVelocity = $Vec3_Sub(other->obj.velocity, self->obj.velocity);
        

        f32 vrel = $Vec3_DotXZ(relVelocity, contactNormal);
        
        if (ApproxZero(vrel))
        {
          contactNormal = $Vec3_NormaliseXZ($Vec3_Sub(other->obj.position, self->obj.position));
          relVelocity = $Vec3_Sub(other->obj.velocity, self->obj.velocity);
          vrel = $Vec3_DotXZ(relVelocity, contactNormal);
        }


        if (vrel > 0.0f)
          continue;

        f32 e = 0.2f;
        f32 j = (-(1.0f - e) * vrel);
        j /= (PLAYER_invMass + PLAYER_invMass);

        // printf("%.1f -> %.1f -> %.1f %.1f -> %.1f %.1f\n", j, vrel, relVelocity.x, relVelocity.z, contactNormal.x, contactNormal.z);
        
        Vec3f impulse = $Vec3_MulS(contactNormal, j);

        self->obj.velocity.x -= PLAYER_invMass * impulse.x * 3.2f;
        self->obj.velocity.z -= PLAYER_invMass * impulse.z * 3.2f;

        other->obj.velocity.x += PLAYER_invMass * impulse.x * 3.2f;
        other->obj.velocity.z += PLAYER_invMass * impulse.z * 3.2f;

      }
  }
  
  self->absVelocity                = $Vec3_Length(self->obj.velocity);

  self->obj.position.RIGHT        += self->obj.velocity.RIGHT * $.fixedDeltaTime;
  self->obj.position.FORWARD      += self->obj.velocity.FORWARD * $.fixedDeltaTime;
  self->obj.position.UP           = 0.0f;
    
}
