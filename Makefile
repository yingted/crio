crio: crio.cpp
	g++ crio.cpp -o crio -pthread
net: net.h net.c
	gcc net.c -o net -pthread
image: image.cpp
	g++ image.cpp -o image -DDEBUG `pkg-config --libs --cflags opencv`
image-release: image.cpp
	g++ image.cpp -o image-release `pkg-config --libs --cflags opencv` -O3
static-test: image
	./image sample.mpg
test: image
	(sleep 2; echo launching...; ./image-release http://127.0.0.1:55556/) & ./launch.sh >/dev/null
#	./image rtsp://user#pass@server:port/path
.PHONY: test static-test
