#include<iostream>
#include"net.c"
extern "C"{
using namespace std;
#define nprintf(...) qprintf(&data->q,__VA_ARGS__)
struct extra{
};
void set(const char*var,const char*val,state_t*data){
	const extra*extras=(extra*)data->extras;
	cerr<<"setting "<<var<<" to "<<val<<endl;
}
void start(int cid,const char*cmd,state_t*data){
	const extra*extras=(extra*)data->extras;
	cerr<<"starting "<<cmd<<" with id "<<cid<<endl;
}
#undef nprintf
int main(){
	extra extras;
	init_net(IP(127,0,0,1),1735,set,start,(void*)&extras);
	return 0;
}
}
