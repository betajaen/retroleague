// Load the TCP Library
net = require('net');

// Keep track of the chat clients
var clients = [];

var maxplayers = 4;

// 
var slots = [false,false,false,false];

function getFreeSlot()
{
  for(var i=0;i < maxplayers;i++)
    if (slots[i] == false)
      return i;
  return -1;
}

// Start a TCP Server
net.createServer(function (socket) {

  // Identify this client
  socket.name = socket.remoteAddress + ":" + socket.remotePort 

  slot = getFreeSlot();
  if (slot == -1)
  {
    // Server full
    broadcast("F", socket);
  }
  else
  {
    clients.push(socket);
    slots[slot] = true;
    team = 1
    if (slot == 0 || slot == 2)
      team = 0;

    // PLAYER > HELLO
    socket.write("H" + slot + "" + team + "\n");

    // ALL > REQUEST STATE
    broadcastAll("R\n", socket);

    socket.on('data', function (data) {
      broadcast(data, socket);
    });
    
    socket.on('error', function () {});

    socket.on('end', function () {
      clients.splice(clients.indexOf(socket), 1);
      // ALL > DELETE PLAYER
      broadcast("D" + slot + "\n");
      slots[slot] = false;
    });
    
    function broadcast(message, sender)
    {
      clients.forEach(function (client)
      {
        if (client === sender) return;
          client.write(message);
      });
      //process.stdout.write(message + "\n");
    }
    function broadcastAll(message)
    {
      clients.forEach(function (client)
      {
        client.write(message);
      });
      //process.stdout.write(message + "\n");
    }
  }
}).listen(5000);

// Put a friendly message on the terminal of the server.
console.log("Retro League /// Running at port 5000\n");