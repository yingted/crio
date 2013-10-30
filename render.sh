#!/bin/bash
width=640
height=480
[ $# -lt 1 ] && echo usage: $0 images && exit
trap "echo exiting; kill $$; kill -9 $$" exit int
while ! exec 3<>/dev/tcp/127.0.0.1/1835
do
	echo connection failed, sleeping >&2
	sleep .1
done
pushd /dev/shm > /dev/null
tmp=`mktemp tmp.XXXXXXXXXX.pov`
trap "echo cleaning up >&2; rm -f $PWD/$tmp; kill $$; kill -9 $$" exit int
boundary=75jqtmd4ur9QjWt4QZfgwVYF
cat << EOF
HTTP/1.0 200 OK
Content-Type: multipart/x-mixed-replace;boundary=$boundary
EOF
while :
do
	echo >&3
	read xft yft rotdeg <&3
	cat > $tmp << EOF
#include "colors.inc"

camera {
  location <0, 2, 0>
  look_at <1, 2, 0>
  rotate <0, $rotdeg, 0>
  translate <$xft, 0, $yft>
}

plane { // the floor
  y, 0  // along the x-z plane (y is the normal vector)
  pigment { checker color Black color White } // checkered pattern
}

box {
  <-27, 0, -13.5>, <27, 1e-4, 13.5>
  pigment {
    rgbf <.596, .541, .435, .2>
  }
  finish { specular .9 roughness .02 }
}

#macro create_net(height,displacement)
  union {
    box {
      <-1e-4, 7.5/12, -11/12>, <1/12, 27.5/12, 11/12>
      clipped_by {
        box {
          <-1, 9.5/12, -9/12>, <1, 25.5/12, 9/12>
          inverse
        }
      }
      pigment { color Grey }
      finish { ambient rgb <1, .2, .2> } // retroreflection
    }

    box {
      <0, 2/12, -22/12>, <1/12, 31.5/12, 22/12>
      pigment { color Yellow filter .8 }
    }

    box {
      <0, 5.5/12, -13/12>, <1/12, 29.5/12, 13/12>
      clipped_by {
        box {
          <-1, 11.5/12, -7/12>, <1, 23.5/12, 7/12>
          inverse
        }
      }
      pigment { color Black }
    }
    translate <26 /*guess*/, height, displacement>
  }
#end

create_net (28/12, 0)
create_net (61/12, -27.375/12)
create_net (61/12, 27.375/12)
create_net (98/12, 0)
box {
  <27-38.75/12, 0, -50.5/12>, <27, 9.25/12, 50.5/12>//not worth it to slant it; maintaining volume
  pigment { color Red }
}

light_source {
  <0, 10, 0> color White
  area_light <40, 0, 0>, <0, 0, 20>, 2, 1
}
EOF
	img="${@:$[RANDOM%$#+1]:1}"
	abs="`popd > /dev/null; readlink -f $img`" && img="$abs"
	cat << EOF

--$boundary
Content-Type: image/jpeg

EOF
povray 2>/dev/null "$tmp" +W$width +H$height +J +A -D +UA +O- | composite -resize ${width}x$height\! png:- "$img" jpg:- || break
done
popd > /dev/null
