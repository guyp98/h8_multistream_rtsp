#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <gst/gst.h>

bool print_gst_launch_only;
std::string input_source;
static gboolean waiting_eos = FALSE;
static gboolean caught_sigint = FALSE;

static void sigint_restore(void)
{
  struct sigaction action;

  memset (&action, 0, sizeof (action));
  action.sa_handler = SIG_DFL;

  sigaction (SIGINT, &action, NULL);
}

/* we only use sighandler here because the registers are not important */
static void
sigint_handler_sighandler(int signum)
{
  /* If we were waiting for an EOS, we still want to catch
   * the next signal to shutdown properly (and the following one
   * will quit the program). */
  if (waiting_eos) {
    waiting_eos = FALSE;
  } else {
    sigint_restore();
  }
  /* we set a flag that is checked by the mainloop, we cannot do much in the
   * interrupt handler (no mutex or other blocking stuff) */
  caught_sigint = TRUE;
}

void add_sigint_handler(void)
{
  struct sigaction action;

  memset(&action, 0, sizeof(action));
  action.sa_handler = sigint_handler_sighandler;

  sigaction(SIGINT, &action, NULL);
}

/* is called every 250 milliseconds (4 times a second), the interrupt handler
 * will set a flag for us. We react to this by posting a message. */
static gboolean check_sigint(GstElement * pipeline)
{
  if (!caught_sigint) {
    return TRUE;
  } else {
    caught_sigint = FALSE;
    waiting_eos = TRUE;
    GST_INFO_OBJECT(pipeline, "handling interrupt. send EOS");
    GST_ERROR_OBJECT(pipeline, "handling interrupt. send EOS");
    gst_element_send_event(pipeline, gst_event_new_eos());

    /* remove timeout handler */
    return FALSE;
  }
}





GstFlowReturn wait_for_end_of_pipeline(GstElement *pipeline)
{
    GstBus *bus;
    GstMessage *msg;
    GstFlowReturn ret = GST_FLOW_ERROR;
    bus = gst_element_get_bus(pipeline);
    gboolean done = FALSE;
    // This function blocks until an error or EOS message is received.
    while(!done)
    {
        msg = gst_bus_timed_pop_filtered(bus, GST_MSECOND * 250, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        if (msg != NULL)
        {
            GError *err;
            gchar *debug_info;
            done = TRUE;
            waiting_eos = FALSE;
            sigint_restore();
            switch (GST_MESSAGE_TYPE(msg))
            {
                case GST_MESSAGE_ERROR:
                {
                    gst_message_parse_error(msg, &err, &debug_info);
                    GST_ERROR("Error received from element %s: %s", GST_OBJECT_NAME(msg->src), err->message);

                    std::string dinfo = debug_info ? std::string(debug_info) : "none";
                    GST_ERROR("Debugging information : %s", dinfo.c_str());

                    g_clear_error(&err);
                    g_free(debug_info);
                    ret = GST_FLOW_ERROR;
                    break;
                }
                case GST_MESSAGE_EOS:
                {
                    GST_INFO("End-Of-Stream reached");
                    ret = GST_FLOW_OK;
                    break;
                }
                default:
                {
                    // We should not reach here because we only asked for ERRORs and EOS
                    GST_WARNING("Unexpected message received %d", GST_MESSAGE_TYPE(msg));
                    ret = GST_FLOW_ERROR;
                    break;
                }
            }
            gst_message_unref(msg);
        }
        check_sigint(pipeline);
    }
    gst_object_unref(bus);
    return ret;
}

void create_pipline(int base_port, int number_of_devices, std::string hef_path, std::string pp_path, std::string& play_pipeline) {
    std::stringstream concatenated_pipeline;

    // Loop to concatenate the string four times
    for (int i = 1; i < 5; i++) {
        int current_port = base_port + i - 1;
        int vdevice_key = (i % number_of_devices) + 1;

        std::string current_pipeline = 
            "udpsrc port="+ std::to_string(current_port) +" address=127.0.0.1 "
            "! application/x-rtp,encoding-name=H264 "
            "! queue "
            "! rtpjitterbuffer mode=0 "
            "! queue "
            "! rtph264depay "
            "! queue "
            "! h264parse "
            "! avdec_h264 "
            "! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 "
            "! videoscale qos=false n-threads=2 "
            "! video/x-raw, pixel-aspect-ratio=1/1 "
            "! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 "
            "! videoconvert n-threads=2 qos=false "
            "! video/x-raw, format=NV12 "
            "! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 "
            "! hailonet hef-path=" + hef_path +
            " batch-size=1 nms-score-threshold=0.3 nms-iou-threshold=0.60 output-format-type=HAILO_FORMAT_TYPE_FLOAT32 vdevice-key="+ std::to_string(vdevice_key) +" "
            "! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 "
            "! hailofilter function-name=yolov5 so-path=" + pp_path + " config-path=null qos=false "
            "! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 "
            "! hailooverlay qos=false "
            "! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 "
            "! videoconvert "
            "! fpsdisplaysink video-sink=autovideosink text-overlay=false sync=false"
            "        "
            ;
        concatenated_pipeline << current_pipeline;
    }

    play_pipeline = "gst-launch-1.0 -v ";
    play_pipeline += concatenated_pipeline.str();


}   



int main(int argc, char* argv[]) {
    add_sigint_handler();

    std::string hef_path = "/home/guyp/work_space/github/h8/resources/yolov5m_nv12.hef";
    std::string pp_path = "/home/guyp/work_space/github/h8/resources/libyolo_hailortpp_post.so";
    int base_port = 5000;
    int number_of_devices = 1;
    
    std::string str_pipline;
    create_pipline(base_port, number_of_devices, hef_path, pp_path, str_pipline);
    
    std::cout << str_pipline << std::endl;

    
    // g_setenv("GST_DEBUG", "*:3", TRUE); 
    gst_init(&argc, &argv);
    std::cout << "Created pipeline string." << std::endl;
    GstElement *pipeline = gst_parse_launch(str_pipline.c_str(), NULL);
    if (!pipeline) {
        g_printerr("Pipeline creation failed\n");
        return -1;
    }


    std::cout << "Parsed pipeline." << std::endl;
    std::cout << "Setting state to playing." << std::endl;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    GstFlowReturn ret = wait_for_end_of_pipeline(pipeline);
    std::cout << "pipeline endedd." << std::endl;
    
    // Free resources
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_deinit();    
    return ret;
}