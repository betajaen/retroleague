#ifndef FUNK_H
#define FUNK_H

#include "data.h"
#include <float.h>


Vec3f RotatePointXZ(Vec3f p, f32 yaw);
f32 ConstrainAngle(f32 x);

void Mesh_MakeRedPlayer(Mesh* mesh);
void Mesh_MakeBluePlayer(Mesh* mesh);
void Mesh_MakeBall(Mesh* mesh);
void Mesh_MakeWheel(Mesh* mesh);

void Game_Tick();

u8  FindPlayerIndex(Player* player);

bool Object_IsAlive(Object* obj);
void Player_Tick(Player* player);
void Ball_Tick(Ball* ball);

void Player_Recv_Update(u8 slot, const char* data);
void Player_Send_Update(u8 slot);

void Player_Send_Ball_Update(Ball* ball);
void Player_Recv_Ball_Update(Ball* ball, const char* data);

void Player_Delete(u8 playerIndex);
void Player_DeleteMe();
u8   Player_New(u16 reference);
Player* Player_GetByReference(u16 reference);
Player* Player_GetByReferenceOrCreate(u16 reference);
Player* Player_GetFirstControlled();

bool Can_Power(Player* player, u32 power);
void Activate_Power(Player* player, u32 power);

inline bool IsAnimating(Animation* anim)
{
  return anim->state != 0;
}

void Animate_Tick(Animation* anim, Vec3f* pos, f32* yaw);
void AnimateMoveXZ(Animation* anim, Vec3f posFrom, Vec3f posTo, f32 yawFrom, f32 yawTo, f32 time, f32 speed);
void AnimateSpin(Animation* anim, f32 amountPerTick,f32 finalAngle, f32 time);

void Net_OnReceiveMessage(const char* message);

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

Vec3f TransformLocalPointToWorldSpaceXZRadOnly(f32 heading, f32 x, f32 z);

void MakePid(Pid* pid, f32 p, f32 i, f32 d);
void MakePidDefaults1(Pid* pid);
f32 UpdatePid(Pid* pid, f32 error, f32 time);

inline f32 PidError(f32 target, f32 current)
{
  return target - current;
}

void Update_Title();
void Start_SinglePlayer();
void FixedUpdate_Singleplayer();
void Update_SinglePlayer();
void Start_Multiplayer();
void FixedUpdate_Multiplayer();
void Update_MultiPlayer();
void Update_NetworkConnect();

void RecalculateOBB(Vec3f* centre, Vec3f* halfSize, f32 heading, ObbXZ* obb);

bool OBBvsOBB(ObbXZ* obb1, ObbXZ* obb2, Vec3f* mtv);

Vec3f OBBGetClosestPoint(ObbXZ* obb1, Vec3f point);

void Net_Start(const char* address, const char* port);

void Net_Stop();

void Net_Update();

bool Net_IsConnected();

bool Net_IsHosting();

u32  Net_GetNumClients();

void Net_Send_CreatePlayer(u16 reference, u8 isBot);

void Net_Send_DeletePlayer(u16 reference);

void Net_Send_UpdatePlayer(u16 reference, u32 time, char* data);

void Net_Send_UpdateBall(char* data);

#endif

