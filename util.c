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

