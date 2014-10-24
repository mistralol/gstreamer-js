
#ifndef __GST_PIPESTATS_H__
#define __GST_PIPESTATS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_PIPESTATS \
  (PipeStats_get_type())

#define GST_PIPESTATS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PIPESTATS,PipeStats))

#define GST_PIPESTATS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PIPESTATS,PipeStatsClass))

#define GST_IS_PIPESTATS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PIPESTATS))

#define GST_IS_PIPESTATS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PIPESTATS))

typedef struct _PipeStats PipeStats;
typedef struct _PipeStatsClass PipeStatsClass;

struct _PipeStats
{
	GstElement element;
	GstPad *sinkpad;
	GstPad *srcpad;

	/* Properties */
	gboolean silent;
	guint period;

	/* Data */
	guint64 total_frames;
	guint64 total_iframes;
	guint64 total_dframes;

	guint64 total_size;
	guint64 total_isize;
	guint64 total_dsize;

	guint64 current_gop;

	guint64 current_frame_count;
	guint64 current_iframe_count;
	guint64 current_dframe_count;

	guint64 current_frame_size;
	guint64 current_iframe_size;
	guint64 current_dframe_size;

	guint64 last_frame_count;
	guint64 last_iframe_count;
	guint64 last_dframe_count;

	guint64 last_frame_size;
	guint64 last_iframe_size;
	guint64 last_dframe_size;

	struct timespec last_time;

	guint64 largest_frame;
	guint64 largest_iframe;
	guint64 largest_dframe;
	guint64 largest_gop;

	guint64 smallest_frame;
	guint64 smallest_iframe;
	guint64 smallest_dframe;
	guint64 smallest_gop;
};

struct _PipeStatsClass 
{
  GstElementClass parent_class;
};

GType PipeStats_get_type (void);

G_END_DECLS

#endif /* __GST_PIH264SINK_H__ */
