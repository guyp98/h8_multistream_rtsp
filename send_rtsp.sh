gst-launch-1.0 -v v4l2src device=/dev/video0 ! videoconvert ! videoscale ! video/x-raw,width=1920,height=1080 ! x264enc tune=zerolatency \
! queue leaky=no max-size-buffers=100 max-size-bytes=0 max-size-time=0 ! rtph264pay config-interval=1 \
! tee name=t     \
t. ! queue ! udpsink host=127.0.0.1 port=5000 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5001 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5002 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5002 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5003 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5004 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5005 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5006 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5007 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5008 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5009 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5010 sync=false
