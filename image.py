#!/usr/bin/python
from cv2 import *
from numpy import *
from sys import *
import random#
import code#
if __name__=="__main__":
	if len(argv)<2:
		print"usage: %s source"%argv[0]
		exit(-1)
	cap=VideoCapture(argv[1])
	while cap.grab():
		ret,frame=cap.retrieve()
		assert ret
		hsv=inRange(cvtColor(frame,COLOR_BGR2HSV),array((0,40,240)),array((180,220,256)))#change 0 to 160 to be more strict
		edges=Canny(hsv,80,120,None,5)#TODO tune
		contours,hierarchy=findContours(hsv,RETR_CCOMP,CHAIN_APPROX_SIMPLE)
		polys=[]
		#code.interact(local=locals())
		if len(contours):
			i=0
			while i>=0:
				drawContours(frame,contours,i,(random.randrange(0,256),255,0))#
				poly=approxPolyDP(contours[i],arcLength(contours[i],True)*.02,True)#TODO tune
				print abs(contourArea(poly))#
				if len(poly)==4 and abs(contourArea(poly))>1000 and isContourConvex(poly):
					polys.append(poly)
					#waitKey()
				i=hierarchy[0,i,0]
		imshow("frame",frame)#
		imshow("hsv",hsv)#
		imshow("edges",edges)#
		print len(polys),"polygons; lengths",map(contourArea,polys)#
		if len(polys)==4:
			print"***found polygons***"#
			#print polys
			if waitKey()==1048689:#
				break#
		else:#
			waitKey(17)#
		#code.interact(local=locals())
		#waitKey()#
