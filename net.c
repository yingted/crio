#include"net.h"
typedef struct extra{
}extra_t;
void set(const char*var,const char*val,state_t*data){
	const extra_t*extras=data->extras;
	fprintf(stderr,"setting %s to %s\n",var,val);
}
void start(int cid,const char*cmd,state_t*data){
	const extra_t*extras=data->extras;
	fprintf(stderr,"starting %s with id %d\n",cmd,cid);
}
int main(){
	extra_t extras={
	};
	net_t*nt=init_net(IP(127,0,0,1),1735,set,start,&extras);
	fputs("press enter to exit\n",stderr);
	int c;
	while((c=getchar())!=EOF&&c!='\n');
	close_net(nt);
	return 0;
}
