#include "funk.h"

static void Animate_MoveXZ(Animation* anim, Vec3f* pos, f32* yaw)
{
  f32 t = (anim->time) / anim->maxTime;

  *pos = $Vec3_LerpXZ(anim->posFrom, anim->posTo, t);
  *yaw = $Rad_Lerp(anim->yawFrom, anim->yawTo, t);
  
  printf("time = %f\n", anim->time);

  f32 delta = anim->speed * $.deltaTime;
  anim->time += delta;
  
  if (anim->time > anim->maxTime)
  {
    anim->state = 0;
  }
}

void Animate_Tick(Animation* anim, Vec3f* pos, f32* yaw)
{
  if (anim->state == 0)
    return;
  
  switch(anim->state)
  {
    case ANIMATION_STATE_MOVE_XZ:
    {
      Animate_MoveXZ(anim, pos, yaw);
    }
    break;
  }
}

void AnimateMoveXZ(Animation* anim, Vec3f posFrom, Vec3f posTo, f32 yawFrom, f32 yawTo, f32 time, f32 speed)
{
  anim->state = ANIMATION_STATE_MOVE_XZ;
  anim->posFrom = posFrom;
  anim->posTo   = posTo;
  anim->yawFrom = yawFrom;
  anim->yawTo   = yawTo;
  anim->time    = 0.0f;
  anim->maxTime = time;
  anim->speed   = speed;
}
