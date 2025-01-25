./v380/v380 --discover
./v380/v380 -u admin -p <PASSWORD> \
\
-addr 192.168.2.96 -mac 4c:60:ba:22:a2:00 -id 12345678 -port 8800 \
\ # -addr 192.168.2.95 -mac 4c:60:ba:21:98:00 -id 87654321 -port 8800 \
\
| gst-launch-1.0 -v fdsrc ! decodebin ! jpegenc ! multipartmux boundary=spionisto ! tcpclientsink host=127.0.0.1 port=9999 # https://gist.github.com/misaelnieto/2409785
# | ffmpeg -i - http://localhost:8090/feed1.ffm
# | ffplay -vcodec h264 -probesize 32 -formatprobesize 0 -avioflags direct -flags low_delay -i -
