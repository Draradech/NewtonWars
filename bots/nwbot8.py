#!/usr/bin/env python

import sys
import socket
import struct
import math
import random
import time

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
    s.connect(("127.0.0.1", 3490))

    s.send(bytes("n Stupobot 8\n", 'UTF-8'))
    readall(s)
    s.send(bytes("b 8\n", 'UTF-8'))

    angle = 0
    speed = 10
    mypid = None
    e = 0
    s.send(bytes("u\nv %f\n" % speed, 'UTF-8'))

    while True:
       data = recvall(s, 8)
       typ, pid = struct.unpack("II", data)
       if typ == 1:
          mypid = pid
          print ("My id: %d" % (mypid, ))
       elif typ == 2:
          print ("Player %d left" % (pid, ))
       elif typ == 3:
          data = recvall(s, 8)
          fx, fy = struct.unpack("ff", data)
          print ("Player %d at %f, %f" % (pid, fx, fy))
       elif typ == 5:
          data = recvall(s, 16)
          a, v = struct.unpack("dd", data)
          print ("Player %d begun shot (v: %lf, a: %lf)" % (pid, v, a))
          if pid == mypid:
             e = e - v
             angle = a + 370.5 / 5
             if angle > 360:
                angle = angle - 360
             if e >= v:
                s.send(bytes("%f\n" % angle, 'UTF-8'))
             else:
                s.send(bytes("u\n", 'UTF-8'))
       elif typ == 6:
          data = recvall(s, 20)
          a, v, n = struct.unpack("ddI", data)
          print ("Player %d finished shot (v: %lf, a: %lf) with %d segments" % (pid, v, a, n))
          for i in range(n):
             data = recvall(s, 8)
             #print ("Curve: %r" % (struct.unpack("ff", data), ))
       elif typ == 7:
          data = recvall(s, 4)
          m = struct.unpack("I", data)[0]
          if m != 3:
             print ("Unsupported gamemode %d, expect strangeness" % m)
       elif typ == 8:
          data = recvall(s, 8)
          e = struct.unpack("d", data)[0]
          if e < 50:
             time.sleep(2)
             s.send(bytes("u\n", 'UTF-8'))
          else:
             s.send(bytes("%f\n" % angle, 'UTF-8'))
       else:
          print ("Unexpected data: %r" % (data, ))



