
#ifndef __GST_DUMPCAPS_H__
#define __GST_DUMPCAPS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_DUMPCAPS \
  (DumpCaps_get_type())

#define GST_DUMPCAPS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DUMPCAPS,DumpCaps))

#define GST_DUMPCAPS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DUMPCAPS,DumpCapsClass))

#define GST_IS_DUMPCAPS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DUMPCAPS))

#define GST_IS_DUMPCAPS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DUMPCAPS))

typedef struct _DumpCaps DumpCaps;
typedef struct _DumpCapsClass DumpCapsClass;

struct _DumpCaps
{
	GstElement element;
	GstPad *sinkpad;
	GstPad *srcpad;
	GstCaps *caps;
	gboolean silent;
};

struct _DumpCapsClass 
{
  GstElementClass parent_class;
};

GType DumpCaps_get_type (void);
gboolean InitDumpCaps(GstPlugin *plugin);

G_END_DECLS

#endif /* __GST_PIH264SINK_H__ */
