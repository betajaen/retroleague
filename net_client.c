#include "funk.h"
#include <stdlib.h>
#include <string.h>


f32 DecodeFloatImpl(char* value)
{
  //float f=23.345466467
  //int i=*(int*)&f;
  i32 i = atoi(value);
  f32 f = *(f32*)&i;
  return f;
}

void DecodeFloat(char* value, f32 * v)
{
  *v = DecodeFloatImpl(value);
}

void DecodeVec3f(char* value, Vec3f* v)
{
  char* e = strchr(value, 'M');
  v->x = DecodeFloatImpl(value);
  value = e + 1;
  e = strchr(value, 'M');
  v->y = DecodeFloatImpl(value);
  value = e + 1;
  v->z = DecodeFloatImpl(value);
}

void DecodeI16(char* value, i16* v)
{
  i32 i = atoi(value);
  *v = (i16) i; 
}

void DecodeU8(char* value, u8* v)
{
  i32 i = atoi(value);
  *v = (u8) i; 
}

void Player_ReceivePartialUpdate(u8 playerIndex, const char* msg)
{
  Player_ReceiveFullUpdate(playerIndex, msg);
}

void Player_ReceiveFullUpdate(u8 playerIndex, const char* msg)
{
  Player* player = &PLAYER[playerIndex];
  player->obj.type = OT_PLAYER;
  player->isNetwork = true;

  char *prop = strtok((char*)msg, ";");
  while(prop)
  {
    char name = prop[0];
    char* value = prop + 1;
    
    switch(name)
    {
      case 'P': DecodeVec3f(value, &player->obj.position);     break;
      case 'V': DecodeVec3f(value, &player->obj.velocity);     break;
      case 'A': DecodeVec3f(value, &player->obj.acceleration); break;
      case 'Y': DecodeI16(value, &player->obj.yaw);            break;
      case 't': DecodeU8(value, &player->team);                break;
      case 's': DecodeFloat(value, &player->steering);         break;
      case 'a': DecodeFloat(value, &player->angularVelocity);  break;
      case 'h': DecodeFloat(value, &player->heading);          break;
      case 'y': DecodeFloat(value, &player->yawRate);          break;
      case 'b': DecodeFloat(value, &player->absVelocity);      break;
      case 'c': DecodeVec3f(value, &player->carAcceleration);  break;
      case 'e': DecodeVec3f(value, &player->carVelocity);      break;
    }

    prop = strtok(NULL, ";");
  }
}

char* WriteChar(char* w, char c)
{
  *w = c;
  w++;
  return w;
}

char* WriteInt(char* w, int c)
{
  int r = sprintf(w, "%i", c);
  w += r;
  return w;
}

char* EncodeFloat(char* w, u8 type, f32 m)
{
  int i=*(int*)&m;
  int r = sprintf(w, "%c%i;", (char) type, i);
  w += r;
  return w;
}

char* EncodeVec3(char* w, u8 type, Vec3f m)
{
  int ix=*(int*)&m.x;
  int iy=*(int*)&m.y;
  int iz=*(int*)&m.z;

  int r = sprintf(w, "%c%iM%iM%i;", (char) type, ix, iy, iz);
  w += r;
  return w;
}

char* EncodeU32(char* w, u8 type, u32 m)
{
  int r = sprintf(w, "%c%u;", (char) type, m);
  w += r;
  return w;
}

char* EncodeU16(char* w, u8 type, u16 m)
{
  int r = sprintf(w, "%c%u;", (char) type, m);
  w += r;
  return w;
}

char* EncodeI16(char* w, u8 type, i16 m)
{
  int r = sprintf(w, "%c%i;", (char) type, m);
  w += r;
  return w;
}

char* EncodeU8(char* w, u8 type, u8 m)
{
  int r = sprintf(w, "%c%i;", (char) type, m);
  w += r;
  return w;
}

void Player_SendFullUpdate(Player* ME)
{
  char* d = $.Mem.TempAllocator(512);

  char* w = d;

  w = WriteChar(w, 'U');
  w = WriteChar(w, '0' + FindPlayerIndex(ME));
  w = WriteInt(w, FRAME_COUNT);
  w = WriteChar(w, '=');

  w = EncodeVec3(w,  'P', ME->obj.position);
  w = EncodeVec3(w,  'V', ME->obj.velocity);
  w = EncodeVec3(w,  'A', ME->obj.acceleration);
  w = EncodeI16(w,   'Y', ME->obj.yaw);
  w = EncodeU8(w,    't', ME->team);
  w = EncodeFloat(w, 's', ME->steering);
  w = EncodeFloat(w, 'a', ME->angularVelocity);
  w = EncodeFloat(w, 'h', ME->heading);
  w = EncodeFloat(w, 'r', ME->yawRate);
  w = EncodeFloat(w, 'b', ME->absVelocity);
  w = EncodeVec3(w,  'c', ME->carAcceleration);
  w = EncodeVec3(w,  'e', ME->carVelocity);



  w = WriteChar(w, '\n');
  w = WriteChar(w, '\0');
  $.Net.SendMessage(strlen(d), d);
}

void Player_SendPartialUpdate(u8 type, void* data)
{
}

void Player_Delete(u8 playerIndex)
{
  memset(&PLAYER[playerIndex], 0, sizeof(Player));
  PLAYER->obj.type = OT_NONE;
  if (BALL.magnet == playerIndex)
    BALL.magnet = 255;
}

void Player_DeleteMe()
{
///   Player_Delete(FindPlayerIndex(ME));
}

void Player_New(u8 playerIndex, u8 team, bool isMe)
{
//  Player* player = &PLAYER[playerIndex];
//  memset(player, 0, sizeof(Player));
//  player->obj.type = OT_PLAYER;
//  
//  if (isMe)
//  {
//    ME = &PLAYER[playerIndex];
//  }
}
