#include "data.h"
#include <math.h>

Vec3f TransformWorldPointToLocalSpaceXZ(Vec3f selfPosition, i32 selfRotation, Vec3f otherPosition)
{
  Vec3f r;
  r.x = 0;
  r.y = 0;
  r.z = 0;

  /*
      x'  =  x * cos(t) - z * sin(t)
      y'  =  x * sin(t) + z * cos(t)
  */
  f32 t = $Deg2Rad(selfRotation), c = cosf(t), s = sinf(t);

  f32 dx = otherPosition.x - selfPosition.x;
  f32 dz = otherPosition.z - selfPosition.z;

  r.x = dx * c - dz * s;
  r.z = dx * s + dz * c;
  
  return r;
}

Vec3f TransformWorldPointToLocalSpaceXZRad(Vec3f selfPosition, f32 rad, Vec3f otherPosition)
{
  Vec3f r;
  r.x = 0;
  r.y = 0;
  r.z = 0;

  /*
      x'  =  x * cos(t) - z * sin(t)
      y'  =  x * sin(t) + z * cos(t)
  */
  f32 c = cosf(rad), s = sinf(rad);

  f32 dx = otherPosition.x - selfPosition.x;
  f32 dz = otherPosition.z - selfPosition.z;

  r.x = dx * c - dz * s;
  r.z = dx * s + dz * c;
  
  return r;
}

Vec3f TransformLocalPointToWorldSpaceXZ(Vec3f selfPosition, i32 selfRotation, Vec3f otherPosition)
{
  Vec3f r;
  r.x = 0;
  r.y = 0;
  r.z = 0;

  /*
      x'  =  x * cos(t) + z * sin(t)
      z'  = -x * sin(t) + z * cos(t)
  */
  f32 t = $Deg2Rad(selfRotation), c = cosf(t), s = sinf(t);

  f32 x1 = otherPosition.x * c + otherPosition.z * s;
  f32 z1 =-otherPosition.x * s + otherPosition.z * c;
  
  r.x = x1 + selfPosition.x;
  r.z = z1 + selfPosition.z;

  return r;
}

Vec3f TransformLocalPointToWorldSpaceXZRad(Vec3f selfPosition, f32 rad, Vec3f otherPosition)
{
  Vec3f r;
  r.x = 0;
  r.y = 0;
  r.z = 0;

  /*
      x'  =  x * cos(t) + z * sin(t)
      z'  = -x * sin(t) + z * cos(t)
  */
  f32 c = cosf(rad), s = sinf(rad);

  f32 x1 = otherPosition.x * c + otherPosition.z * s;
  f32 z1 =-otherPosition.x * s + otherPosition.z * c;
  
  r.x = x1 + selfPosition.x;
  r.z = z1 + selfPosition.z;

  return r;
}


bool IntersectPointXZ(Vec3f center, Vec3f halfSize, Vec3f point, IntersectPointResult* outResult)
{
  f32 dx = point.x - center.x;
  f32 px = halfSize.x - fabsf(dx);
  if (px <= 0)
  {
    return false;
  }
  f32 dz = point.z - center.z;
  f32 pz = halfSize.z - fabsf(dz);
  if (pz <= 0)
  {
    return false;
  }

  if (px < pz)
  {
    f32 sx = $SignF(dx);
    outResult->delta.x = px * sx;
    outResult->normal.x = sx;
    outResult->pos.x = center.x + (halfSize.x * sx);
    outResult->pos.y = point.y;
  }
  else
  {
    f32 sz = $SignF(dz);
    outResult->delta.z = pz * sz;
    outResult->normal.z = sz;
    outResult->pos.x = point.x;
    outResult->pos.z = center.z + (halfSize.z * sz);
  }
  
  return true;
}

bool IntersectPointXZRadius(Vec3f center, Vec3f halfSize, Vec3f min, Vec3f max, Vec3f point, f32 radius, IntersectPointResult* outResult)
{
  Vec3f closest;

  closest.x = $Clamp(point.x, min.x, max.x);
  closest.z = $Clamp(point.z, min.z, max.z);

  f32 distancex = point.x - closest.x;
  f32 distancez = point.z - closest.z;

  f32 distanceSquared = $Squared(distancex) + $Squared(distancez);
  Vec3f localClosest = closest;

  if (distanceSquared < (radius * radius))
  {
    return IntersectPointXZ(center, halfSize, localClosest, outResult);
  }
  else
  {
    return false;
  }
}
