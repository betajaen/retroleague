#ifndef DATA_H
#define DATA_H

#include <math.h>
#include <stdio.h>
#include "synthwave.h"

#define IS_STREAMING 1

#define MAX_PLAYERS 4
#define BOUNDS_SIZE_F 200.0f
#define BOUNDS_SIZE_I 200
#define BOUNDS_SIZE_HALF_F (BOUNDS_SIZE_F * 0.5f)

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
#define CONTROL_TURN_LEFT  5
#define CONTROL_TURN_RIGHT 6
#define CONTROL_UP         7
#define CONTROL_DOWN       8
#define SOUND_TEST         9

#define OT_NONE   0
#define OT_PLAYER 1
#define OT_BALL   2
#define OT_WHEEL  3

typedef struct
{
  u8    id;
  u8    type;
  Vec3f position, velocity, acceleration;
  i16   yaw;
} Object;

typedef struct 
{
  Object obj;
  u8 team;                // 0 - red, 1 - blue
  i8 steering;            // steering in whatever units. - to +
  i8 acceleratorBrake;    // < 0 brake, > 0 accelerate, 0 = none
  f32 angularVelocity;    // Angular velocity of player - not related to yaw, Stored in 1/10th degrees ($Rad2Deg(DEG) * 100)
  f32 heading;              // True yaw
  f32 yawRate;
  f32 absVelocity;
  Vec3f carAcceleration, carVelocity;
} Player;

typedef struct
{
  Object obj;
  u8     red, blue;
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
extern Ball    BALL;
extern Font    FONT;
extern Bitmap  ART;
extern Mesh    MESH_PLAYER;
extern Mesh    MESH_BALL;
extern Vec3f   CAMERA_POSITION;
extern Vec3f   CAMERA_ROTATION;

#endif

