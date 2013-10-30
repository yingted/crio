#!/bin/sh
cat << EOF >&3
HTTP/1.0 200 OK
Content-Type: video/mpeg

EOF
gst-launch -v souphttpsrc location="http://127.0.0.1:55555/" do-timestamp=true is_live=true ! multipartdemux ! jpegdec ! 'video/x-raw-yuv,format=(fourcc)I420' ! ffenc_mpeg1video ! fdsink fd=3
