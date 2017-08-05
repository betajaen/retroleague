require 'socket'
sock = TCPSocket.new('192.168.1.10', 3000)
sock.write 'GETHELLO'
puts sock.read()
sock.close