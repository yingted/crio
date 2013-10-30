#include<cv.h>
#include<highgui.h>
#include<iostream>
using namespace std;
using namespace cv;
#ifdef DEBUG
#define d(x) cerr<<x<<endl
#else
#define d(x)
#endif
int main(int argc,char*argv[]){
	if(argc<2){
		cerr<<"usage: "<<argv[0]<<" stream"<<endl;
		return -1;
	}
	VideoCapture cap(argv[1]);
	if(!cap.isOpened())
		return -1;
	Mat prev;
	cap>>prev;
	d("press a key to exit");
	for(Mat frame,hsv,rgb;waitKey(1)<0;prev=frame){
		cap>>frame;
		if(frame.size()!=prev.size())break;
		inRange(frame,Scalar(128,192,0),Scalar(256,256,64),rgb);
		cvtColor(frame,frame,CV_RGB2HSV);
		#ifdef FAKE
		inRange(frame,Scalar(40,64,216),Scalar(64,192,256),hsv);
		#else
		inRange(frame,Scalar(16,192,192),Scalar(48,256,256),hsv);
		#endif
		cvtColor(frame,frame,CV_HSV2RGB);
		vector<Vec4i>lines,acc;
		vector<int>vote;
		HoughLinesP(hsv,lines,1,CV_PI/180,15,50,90);//dist step,angle step,score,min len,max gap
		for(size_t i=0;i<lines.size();++i){
			line(frame,Point(lines[i][0],lines[i][1]),Point(lines[i][2],lines[i][3]),Scalar(255,0,0));
			int best=0,cost=~(1<<31);
			for(int j=0;j<acc.size();++j){
#define sq(x) ((x)*(x))
#define cost(l,x,y) (sq((l[0]-x)*(l[3]-y)-(l[1]-y)*(l[2]-x))/(sq(l[0]-l[2])+sq(l[1]-l[3]))+2) //increase this offset to weight line centers stronger than corners
				const int curcost=abs(cost(lines[i],acc[j][0],acc[j][1])*cost(lines[i],acc[j][2],acc[j][3])*cost(acc[j],lines[i][0],lines[i][1])*cost(acc[j],lines[i][2],lines[i][3]));
				
#undef cost
#define sql(l) (sq(l[0]-l[2])+sq(l[1]-l[3]))
				if(curcost<cost&&10*sq((lines[i][3]-lines[i][1])*(acc[j][2]-acc[j][0])-(acc[j][3]-acc[j][1])*(lines[i][2]-lines[i][0]))/sql(lines[i])/sql(acc[j])<1){//coef=sec(max angle difference)
#undef sql
					cost=curcost;
					best=j;
				}
			}
			d("line with cost "<<cost);
			if(cost>100000000){//"transformation cost"
				acc.push_back(lines[i]);
				vote.push_back(1);
			}else{
				++vote[best];
				int x[4]={lines[i][0],lines[i][2],acc[best][0],acc[best][2]},
					y[4]={lines[i][1],lines[i][3],acc[best][1],acc[best][3]},pair=-1;
				for(int j=0,far=-1;j<16;++j)
					if((j>>2)<(j&3)){
						const int sqdist=sq(x[j>>2]-x[j&3])+sq(y[j>>2]-y[j&3]);
						if(sqdist>far){
							pair=j;
							far=sqdist;
						}
					}
				acc[best][0]=x[pair>>2];
				acc[best][1]=y[pair>>2];
				acc[best][2]=x[pair&3];
				acc[best][3]=y[pair&3];
			}
		}
		vector<Point>start,end;
		if(acc.size()>=4){//tweak to get less frames (these are not the four final ones)
			d(lines.size()<<" lines; "<<acc.size()<<" accumulators");
			for(size_t i=0;i<acc.size();++i){
				d("vote "<<vote[i]);
				if(vote[i]>1){//tweak to increase error tolerance
					line(frame,Point(acc[i][0],acc[i][1]),Point(acc[i][2],acc[i][3]),Scalar(0,0,255),sqrt(vote[i]));
					start.push_back(Point(acc[i][0],acc[i][1]));
					end.push_back(Point(acc[i][2],acc[i][3]));
					start.push_back(Point(acc[i][2],acc[i][3]));
					end.push_back(Point(acc[i][0],acc[i][1]));
					d("("<<acc[i][0]<<","<<acc[i][1]<<") ("<<acc[i][2]<<","<<acc[i][3]<<")");
				}
			}
		}
		if(start.size()>=8){//contingent on previous if
			int maxrate=1<<31,A,B,C,D;
			for(size_t a=0,l=start.size(),dst=0;a<l;++a)
				for(size_t b=0;b<l;++b){
#define dist(p,q) sqrt(sq(p.x-q.x)+sq(p.y-q.y))
#define uniq(p,q) if((p>>1)==(q>>1))continue;
					uniq(a,b);
					dst+=dist(end[a],start[b]);
#define cross(p,q) ((p).x*(q).y-(q).x*(p).y)
					for(size_t c=0;c<l;++c){
						uniq(a,c);
						uniq(b,c);
						dst+=dist(end[b],start[c]);
#define mp(p,q) Point((p.x+q.x)/2,(p.y+q.y)/2)
						for(size_t d=0;d<l;++d){
							uniq(a,d);
							uniq(b,d);
							uniq(c,d);
#undef uniq
#define tri(p,q,r) cross(mp(end[p],start[q]),mp(end[q],start[r]))
#define dblarea abs(tri(a,b,c)+tri(b,c,d)+tri(c,d,a)+tri(d,a,b))
#define perim (dst+dist(end[c],start[d])+dist(end[d],start[a]))
							int rate=dblarea-sq(perim)/150;//forces of expanding and contracting
							d("A: "<<dblarea/2<<"; P:"<<perim);
							d("testing "<<rate<<" at "<<a<<","<<b<<","<<c<<","<<d);
#undef dblarea
#undef perim
							if(rate>maxrate){
								A=a;B=b;C=c;D=d;
								maxrate=rate;
							}
						}
						dst-=dist(end[b],start[c]);
					}
					dst-=dist(end[a],start[b]);
#undef cross
#undef dist
#undef sq
				}
			d("rate "<<maxrate<<" at "<<A<<","<<B<<","<<C<<","<<D);
#define draw(l) line(frame,start[l],end[l],Scalar(0,255,0),2)
			draw(A);draw(B);draw(C);draw(D);
#undef draw
			line(frame,mp(end[A],start[B]),mp(end[B],start[C]),Scalar(0,0,0),2);
			line(frame,mp(end[B],start[C]),mp(end[C],start[D]),Scalar(0,0,0),2);
			line(frame,mp(end[C],start[D]),mp(end[D],start[A]),Scalar(0,0,0),2);
			line(frame,mp(end[D],start[A]),mp(end[A],start[B]),Scalar(0,0,0),2);
#undef mp
		}
		imshow("image",frame);
		imshow("rgb",rgb);
		imshow("hsv",hsv);
		if(start.size()>=4){
			d("press any key to continue");
			waitKey();
			while(waitKey(1)>=0);
		}
	}
	return 0;
}
