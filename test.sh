#!/bin/bash
(
boundary=asdfadfhfdklgfjhkldsfjgn
cat << EOF
HTTP/1.0 200 OK
Content-Type: multipart/x-mixed-replace;boundary=$boundary
EOF
while :
do
cat << EOF

--$boundary
EOF
sleep 1
cat << EOF
Content-Type: image/jpeg
Content-Length: -1

EOF
echo sending frame >&2
((++i))
convert -background grey -fill green -pointsize 72 label:$i jpg:-
done
) | nc -l 55555
