#!/usr/bin/python
from Box2D import *
from pygame import init,joystick,event
from math import *
import random
init()
class Robot():
	IMPULSE=100000#impulse in kg*ft*s^-1
	def __init__(self):
		self.world=b2World(gravity=(0,0))
		for x,y,ex,ey in(#50x80 field with 4 static objects (walls) TODO add 2; locations are in (cx,cy,w/2,h/2)
				(-27.5,0,.5,13.5),
				(27.5,0,.5,13.5),
				(0,-14,27,.5),
				(0,14,27,.5),
			):
			body=self.world.CreateStaticBody(
				position=(x,y),
				shapes=b2PolygonShape(box=(ex,ey)),
			)#friction .2
		self.robot=self.world.CreateDynamicBody(position=(-20,0))
		box=self.robot.CreatePolygonFixture(box=(3,3),density=100,friction=.7)#TODO use right parameters; 100 kg? 3'x3f; friction does NOTHING for now
		self.robot.linearDamping=.7
		self.robot.angularDamping=1.4
		self.joy=None
		if joystick.get_count()<1:
			print"warning: no joystick"
			self.joy=None
		else:
			self.joy=joystick.Joystick(0)
			self.joy.init()
			print"joystick",self.joy.get_name()
	def step(self,delay):
		if self.joy:
			event.get()
			curve,magnitude=[self.joy.get_axis(x)for x in xrange(2)]
		else:
			curve,magnitude=[random.uniform(-1,1)for _ in xrange(2)]
		self.robot.ApplyLinearImpulse(impulse=b2Vec2(cos(self.robot.angle),-sin(self.robot.angle))*(-magnitude*Robot.IMPULSE),point=self.robot.position)
		self.robot.ApplyTorque(curve*Robot.IMPULSE)
		self.world.Step(delay,10,10)
		self.world.ClearForces()
	def __iter__(self):
		return{
			"MOV":" ".join(map(str,self.robot.position)),
		}.items().__iter__()
if __name__=="__main__":
	r=Robot()
	print r.robot
	for i in xrange(10):
		print i,"iterations"
		for k,v in r:
			print k,"=",v
		r.step(1)
	print "finished"
	for k,v in r:
		print k,"=",v
