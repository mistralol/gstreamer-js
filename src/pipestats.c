
#include <glib.h>
#include <gst/gst.h>
#include <stdlib.h>

#include <time.h>

#include "pipestats.h"

GST_DEBUG_CATEGORY_STATIC (pipestats_debug);
#define GST_CAT_DEFAULT pipestats_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
	PROP_SILENT = 1,
	PROP_BUS,
	PROP_PERIOD,

	/* Accesses For Data */
	PROP_TOTAL_FRAMES,
	PROP_TOTAL_IFRAMES,
	PROP_TOTAL_DFRAMES,
	PROP_TOTAL_SIZE,
	PROP_TOTAL_ISIZE,
	PROP_TOTAL_DSIZE,

	PROP_LARGEST_FRAME,
	PROP_LARGEST_IFRAME,
	PROP_LARGEST_DFRAME,
	PROP_LARGEST_GOP,
	PROP_SMALLEST_FRAME,
	PROP_SMALLEST_IFRAME,
	PROP_SMALLEST_DFRAME,
	PROP_SMALLEST_GOP,

	PROP_FRAME_RATE,
	PROP_DATA_RATE
};

#define pipestats_parent_class parent_class
G_DEFINE_TYPE (PipeStats, PipeStats, GST_TYPE_ELEMENT);


static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE (
	"sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE (
	"src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
);

static void PipeStats_Reset(PipeStats *stats)
{
	stats->total_frames = 0;
	stats->total_iframes = 0;
	stats->total_dframes = 0;

	stats->total_size = 0;
	stats->total_isize = 0;
	stats->total_dsize = 0;

	stats->current_gop = 0;

	stats->current_frame_count = 0;
	stats->current_iframe_count = 0;
	stats->current_dframe_count = 0;

	stats->current_frame_size = 0;
	stats->current_iframe_size = 0;
	stats->current_dframe_size = 0;

	stats->last_frame_count = 0;
	stats->last_iframe_count = 0;
	stats->last_dframe_count = 0;

	stats->last_frame_size = 0;
	stats->last_iframe_size = 0;
	stats->last_dframe_size = 0;

	if (clock_gettime(CLOCK_MONOTONIC, &stats->last_time) < 0)
		abort();
	stats->last_time.tv_nsec = 0;

	stats->largest_frame = 0;
	stats->largest_iframe = 0;
	stats->largest_dframe = 0;
	stats->largest_gop = 0;

	stats->smallest_frame = G_MAXINT;
	stats->smallest_iframe = G_MAXINT;
	stats->smallest_dframe = G_MAXINT;
	stats->smallest_gop = G_MAXINT;
}

static void PipeStats_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	PipeStats *stats = GST_PIPESTATS (object);

	switch (prop_id) {
		case PROP_SILENT:
			stats->silent = g_value_get_boolean (value);
			break;
		case PROP_BUS:
			stats->bus = g_value_get_boolean (value);
			break;
		case PROP_PERIOD:
			stats->period = g_value_get_int(value);
			break;

		/* These are read only! */
		case PROP_TOTAL_FRAMES:
		case PROP_TOTAL_IFRAMES:
		case PROP_TOTAL_DFRAMES:
		case PROP_TOTAL_SIZE:
		case PROP_TOTAL_ISIZE:
		case PROP_TOTAL_DSIZE:
		case PROP_LARGEST_FRAME:
		case PROP_LARGEST_IFRAME:
		case PROP_LARGEST_DFRAME:
		case PROP_LARGEST_GOP:
		case PROP_SMALLEST_FRAME:
		case PROP_SMALLEST_IFRAME:
		case PROP_SMALLEST_DFRAME:
		case PROP_SMALLEST_GOP:
		case PROP_FRAME_RATE:
		case PROP_DATA_RATE:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void PipeStats_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	PipeStats *stats = GST_PIPESTATS (object);

	switch (prop_id) {
		case PROP_SILENT:
			g_value_set_boolean (value, stats->silent);
			break;
		case PROP_BUS:
			g_value_set_boolean (value, stats->bus);
			break;
		case PROP_PERIOD:
			g_value_set_int(value, stats->period);
			break;
		case PROP_TOTAL_FRAMES:
			g_value_set_int64(value, stats->total_frames);
			break;
		case PROP_TOTAL_IFRAMES:
			g_value_set_int64(value, stats->total_iframes);
			break;
		case PROP_TOTAL_DFRAMES:
			g_value_set_int64(value, stats->total_dframes);
			break;
		case PROP_TOTAL_SIZE:
			g_value_set_int64(value, stats->total_size);
			break;
		case PROP_TOTAL_ISIZE:
			g_value_set_int64(value, stats->total_isize);
			break;
		case PROP_TOTAL_DSIZE:
			g_value_set_int64(value, stats->total_dsize);
			break;
		case PROP_LARGEST_FRAME:
			g_value_set_int64(value, stats->largest_frame);
			break;
		case PROP_LARGEST_IFRAME:
			g_value_set_int64(value, stats->largest_iframe);
			break;
		case PROP_LARGEST_DFRAME:
			g_value_set_int64(value, stats->largest_dframe);
			break;
		case PROP_LARGEST_GOP:
			g_value_set_int64(value, stats->largest_gop);
			break;
		case PROP_SMALLEST_FRAME:
			g_value_set_int64(value, stats->smallest_frame);
			break;
		case PROP_SMALLEST_IFRAME:
			g_value_set_int64(value, stats->smallest_iframe);
			break;
		case PROP_SMALLEST_DFRAME:
			g_value_set_int64(value, stats->smallest_dframe);
			break;
		case PROP_SMALLEST_GOP:
			g_value_set_int64(value, stats->smallest_gop);
			break;
		case PROP_FRAME_RATE:
			{
				double tmp = ((double) stats->last_frame_count) / stats->period;
				g_value_set_double(value, tmp);
			}
			break;
		case PROP_DATA_RATE:
			{
				double tmp = ((double) stats->last_frame_size) / stats->period;
				g_value_set_double(value, tmp);
			}
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GstFlowReturn PipeStats_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	struct timespec current_time;
	PipeStats *stats = GST_PIPESTATS(parent);

	if (clock_gettime(CLOCK_MONOTONIC, &current_time) < 0)
		abort();

	stats->total_frames++;
	stats->total_size += gst_buffer_get_size(buf);

	stats->current_frame_count++;
	stats->current_frame_size += gst_buffer_get_size(buf);

	if (gst_buffer_get_size(buf) > stats->largest_frame)
	{
		GST_INFO("New Largest Frame %" G_GUINT64_FORMAT " -> %" G_GUINT64_FORMAT, stats->largest_frame, stats->largest_frame);
		stats->largest_frame = gst_buffer_get_size(buf);
	}

	if (gst_buffer_get_size(buf) < stats->smallest_frame)
	{
		GST_INFO("New Smallest Frame %" G_GUINT64_FORMAT " -> %" G_GUINT64_FORMAT, stats->smallest_frame, stats->smallest_frame);
		stats->smallest_frame = gst_buffer_get_size(buf);
	}

	if (GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT))
	{
		stats->total_dframes++;
		stats->total_dsize += gst_buffer_get_size(buf);

		stats->current_dframe_count++;
		stats->current_dframe_size += gst_buffer_get_size(buf);

		stats->current_gop++;

		if (gst_buffer_get_size(buf) > stats->largest_dframe)
		{
			GST_INFO("New Largest DFrame %" G_GUINT64_FORMAT " -> %" G_GUINT64_FORMAT, stats->largest_dframe, stats->largest_dframe);
			stats->largest_dframe = gst_buffer_get_size(buf);
		}

		if (gst_buffer_get_size(buf) < stats->smallest_dframe)
		{
			GST_INFO("New Smallest DFrame %" G_GUINT64_FORMAT " -> %" G_GUINT64_FORMAT, stats->smallest_dframe, stats->smallest_dframe);
			stats->smallest_dframe = gst_buffer_get_size(buf);
		}
	}
	else
	{
		stats->total_iframes++;
		stats->total_isize += gst_buffer_get_size(buf);

		stats->current_iframe_count++;
		stats->current_iframe_size += gst_buffer_get_size(buf);

		if (stats->current_gop > stats->largest_gop)
		{
			GST_INFO("New Largest Gop Count %" G_GUINT64_FORMAT " -> %" G_GUINT64_FORMAT, stats->largest_gop, stats->current_gop);
			stats->largest_gop = stats->current_gop;
			stats->current_gop = 0;
		}

		if (gst_buffer_get_size(buf) > stats->largest_iframe)
		{
			GST_INFO("New Largest IFrame %" G_GUINT64_FORMAT " -> %" G_GUINT64_FORMAT, stats->largest_frame, stats->largest_iframe);
			stats->largest_iframe = gst_buffer_get_size(buf);
		}

		if (gst_buffer_get_size(buf) < stats->smallest_iframe)
		{
			GST_INFO("New Smallest IFrame %" G_GUINT64_FORMAT " -> %" G_GUINT64_FORMAT, stats->smallest_iframe, stats->smallest_iframe);
			stats->smallest_iframe = gst_buffer_get_size(buf);
		}
	}

	if (current_time.tv_sec >= stats->last_time.tv_sec + stats->period)
	{
		stats->last_frame_count = stats->current_frame_count;
		stats->last_iframe_count = stats->current_iframe_count;
		stats->last_dframe_count = stats->current_dframe_count;

		stats->last_frame_size = stats->current_frame_size;
		stats->last_iframe_size = stats->current_iframe_size;
		stats->last_dframe_size = stats->current_dframe_size;

		stats->current_frame_count = 0;
		stats->current_iframe_count = 0;
		stats->current_dframe_count = 0;

		stats->current_frame_size = 0;
		stats->current_iframe_size = 0;
		stats->current_dframe_size = 0;

		stats->last_time.tv_sec = current_time.tv_sec;
		//stats->last_time.tv_nsec = current_time.tv_nsec; //Not needed just let it stay 0 more work is required to make this more accurate

		if (stats->silent == FALSE)
		{
			g_print("Frames: %" G_GUINT64_FORMAT " Size: %" G_GUINT64_FORMAT " Frame Rate: %f Data Rate: %f KBytes/Sec %f KBits/sec\n", 
					stats->total_frames,
					stats->total_size,
					(double) (stats->last_frame_count) / stats->period,
					((double) (stats->last_frame_size) / stats->period) / 1024,
					(((double) (stats->last_frame_size) / stats->period) / 1024) * 8
			);
		}
		
		if (stats->bus == TRUE)
		{
			GstStructure *data = gst_structure_new("pipestats",
				"FrameRate", G_TYPE_DOUBLE, (double) (stats->last_frame_count) / stats->period,
				"BitRate", G_TYPE_DOUBLE, (((double) (stats->last_frame_size) / stats->period) / 1024) * 8,
				"TotalFrames", G_TYPE_UINT64, stats->total_frames,
				"TotalIFrames", G_TYPE_UINT64, stats->total_iframes,
				"TotalDFrames", G_TYPE_UINT64, stats->total_dframes,
				"TotalSize", G_TYPE_UINT64, stats->total_size,
				"TotalISize", G_TYPE_UINT64, stats->total_isize,
				"TotalDSize", G_TYPE_UINT64, stats->total_dsize,
				"CurrentGOP", G_TYPE_UINT64, stats->current_gop,	
				"CurrentFrameCount", G_TYPE_UINT64, stats->current_frame_count,
				"CurrentIFrameCount", G_TYPE_UINT64, stats->current_iframe_count,
				"CurrentDFrameCount", G_TYPE_UINT64, stats->current_dframe_count,
				"CurrentFrameSize", G_TYPE_UINT64, stats->current_frame_size,
				"CurrentIFrameSize", G_TYPE_UINT64, stats->current_iframe_size,
				"CurrentDFrameSize", G_TYPE_UINT64, stats->current_dframe_size,
				"LargestFrame", G_TYPE_UINT64, stats->largest_frame,
				"LargestIFrame", G_TYPE_UINT64, stats->largest_iframe,
				"LargestDFramge", G_TYPE_UINT64, stats->largest_dframe,
				"LargestGOP", G_TYPE_UINT64, stats->largest_gop,
				"SmallestFrame", G_TYPE_UINT64, stats->smallest_frame,
				"SmallestIFrame", G_TYPE_UINT64, stats->smallest_iframe,
				"SmallestDFramge", G_TYPE_UINT64, stats->smallest_dframe,
				"SmallestGOP", G_TYPE_UINT64, stats->smallest_gop,
				
			NULL);
			GstMessage *msg = gst_message_new_element(GST_OBJECT(stats), data);
			gst_element_post_message(GST_ELEMENT_CAST(stats), msg);
		}
	}
	return gst_pad_push (stats->srcpad, buf);
}

static gboolean PipeStats_event (GstPad *pad, GstObject *parent, GstEvent  *event)
{
	PipeStats* stats = GST_PIPESTATS(gst_pad_get_parent (pad));
	gboolean ret = FALSE;

	switch (GST_EVENT_TYPE (event)) {
		case GST_EVENT_CAPS: /* Simply forward caps */
		{
			GstCaps *caps = NULL;

			gst_event_parse_caps (event, &caps);
			GstCaps *ncaps = gst_caps_copy(caps);
			ret = gst_pad_set_caps(stats->srcpad, ncaps);
			gst_caps_unref(ncaps);
			gst_event_unref(event);
			break;
		}
		default:
			ret = gst_pad_event_default(pad, parent, event);
			break;
	}

	return ret;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void PipeStats_init (PipeStats *stats)
{
	stats->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	stats->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

	GST_PAD_SET_PROXY_CAPS(stats->sinkpad);
	GST_PAD_SET_PROXY_CAPS(stats->srcpad);

	gst_pad_set_event_function (stats->sinkpad, PipeStats_event);
	gst_pad_set_chain_function (stats->sinkpad, PipeStats_chain);

	gst_element_add_pad (GST_ELEMENT (stats), stats->sinkpad);
	gst_element_add_pad (GST_ELEMENT (stats), stats->srcpad);

	stats->silent = FALSE;
	stats->period = 5;
	stats->bus = FALSE;

	PipeStats_Reset(stats);
}


static void PipeStats_class_init (PipeStatsClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->set_property = PipeStats_set_property;
	gobject_class->get_property = PipeStats_get_property;

	//gstelement_class->change_state = PipeStats_change_state;

	g_object_class_install_property (gobject_class, PROP_SILENT,
		g_param_spec_boolean ("silent", "Silent", "Produce Output about stats on stdout", FALSE, G_PARAM_READWRITE));
		
	g_object_class_install_property (gobject_class, PROP_BUS,
		g_param_spec_boolean ("bus", "bus", "Send bus events", FALSE, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_PERIOD,
		g_param_spec_int ("period", "Period", "Number of seconds worth of data for calculating averages", 0, 60, 5, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_TOTAL_FRAMES,
		g_param_spec_int64 ("total_frames", "Total Frames", "Total Number of Frames Seen", 0, G_MAXINT64, 0, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_TOTAL_IFRAMES,
		g_param_spec_int64 ("total_iframes", "Total IFrames", "Total Number of IFrames Seen", 0, G_MAXINT64, 0, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_TOTAL_DFRAMES,
		g_param_spec_int64 ("total_dframes", "Total DFrames", "Total Number of DFrames Seen", 0, G_MAXINT64, 0, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_TOTAL_SIZE,
		g_param_spec_int64 ("total_size", "Total Size", "Total Size of data seen", 0, G_MAXINT64, 0, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_TOTAL_ISIZE,
		g_param_spec_int64 ("total_isize", "Total ISize", "Total Size of data seen in IFrames", 0, G_MAXINT64, 0, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_TOTAL_DSIZE,
		g_param_spec_int64 ("total_dsize", "Total DSize", "Total Size of data seen in DFrames", 0, G_MAXINT64, 0, G_PARAM_READABLE));

	g_object_class_install_property (gobject_class, PROP_LARGEST_FRAME,
		g_param_spec_int64 ("largest_frame", "Largest Frame", "Largest Frame Seen", 0, G_MAXINT64, 0, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_LARGEST_IFRAME,
		g_param_spec_int64 ("largest_iframe", "Largest IFrame", "Largest IFrame Seen", 0, G_MAXINT64, 0, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_LARGEST_DFRAME,
		g_param_spec_int64 ("largest_dframe", "Largest DFrame", "Largest DFrame Seen", 0, G_MAXINT64, 0, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_LARGEST_GOP,
		g_param_spec_int64 ("largest_gop", "Largest GOP", "Largest GOP Seen", 0, G_MAXINT64, 0, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_SMALLEST_FRAME,
		g_param_spec_int64 ("smallest_frame", "Smallest Frame", "Smallest Frame Seen", 0, G_MAXINT64, G_MAXINT64, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_SMALLEST_IFRAME,
		g_param_spec_int64 ("smallest_iframe", "Smallest IFrame", "Smallest IFrame Seen", 0, G_MAXINT64, G_MAXINT64, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_SMALLEST_DFRAME,
		g_param_spec_int64 ("smallest_dframe", "Smallest DFrame", "Smallest DFrame Seen", 0, G_MAXINT64, G_MAXINT64, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_SMALLEST_GOP,
		g_param_spec_int64 ("smallest_gop", "Smallest GOP", "Smallest GOP Seen", 0, G_MAXINT64, G_MAXINT64, G_PARAM_READABLE));

	g_object_class_install_property (gobject_class, PROP_FRAME_RATE,
		g_param_spec_double ("frame_rate", "Current Frame Rate", "Gets Frame Rate From Last Period", 0, G_MAXFLOAT, 0, G_PARAM_READABLE));
	g_object_class_install_property (gobject_class, PROP_DATA_RATE,
		g_param_spec_double ("data_rate", "Current Data Rate", "Gets Data Rate From Last Period in bytes/sec", 0, G_MAXFLOAT, 0, G_PARAM_READABLE));


	gst_element_class_set_details_simple(gstelement_class,
		"PipeStats",
		"PipeStats",
		"Pipe Line Stats",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

gboolean InitPipeStats(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (pipestats_debug, "pipestats", 0, "PipeStats");
	return gst_element_register (plugin, "pipestats", GST_RANK_NONE, GST_TYPE_PIPESTATS);
}


