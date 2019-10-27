# v380
Extract h264 from v380 camera where you can use it to feed another application

# Usage example
### Connect by IP and feed it to ffplay
`./v380 -u admin -p password -port 8800 -ip 192.168.1.2 | ffplay -vf \"setpts = N / (25 * TB)\" -i -`
### Connect by id and feed it to ffplay
`./v380 -u admin -p password -port 8800 -id 123456 | ffplay -vf \"setpts = N / (25 * TB)\" -i -`
### Connect by mac address and feed it to ffmpeg to ffserver
`./v380 -p password -mac 11:22:33:aa:bb:cc | ffmpeg -i - http://localhost:8090/feed1.ffm`
### Feed to gstreamer hosting mjpeg
The http server can be found here: https://gist.github.com/misaelnieto/2409785  
`./v380 -p password -mac 11:22:33:aa:bb:cc | gst-launch-1.0 -v fdsrc ! decodebin ! jpegenc ! multipartmux boundary=spionisto ! tcpclientsink host=127.0.0.1 port=9999
`

### The camera can be discovered by using `--discover` command
`v380 --discover`

# Compiling
Use Visual Studio 2015 or on linux use `make` under v380 subdirectory
