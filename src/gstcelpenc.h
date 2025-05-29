#ifndef __GST_CELP_ENC_H__
#define __GST_CELP_ENC_H__

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>
#include <gst/audio/gstaudioencoder.h>

// CELP constants (from celp.h)
#define FRAME_SIZE 240
#define STREAMBITS 144

G_BEGIN_DECLS

#define GST_TYPE_CELP_ENC (gst_celp_enc_get_type())
G_DECLARE_FINAL_TYPE(GstCelpEnc, gst_celp_enc, GST, CELP_ENC, GstAudioEncoder)

struct _GstCelpEnc {
    GstAudioEncoder parent;

    /* Properties */
    gboolean error_protection;

    /* State */
    gint16 snew[FRAME_SIZE];
    gint frame_count;
    char packedbits[STREAMBITS / 8];
};

void celp_init(int prot);
int celp_encode(short iarf[FRAME_SIZE], char packedbits[STREAMBITS / 8]);

G_END_DECLS

#endif /* __GST_CELP_ENC_H__ */