#!/usr/bin/env python

import sys
import socket
import struct
import math
import random
import time

class Network:
   def readall(self):
      timeout = self.s.gettimeout()
      self.s.settimeout(1)
      while True:
         try:
            self.s.recv(4096)
         except socket.timeout:
            break
      self.s.settimeout(timeout)

   def recvall(self, count):
      buf = b''
      while count:
         newbuf = self.s.recv(count)
         if not newbuf: return None
         buf += newbuf
         count -= len(newbuf)
      return buf

   def __init__(self, cnt):
      self.cnt = cnt
      self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      self.s.connect(("127.0.0.1", 3490))

      self.s.send(bytes("n Stupobot 8\n", 'UTF-8'))
      self.readall()
      self.s.send(bytes("b 8\n", 'UTF-8'))

      self.s.send(bytes("u\nv %f\n" % self.cnt.speed, 'UTF-8'))

   def step(self, cnt):
      data = self.recvall(8)
      typ, pid = struct.unpack("II", data)
      if typ == 1:
         self.cnt.mypid = pid
         print ("My id: %d" % (self.cnt.mypid, ))
      elif typ == 2:
         print("Player %d left" % (pid, ))
      elif typ == 3:
         data = self.recvall(8)
         fx, fy = struct.unpack("ff", data)
         print("Player %d at %f, %f" % (pid, fx, fy))
      elif typ == 5:
         data = self.recvall(16)
         a, v = struct.unpack("dd", data)
         print("Player %d begun shot (v: %lf, a: %lf)" % (pid, v, a))
         if pid == self.cnt.mypid:
            self.cnt.e = self.cnt.e - v
            self.cnt.angle = a + 370.5 / 5
            if self.cnt.angle > 360:
               self.cnt.angle = self.cnt.angle - 360
            if self.cnt.e >= v:
               self.s.send(bytes("%f\n" % self.cnt.angle, 'UTF-8'))
            else:
               self.s.send(bytes("u\n", 'UTF-8'))
      elif typ == 6:
         data = self.recvall(20)
         a, v, n = struct.unpack("ddI", data)
         print("Player %d finished shot (v: %lf, a: %lf) with %d segments" % (pid, v, a, n))
         for i in range(n):
            data = self.recvall(8)
            #print("Curve: %r" % (struct.unpack("ff", data), ))
      elif typ == 7:
         data = self.recvall(4)
         m = struct.unpack("I", data)[0]
         if m != 3:
            print("Unsupported gamemode %d, expect strangeness" % m)
      elif typ == 8:
         data = self.recvall(8)
         self.cnt.e = struct.unpack("d", data)[0]
         if self.cnt.e < 50:
            time.sleep(2)
            self.s.send(bytes("u\n", 'UTF-8'))
         else:
            self.s.send(bytes("%f\n" % self.cnt.angle, 'UTF-8'))
      else:
         print("Unexpected data: %r" % (data, ))

         
class Controller:
   angle = 0
   speed = 10
   mypid = None
   e = 0

   def step(self):
      self.net.step(self)

   def __init__(self):
      self.net = Network(self)

      
if __name__ == "__main__":
   cnt = Controller()

   while True:
      cnt.step()












