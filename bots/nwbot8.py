#!/usr/bin/env python
import sys
import socket
import struct
import math
import random
import time
import select



class Network:
   def __init__(self, bot, name, ip):
      self.bot = bot
      self.initMsg()
      self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      self.s.connect((ip, 3490))
      self.send("n %s\n" % name)
      self.readall()
      self.send("b 8\n")

   def initMsg(self):
      self.cntHead = 8
      self.cntHead2 = -1
      self.cntBody = -1
      self.bufHead = b''
      self.bufHead2 = b''
      self.bufBody = b''
      self.msgID = 0
      self.pid = -1

   def send(self, message):
      self.s.send(bytes(message, 'UTF-8'))

   def readall(self):
      self.s.settimeout(1)
      while True:
         try:
            self.s.recv(1024)
         except socket.timeout:
            break

   def shoot(self, a):
      self.send("%lf\n" % a)

   def requestEnergy(self):
      self.send("u\n")

   def step(self):
      while self.stepSingle() > 0:
         pass

   def stepSingle(self):
      r, w, x = select.select((self.s, ), (), (), 0)
      if len(r) == 0:
         return 0
      if self.cntHead > 0:
         data = self.s.recv(self.cntHead)
         self.cntHead -= len(data)
         self.bufHead += data
         if self.cntHead == 0:
            self.evalHead()
      elif self.cntHead2 > 0:
         data = self.s.recv(self.cntHead2)
         self.cntHead2 -= len(data)
         self.bufHead2 += data
         if self.cntHead2 == 0:
            self.evalHead2()
      elif self.cntBody > 0:
         data = self.s.recv(self.cntBody)
         self.cntBody -= len(data)
         self.bufBody += data
         if self.cntBody == 0:
            self.evalBody()
      return len(r)

   def evalHead(self):
      self.msgID, self.pid = struct.unpack('II', self.bufHead)
      if self.msgID == 1:
         self.bot.ownId(self.pid)
         self.initMsg()
      elif self.msgID == 2:
         self.bot.playerLeave(self.pid)
         self.initMsg()
      elif self.msgID == 3:
         self.cntBody = 8
      elif self.msgID == 5:
         self.cntBody = 16
      elif self.msgID == 6:
         self.cntHead2 = 20
      elif self.msgID == 7:
         self.cntBody = 4
      elif self.msgID == 8:
         self.cntBody = 8
      else:
         print("unexpected message id in head: %d" % self.msgID)

   def evalHead2(self):
      if self.msgID == 6:
         a, v, n = struct.unpack("ddI", self.bufHead2)
         self.cntBody = n * 8
      else:
         print("unexpected message id in head2: %d" % self.msgID)

   def evalBody(self):
      if self.msgID == 3:
         fx, fy = struct.unpack("ff", self.bufBody)
         self.bot.playerPos(self.pid, fx, fy)
      elif self.msgID == 5:
         a, v = struct.unpack("dd", self.bufBody)
         self.bot.shotBegin(self.pid, a, v)
      elif self.msgID == 6:
         a, v, n = struct.unpack("ddI", self.bufHead2)
         curve = []
         for i in range(n):
            curve.append(struct.unpack_from("ff", self.bufBody, 8 * i))
         self.bot.shotFin(self.pid, a, v, curve)
      elif self.msgID == 7:
         m, = struct.unpack("I", self.bufBody)
         if m != 3:
            print("unsupported gamemode %d, expect strangeness" % m)
      elif self.msgID == 8:
         e, = struct.unpack("d", self.bufBody)
         self.bot.ownEnergy(e)
      else:
         print("unexpected message id in body: %d" % self.msgID)
      self.initMsg()



class Stupobot:
   def __init__(self, ip):
      self.net = Network(self, "Stupobot 8", ip)
      self.angle = 0
      self.mypid = None
      self.energy = 0
      self.nextTime = time.time()

   def step(self):
      self.net.step()
      t = time.time()
      if t > self.nextTime:
         self.net.requestEnergy()

   def ownId(self, pid):
      self.mypid = pid
      print ("My id: %d" % self.mypid)

   def playerLeave(self, pid):
      print("Player %d left" % pid)

   def playerPos(self, pid, fx, fy):
      print("Player %d at %f, %f" % (pid, fx, fy))

   def shotBegin(self, pid, a, v):
      print("Player %d begun shot (v: %lf, a: %lf)" % (pid, v, a))
      if pid == self.mypid:
         self.energy -= v
         self.angle = (self.angle + 361 / 5) % 360
         if self.energy >= v:
            self.net.shoot(self.angle)
         else:
            self.net.requestEnergy()

   def shotFin(self, pid, a, v, curve):
      print("Player %d finished shot (v: %lf, a: %lf) with %d segments" % (pid, v, a, len(curve)))
      for fx, fy in curve:
         pass #print("   Curve: %f, %f" % (fx, fy))

   def ownEnergy(self, e):
      print("Own energy: %lf" % e)
      if e < 50:
         self.nextTime = time.time() + 50 - e
      else:
         self.nextTime += 5 # will be updated when all shots are fired, should never take more than 5s
         self.energy = e
         self.net.shoot(self.angle)



if __name__ == "__main__":
   bot = Stupobot("192.168.2.123")

   while True:
      bot.step()
      time.sleep(0.1)




