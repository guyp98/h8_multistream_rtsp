#!/bin/bash

base_port=5000  # Set the base port number
hef_path="/home/guyp/work_space/github/h8/resources/yolov5m_nv12.hef"
pp_path="/home/guyp/work_space/github/h8/resources/libyolo_hailortpp_post.so"
# hef_path="/local/workspace/tappas/apps/h8/gstreamer/general/detection/resources/yolov5m_nv12.hef"
# pp_path="/local/workspace/tappas/apps/h8/gstreamer/libs/post_processes//libyolo_hailortpp_post.so"
number_of_devices=1

pipeline_template="udpsrc port=%d address=127.0.0.1 \
! application/x-rtp,encoding-name=H264 \
! queue \
! rtpjitterbuffer mode=0 \
! queue \
! rtph264depay \
! queue \
! h264parse \
! avdec_h264 \
! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 \
! videoscale qos=false n-threads=2 \
! video/x-raw, pixel-aspect-ratio=1/1 \
! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 \
! videoconvert n-threads=2 qos=false \
! video/x-raw, format=NV12 \
! queue name=hailonet%d_queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 \
! hailonet hef-path=$hef_path \
batch-size=1 nms-score-threshold=0.3 nms-iou-threshold=0.60 output-format-type=HAILO_FORMAT_TYPE_FLOAT32 vdevice-key=%d \
! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 \
! hailofilter function-name=yolov5 so-path=$pp_path config-path=null qos=false \
! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 \
! hailooverlay qos=false \
! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 \
! videoconvert \
! fpsdisplaysink video-sink=autovideosink text-overlay=false sync=false \
        "

# Loop to concatenate the string four times
for ((i=1; i<5; i++)); do
    current_port=$((base_port + i - 1))
    vdevice_key=$(((i % number_of_devices) + 1 ))
    current_pipeline=$(printf "$pipeline_template" "$current_port" "$i" "$vdevice_key")
    concatenated_pipeline+="$current_pipeline"
done

play_pipeline="gst-launch-1.0 -v "
play_pipeline+="$concatenated_pipeline"

# Print or use the concatenated string as needed
echo "$play_pipeline"
eval "$play_pipeline"
