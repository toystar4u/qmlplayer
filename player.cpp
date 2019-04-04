#include "player.h"

#include <QDebug>


//------------------


static gboolean
bus_callback (   GstBus *bus,
                 GstMessage *message,
                 gpointer data)
{

    //UserData * userData= static_cast<UserData *>data;

    g_print ("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));

    switch (GST_MESSAGE_TYPE (message)) {

    case GST_MESSAGE_ERROR: {
        GError *err;
        gchar *debug;
        gst_message_parse_error (message, &err, &debug);
        g_print ("Error: %s\n", err->message);
        g_error_free (err);
        g_free (debug);
        //g_main_loop_quit (loop);
        break;
    }
    case GST_MESSAGE_WARNING: {
        GError *err;
        gchar *debug;
        gst_message_parse_warning (message, &err, &debug);
        g_print ("Warnning: %s\n", err->message);
        g_error_free (err);
        g_free (debug);
        //g_main_loop_quit (loop);
        break;
    }
    case GST_MESSAGE_INFO:{
        GError *err;
        gchar *debug;
        gst_message_parse_info (message, &err, &debug);
        g_print ("Info: %s\n", err->message);
        g_error_free (err);
        g_free (debug);
        //g_main_loop_quit (loop);
    }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        //g_main_loop_quit (loop);
        break;

    case GST_MESSAGE_NEED_CONTEXT: {
        const gchar *context_type;
        GstContext *context = NULL;

        gst_message_parse_context_type (message, &context_type);
        g_print("got need context %s\n", context_type);

        break;
    }


    default:
        /* unhandled message */
        break;
    }
    /* we want to be notified again the next time there is a message
    * on the bus, so returning TRUE (FALSE means we want to stop watching
    * for messages on the bus and our callback should not be called again)
    */
    return TRUE;
}



/* This function will be called by the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *new_pad, UserData *data) {
    // we're only use the video stream

    GstPad *sink_pad_audio = gst_element_get_static_pad (data->fakesink, "sink");
    GstPad *sink_pad_video = gst_element_get_static_pad (data->glupload, "sink");

    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

    ///* If our converter is already linked, we have nothing to do here */
    //if (gst_pad_is_linked (sink_pad)) {
    //  g_print ("  We are already linked. Ignoring.\n");
    //  goto exit;
    //}

    /* Check the new pad's type */
    new_pad_caps = gst_pad_query_caps (new_pad, NULL);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);

    // Attempt the link for video
    if (!g_str_has_prefix (new_pad_type, "audio/x-raw")) {

        g_print ("  It has type '%s' which is raw video. Connecting.\n", new_pad_type);

        /* Attempt the link */
        ret = gst_pad_link (new_pad, sink_pad_video);
        if (GST_PAD_LINK_FAILED (ret)) {
            g_print ("  Type is '%s' but link failed.\n", new_pad_type);
        } else {
            g_print ("  Link succeeded (type '%s').\n", new_pad_type);
        }

        goto exit;
    }


    /* Attempt the link for audio */
    ret = gst_pad_link (new_pad, sink_pad_audio);
    if (GST_PAD_LINK_FAILED (ret)) {
        g_print ("  Type is '%s' but link failed.\n", new_pad_type);
    } else {
        g_print ("  Link succeeded (type '%s').\n", new_pad_type);
    }


exit:
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL)
        gst_caps_unref (new_pad_caps);

    /* Unreference the sink pad */
    gst_object_unref (sink_pad_audio);
    gst_object_unref (sink_pad_video);
}





Player::Player(QObject *parent) : QObject(parent)
{

}


Player::Player(char *uri)
{

    // create pipe line

    // Pipeline object
    m_data.pipeline = gst_pipeline_new (NULL);

#if 1


#if 1


    m_data.src = gst_element_factory_make("uridecodebin", NULL);
    g_object_set (m_data.src , "uri", uri, NULL);

    // convert to GL texture (as qmlglsink expects, also this force processing the video accelerated by the GPU)
    // and color convert to RGBA (automatically add "glcolorconvert")
    m_data.glupload = gst_element_factory_make ("glupload", NULL);

    // fakesink for audio
    //data.audioq = gst_element_factory_make ("queue", "audioqueue");
    m_data.fakesink = gst_element_factory_make ("fakesink", NULL);

    // QML element sink
    /* the plugin must be loaded before loading the qml file to register the
     * GstGLVideoItem qml item */
    m_data.sink = gst_element_factory_make ("qmlglsink", NULL);
    g_object_set (m_data.sink, "sync", true, NULL);

    // link the elements in the pipeline
    gst_bin_add_many (GST_BIN (m_data.pipeline), m_data.src, m_data.fakesink,  m_data.glupload, m_data.sink, NULL);

    // connect link except for dynamic padds
    gst_element_link_many ( m_data.glupload, m_data.sink, NULL);

    // uridecodebin creates dynamic pads;
    // so, dynamic connection for src:video --> glupload and src:audio -> fakesink
    /* Connect to the pad-added signal */
    g_signal_connect (m_data.src, "pad-added", G_CALLBACK (pad_added_handler), &m_data);



    GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (m_data.pipeline));
    gint bus_watch_id = gst_bus_add_watch (bus, bus_callback, NULL);
    gst_object_unref (bus);


#else

    // v4l2 src, selecting device /dev/video1
    GstElement *src = gst_element_factory_make("v4l2src", NULL);

    //g_object_set (src, "device", "/dev/video2", NULL);
    g_object_set (src, "device", uri, NULL);

    // caps selection: format, width, height...
    GstElement *capsfilter = gst_element_factory_make("capsfilter", NULL);
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                        //                "format", G_TYPE_STRING, "NV12",
                                        "width", G_TYPE_INT, 640,
                                        "height", G_TYPE_INT, 480,
                                        "framerate", GST_TYPE_FRACTION, 30, 1, NULL);

    g_object_set (capsfilter, "caps", caps, NULL);

    //GstElement *dec = gst_element_factory_make ("decodebin", NULL);

    // convert to GL texture (as qmlglsink expects, also this force processing the video accelerated by the GPU)
    // and color convert to RGBA (automatically add "glcolorconvert")
    GstElement *glupload = gst_element_factory_make ("glupload", NULL);


    // QML element sink
    /* the plugin must be loaded before loading the qml file to register the
     * GstGLVideoItem qml item */
    m_data.sink = gst_element_factory_make ("qmlglsink", NULL);
    g_object_set (m_data.sink, "sync", false, NULL);

    // link the elements in the pipeline
    gst_bin_add_many (GST_BIN (m_data.pipeline), src, capsfilter, glupload, m_data.sink, NULL);
    gst_element_link_many (src, capsfilter,  glupload, m_data.sink, NULL);
#endif

#else

    GstElement *src = gst_element_factory_make ("videotestsrc", NULL);


    GstElement *glupload = gst_element_factory_make ("glupload", NULL);

    GstElement *filter = gst_element_factory_make ("capsfilter", "filter");

    /* the plugin must be loaded before loading the qml file to register the
     * GstGLVideoItem qml item */
    m_data.sink = gst_element_factory_make ("qmlglsink", NULL);

    g_assert (src && glupload && m_data.sink);

    gst_bin_add_many (GST_BIN (m_data.pipeline), src, filter, glupload, m_data.sink, NULL);
    gst_element_link_many (src, filter, glupload, m_data.sink, NULL);


    GstCaps *filtercaps;
    filtercaps = gst_caps_new_simple ("video/x-raw",
                                      "format", G_TYPE_STRING, "NV12",
                                      "width", G_TYPE_INT, 854,
                                      "height", G_TYPE_INT, 480,
                                      "framerate", GST_TYPE_FRACTION, 30, 1,
                                      NULL);
    g_object_set (G_OBJECT (filter), "caps", filtercaps, NULL);
    gst_caps_unref (filtercaps);


    GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (m_data.pipeline));
    gint bus_watch_id = gst_bus_add_watch (bus, bus_callback, NULL);
    gst_object_unref (bus);


#endif
}

Player::~Player()
{
    if(m_data.pipeline){
        gst_element_set_state (m_data.pipeline, GST_STATE_NULL);
        gst_object_unref (m_data.pipeline);
    }
}

void Player::setVideoItem(QQuickItem *videoItem)
{
    if(m_data.sink)
        g_object_set(m_data.sink, "widget", videoItem, NULL);       // set "widget" property of qmlglsink with videoitem

}
GstElement *Player::pipeline()
{
    return m_data.pipeline;
}
bool Player::playing()
{
    return m_isPlaying;
}
void Player::pause (){
    qDebug()<<"Player: pause";
    if (m_data.pipeline){
        gst_element_set_state (m_data.pipeline, GST_STATE_PAUSED);

        /* wait until it's up and running or failed */
        if (gst_element_get_state (m_data.pipeline, NULL, NULL, 2 * GST_SECOND) == GST_STATE_CHANGE_FAILURE) {
            qDebug()<< "Failed to go into PAUSED state";
        }else m_isPlaying = false;

    }
}

void Player::resume(){
    qDebug()<<"Player: resume";
    if (m_data.pipeline){
        gst_element_set_state (m_data.pipeline, GST_STATE_PLAYING);

        /* wait until it's up and running or failed */
        if (gst_element_get_state (m_data.pipeline, NULL, NULL,  2 * GST_SECOND) == GST_STATE_CHANGE_FAILURE) {
            qDebug()<< "Failed to go into PLAYING state";
        }else m_isPlaying = true;

    }
}


void Player::toggle(){
    qDebug()<<"Player: toggle";
    if (m_data.pipeline){
        if(m_isPlaying){
            pause();
        }else {
            resume();
        }

    }
}
