#include "gstcelpenc.h"
#include "celp.h"
#include <gst/audio/audio.h>
#include <string.h>

GST_DEBUG_CATEGORY_STATIC(gst_celp_enc_debug);
#define GST_CAT_DEFAULT gst_celp_enc_debug

#define CELP_OK 0
#define BITRATE 2400  // 2.4 kbps

enum { PROP_0, PROP_ERROR_PROTECTION };

G_DEFINE_TYPE(GstCelpEnc, gst_celp_enc, GST_TYPE_AUDIO_ENCODER);

static gboolean gst_celp_enc_start(GstAudioEncoder* enc) {
    GstCelpEnc* self = GST_CELP_ENC(enc);

    // Initialize CELP codec
    celp_init(self->error_protection ? 1 : 0);
    self->frame_count = 0;
    return TRUE;
}

static gboolean gst_celp_enc_stop(GstAudioEncoder* enc) {
    return TRUE;
}

static GstFlowReturn gst_celp_enc_handle_frame(GstAudioEncoder* enc, GstBuffer* buffer) {
    GstCelpEnc* self = GST_CELP_ENC(enc);
    GstMapInfo map;
    gint16* samples;
    gint i, num_samples;

    if (!buffer)
        return GST_FLOW_OK;

    gst_buffer_map(buffer, &map, GST_MAP_READ);
    samples = (gint16*)map.data;
    num_samples = map.size / sizeof(gint16);

    for (i = 0; i < num_samples; i++) {
        self->snew[self->frame_count++] = samples[i];

        if (self->frame_count == FRAME_SIZE) {
            int result = celp_encode((short*)self->snew, self->packedbits);
            if (result != CELP_OK) {
                GST_WARNING_OBJECT(self, "CELP encoding error: %d", result);
            } else {
                GstBuffer* outbuf = gst_buffer_new_allocate(NULL, STREAMBITS / 8, NULL);
                gst_buffer_fill(outbuf, 0, self->packedbits, STREAMBITS / 8);
                gst_audio_encoder_finish_frame(enc, outbuf, FRAME_SIZE);
            }
            self->frame_count = 0;
        }
    }

    gst_buffer_unmap(buffer, &map);
    return GST_FLOW_OK;
}

static gboolean gst_celp_enc_set_format(GstAudioEncoder* enc, GstAudioInfo* info) {
    GstCelpEnc* self = GST_CELP_ENC(enc);

    // Validate input format
    if (GST_AUDIO_INFO_RATE(info) != 8000) {
        GST_ERROR_OBJECT(self, "Unsupported sample rate: %d", GST_AUDIO_INFO_RATE(info));
        return FALSE;
    }

    if (GST_AUDIO_INFO_CHANNELS(info) != 1) {
        GST_ERROR_OBJECT(self, "Unsupported channel count: %d", GST_AUDIO_INFO_CHANNELS(info));
        return FALSE;
    }

    if (GST_AUDIO_INFO_FORMAT(info) != GST_AUDIO_FORMAT_S16LE) {
        GST_ERROR_OBJECT(self, "Unsupported format: %s", gst_audio_format_to_string(GST_AUDIO_INFO_FORMAT(info)));
        return FALSE;
    }

    // Set output caps
    GstCaps* outcaps = gst_caps_new_simple("audio/x-celp", "rate", G_TYPE_INT, 8000, "channels", G_TYPE_INT, 1, "framed",
                                           G_TYPE_BOOLEAN, TRUE, NULL);

    gboolean ret = gst_audio_encoder_set_output_format(enc, outcaps);
    gst_caps_unref(outcaps);

    GST_INFO_OBJECT(self, "Format set successfully: %dHz, %d channels", GST_AUDIO_INFO_RATE(info), GST_AUDIO_INFO_CHANNELS(info));

    return ret;
}

static void gst_celp_enc_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
    GstCelpEnc* self = GST_CELP_ENC(object);

    switch (prop_id) {
        case PROP_ERROR_PROTECTION:
            self->error_protection = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_celp_enc_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
    GstCelpEnc* self = GST_CELP_ENC(object);

    switch (prop_id) {
        case PROP_ERROR_PROTECTION:
            g_value_set_boolean(value, self->error_protection);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_celp_enc_class_init(GstCelpEncClass* klass) {
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);
    GstAudioEncoderClass* base_class = GST_AUDIO_ENCODER_CLASS(klass);

    gobject_class->set_property = gst_celp_enc_set_property;
    gobject_class->get_property = gst_celp_enc_get_property;

    g_object_class_install_property(gobject_class, PROP_ERROR_PROTECTION,
                                    g_param_spec_boolean("error-protection", "Error Protection", "Enable error protection coding",
                                                         FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    gst_element_class_set_static_metadata(element_class, "CELP Audio Encoder", "Codec/Encoder/Audio",
                                          "Encodes audio in CELP format", "Emin xeome@proton.me");

    static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                                        GST_STATIC_CAPS("audio/x-raw, "
                                                                                        "format = (string) S16LE, "
                                                                                        "rate = (int) 8000, "
                                                                                        "channels = (int) 1"));

    static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                                       GST_STATIC_CAPS("audio/x-celp, "
                                                                                       "rate = (int) 8000, "
                                                                                       "channels = (int) 1, "
                                                                                       "framed = (boolean) true"));

    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    base_class->start = GST_DEBUG_FUNCPTR(gst_celp_enc_start);
    base_class->stop = GST_DEBUG_FUNCPTR(gst_celp_enc_stop);
    base_class->set_format = GST_DEBUG_FUNCPTR(gst_celp_enc_set_format);
    base_class->handle_frame = GST_DEBUG_FUNCPTR(gst_celp_enc_handle_frame);

    GST_DEBUG_CATEGORY_INIT(gst_celp_enc_debug, "celpenc", 0, "CELP Encoder");
}

static void gst_celp_enc_init(GstCelpEnc* self) {
    self->error_protection = FALSE;
    self->frame_count = 0;
}