#include "funk.h"
#include <string.h>
#include <stdlib.h>

#define NET_MSG_CLIENT_HELLO          'h'    // Client Connection
#define NET_MSG_CLIENT_GOODBYE        'g'    // Client Disconnection
#define NET_MSG_CLIENT_PARTIAL_UPDATE 'p'    // Player sends partial update
#define NET_MSG_CLIENT_UPDATE         'u'    // Player send full update
#define NET_MSG_CLIENT_REQUEST_STATE  'r'    // Response from server for full update

#define NET_MSG_SERVER_FULL           'F'    // Server is full.
#define NET_MSG_SERVER_HELLO          'H'    // You have connected
#define NET_MSG_SERVER_GOODBYE        'G'    // You have disconnected
#define NET_MSG_SERVER_PARTIAL_UPDATE 'P'    // Here is a partial update
#define NET_MSG_SERVER_UPDATE         'U'    // Here is a full update
#define NET_MSG_SERVER_REQUEST_STATE  'R'    // I want full full update
#define NET_MSG_SERVER_DELETE_PLAYER  'D'    // Other Player has left

// PLAYER_INDEX
// >> Byte
// >> Starts at 0 to 7

// Time stamp
// >> U32


bool CheckTimestamp(u8 playerIndex, u32 timestamp)
{
  if (timestamp < PLAYER[playerIndex].timestamp)
  {
    // old message
    printf("Old message from Player: %i", playerIndex);
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

void ReceiveMessage(const char* message)
{
  u8 messageType = message[0];
  
  switch(messageType)
  {
    // F
    // PLAYER_INDEX
    case NET_MSG_SERVER_FULL:
    {
      if (strlen(message) != 1)
        return;

      printf("[SERVER] Server is full.\n");
    }
    break;
    // H
    // PLAYER_INDEX
    // TEAM
    case NET_MSG_SERVER_HELLO:
    {
      if (strlen(message) != 3)
        return;

     // printf("[SERVER] I have Connected.\n");
      memset(PLAYER, 0, sizeof(PLAYER));
      u8 playerIndex = ReadSingleDigit(message, 1);
      u8 team = ReadSingleDigit(message, 2);
      Player_New(playerIndex, team, true);
    }
    break;
    // G
    case NET_MSG_SERVER_GOODBYE:
    {
      if (strlen(message) != 1)
        return;

    //  printf("[SERVER] I have Disconnected.\n");
      if (ME)
      {
        Player_DeleteMe();
      }
    }
    break;
    // 0 P              1
    // 1 PLAYER_INDEX   2
    // 2 TIMESTAMP      6
    case NET_MSG_SERVER_PARTIAL_UPDATE:
    {
      u8 playerIndex = ReadSingleDigit(message, 1);
//      printf("[SERVER] Received Partial Update %i\n", playerIndex);
      u32 timeStamp  = ReadUint(message, 2);
      if (CheckTimestamp(playerIndex, timeStamp) == false)
        return;
      Player_ReceivePartialUpdate(playerIndex, &message[6]);
    }
    break;
    // 0 U             1
    // 1 PLAYER_INDEX  2
    // 2 TIMESTAMP     6
    case NET_MSG_SERVER_UPDATE:
    {
      u8 playerIndex = ReadSingleDigit(message, 1);
 //     printf("[SERVER] Received Full Update %i\n", playerIndex);
      u32 timeStamp  = ReadUint(message, 2);
      if (CheckTimestamp(playerIndex, timeStamp) == false)
        return;
      Player_ReceiveFullUpdate(playerIndex, &message[6]);
    }
    break;
    case NET_MSG_SERVER_REQUEST_STATE:
    {
  //    printf("[SERVER] Wants full Update\n");
      Player_SendFullUpdate();
    }
    break;
    // 0 D
    // 1 PLAYER_INDEX  2
    case NET_MSG_SERVER_DELETE_PLAYER:
    {
   //   printf("[SERVER] Delete player");
      u8 playerIndex = ReadSingleDigit(message, 1);
      Player_Delete(playerIndex);
    }
    break;
  }
}
