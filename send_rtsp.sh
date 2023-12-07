gst-launch-1.0 -v v4l2src device=/dev/video0 ! videoconvert ! x264enc tune=zerolatency \
! queue leaky=no max-size-buffers=100 max-size-bytes=0 max-size-time=0 ! rtph264pay config-interval=1 \
! tee name=t     \
t. ! queue ! udpsink host=127.0.0.1 port=5000 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5001 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5002 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5002 sync=false     \
t. ! queue ! udpsink host=127.0.0.1 port=5003 sync=false