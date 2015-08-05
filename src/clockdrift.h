
#ifndef __GST_CLOCKDRIFT_H__
#define __GST_CLOCKDRIFT_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_CLOCKDRIFT \
  (ClockDrift_get_type())

#define GST_CLOCKDRIFT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CLOCKDRIFT,ClockDrift))

#define GST_CLOCKDRIFT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CLOCKDRIFT,ClockDriftClass))

#define GST_IS_CLOCKDRIFT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CLOCKDRIFT))

#define GST_IS_CLOCKDRIFT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CLOCKDRIFT))

typedef struct _ClockDrift ClockDrift;
typedef struct _ClockDriftClass ClockDriftClass;

struct _ClockDrift
{
	GstElement element;
	GstPad *sinkpad;
	GstPad *srcpad;
	GstClockTime first_buffer;
	GstClockTime first_mono;

	gint last_drift;
	gint max_drift;
	gint min_drift;

	gint error_drift;

	gboolean silent;
};

struct _ClockDriftClass 
{
  GstElementClass parent_class;
};

GType ClockDrift_get_type (void);
gboolean InitClockDrift(GstPlugin *plugin);

G_END_DECLS

#endif /* __GST_PIH264SINK_H__ */
