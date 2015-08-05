
#ifndef __GST_BUFFERJITTER_H__
#define __GST_BUFFERJITTER_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_BUFFERJITTER \
  (BufferJitter_get_type())

#define GST_BUFFERJITTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_BUFFERJITTER,BufferJitter))

#define GST_BUFFERJITTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_BUFFERJITTER,BufferJitterClass))

#define GST_IS_BUFFERJITTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_BUFFERJITTER))

#define GST_IS_BUFFERJITTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_BUFFERJITTER))

typedef struct _BufferJitter BufferJitter;
typedef struct _BufferJitterClass BufferJitterClass;

struct _BufferJitter
{
	GstElement element;
	GstPad *sinkpad;
	GstPad *srcpad;

	gboolean silent;
	gboolean NeedTimestamp;
	GstClockTime lasttimestamp;

	GstClockTimeDiff gap_smallest;
	GstClockTimeDiff gap_largest;
	GstClockTimeDiff gap_total;
	guint64 buffer_count;

	guint64 ignore_first_orig;
	guint64 ignore_first;
};

struct _BufferJitterClass 
{
	GstElementClass parent_class;
};

GType BufferJitter_get_type (void);
gboolean InitBufferJitter(GstPlugin *plugin);

G_END_DECLS

#endif /* __GST_PIH264SINK_H__ */
