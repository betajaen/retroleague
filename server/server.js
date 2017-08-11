// Load the TCP Library
net = require('net');
var StringBuilder = require('stringbuilder')
StringBuilder.extend('string');

var readline      = require('readline');
var port          = 27960;
var clients       = [];
var teams         = [0,1,0,1];
var clientsWithSlots = [null,null,null,null];
var references    = [0,0,0,0];
var types         = [0,0,0,0];
var lastStates    = ["","","",""];
var lastBallState = "";
var times          = [0,0,0,0]
var host          = null;

function NewSlot(type, reference)
{
  for(var slot=0;slot < types.length;slot++)
  {
    if (types[slot] == 0)
    {
      types[slot] = type;
      references[slot] = reference;
      lastStates[slot] = "";
      times[slot] = 0;
      clientsWithSlots[slot] = null;
      return slot;
    }
  }
  return -1;
}

function FindSlot(reference)
{
  return references.indexOf(reference);
}

function DeleteSlot(slot)
{
  references[slot] = 0;
  types[slot] = 0;
  lastStates[slot] = "";
  times[slot] = 0;
  clientsWithSlots[slot] = null;
}

function GetNumActiveSlots()
{
  var count = 0;
  for(var ii=0;ii < types.length;ii++)
  {
    if (types[ii] != 0)
      count++;
  }
  return count;
}

function Send(from, to, msg)
{
  if (to == null)
  {
    clients.forEach(function (client)
    {
      if (from != null && from == client)
        return;
      
      console.log(">> to:" + client.name + ", len:" + msg.length.toString() + ", message:" + msg.toString());
      client.write(msg.toString() + "\n");
    });
  }
  else
  {
    console.log(">> to:" + to.name + ", len:" + msg.length.toString() + ", message:" + msg.toString());
    to.write(msg.toString() + "\n");
  }
}

function Net_SendStatus(from, to, numClients, isHost)
{
  Send(from, to, String.format("S {0} {1}", numClients.toString(), isHost ? "1" : "0"));
}

function Net_OnConnection(client)
{
  console.log("Client Connection: " + client.name);
  if (clients.length == 1)
  {
    host = client;
  }
  Net_SendStatus(null, client, clients.length, clients.length == 1);
  
  for(var ii=0;ii < types.length;ii++)
  {
      if (clientsWithSlots[ii] != client && types[ii] != 0)
      {
        Send(null, client, String.format("S {0} {1}", GetNumActiveSlots().toString(), client == host));
        Send(null, client, String.format("C {0} {1} 0 {2}", references[ii], types[ii], teams[ii]));
        Send(null, client, String.format("U {0} {1} {2}", references[ii], times[ii], lastStates[ii]));
      }
      
      if (lastBallState != "")
      {
        Send(null, client, String.format("B {0}", lastBallState));
      }
  }
}

function Net_OnDisconnection(client)
{
  console.log("Client Disconnection: " + client.name);

  for(var i=0;i < types.length;i++)
  {
    if (clientsWithSlots[i] == client)
      DeleteSlot(i);
  }

  if (host == client)
  {
    host = (clients.length > 0) ? clients[0] : null;
    
    if (host)
    {
      Net_SendStatus(host, null, GetNumActiveSlots(), false);
      Net_SendStatus(null, host, GetNumActiveSlots(), true);
    }
  }

  if (clients.length == 0)
  {
    lastBallState = "";
  }

}

function Net_CreatePlayer(client, reference, isBot)
{
  var type = isBot ? 2 : 1;
  var slot = NewSlot(type, reference);

  if (slot != -1)
  {
    clientsWithSlots[slot] = client;
    Send(null, client, String.format("C {0} {1} 1 {2}", reference, type, teams[slot]));
    Send(client, null, String.format("C {0} {1} 0 {2}", reference, type, teams[slot]));
  }
  else
  {
    Send(null, client, String.format("C {0} 0 0 0", reference));  // No.
  }
}

function Net_DeletePlayer(client, reference)
{
  var slot = FindSlot(reference);
  if (slot != -1)
  {
    DeleteSlot(slot);
    
    Send(null, client, String.format("D {0}", reference));
    Send(client, null, String.format("D {0}", reference));
  }
}

function Net_UpdatePlayer(client, reference, time, data)
{
  var slot = FindSlot(reference);
  //console.log(slot);
  if (slot != -1)
  {
    times[slot]     = time;
    lastStates[slot] = data;
    Send(client, null, String.format("U {0} {1} {2}", reference, time, data));
  }
}

function Net_UpdateBall(client, data)
{
  lastBallState = data;
  Send(client, null, String.format("B {0}", data));
}

function Net_OnMessage(client, msg)
{
  console.log("<< (" + client.toString() + ") " + msg.toString());

  if (msg.length < 2)
    return;

  if (msg[0] == 'C')
  {
    // Create Player
    var p = msg.split(' ', 3);
    Net_CreatePlayer(client, parseInt(p[1]), parseInt(p[2]))
  }
  else if (msg[0] == 'D')
  {
    // Delete Player
    var p = msg.split(' ', 2);
    Net_DeletePlayer(client, parseInt(p[1]));
  }
  else if (msg[0] == 'U')
  {
    // Update Player
    var p = msg.split(' ', 4);
    Net_UpdatePlayer(client, parseInt(p[1]), parseInt(p[2]), p[3])
  }
  else if (msg[0] == 'B')
  {
    // Update Ball
    var p = msg.split(' ', 2);
    Net_UpdateBall(client, p[1])
  }

}






// Start a TCP Server
net.createServer(function (client)
{
  client.name = client.remoteAddress + ":" + client.remotePort 
  clients.push(client);
  Net_OnConnection(client);

  var chunk = "";
  client.on('data', function(data) {

    chunk += data.toString(); // Add string on the end of the variable 'chunk'
    d_index = chunk.indexOf('\n'); // Find the delimiter
    
    // While loop to keep going until no delimiter can be found
    while (d_index > -1) {         
        line = chunk.substring(0, d_index); // Create string up until the delimiter
        Net_OnMessage(client, line);
        chunk = chunk.substring(d_index+1); // Cuts off the processed chunk
        d_index = chunk.indexOf('\n'); // Find the new delimiter
    }      
  });

  client.on('error', function() {});

  client.on('end', function() {
      clients.splice(clients.indexOf(client), 1);
      Net_OnDisconnection(client);
  });

}).listen(port);

// Put a friendly message on the terminal of the server.
console.log("Retro League /// Running at port " + port + "\n");