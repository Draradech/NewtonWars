import socket
import struct

def readall(s):
   timeout = s.gettimeout()
   s.settimeout(1)
   while True:
      try:
         s.recv(4096)
      except socket.timeout:
         break
   s.settimeout(timeout)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("94.45.232.125", 3490))

s.send("n Drarabot\n")
readall(s)
s.send("b\n")

angle = 0
while True:
   data = s.recv(8)
   typ, pid = struct.unpack("II", data)
   if typ == 1:
      mypid = pid
      print "My id: %d" % mypid
   elif typ == 2:
      print "Player %d left" % pid
      s.send("%f\n" % angle)
   elif typ == 3:
      data = s.recv(8)
      fx, fy = struct.unpack("ff", data)
      print "Player %d at %f, %f" % (pid, fx, fy)
      s.send("%f\n" % angle)
   elif typ == 4:
      data = s.recv(4)
      n = struct.unpack("I", data)[0]
      print "Player %d finished shot with %d segments:" % (pid, n)
      while n > 0:
         n -= 1
         data = s.recv(8)
         print repr(struct.unpack("ff", data))
      if pid == mypid:
         angle += 5
         s.send("%f\n" % angle)
      

