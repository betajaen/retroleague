#ifndef FUNK_H
#define FUNK_H

#include "data.h"
#include <float.h>


Vec3f RotatePointXZ(Vec3f p, f32 yaw);
f32 ConstrainAngle(f32 x);

void Mesh_MakePlayer(Mesh* mesh);
void Mesh_MakeBall(Mesh* mesh);

void Game_Tick();


bool Object_IsAlive(Object* obj);
void Player_Tick(Player* player);
void Ball_Tick(Ball* ball);

// Decode angle to radians
inline f32 DecodeAngle(i16 angle)
{
  return $Deg2Rad( ((f32) (angle)) / 10.0f);
}

// Encode radians to angle
inline i16 EncodeAngle(f32 rad)
{
  return (i16) ($Rad2Deg(rad) * 10.0f);
}

inline f32 ApproxZero(f32 v)
{
  return !(fabsf(v) > FLT_EPSILON);
}

bool IntersectPointXZ(Vec3f center, Vec3f halfSize, Vec3f point, IntersectPointResult* outResult);

Vec3f TransformPointToLocalSpaceXZ(Vec3f selfPosition, i32 selfRotation, Vec3f otherPosition);


#endif

