#include "gstcelpdec.h"
#include "celp.h"
#include <gst/audio/audio.h>
#include <string.h>

GST_DEBUG_CATEGORY_STATIC(gst_celp_dec_debug);
#define GST_CAT_DEFAULT gst_celp_dec_debug

#define CELP_OK 0

enum { PROP_0, PROP_ERROR_PROTECTION };

G_DEFINE_TYPE(GstCelpDec, gst_celp_dec, GST_TYPE_AUDIO_DECODER);

static gboolean gst_celp_dec_start(GstAudioDecoder* dec) {
    GstCelpDec* self = GST_CELP_DEC(dec);

    // Initialize CELP codec
    celp_init(self->error_protection ? 1 : 0);
    self->frame_count = 0;
    return TRUE;
}

static gboolean gst_celp_dec_stop(GstAudioDecoder* dec) {
    return TRUE;
}

static GstFlowReturn gst_celp_dec_handle_frame(GstAudioDecoder* dec, GstBuffer* buffer) {
    GstCelpDec* self = GST_CELP_DEC(dec);
    GstMapInfo map;
    gint16 output[FRAME_SIZE];

    if (!buffer)
        return GST_FLOW_OK;

    gst_buffer_map(buffer, &map, GST_MAP_READ);

    if (map.size != STREAMBITS / 8) {
        GST_WARNING_OBJECT(self, "Invalid frame size: %" G_GSIZE_FORMAT, map.size);
        gst_buffer_unmap(buffer, &map);
        return GST_FLOW_ERROR;
    }

    // Copy packed bits
    memcpy(self->packedbits, map.data, STREAMBITS / 8);
    gst_buffer_unmap(buffer, &map);

    // Decode frame
    int result = celp_decode(self->packedbits, output);
    if (result != CELP_OK) {
        GST_WARNING_OBJECT(self, "CELP decoding error: %d", result);
        return GST_FLOW_ERROR;
    }

    // Create output buffer
    GstBuffer* outbuf = gst_buffer_new_allocate(NULL, sizeof(output), NULL);
    gst_buffer_fill(outbuf, 0, output, sizeof(output));

    return gst_audio_decoder_finish_frame(dec, outbuf, 1);
}

static gboolean gst_celp_dec_set_format(GstAudioDecoder* dec, GstCaps* caps) {
    GstCelpDec* self = GST_CELP_DEC(dec);
    GstStructure* structure;
    gint rate, channels;

    structure = gst_caps_get_structure(caps, 0);

    if (!gst_structure_get_int(structure, "rate", &rate) || rate != 8000) {
        GST_ERROR_OBJECT(self, "Invalid or missing sample rate: %d", rate);
        return FALSE;
    }

    if (!gst_structure_get_int(structure, "channels", &channels) || channels != 1) {
        GST_ERROR_OBJECT(self, "Invalid or missing channel count: %d", channels);
        return FALSE;
    }

    // Set output format
    GstAudioInfo info;
    gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16LE, 8000, 1, NULL);

    GST_INFO_OBJECT(self, "Format set successfully: %dHz, %d channels", rate, channels);

    return gst_audio_decoder_set_output_format(dec, &info);
}

static void gst_celp_dec_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
    GstCelpDec* self = GST_CELP_DEC(object);

    switch (prop_id) {
        case PROP_ERROR_PROTECTION:
            self->error_protection = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_celp_dec_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
    GstCelpDec* self = GST_CELP_DEC(object);

    switch (prop_id) {
        case PROP_ERROR_PROTECTION:
            g_value_set_boolean(value, self->error_protection);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_celp_dec_class_init(GstCelpDecClass* klass) {
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);
    GstAudioDecoderClass* base_class = GST_AUDIO_DECODER_CLASS(klass);

    gobject_class->set_property = gst_celp_dec_set_property;
    gobject_class->get_property = gst_celp_dec_get_property;

    g_object_class_install_property(gobject_class, PROP_ERROR_PROTECTION,
                                    g_param_spec_boolean("error-protection", "Error Protection", "Enable error protection coding",
                                                         FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    gst_element_class_set_static_metadata(element_class, "CELP Audio Decoder", "Codec/Decoder/Audio",
                                          "Decodes audio in CELP format", "Your Name <your.email@example.com>");

    static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                                        GST_STATIC_CAPS("audio/x-celp, "
                                                                                        "rate = (int) 8000, "
                                                                                        "channels = (int) 1, "
                                                                                        "framed = (boolean) true"));

    static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                                       GST_STATIC_CAPS("audio/x-raw, "
                                                                                       "format = (string) S16LE, "
                                                                                       "rate = (int) 8000, "
                                                                                       "channels = (int) 1"));

    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    base_class->start = GST_DEBUG_FUNCPTR(gst_celp_dec_start);
    base_class->stop = GST_DEBUG_FUNCPTR(gst_celp_dec_stop);
    base_class->set_format = GST_DEBUG_FUNCPTR(gst_celp_dec_set_format);
    base_class->handle_frame = GST_DEBUG_FUNCPTR(gst_celp_dec_handle_frame);

    GST_DEBUG_CATEGORY_INIT(gst_celp_dec_debug, "celpdec", 0, "CELP Decoder");
}

static void gst_celp_dec_init(GstCelpDec* self) {
    self->error_protection = FALSE;
    self->frame_count = 0;
}