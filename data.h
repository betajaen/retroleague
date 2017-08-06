#ifndef DATA_H
#define DATA_H

#include <math.h>
#include <stdio.h>
#include "synthwave.h"

#define IS_STREAMING 1

#define MAX_PLAYERS 4
#define BOUNDS_SIZE_F 150.0f
#define BOUNDS_SIZE_I 150
#define BOUNDS_SIZE_HALF_F (BOUNDS_SIZE_F * 0.5f)

#define GOAL_SIZE_X_F 50.0f
#define GOAL_SIZE_X_I 50
#define GOAL_SIZE_XH_F (GOAL_SIZE_X_F * 0.5f)

#define DB16_TRANSPARENT 0
#define DB16_VERY_DARK_VIOLET 1
#define DB16_SHADOWY_LAVENDER 2
#define DB16_FLINT 3
#define DB16_REGENT_GREY 4
#define DB16_PEPPERMINT 5
#define DB16_RED_EARTH 6
#define DB16_ROOT_BEER 7
#define DB16_FADED_RED 8
#define DB16_BRONZE 9
#define DB16_BIRTHDAY_SUIT 10
#define DB16_BANANA 11
#define DB16_DEEP_KOAMARU 12
#define DB16_INDIGO 13
#define DB16_CADET_BLUE 14
#define DB16_HUNTER_GREEN 15
#define DB16_LEAF 16

#define CONTROL_FORWARD    1
#define CONTROL_BACKWARD   2
#define CONTROL_LEFT       3
#define CONTROL_RIGHT      4
#define CONTROL_CAMERA_LEFT  5
#define CONTROL_CAMERA_RIGHT 6
#define CONTROL_UP         7
#define CONTROL_AUTOPILOT       8
#define CONTROL_HANDBRAKE  9
#define CONTROL_CHEAT_FLIP_180   10
#define CONTROL_POWER_PUNT 12
#define CONTROL_POWER_MAGNET 13
#define CONTROL_POWER_SPIN 14
#define CONTROL_START_SINGLE 15
#define CONTROL_START_MULTI 16

#define SOUND_TEST         17

#define OT_NONE   0
#define OT_PLAYER 1
#define OT_BALL   2
#define OT_WHEEL  3

#define GAME_STATE_TITLE           0
#define GAME_STATE_SINGLE          1
#define GAME_STATE_NETWORK_CONNECT 2
#define GAME_STATE_MULTI           3

typedef struct
{
  u8    id;
  u8    type;
  Vec3f position, velocity, acceleration;
  i16   yaw;
} Object;

typedef struct
{
 f32 prevError, intAccum, p, i, d, max, min;
} Pid;

typedef struct
{
  u8 state;
  Pid a, b;
} Ai;

#define ANIMATION_STATE_NONE 0
#define ANIMATION_STATE_MOVE_XZ 1
#define ANIMATION_STATE_SPIN 2

typedef struct
{
  u8 state;
  Vec3f posFrom, posTo;
  f32   yawFrom, yawTo;
  f32   time, maxTime, speed;
} Animation;

#define POWER_PUNT   0
#define POWER_MAGNET 1
#define POWER_SPIN   2
#define MAX_POWERS   3

#define POWER_BIT_PUNT    1
#define POWER_BIT_MAGNET  2
#define POWER_BIT_SPIN    4

extern const char* POWERS_NAME[MAX_POWERS];

typedef struct 
{
  Object obj;
  Ai ai;
  Animation anim;
  u8 autopilot;
  u8 team;                  // 0 - red, 1 - blue
  u8 isNetwork;
  u8 powerControls;         // 
  u8 powerAvailable;        // 
  f32 powerCooldown[MAX_POWERS];
  f32 steering;             // steering in radians
  i8 acceleratorBrake;      // < 0 brake, > 0 accelerate, 0 = none
  i8 handBrake;           
  f32 angularVelocity;      // Angular velocity of player - not related to yaw, Stored in 1/10th degrees ($Rad2Deg(DEG) * 100)
  f32 heading;              // True yaw
  f32 yawRate;
  f32 absVelocity;
  Vec3f carAcceleration, carVelocity;
  u32 timestamp;
} Player;

typedef struct
{
  Object obj;
  u8     red, blue;
  u8     magnet;
  f32    magnetTime;
} Ball;

typedef struct
{
  Object obj;
  u8     deathTime;
} Wheel;

typedef struct
{
  Vec3f delta, normal, pos;
} IntersectPointResult;

extern f32     DELTA;
extern Surface SURFACE;
extern Scene   SCENE;
extern Canvas  CANVAS;
extern Player  PLAYER[MAX_PLAYERS];
extern Player* ME;
extern u32     ME_INDEX;
extern Ball    BALL;
extern Font    FONT;
extern Bitmap  ART;
extern Mesh    MESH_PLAYER;
extern Mesh    MESH_BALL;
extern Vec3f   CAMERA_POSITION;
extern Vec3f   CAMERA_ROTATION;
extern f32     CAMERA_THETA;
extern f32     CAMERA_THETA_TIME;
extern u32     FRAME_COUNT;
extern Bitmap  TITLE;
extern i32     GAME_STATE;

#endif

