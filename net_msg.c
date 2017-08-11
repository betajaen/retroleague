#include "funk.h"
#include <string.h>
#include <stdlib.h>

bool CheckTimestamp(u8 playerIndex, u32 timestamp)
{
  if (timestamp < PLAYER[playerIndex].timestamp)
  {
    // old message
    //printf("Old message from Player: %i", playerIndex);
    return false;
  }

  PLAYER[playerIndex].timestamp = timestamp;

  return true;
}

u8 ReadSingleDigit(const char* message, u32 index)
{
  return message[index] - '0';
}

u32 ReadUint(const char* message, u32 index)
{
  return atoi(&message[index]);
}

static bool isHosting, isConnected;
static u32    numClients;

bool Net_IsHosting()
{
  return isHosting;
}

bool Net_IsAlive()
{
  return isConnected;
}

u32  Net_GetNumClients()
{
  return numClients;
}

void Net_Receive_Status()
{
  u32 nbClients = 0, shouldHost;
  $.Net.RecvLine("S %i %i", &nbClients, &shouldHost);
  
  isConnected = true;
  isHosting   = (shouldHost == 1);
  numClients  = (nbClients);
  
  // @TODO If nbClients == 1, then we should add bots.
  //       If nbClients != 0, then they should be bots.

  //printf("Received Status: NumClients=%i, IsHosting=%i\n", numClients, isHosting);
}

// Client wants to connect to server
// reference = Internal reference of local player to connect
void Net_Send_CreatePlayer(u16 reference, u8 isBot)
{
  $.Net.SendLine("C %i %i", reference, isBot);
}

void Net_Receive_CreatePlayer()
{
  u32 reference = 0, type = 0, isController = 0, team = 0;
  $.Net.RecvLine("C %i %i %i %i", &reference, &type, &isController, &team);
  printf("Received Connected Reference=%i Type=%i IsController=%i Team=%i\n", reference, type, isController, team);
  
  // TODO
  // Find player by reference, and set it up.
  Player* player = Player_GetByReferenceOrCreate(reference);
  if (player == NULL)
  {
    printf("Slots are full!");
    return;
  }

  player->team = team;
  player->obj.type = OT_PLAYER;
  player->multiplayerType = type;
  player->multiplayerIsControlled = isController;
}

void Net_Send_DeletePlayer(u16 reference)
{
  $.Net.SendLine("D %i", reference);
}

void Net_Receive_DeletePlayer()
{
  u32 reference;
  $.Net.RecvLine("D %i", &reference);
  
  Player* player = Player_GetByReference(reference);
  if (player == NULL)
  {
    return;
  }
  
  memset(player, 0, sizeof(Player));
  
  if (LOCAL[0] == player)
    LOCAL[0] = NULL;
  if (LOCAL[1] == player)
    LOCAL[1] = NULL;
  
}

void Net_Send_UpdatePlayer(u16 reference, u32 time, char* data)
{
  $.Net.SendLine("U %i %i %s", reference, time, data);
}

void Net_Receive_Update()
{
  u32   dataLength = ($.Net.RecvLine(NULL));
  char* data = $.Mem.TempAllocator(dataLength + 1);
  data[0] = '\0';

  u32 playerReference = 0, time = 0;
  u32 length = $.Net.RecvLine("U %i %i %s", &playerReference, &time, data);
  
  Player* player = Player_GetByReference(playerReference);
  if (player == NULL)
  {
    //printf("Unknown Player Reference=%i\n", playerReference);
    return;
  }

  Player_Recv_Update(FindPlayerIndex(player), data);
}

void Net_Send_UpdateBall(char* data)
{
  $.Net.SendLine("B %s", data);
  //printf("Ball:SEND => %s\n", data);
}

void Net_Recv_UpdateBall()
{
  u32   dataLength = ($.Net.RecvLine(NULL));
  char* data = $.Mem.TempAllocator(dataLength + 1);
  data[0] = '\0';

  u32 length = $.Net.RecvLine("B %s", data);
  
  Player_Recv_Ball_Update(&BALL, data);
  //printf("Ball:RECV => %s\n", data);
}

void Net_Start(const char* host, const char* port)
{
  isConnected = false;
  isHosting = false;
  $.Net.Connect(host, atoi(port));

  // Only considered connected when get a status message
}

void Net_Stop()
{
  $.Net.Disconnect();
}

void Net_Update()
{
  $.Net.RecvLines();

  while(true)
  {
    const char* str;
    u32 length = $.Net.PeekLine(&str);
    if (length <= 1)
      break;
    
    switch (str[0])
    {
      default:  printf("Unknown Message %s\n", str);
                $.Net.SkipLine();               break;
      case 'S': Net_Receive_Status();           break;
      case 'C': Net_Receive_CreatePlayer();     break;
      case 'D': Net_Receive_DeletePlayer();     break;
      case 'U': Net_Receive_Update();           break;
      case 'B': Net_Recv_UpdateBall();          break;
    }
  }
}
