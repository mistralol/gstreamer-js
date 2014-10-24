
#ifndef __GST_PIPELINE_STATS_H__
#define __GST_PIPELINE_STATS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_PIH264SINK \
  (gst_pih264sink_get_type())
#define GST_PIH264SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PIH264SINK,GstPih264Sink))
#define GST_PIH264SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PIH264SINK,GstPih264SinkClass))
#define GST_IS_PIH264SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PIH264SINK))
#define GST_IS_PIH264SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PIH264SINK))

typedef struct _GstPih264Sink      GstPih264Sink;
typedef struct _GstPih264SinkClass GstPih264SinkClass;

struct video_dev
{
	ILCLIENT_T *client;

	COMPONENT_T *video_decode;
	COMPONENT_T *video_scheduler;
	COMPONENT_T *video_render;
	COMPONENT_T *clock;

	OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
	OMX_VIDEO_PARAM_PORTFORMATTYPE format;

	TUNNEL_T tunnel[4];
	COMPONENT_T *list[5];   // Used for Cleanup

	int firstpacket;
	int port_settings_changed;
};


struct _GstPih264Sink
{
	GstElement element;

	GstPad *sinkpad;

	gboolean silent;

	struct video_dev *dev;
};

struct _GstPih264SinkClass 
{
  GstElementClass parent_class;
};

GType gst_pih264sink_get_type (void);

G_END_DECLS

#endif /* __GST_PIH264SINK_H__ */
