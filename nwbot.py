#!/usr/bin/env python
import socket
import struct
import math
import random

def readall(s):
   timeout = s.gettimeout()
   s.settimeout(1)
   while True:
      try:
         s.recv(4096)
      except socket.timeout:
         break
   s.settimeout(timeout)

def recvall(sock, count):
   buf = b''
   while count:
      newbuf = sock.recv(count)
      if not newbuf: return None
      buf += newbuf
      count -= len(newbuf)
   return buf

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 3490))

s.send("n Stupobot\n")
readall(s)
s.send("b\n")

angle = random.uniform(-180, 180)
speed = 10
mypid = None
s.send("v %f\n%f\n" % (speed, angle))

while True:
   data = recvall(s, 8)
   typ, pid = struct.unpack("II", data)
   if typ == 1:
      mypid = pid
      print "My id: %d" % mypid
   elif typ == 2:
      print "Player %d left" % pid
      s.send("r\n")
   elif typ == 3:
      data = recvall(s, 8)
      fx, fy = struct.unpack("ff", data)
      print "Player %d at %f, %f" % (pid, fx, fy)
      s.send("r\n")
   elif typ == 4:
      data = recvall(s, 4)
      n = struct.unpack("I", data)[0]
      print "Player %d finished shot with %d segments" % (pid, n)
      while n > 0:
         n -= 1
         recvall(s, 8)
      if pid == mypid:
         angle += 361 / 3.0
	 if angle > 180:
	   angle -= 360
         s.send("v %f\n%f\n" % (speed, angle))
   else:
      print "Unexpected data: %s" % repr(data)



