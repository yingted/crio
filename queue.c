#ifndef QUEUE_C
#define QUEUE_C
#include<pthread.h>
#ifdef __cplusplus
extern "C"{
#endif
typedef struct queue{
	void**buf;
	int size;
	pthread_mutex_t lock;
	pthread_cond_t empty,full;
	volatile int start,len;
}queue_t;
#define QUEUE(x) {buf:(void**)x,size:sizeof(x)/sizeof(x[0]),lock:PTHREAD_MUTEX_INITIALIZER,empty:PTHREAD_COND_INITIALIZER,full:PTHREAD_COND_INITIALIZER,start:0,len:0}
void queue_enqueue(queue_t*q,void*e){
	fprintf(stderr,"enqueue {%s}\n",(char*)e);
	pthread_mutex_lock(&q->lock);
	while(q->len==q->size)
		pthread_cond_wait(&q->empty,&q->lock);
	q->buf[(q->start+q->len++)%q->size]=e;
	pthread_mutex_unlock(&q->lock);
	pthread_cond_signal(&q->full);
	fprintf(stderr,"end enqueue {%s}\n",(char*)e);
}
void*queue_dequeue(queue_t*q){
	fprintf(stderr,"dequeue\n");
	void*e;
	pthread_mutex_lock(&q->lock);
	if(!q->len)
		for(;;){
			pthread_cond_wait(&q->full,&q->lock);
			if(q->len)break;
			pthread_testcancel();
		}
	e=q->buf[q->start];
	q->start=(q->start+1)%q->size;
	--q->len;
	pthread_mutex_unlock(&q->lock);
	pthread_cond_signal(&q->empty);
	fprintf(stderr,"end dequeue {%s}\n",(char*)e);
	return e;
}
#ifdef __cplusplus
}
#endif
#endif
