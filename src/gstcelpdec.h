#ifndef __GST_CELP_DEC_H__
#define __GST_CELP_DEC_H__

#include <gst/gst.h>
#include <gst/audio/gstaudiodecoder.h>

// CELP constants (from celp.h)
#define FRAME_SIZE 240
#define STREAMBITS 144

G_BEGIN_DECLS

#define GST_TYPE_CELP_DEC (gst_celp_dec_get_type())
G_DECLARE_FINAL_TYPE(GstCelpDec, gst_celp_dec, GST, CELP_DEC, GstAudioDecoder)

struct _GstCelpDec {
    GstAudioDecoder parent;

    /* Properties */
    gboolean error_protection;

    /* State */
    gint frame_count;
    char packedbits[STREAMBITS / 8];
};

int celp_decode(char packedbits[STREAMBITS / 8], short pf[FRAME_SIZE]);

G_END_DECLS

#endif /* __GST_CELP_DEC_H__ */