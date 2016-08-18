#!/usr/bin/env python

import sys
import socket
import struct
import math
import random

if sys.version_info[0] < 3:
    def bytes(data, encoding):
        return data

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

if __name__ == "__main__":
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("192.168.2.123", 3490))

    s.send(bytes("n Stupobot\n", 'UTF-8'))
    readall(s)
    s.send(bytes("b\n", 'UTF-8'))

    angle = random.uniform(-180, 180)
    speed = 10
    mypid = None
    s.send(bytes("v %f\n%f\n" % (speed, angle), 'UTF-8'))

    while True:
       data = recvall(s, 8)
       typ, pid = struct.unpack("II", data)
       if typ == 1:
          mypid = pid
          print ("My id: %d" % (mypid, ))
       elif typ == 2:
          print ("Player %d left" % (pid, ))
          s.send(bytes("r\n", 'UTF-8'))
       elif typ == 3:
          data = recvall(s, 8)
          fx, fy = struct.unpack("ff", data)
          print ("Player %d at %f, %f" % (pid, fx, fy))
          s.send(bytes("r\n", 'UTF-8'))
       elif typ == 4:
          data = recvall(s, 4)
          n = struct.unpack("I", data)[0]
          print ("Player %d finished shot with %d segments" % (pid, n))
          for i in range(n):
             data = recvall(s, 8)
             print ("Courve: %r" % (struct.unpack("ff", data), ))
          if pid == mypid:
             angle += 361 / 36.0
             if angle > 180:
                angle -= 360
             s.send(bytes("v %f\n%f\n" % (speed, angle), 'UTF-8'))
       else:
          print ("Unexpected data: %r" % (data, ))



