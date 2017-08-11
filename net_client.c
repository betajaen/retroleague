#include "funk.h"
#include <stdlib.h>
#include <string.h>


f32 DecodeFloatImpl(char* value)
{
  //float f=23.345466467
  //int i=*(int*)&f;
  char m[9] = { 0 };
  memcpy(&m[0], value, 8);

  u32 i = strtoul(m, NULL, 16);
  f32 f = *(f32*)&i;
  return f;
}

void DecodePlayer(char* value, u8* localPlayerSlot)
{
  i32 i = strtol(value, NULL, 16);
  u16 reference = (u16) i; 

  if (reference == 0)
  {
    *localPlayerSlot = 255;
    return;
  }
  
  Player* player = Player_GetByReference(reference);
  if (player == NULL)
    *localPlayerSlot = 255;
  else
    *localPlayerSlot = FindPlayerIndex(player);
  
}

void DecodeFloat(char* value, f32 * v)
{
  *v = DecodeFloatImpl(value);
}

void DecodeVec3XZ(char* value, Vec3f* v)
{
  char* e = strchr(value, '_');
  v->x = DecodeFloatImpl(value);
  value = e + 1;
  v->y = 0.0f;
  v->z = DecodeFloatImpl(value);
}

void DecodeI16(char* value, i16* v)
{
  i32 i = strtol(value, NULL, 16);
  *v = (i16) i; 
}

void DecodeU8(char* value, u8* v)
{
  u32 i = strtoul(value, NULL, 16);
  *v = (u8) i; 
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

char* EncodePlayer(char* w, u8 type, u8 localPlayerSlot)
{
  u16 reference = 0;
  
  if (localPlayerSlot != 255)
    reference = PLAYER[localPlayerSlot].multiplayerReference;

  int r = sprintf(w, "%c%04X;", (char) type, localPlayerSlot);
  w += r;
  return w;
}

char* EncodeFloat(char* w, u8 type, f32 m)
{
  u32 i=*(u32*)&m;
  int r = sprintf(w, "%c%08X;", (char) type, i);
  w += r;
  return w;
}

char* EncodeVec3XZ(char* w, u8 type, Vec3f m)
{
  u32 ix=*(u32*)&m.x;
  u32 iz=*(u32*)&m.z;

  int r = sprintf(w, "%c%08X_%08X;", (char) type, ix, iz);
  w += r;
  return w;
}

char* EncodeU32(char* w, u8 type, u32 m)
{
  int r = sprintf(w, "%c%08X;", (char) type, m);
  w += r;
  return w;
}

char* EncodeU16(char* w, u8 type, u16 m)
{
  int r = sprintf(w, "%c%04X;", (char) type, m);
  w += r;
  return w;
}

char* EncodeI16(char* w, u8 type, i16 m)
{
  int r = sprintf(w, "%c%04X;", (char) type, m);
  w += r;
  return w;
}

char* EncodeU8(char* w, u8 type, u8 m)
{
  int r = sprintf(w, "%c%02X;", (char) type, m);
  w += r;
  return w;
}

void Player_Send_Update(u8 slot)
{
  Player* player = &PLAYER[slot];

  player->timestamp++;

  char* d = $.Mem.TempAllocator(512);
  char* w = d;
  w = EncodeVec3XZ(w,  'P', player->obj.position);
  w = EncodeVec3XZ(w,  'V', player->obj.velocity);
  w = EncodeVec3XZ(w,  'A', player->obj.acceleration);
  w = EncodeI16(w,     'Y', player->obj.yaw);
  w = EncodeU8(w,      't', player->team);
  w = EncodeFloat(w,   's', player->steering);
  w = EncodeFloat(w,   'a', player->angularVelocity);
  w = EncodeFloat(w,   'h', player->heading);
  w = EncodeFloat(w,   'r', player->yawRate);
  w = EncodeU8(w,      'g', player->score);
  w = EncodeU8(w,      'j', player->justScored);
  w = EncodeFloat(w,   'J', player->scoreTime);
  w = EncodeFloat(w,   '0', player->powerCooldown[0]);
  w = EncodeFloat(w,   '1', player->powerCooldown[1]);
  w = EncodeFloat(w,   '2', player->powerCooldown[2]);
  w = EncodeU8(w,      'K', player->powerAvailable);
  w = EncodeU8(w,      'k', player->powerControls);

  Net_Send_UpdatePlayer(player->multiplayerReference, player->timestamp, d);
}

void Player_Recv_Update(u8 slot, const char* msg)
{
  Player* player = &PLAYER[slot];
  player->obj.type = OT_PLAYER;
  player->isNetwork = true;

  char *prop = strtok((char*)msg, ";");
  while(prop)
  {
    char name = prop[0];
    char* value = prop + 1;
    
    switch(name)
    {
      case 'P': DecodeVec3XZ(value, &player->obj.position);     break;
      case 'V': DecodeVec3XZ(value, &player->obj.velocity);     break;
      case 'A': DecodeVec3XZ(value, &player->obj.acceleration); break;
      case 'Y': DecodeI16(value, &player->obj.yaw);             break;
      case 't': DecodeU8(value, &player->team);                 break;
      case 's': DecodeFloat(value, &player->steering);          break;
      case 'a': DecodeFloat(value, &player->angularVelocity);   break;
      case 'h': DecodeFloat(value, &player->heading);           break;
      case 'r': DecodeFloat(value, &player->yawRate);           break;
      case 'g': DecodeU8(value, &player->score);                break;
      case 'j': DecodeU8(value, &player->justScored);           break;
      case 'J': DecodeFloat(value, &player->scoreTime);         break;
      case '0': DecodeFloat(value, &player->powerCooldown[0]);  break;
      case '1': DecodeFloat(value, &player->powerCooldown[1]);  break;
      case '2': DecodeFloat(value, &player->powerCooldown[2]);  break;
      case 'K': DecodeU8(value, &player->powerAvailable);       break;
      case 'k': DecodeU8(value, &player->powerControls);        break;
    }

    prop = strtok(NULL, ";");
  }
}

void Player_Send_Ball_Update(Ball* ball)
{
  char* d = $.Mem.TempAllocator(512);
  char* w = d;
  w = EncodeVec3XZ(w, 'P', ball->obj.position);
  w = EncodeVec3XZ(w, 'V', ball->obj.velocity);
  w = EncodeVec3XZ(w, 'A', ball->obj.acceleration);
  w = EncodeI16(w,    'Y', ball->obj.yaw);
  w = EncodeU8(w,     'b', ball->blue);
  w = EncodeU8(w,     'r', ball->red);
  w = EncodePlayer(w, 't', ball->lastTouch);
  w = EncodeFloat(w,  'm', ball->magnetTime);
  w = EncodePlayer(w, 'M', ball->magnet);
  w = EncodeFloat(w,  'T', ball->gameTime);

  Net_Send_UpdateBall(d);
}

void Player_Recv_Ball_Update(Ball* ball, const char* msg)
{
  char *prop = strtok((char*)msg, ";");
  while(prop)
  {
    char name = prop[0];
    char* value = prop + 1;
    
    switch(name)
    {
      case 'P': DecodeVec3XZ(value, &ball->obj.position);     break;
      case 'V': DecodeVec3XZ(value, &ball->obj.velocity);     break;
      case 'A': DecodeVec3XZ(value, &ball->obj.acceleration); break;
      case 'Y': DecodeI16(value, &ball->obj.yaw);             break;
      case 'r': DecodeU8(value, &ball->red);                  break;
      case 'b': DecodeU8(value, &ball->blue);                 break;
      case 't': DecodePlayer(value, &ball->lastTouch);        break;
      case 'm': DecodeFloat(value, &ball->magnetTime);        break;
      case 'M': DecodePlayer(value, &ball->magnet);           break;
      case 'T': DecodeFloat(value, &ball->gameTime);          break;
    }

    prop = strtok(NULL, ";");
  }
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

u8 Player_New(u16 reference)
{
  for(u32 i=0;i < MAX_PLAYERS;i++)
  {
    if (PLAYER[i].obj.type == OT_NONE)
    {
      PLAYER[i].multiplayerReference = reference;
      PLAYER[i].isNetwork            = 1;
      return i;
    }
  }
  return 255;
}

Player* Player_GetByReference(u16 reference)
{
  for(u32 i=0;i < MAX_PLAYERS;i++)
  {
    if (PLAYER[i].obj.type != OT_NONE && PLAYER[i].multiplayerReference == reference)
    {
      return &PLAYER[i];
    }
  }
  return NULL;
}

Player* Player_GetByReferenceOrCreate(u16 reference)
{
  for(u32 i=0;i < MAX_PLAYERS;i++)
  {
    if (PLAYER[i].obj.type != OT_NONE && PLAYER[i].multiplayerReference == reference)
    {
      return &PLAYER[i];
    }
  }

  u8 slot = Player_New(reference);
  if (slot == 255)
    return NULL;

  return &PLAYER[slot];
}

Player* Player_GetFirstControlled()
{
  for(u32 i=0;i < MAX_PLAYERS;i++)
  {
    if (PLAYER[i].obj.type != OT_NONE && PLAYER[i].multiplayerIsControlled == 1)
    {

      return &PLAYER[i];
    }
  }
  return NULL;
}
