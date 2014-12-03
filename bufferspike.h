
#ifndef __GST_BUFFERSPIKE_H__
#define __GST_BUFFERSPIKE_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_BUFFERSPIKE \
  (BufferSpike_get_type())

#define GST_BUFFERSPIKE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_BUFFERSPIKE,BufferSpike))

#define GST_BUFFERSPIKE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_BUFFERSPIKE,BufferSpikeClass))

#define GST_IS_BUFFERSPIKE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_BUFFERSPIKE))

#define GST_IS_BUFFERSPIKE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_BUFFERSPIKE))

typedef struct _BufferSpike BufferSpike;
typedef struct _BufferSpikeClass BufferSpikeClass;

struct _BufferSpike
{
	GstElement element;
	GstPad *sinkpad;
	GstPad *srcpad;

	GRand *rand;

	guint32 range_start;
	guint32 range_end;
	guint32 probability;
	guint8 value;
};

struct _BufferSpikeClass 
{
  GstElementClass parent_class;
};

GType BufferSpike_get_type (void);
gboolean InitBufferSpike(GstPlugin *plugin);

G_END_DECLS

#endif /* __GST_PIH264SINK_H__ */
