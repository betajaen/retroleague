#include "funk.h"
f32 ConstrainAngle(f32 x)
{
    x = fmodf(x, 360.0f);
    if (x < 0.0f)
        x += 360.0f;
    return x;
}

Vec3f RotatePointXZ(Vec3f p, f32 yaw)
{
  f32 c = cosf($Deg2Rad(yaw));
  f32 s = sinf($Deg2Rad(yaw));

  Vec3f p1;
  p1.x = p.x * c - p.z * s;
  p1.y = p.y;
  p1.z = p.z * c + p.x * s;

  return p1;
}

void MakePid(Pid* pid, f32 p, f32 i, f32 d)
{
  pid->p = p;
  pid->i = i;
  pid->d = d;
  pid->max = FLT_MAX;
  pid->min = FLT_MIN;
  pid->prevError = 0.0f;
  pid->intAccum = 0.0f;
}

void MakePidDefaults1(Pid* pid)
{
  MakePid(pid, 1.0f, 0.01f, 0.001f);
}

f32 UpdatePid(Pid* pid, f32 error, f32 time)
{
  pid->intAccum += error * time;
  float action = (pid->p * error) + (pid->i * pid->intAccum) + (pid->d * (error - pid->prevError) / time);
  float clamped = $Clamp(action, pid->min, pid->max);
  
  if (!ApproxEqual(clamped, action))
  {
    pid->intAccum -= error * time;
  }

  pid->prevError = error;
  return action;

}

