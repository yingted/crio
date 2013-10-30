trap 'kill $renderer $server $converter 2>/dev/null; kill -9 $renderer $server $converter 2>/dev/null' exit
socat -U tcp4-listen:55555,fork,reuseaddr,forever system:"./render.sh backgrounds/*" & renderer=$!
python simulator.py & server=$!
socat -U tcp4-listen:55556,fork,reuseaddr,forever exec:./convert.sh,fdout=3 & converter=$!
while echo connecting... >&2
do
	nc 127.0.0.1 1735
	[ $? -ne 1 ] && break
	sleep .1
done
