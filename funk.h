#ifndef FUNK_H
#define FUNK_H

#include "data.h"
#include <float.h>


Vec3f RotatePointXZ(Vec3f p, f32 yaw);
f32 ConstrainAngle(f32 x);

void Mesh_MakePlayer(Mesh* mesh);
void Mesh_MakeBall(Mesh* mesh);

void Game_Tick();

u8  FindPlayerIndex(Player* player);

bool Object_IsAlive(Object* obj);
void Player_Tick(Player* player);
void Ball_Tick(Ball* ball);

void Player_ReceivePartialUpdate(u8 playerIndex, const char* msg);
void Player_ReceiveFullUpdate(u8 playerIndex, const char* msg);
void Player_SendFullUpdate();
void Player_SendPartialUpdate(u8 type, void* data);
void Player_Delete(u8 playerIndex);
void Player_DeleteMe();
void Player_New(u8 playerIndex, u8 team, bool isMe);

bool Can_Power(Player* player, u32 power);
void Activate_Power(Player* player, u32 power);

inline bool IsAnimating(Animation* anim)
{
  return anim->state != 0;
}

void Animate_Tick(Animation* anim, Vec3f* pos, f32* yaw);
void AnimateMoveXZ(Animation* anim, Vec3f posFrom, Vec3f posTo, f32 yawFrom, f32 yawTo, f32 time, f32 speed);
void AnimateSpin(Animation* anim, f32 amountPerTick,f32 finalAngle, f32 time);

void ReceiveMessage(const char* message);

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

inline bool ApproxEqual(f32 a, f32 b)
{
  return !fabsf(b - a) <= FLT_EPSILON;
}

inline f32 AbsDifference(f32 a, f32 b)
{
  return fabsf(b - a);
}

bool IntersectPointXZ(Vec3f center, Vec3f halfSize, Vec3f point, IntersectPointResult* outResult);

bool IntersectPointXZRadius(Vec3f center, Vec3f halfSize, Vec3f min, Vec3f max, Vec3f point, f32 radius, IntersectPointResult* outResult);

Vec3f TransformWorldPointToLocalSpaceXZ(Vec3f selfPosition, i32 selfRotation, Vec3f otherPosition);

Vec3f TransformWorldPointToLocalSpaceXZRad(Vec3f selfPosition, f32 heading, Vec3f otherPosition);

Vec3f TransformLocalPointToWorldSpaceXZ(Vec3f selfPosition, i32 selfRotation, Vec3f otherPosition);

Vec3f TransformLocalPointToWorldSpaceXZRad(Vec3f selfPosition, f32 heading, Vec3f otherPosition);

void MakePid(Pid* pid, f32 p, f32 i, f32 d);
void MakePidDefaults1(Pid* pid);
f32 UpdatePid(Pid* pid, f32 error, f32 time);

inline f32 PidError(f32 target, f32 current)
{
  return target - current;
}

void Draw_Title();
void Update_Scene();
void Start_SinglePlayer();
void Tick_Singleplayer();
void Draw_Singleplayer();
void Start_Multiplayer();
void Tick_Multiplayer();
void Draw_Multiplayer();

#endif

