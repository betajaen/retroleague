#include "data.h"
#include <math.h>

Vec3f TransformPointToLocalSpaceXZ(Vec3f selfPosition, i32 selfRotation, Vec3f otherPosition)
{
  Vec3f r;
  r.x = 0;
  r.y = 0;
  r.z = 0;
  // @TODO
  return r;
}

bool IntersectPointXZ(Vec3f center, Vec3f halfSize, Vec3f point, IntersectPointResult* outResult)
{
  f32 dx = point.x - center.x;
  f32 px = halfSize.x - fabsf(dx);
  if (px <= 0)
    return false;

  f32 dz = point.z - center.z;
  f32 pz = halfSize.z - fabsf(dz);
  if (pz <= 0)
    return false;

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


#if 0
intersectPoint: (point) ->
    dx = point.x - this.pos.x
    px = this.half.x - abs(dx)
    return null if px <= 0

    dy = point.y - this.pos.y
    py = this.half.y - abs(dy)
    return null if py <= 0

    hit = new Hit(this)
    if px < py
      sx = sign(dx)
      hit.delta.x = px * sx
      hit.normal.x = sx
      hit.pos.x = this.pos.x + (this.half.x * sx)
      hit.pos.y = point.y
    else
      sy = sign(dy)
      hit.delta.y = py * sy
      hit.normal.y = sy
      hit.pos.x = point.x
      hit.pos.y = this.pos.y + (this.half.y * sy)
    return hit


    #endif