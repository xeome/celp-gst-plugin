#include "gstcelpenc.h"
#include "gstcelpdec.h"
#include <gst/gst.h>

#define VERSION "0.1.0"
#define PACKAGE "gst-celp-plugin"

static gboolean plugin_init(GstPlugin* plugin) {
    if (!gst_element_register(plugin, "celpenc", GST_RANK_PRIMARY, GST_TYPE_CELP_ENC))
        return FALSE;

    if (!gst_element_register(plugin, "celpdec", GST_RANK_PRIMARY, GST_TYPE_CELP_DEC))
        return FALSE;

    return TRUE;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  celp,
                  "CELP Audio Codec",
                  plugin_init,
                  VERSION,
                  "LGPL",
                  PACKAGE,
                  "https://gstreamer.net/")