#include "data.h"
#include <math.h>
#include <float.h>

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

Vec3f TransformLocalPointToWorldSpaceXZRadOnly(f32 rad, f32 x, f32 z)
{
  /*
      x'  =  x * cos(t) + z * sin(t)
      z'  = -x * sin(t) + z * cos(t)
  */
  f32 c = cosf(rad), s = sinf(rad);

  f32 x1 = x * c + z * s;
  f32 z1 =-x * s + z * c;

  return $Vec3_Xyz(x1, 0.0f, z1);
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
    outResult->pos.z = point.z;
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

void RecalculateOBB(Vec3f* centre, Vec3f* halfSize, f32 heading, ObbXZ* obb)
{
  obb->halfSize = *halfSize;
  obb->size = $Vec3_Xyz(2.0f * halfSize->x, 0, 2.0f * halfSize->z);
  obb->pos  = *centre;
  f32 c = cosf(heading);
  f32 s = sinf(heading);
  obb->axis = $Vec3_Xyz(c, 0, s);

  obb->a[0] = TransformLocalPointToWorldSpaceXZRadOnly(heading, 1, 0);
  obb->a[1] = TransformLocalPointToWorldSpaceXZRadOnly(heading, 0, 1);

  obb->rect[0] = TransformLocalPointToWorldSpaceXZRadOnly(heading, -halfSize->x, +halfSize->z);
  obb->rect[1] = TransformLocalPointToWorldSpaceXZRadOnly(heading, +halfSize->x, +halfSize->z);
  obb->rect[2] = TransformLocalPointToWorldSpaceXZRadOnly(heading, +halfSize->x, -halfSize->z);
  obb->rect[3] = TransformLocalPointToWorldSpaceXZRadOnly(heading, -halfSize->x, -halfSize->z);
  
  obb->rect[0].x += obb->pos.x;   obb->rect[0].z += obb->pos.z;
  obb->rect[1].x += obb->pos.x;   obb->rect[1].z += obb->pos.z;
  obb->rect[2].x += obb->pos.x;   obb->rect[2].z += obb->pos.z;
  obb->rect[3].x += obb->pos.x;   obb->rect[3].z += obb->pos.z;
}

typedef struct
{
  f32 first, second;
} F32Pair;

void projectToAxis(ObbXZ* obb, Vec3f axis, F32Pair* proj)
{
  f32 min =  FLT_MAX;
  f32 max = -FLT_MAX;
  
  Vec3f* rect = obb->rect;
  for(u32 ii=0;ii < 4;ii++)
  {
    f32 d = $Vec3_Dot(rect[ii], axis);
    min = $Min(min, d);
    max = $Max(max, d);
  }
  
  proj->first  = min;
  proj->second = max;
}

bool AxisCollide(ObbXZ* a, ObbXZ* b, Vec3f axis)
{
  
  F32Pair aProj, bProj;
  projectToAxis(a, axis, &aProj);
  projectToAxis(b, axis, &bProj);
  
  if (aProj.second < bProj.first || bProj.second < aProj.first)
    return false;

  axis = $Vec3_Xyz(-axis.z, 0, axis.x);
  
  projectToAxis(a, axis, &aProj);
  projectToAxis(b, axis, &bProj);
  
  if (aProj.second < bProj.first || bProj.second < aProj.first)
    return false;
  
  return true;
}


// Separating Axis Theorem (SAT) collision test
// Minimum Translation Vector (MTV) is returned for the first Oriented Bounding Box (OBB)
bool OBBvsOBB(ObbXZ* obb1, ObbXZ* obb2, Vec3f* mtv)
{
  return AxisCollide(obb1, obb2, obb1->axis) && AxisCollide(obb1, obb2, obb2->axis);
}

Vec3f OBBGetClosestPoint(ObbXZ* obb, Vec3f targetPoint)
{
  Vec3f d = $Vec3_Sub(targetPoint, obb->pos);
  Vec3f closestPoint = obb->pos; // Start at the center point of the OBB.

  $Vec3_Add(
    closestPoint,
    $Vec3_MulS(obb->a[0], 
      $Clamp($Vec3_Dot(d, obb->a[0]), -obb->halfSize.e[0], obb->halfSize.e[0])
      ) 
  );
  
  $Vec3_Add(
    closestPoint,
    $Vec3_MulS(obb->a[1], 
      $Clamp($Vec3_Dot(d, obb->a[1]), -obb->halfSize.e[2], obb->halfSize.e[2])
      ) 
  );
  
  return closestPoint;
}
