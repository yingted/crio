#!/usr/bin/python
from physics import Robot
from twisted.internet import reactor
from twisted.internet.endpoints import TCP4ServerEndpoint
from twisted.internet.protocol import Factory
from twisted.internet.task import LoopingCall
from twisted.protocols.basic import LineOnlyReceiver
from Box2D import b2_pi
class Crio(LineOnlyReceiver):
	delimiter="\n"
	def update(self):
		self.robot.step(.1)
		msg=[]
		for k,v in self.robot:
			msg.append("SET "+k+" "+v)
		self.log("\n".join(msg))
	def ping(self):
		self.log("PING "+str(self.hi))
		self.hi+=1
	def sched(self):
		for cmd,state in self.tasks:
			prog=getattr(self,"iter_"+cmd)(state)
			self.log("PROGRESS "+str(state.cid)+" "+cmd+" "+str(prog))
			if prog==1:
				del self.tasks[cmd]
	def log(self,msg):
		print msg
		reactor.callInThread(self.sendLine,msg)
	def __init__(self,robot):
		self.robot=robot
		self.tasks={}
		self.lo=self.hi=1
		self.bg=[]
	def connectionMade(self):
		for fun,delay in((self.update,.1),(self.sched,.1),(self.ping,1)):
			call=LoopingCall(fun)
			call.start(delay)
			self.bg.append(call)
	def connectionLost(self,reason):
		for call in self.bg:
			self.bg.remove(call)
			call.stop()
	def lineReceived(self,line):
		line=line.split(" ",1)
		fun=getattr(self,"do_"+line[0],None)
		if fun:
			fun(None if len(line)<2 else line[1])
		else:
			self.log("PRINT no such method "+line[0]+" in "+" ".join(line))
	def do_PING(self,line):
		if self.lo<self.hi:
			self.log("PRINT %s order ping %i"%("in"if self.lo==int(line.split(" ")[0]) else"out of",self.lo))
			self.lo+=1
		else:
			self.log("PRINT no pending pings")
	def do_SET(self,line):
		var,val=line.split(" ",1)
		pass
	def do_START(self,line):
		cid,cmd=line.split(" ",1)
		if cmd in self.tasks:
			self.log("PROGRESS "+line+" 1")
		fun=getattr(self,"iter_"+cmd,None)
		if fun:
			self.tasks[cmd]={"cid":int(cid)}
		else:
			self.log("PRINT no such command "+cmd)
class WrapperFactory(Factory):
	def __init__(self,proto,robot):
		self.robot=robot
		self.proto=proto
	def buildProtocol(self,addr):
		if addr.type=="TCP"and addr.host=="127.0.0.1":
			print "connected to",addr
			return self.proto(self.robot)
class Position(LineOnlyReceiver):
	delimiter="\n"
	def __init__(self,robot):
		self.body=robot.robot
	def lineReceived(self,line):
		self.sendLine(" ".join(map(str,(self.body.position[0],self.body.position[1],self.body.angle/b2_pi*180))))
if __name__=="__main__":
	r=Robot()
	dashboard=TCP4ServerEndpoint(reactor,1735)
	dashboard.listen(WrapperFactory(Crio,r))
	camera=TCP4ServerEndpoint(reactor,1835)
	camera.listen(WrapperFactory(Position,r))
	reactor.run()
