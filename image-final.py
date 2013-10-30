#!/usr/bin/python
pymin=min
from cv2 import *
from numpy import *
from sys import *
from math import *
from twisted.internet import reactor
from twisted.internet.endpoints import TCP4ClientEndpoint
from twisted.internet.protocol import ReconnectingClientFactory
from twisted.protocols.basic import LineOnlyReceiver
class VisionSender(LineOnlyReceiver):
        delimiter="\n"
	def updateCb(self,sx,dy):
		reactor.callFromThread(self.sendLine,"SET ABS %f %f"%(3717./abs(dy),atan((sx/320-1)*.8391)/pi*180))
        def connectionMade(self):
		reactor.callInThread(self.update,VideoCapture('http://10.43.8.11/axis-cgi/mjpg/video.cgi?resolution=640x480&fps=12'if len(argv)==1 else argv[1]))
	def connectionLost(self,reason):
		print"killing connection"
		self.update=lambda a:None
	def update(self,cap):
		if cap.grab():
			ret,frame=cap.retrieve()
			assert ret
			hsv=inRange(cvtColor(frame,COLOR_BGR2HSV),array((0,40,240)),array((180,220,256)))#change 0 to 160 to be more strict
			edges=Canny(hsv,80,120,None,5)#TODO tune
			contours,hierarchy=findContours(hsv,RETR_CCOMP,CHAIN_APPROX_SIMPLE)
			polys=[]
			if len(contours):
				i=0
				while i>=0:
					poly=approxPolyDP(contours[i],arcLength(contours[i],True)*.02,True)#TODO tune
					if len(poly)==4 and abs(contourArea(poly))>1000 and isContourConvex(poly):
						polys.append(poly)
					i=hierarchy[0,i,0]
			if 1<=len(polys)<=4:
				polys=map(lambda x:x[1],sorted(map(lambda x:(x.min(axis=0)[0,1],x),polys)))
				polyc=array(polys).mean(axis=1)
				if len(polys)==3 and polyc[0][0][1]-polyc[1][0][1]<polyc[1][0][1]-polyc[2][0][1]:
					polys=[polys[2]]
				polys=map(lambda x:x[0],sorted(polys[0],lambda x,y:-1 if x[0,1]<y[0,1]else int(x[0,1]-y[0,1])))
				self.updateCb(array(polys).mean(axis=0)[0],7./4*(polys[3][1]+polys[2][1]-polys[1][1]-polys[0][1]))
		reactor.callInThread(self.update,cap)
class VisionSenderFactory(ReconnectingClientFactory):
	factor=1
	initialDelay=.05
	def buildProtocol(self,addr):
		print"connected!"
		self.resetDelay()
		return VisionSender()
	def startedConnecting(self,connector):
		print"connecting..."
	def clientConnectionLost(self,connector,reason):
		print"disconnected"
		ReconnectingClientFactory.clientConnectionLost(self,connector,reason)
	def clientConnectionFailed(self,connector,reason):
		print"failed"
		ReconnectingClientFactory.clientConnectionFailed(self,connector,reason)
reactor.connectTCP("10.43.8.2"if len(argv)==1 else"127.0.0.1",1735,VisionSenderFactory())
reactor.run()
