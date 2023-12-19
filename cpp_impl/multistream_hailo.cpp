#include "hailo_objects.hpp"
#include "gst_hailo_meta.hpp"
#include "hailo/hailort.h"
#include "tensor_meta.hpp"


#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

bool print_gst_launch_only;
std::string input_source;
static gboolean waiting_eos = FALSE;
static gboolean caught_sigint = FALSE;
bool show_debug = false;

enum class Buffer_from_piplne_method {
    probe,
    appsink
};


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

void create_pipline(int number_of_sources, int base_port, int number_of_devices, std::string hef_path, std::string pp_path, Buffer_from_piplne_method read_from, std::string& play_pipeline) {
    std::stringstream concatenated_pipeline;

    // Loop to concatenate the string four times
    for (int i = 1; i < number_of_sources+1; i++) {
        int current_port = base_port + i - 1;
        int vdevice_key = (i % number_of_devices) + 1;

        std::string sink =  "! hailooverlay name=hailooverlay"+std::to_string(i)+" qos=false "
                            "! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 "
                            "! videoconvert "
                            "! fpsdisplaysink video-sink=autovideosink text-overlay=true sync=false silent=false ";
        if(read_from == Buffer_from_piplne_method::appsink)
            sink = "! appsink name=appsink" + std::to_string(i) + " emit-signals=true max-buffers=1 drop=true ";            
        
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
            "! queue name=hailonet"+ std::to_string(i) +"_queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 "
            "! hailonet name=hailonet"+std::to_string(i)+" hef-path=" + hef_path +
            " batch-size=1 nms-score-threshold=0.3 nms-iou-threshold=0.60 output-format-type=HAILO_FORMAT_TYPE_FLOAT32 vdevice-key="+ std::to_string(vdevice_key) +" "
            "! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 "
            "! hailofilter name=hailofilter"+std::to_string(i)+" function-name=yolov5 so-path=" + pp_path + " config-path=null qos=false "
            "! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 "
            ""+sink+""
            "        "
            ;
        concatenated_pipeline << current_pipeline;
    }

    play_pipeline += concatenated_pipeline.str();


}   
void get_tensors_from_meta(GstBuffer *buffer, HailoROIPtr roi)
{
    gpointer state = NULL;
    GstMeta *meta;
    GstParentBufferMeta *pmeta;
    GstMapInfo info;
    while ((meta = gst_buffer_iterate_meta_filtered(buffer, &state, GST_PARENT_BUFFER_META_API_TYPE)))
    {
        pmeta = reinterpret_cast<GstParentBufferMeta *>(meta);
        (void)gst_buffer_map(pmeta->buffer, &info, GST_MAP_READWRITE);
        // check if the buffer has tensor metadata
        if (!gst_buffer_get_meta(pmeta->buffer, g_type_from_name(TENSOR_META_API_NAME)))
        {
            gst_buffer_unmap(pmeta->buffer, &info);
            continue;
        }
        const hailo_vstream_info_t vstream_info = reinterpret_cast<GstHailoTensorMeta *>(gst_buffer_get_meta(pmeta->buffer, g_type_from_name(TENSOR_META_API_NAME)))->info;
        roi->add_tensor(std::make_shared<HailoTensor>(reinterpret_cast<uint8_t *>(info.data), vstream_info));
        gst_buffer_unmap(pmeta->buffer, &info);
    }
}

static GstPadProbeReturn probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    int source_number = GPOINTER_TO_INT(user_data);
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if(show_debug){
        if (buffer) {
            
            guint size = gst_buffer_get_size(buffer);
            g_print("Buffer size: %u bytes\n", size);
            g_print("Received buffer from hailofilter source %d\n", source_number);
            

            // Check buffer type
            GstCaps *caps = gst_pad_get_current_caps(pad);
            if (caps) {
                gchar *caps_string = gst_caps_to_string(caps);
                g_print("Buffer type: %s\n", caps_string);
                g_free(caps_string);
                gst_caps_unref(caps);
            } else {
                g_print("Failed to get buffer type\n");
            }
            g_print("****\n");
            
        } else {
            g_print("Received NULL buffer from hailofilter source %d\n", source_number);
        }
    }
    
    
    HailoROIPtr hailo_roi = get_hailo_main_roi(buffer, false);
    get_tensors_from_meta(buffer, hailo_roi);
    for(const auto& obj: hailo_roi->get_objects()){
        if(obj->get_type() == HAILO_DETECTION){
            HailoDetectionPtr detection = std::dynamic_pointer_cast<HailoDetection>(obj);
            std::cout << detection->get_label() << " detected" << " in stream" << source_number << std::endl;
        }
    }

    return GST_PAD_PROBE_OK;
}

static GstFlowReturn new_sample_callback(GstAppSink *sink, gpointer user_data) {
    GstSample *sample = gst_app_sink_pull_sample(sink);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    int source_number = GPOINTER_TO_INT(user_data);
    if(show_debug){
        g_print("Received buffer from appsink source %d\n", source_number);
        g_print("Buffer type: %s\n", gst_structure_get_name(structure));
        g_print("****\n");
    }
    
    HailoROIPtr hailo_roi = get_hailo_main_roi(buffer, false);
    get_tensors_from_meta(buffer, hailo_roi);
    for(const auto& obj: hailo_roi->get_objects()){
        if(obj->get_type() == HAILO_DETECTION){
            HailoDetectionPtr detection = std::dynamic_pointer_cast<HailoDetection>(obj);
            std::cout << detection->get_label() << " detected" << " in stream" << source_number << std::endl;
        }
    }
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void set_callbacks(GstElement *pipeline, int number_of_sources, Buffer_from_piplne_method method = Buffer_from_piplne_method::probe){
    for(int i = 1; i < number_of_sources+1; i++) {
        if(method == Buffer_from_piplne_method::probe){
            GstElement *hailofilter = gst_bin_get_by_name(GST_BIN(pipeline), ("hailofilter" + std::to_string(i)).c_str());
            if (!hailofilter) {
                g_printerr("hailofilter not found\n");
            }
            GstPad *srcpad = gst_element_get_static_pad(hailofilter, "src");
            gst_pad_add_probe(srcpad, GST_PAD_PROBE_TYPE_BUFFER, probe_cb, GINT_TO_POINTER(i), NULL);
            gst_object_unref(srcpad);
            gst_object_unref(hailofilter);
        }
        else if(method == Buffer_from_piplne_method::appsink){
            GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), ("appsink" + std::to_string(i)).c_str());
            if (!appsink) {
                g_printerr("appsink not found\n");
            }
            //set callback to read buffers from pipline when they are ready
            g_signal_connect(GST_APP_SINK(appsink), "new-sample", G_CALLBACK(new_sample_callback), GINT_TO_POINTER(i));
            gst_object_unref(appsink);
        }
    }
}

int main(int argc, char* argv[]) {
    add_sigint_handler();

    std::string hef_path = "../resources/yolov5m_nv12.hef";
    std::string pp_path = "../resources/libyolo_hailortpp_post.so";
    int base_port = 5000;
    int number_of_devices = 1;
    int number_of_sources = 4;
    Buffer_from_piplne_method read_from = Buffer_from_piplne_method::probe;

    std::string str_pipline;
    create_pipline(number_of_sources, base_port, number_of_devices, hef_path, pp_path, read_from, str_pipline);
    
    std::cout <<  "gst-launch-1.0 -v " << str_pipline << std::endl;
    
    if(show_debug)
        g_setenv("GST_DEBUG", "*:3", TRUE);
         
    gst_init(&argc, &argv);
    std::cout << "Created pipeline string." << std::endl;
    GstElement *pipeline = gst_parse_launch(str_pipline.c_str(), NULL);
    if (!pipeline) {
        g_printerr("Pipeline creation failed\n");
        return -1;
    }

    //set callback to read buffers from pipline
    set_callbacks(pipeline, number_of_sources, read_from);

    

    //start playing
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