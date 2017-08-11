#include "funk.h"


void Ball_Tick(Ball* ball)
{
  
  bool inGoalArea = (ball->obj.position.x >= -GOAL_SIZE_XH_F && ball->obj.position.x <= GOAL_SIZE_XH_F);
  
  if (ball->obj.position.x < -BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.x = -BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.x = -ball->obj.velocity.x;
    ball->obj.acceleration.x = -ball->obj.acceleration.x;
    ball->magnet = 255;
  }
  else if (ball->obj.position.x > BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.x = BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.x = -ball->obj.velocity.x;
    ball->obj.acceleration.x = -ball->obj.acceleration.x;
    ball->magnet = 255;
  }
  
  if (inGoalArea == false && ball->obj.position.z < -BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.z = -BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.z = -ball->obj.velocity.z;
    ball->obj.acceleration.z = -ball->obj.acceleration.z;
    ball->magnet = 255;
  }
  else if (inGoalArea == false && ball->obj.position.z > BOUNDS_SIZE_HALF_F)
  {
    ball->obj.position.z = BOUNDS_SIZE_HALF_F;
    ball->obj.velocity.z = -ball->obj.velocity.z;
    ball->obj.acceleration.z = -ball->obj.acceleration.z;
    ball->magnet = 255;
  }

  if (inGoalArea && ball->obj.position.z < -BOUNDS_SIZE_HALF_F)
  {
    //printf("Blue Scored %i\n", ball->lastTouch);

    ball->blue++;
    ball->obj.position = $Vec3_Xyz(0,0,0);
    ball->obj.velocity = $Vec3_Xyz(0,0,0);
    ball->obj.acceleration = $Vec3_Xyz(0,0,0);
    ball->magnet = 255;

    if (ball->blue >= 100)
      ball->gameTime = 0.0f;
    
    if (ball->lastTouch != 255)
    {
      PLAYER[ball->lastTouch].justScored = 1;
      PLAYER[ball->lastTouch].scoreTime  = 1.0f;
    }

    ball->lastTouch                = 255;
  }
  else if (inGoalArea && ball->obj.position.z > +BOUNDS_SIZE_HALF_F)
  {
    //printf("Red Scored by %i\n", ball->lastTouch);

    ball->red++;
    ball->obj.position = $Vec3_Xyz(0,0,0);
    ball->obj.velocity = $Vec3_Xyz(0,0,0);
    ball->obj.acceleration = $Vec3_Xyz(0,0,0);
    ball->magnet = 255;
    
    if (ball->blue >= 100)
      ball->gameTime = 0.0f;

    if (ball->lastTouch != 255)
    {
      PLAYER[ball->lastTouch].justScored = 1;
      PLAYER[ball->lastTouch].scoreTime  = 1.0f;
    }

    ball->lastTouch                = 255;
  }

  if (ball->magnet == 255)
  {
    ball->obj.velocity.FORWARD    += ball->obj.acceleration.FORWARD * $.fixedDeltaTime;
    ball->obj.velocity.RIGHT      += ball->obj.acceleration.RIGHT   * $.fixedDeltaTime;
  
    ball->obj.position.FORWARD    += ball->obj.velocity.FORWARD     * $.fixedDeltaTime;
    ball->obj.position.y           = 0;
    ball->obj.position.RIGHT      += ball->obj.velocity.RIGHT       * $.fixedDeltaTime;
  
    ball->obj.velocity.FORWARD    *= 0.95f;
    ball->obj.velocity.RIGHT      *= 0.95f;
  

    ball->obj.acceleration.FORWARD = 0;
    ball->obj.acceleration.RIGHT   = 0;
  }
  else
  {
    Player* player = &PLAYER[ball->magnet];
    
    Vec3f fwd;
    fwd.FORWARD = 3.5f;
    fwd.RIGHT = 0;
    fwd.UP = 0;

    fwd = TransformLocalPointToWorldSpaceXZRad(player->obj.position, player->heading, fwd);

    ball->obj.velocity.FORWARD    = 0;
    ball->obj.velocity.RIGHT      = 0;
    ball->obj.position.FORWARD    = fwd.FORWARD;
    ball->obj.position.y          = 0;
    ball->obj.position.RIGHT      = fwd.RIGHT;
    ball->obj.yaw                 = (ball->obj.yaw + 3) % 360;

    ball->lastTouch               = FindPlayerIndex(player);
    
    ball->magnetTime -= $.fixedDeltaTime;
    if (ball->magnetTime <= 0.0f)
    {
      ball->magnet = 255;
    }

  }
}
