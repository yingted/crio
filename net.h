#ifndef NET_C
#define NET_C
#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<pthread.h>
#include<string.h>
#include<stdarg.h>
#include"queue.c"
#ifdef __cplusplus
extern "C"{
#endif
#define DEBUG
#define IP(a,b,c,d)(((a<<8|b)<<8|c)<<8|d)
#ifdef DEBUG
void die(const char*msg){
	perror(msg);
	exit(1);
}
int trycatch(int code,const char*msg){
	if(code<0)
		die(msg);
	return code;
}
#define trycatch(x,y) trycatch(x,"could not "y)
#else
#define trycatch(x,y) x
#define fprintf(...)
#endif
typedef struct counter{
	int val;
	pthread_mutex_t lock;
}counter_t;
#define COUNTER_INITIALIZER {val:0,lock:PTHREAD_MUTEX_INITIALIZER}
#define READ_COUNTER(c,x) {pthread_mutex_lock(&c.lock);x=c.val;pthread_mutex_unlock(&c.lock);}
#define INC_COUNTER(c,x) {pthread_mutex_lock(&c.lock);x=++c.val;pthread_mutex_unlock(&c.lock);}
#define eputs(x) fputs(x"\n",stderr);
typedef struct state{
	queue_t q;
	int dst;
	void*extras;
}state_t;
typedef struct net{
	pthread_t thd;
	int ip,sock;
	void(*set)(const char*,const char*,state_t*);
	void(*start)(int,const char*,state_t*);
	void*extras;
}net_t;
int qprintf(queue_t*q,const char *fmt,...){
	va_list arg;
	va_start(arg,fmt);
	char*msg;
	const int ret=vasprintf(&msg,fmt,arg);
	fprintf(stderr,"sending: %s",msg);
	queue_enqueue(q,msg);
	va_end(arg);
	return ret;
}
void*writer_thread(void*x){
	eputs("writer thread started");
	char*msg;
	state_t*e=(state_t*)x;
	for(;;){
		msg=(char*)queue_dequeue(&e->q);
		fprintf(stderr,"send(): {%s\n}",msg);
		if(send(e->dst,msg,strlen(msg),0)<0)
			eputs("send message");
		free(msg);
	}
}
void*server_thread(void*tdata){
	const net_t*nt=(net_t*)tdata;
	pthread_cleanup_push((void(*)(void*))&close,(void*)nt->sock);
	for(;;){
		char*msgs[100];
		state_t data={q:QUEUE(msgs),dst:0,extras:nt->extras};//dst:0 is needed for g++ since c++ support isn't complete
		trycatch(data.dst=accept(nt->sock,NULL,NULL),"accept connection");//TODO ip filter
		pthread_cleanup_push((void(*)(void*))&close,(void*)data.dst);
		eputs("connected");//don't spawn a thread since we only serve one client
		pthread_t writer;
		pthread_create(&writer,NULL,writer_thread,(void*)&data);
		eputs("read loop started");
#define nprintf(...) qprintf(&data.q,__VA_ARGS__)
		counter_t done=COUNTER_INITIALIZER,pending=COUNTER_INITIALIZER;
		int len=128,pos=0,add;
		char*buf=(char*)malloc(len);
		while((add=recv(data.dst,&buf[pos],len-pos,0))>0){
			while(add){
#define check(x,err) if(x);else{nprintf("PRINT "err": %s\n",buf);continue;}
#define shift(buf,var,name) char*var=strchr(buf,' ');check(var,"no "name);*var++='\0';
				if(buf[pos]=='\n'){
					buf[pos]='\0';
					shift(buf,arg,"arguments");
					if(!strcmp("SET",buf)){
						shift(arg,val,"value");
						nt->set(arg,val,&data);
					}else if(!strcmp("START",buf)){
						int cid;
						check(sscanf(arg,"%i",&cid),"could not parse command id");
						shift(arg,cmd,"command");
						nt->start(cid,cmd,&data);
					}else if(!strcmp("PING",buf)){
						int last,cur,max;
						INC_COUNTER(done,last);
						READ_COUNTER(pending,max);
						sscanf(arg,"%d",&cur);
						fprintf(stderr,"recv ping %d\n",cur);
						if(cur!=last)
							nprintf("PRINT expected ping %d, got %d\n",last,cur);
						else if(cur>=max)
							nprintf("PRINT premature ping %d\n",cur);
					}else nprintf("PRINT unknown command %s\n",buf);
				}
				++pos;
				--add;
				if(!buf[pos-1]){
					int i;
					for(i=0;i<add;++i)
						buf[i]=buf[pos+i];
					pos=0;
				}
#undef check
			}
			if(len==pos)
				buf=(char*)realloc((void*)buf,len<<=1);
		}
#undef nprintf
		if(add<0)
			perror("connection terminated");
		eputs("closed");
		if(pthread_cancel(writer))
			eputs("could not kill writer");
		if(pthread_join(writer,NULL))
			eputs("could not finish writing");
		pthread_cleanup_pop(1);
	}
	pthread_cleanup_pop(0);
}
net_t*init_net(int ip,int port,void(*set)(const char*,const char*,state_t*),void(*start)(int,const char*,state_t*),void*extras){
	const int sock=socket(AF_INET,SOCK_STREAM,0);
	trycatch(sock<0,"create socket");
	net_t*nt=(net_t*)malloc(sizeof(net_t));
	trycatch((int)nt,"allocate thread");
	static const int one=1;
	trycatch(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one)),"enable socket reuse");
	const struct sockaddr_in addr={
		sin_family:AF_INET,//named initializers are C-only
		sin_port:htons(port),
		sin_addr:{
			s_addr:htonl(ip),
		},
		sin_zero:{0},
	};
	trycatch(bind(sock,(struct sockaddr*)&addr,sizeof(addr))<0,"bind to port");
	trycatch(listen(sock,1),"listen");//only allow one client
	nt->extras=extras;
	nt->ip=ip;
	nt->set=set;
	nt->start=start;
	nt->sock=sock;
	trycatch(pthread_create(&nt->thd,NULL,&server_thread,nt),"create thread");
	return nt;
}
void close_net(net_t*nt){
	trycatch(pthread_cancel(nt->thd),"cancel thread");
	pthread_join(nt->thd,NULL);
	free(nt);
}
#ifdef __cplusplus
}
#endif
#endif
