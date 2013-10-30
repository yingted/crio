#!/usr/bin/python
from cv2 import *
from numpy import *
from sys import *
from math import *
from twisted.internet import reactor
from twisted.internet.endpoints import TCP4ServerEndpoint
from twisted.internet.protocol import Factory
from twisted.protocols.basic import LineOnlyReceiver
class VisionSender(LineOnlyReceiver):
        delimiter="\n"
	def updateCb(self,sx,dy,p,r):
		dy=abs(dy)
		dist=4010.5/dy
		self.r=atan((sx/320-1)*.8391)
		return
		l1,l2,r1,r2=map(lambda x:x[0,1],sorted(r,lambda a,b:int(a[0,0]-b[0,0])))
		lh=abs(l2-l1)
		rh=abs(r2-r1)
		r=pow(1.*lh/rh,10./11)
		distisq=(12*dist)**2
		theta=pi/2-acos(((((distisq+363)*r+(distisq-605))*r+(605-distisq))*r-distisq-363)/(528*dist*(r**3+1)))
		#import code
		#code.interact(local=locals())
		self.y=-dist*sin(theta)
		self.x=27-dist*cos(theta)
		self.p=p
		self.count+=1
        def connectionMade(self):
		self.dist=self.r=0
		#self.p=[]
		reactor.callInThread(self.update,VideoCapture('http://10.43.8.11/axis-cgi/mjpg/video.cgi?resolution=640x480&fps=12'if len(argv)==1 else argv[1]))
	def connectionLost(self,reason):
		self.update=lambda x:None
        def lineReceived(self,line):
		self.sendLine("SET ABS %f %f %f"%(self.x/27.,self.y/13.5,self.r/pi*180)+"".join(map(lambda p:" %f %f"%(p[0]/640,p[1]/480),self.p)))
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
			if 2<=len(polys)<=4:
				polyc=[poly.mean(axis=0)[0] for poly in polys]
				t=range(len(polyc))
				def sort(l):
					t.sort(lambda x,y:l(polyc[x],polyc[y]))
					return[polyc[x] for x in t],[polys[x] for x in t]
				if len(polys)==4:
					polyc,polys=sort(lambda x,y:-1 if x[0]<y[0]else int(x[0]-y[0]))
					left,top,bottom,right=polyc
					self.updateCb(array(polyc).mean(axis=0)[0],bottom[1]-top[1],polyc,polys[1])
				elif len(polys)==3:
					polyc,polys=sort(lambda y,x:-1 if x[1]<y[1]else int(x[1]-y[1]))
					top,middle,bottom=polyc
					dy=abs(bottom[1]-top[1])
					sx=(top[0]+bottom[0])/2
					if (top[1]-middle[1])*2<middle[1]-bottom[1]:
						dy*=2#top is side
						sx=(sx*2+middle[0])/3
					self.updateCb(sx,dy,polyc,polys[0])
				elif len(polys)==2:
					polyc,polys=sort(lambda y,x:-1 if x[1]<y[1]else int(x[1]-y[1]))
					middle,bottom=polyc
					self.updateCb(bottom[0],2*(middle[1]-bottom[1]),polyc,polys[1])
		reactor.callInThread(self.update,cap)
class VisionSenderFactory(Factory):
	def buildProtocol(self,addr):
		if addr.type=="TCP"and addr.host=="127.0.0.1":#TODO change
			print"connected to",addr
			return VisionSender()
TCP4ServerEndpoint(reactor,9999).listen(VisionSenderFactory())
reactor.run()
