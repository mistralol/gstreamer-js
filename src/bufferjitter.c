
#include <gst/gst.h>

#include "bufferjitter.h"

GST_DEBUG_CATEGORY_STATIC (bufferjitter_debug);
#define GST_CAT_DEFAULT bufferjitter_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
	PROP_SILENT = 1,
	PROP_SMALLEST_GAP,
	PROP_LARGEST_GAP,
	PROP_AVERAGE_GAP,
	PROP_IGNORE_FIRST
};

#define BufferJitter_parent_class parent_class
G_DEFINE_TYPE (BufferJitter, BufferJitter, GST_TYPE_ELEMENT);


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

static void BufferJitter_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	BufferJitter *this = GST_BUFFERJITTER (object);

	switch (prop_id) {
		case PROP_SILENT:
			this->silent = g_value_get_boolean (value);
			break;
		case PROP_SMALLEST_GAP:
		case PROP_LARGEST_GAP:
		case PROP_AVERAGE_GAP:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
		case PROP_IGNORE_FIRST:
			this->ignore_first_orig = g_value_get_int64(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void BufferJitter_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	BufferJitter *this = GST_BUFFERJITTER (object);

	switch (prop_id) {
		case PROP_SILENT:
			g_value_set_boolean (value, this->silent);
			break;
		case PROP_SMALLEST_GAP:
			g_value_set_int64(value, GST_TIME_AS_MSECONDS(this->gap_smallest));
			break;
		case PROP_LARGEST_GAP:
			g_value_set_int64(value, GST_TIME_AS_MSECONDS(this->gap_largest));
			break;
		case PROP_AVERAGE_GAP:
			if (this->buffer_count == 0) //cover div by zero
			{
				g_value_set_int64(value, 0);
			}
			else
			{
				g_value_set_int64(value, GST_TIME_AS_MSECONDS(this->gap_total / this->buffer_count));
			}
			break;
		case PROP_IGNORE_FIRST:
			g_value_set_int64(value, this->ignore_first_orig);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GstFlowReturn BufferJitter_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	BufferJitter *this = GST_BUFFERJITTER(parent);

	if (this->ignore_first > 0)
	{
		this->ignore_first--;
		return gst_pad_push (this->srcpad, buf);
	}

	if (GST_BUFFER_PTS(buf) != GST_CLOCK_TIME_NONE)
	{
		if (this->NeedTimestamp == TRUE)
		{
			this->lasttimestamp = GST_BUFFER_PTS(buf);
			this->NeedTimestamp = FALSE;
		}
		else
		{
			GstClockTimeDiff diff = GST_CLOCK_DIFF(this->lasttimestamp, GST_BUFFER_PTS(buf));
			if (diff < this->gap_smallest)
			{
				GST_INFO("New Smallest Gap  %" G_GUINT64_FORMAT " MS", GST_TIME_AS_MSECONDS(diff));
				this->gap_smallest = diff;
			}
			if (diff > this->gap_largest)
			{
				GST_INFO("New Largest Gap  %" G_GUINT64_FORMAT " MS", GST_TIME_AS_MSECONDS(diff));
				this->gap_largest = diff;
			}

			this->gap_total += diff;
			this->buffer_count++;
			if (this->silent == FALSE)
			{
				g_print("BUFFERJITTER Smallest Gap: %" G_GUINT64_FORMAT " MS Largest Gap: %" G_GUINT64_FORMAT " MS Average Gap: %" G_GUINT64_FORMAT " MS\n", 
					GST_TIME_AS_MSECONDS(this->gap_smallest),
					GST_TIME_AS_MSECONDS(this->gap_largest),
					GST_TIME_AS_MSECONDS(this->gap_total / this->buffer_count));
			}
			this->lasttimestamp = GST_BUFFER_PTS(buf);
		}
	}
	else
	{
		if (this->silent == FALSE)
		{
			g_print("No Timestamp");
		}
		this->NeedTimestamp = TRUE;
	}

	return gst_pad_push (this->srcpad, buf);
}

static GstStateChangeReturn BufferJitter_change_state(GstElement *element, GstStateChange transition)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	BufferJitter *this = GST_BUFFERJITTER(element);

	if (GST_IS_BUFFERJITTER(element) == FALSE)
		return GST_FLOW_ERROR;

	switch(transition)
	{
		case GST_STATE_CHANGE_READY_TO_PAUSED:
			this->ignore_first = this->ignore_first_orig;
			break;
		default:
			break;
	}

	ret = GST_ELEMENT_CLASS(parent_class)->change_state (element, transition);
	if (ret == GST_STATE_CHANGE_FAILURE)
		return ret;

	switch(transition)
	{
		case GST_STATE_CHANGE_READY_TO_NULL:
			break;
		default:
			break;
	}

	return ret;
}

static gboolean BufferJitter_event (GstPad *pad, GstObject *parent, GstEvent  *event)
{
	BufferJitter* this = GST_BUFFERJITTER(gst_pad_get_parent (pad));
	gboolean ret = FALSE;

	switch (GST_EVENT_TYPE (event)) {
		case GST_EVENT_CAPS: /* Simply forward caps */
		{
			GstCaps *caps = NULL;

			gst_event_parse_caps (event, &caps);
			GstCaps *ncaps = gst_caps_copy(caps);
			ret = gst_pad_set_caps(this->srcpad, ncaps);
			gst_caps_unref(ncaps);
			break;
		}
		default:
			ret = gst_pad_event_default (pad, parent, event);
			break;
	}

	return ret;
}


/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void BufferJitter_init (BufferJitter *this)
{
	this->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	this->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

	GST_PAD_SET_PROXY_CAPS(this->sinkpad);
	GST_PAD_SET_PROXY_CAPS(this->srcpad);

	gst_pad_set_event_function (this->sinkpad, BufferJitter_event);
	gst_pad_set_chain_function (this->sinkpad, BufferJitter_chain);

	gst_element_add_pad (GST_ELEMENT (this), this->sinkpad);
	gst_element_add_pad (GST_ELEMENT (this), this->srcpad);

	this->silent = TRUE;
	this->NeedTimestamp = TRUE;
	this->lasttimestamp = 0;

	this->gap_smallest = G_MAXINT64;
	this->gap_largest = 0;
	this->gap_total = 0;
	this->buffer_count = 0;

	this->ignore_first_orig = 0;
	this->ignore_first = 0;
}


static void BufferJitter_class_init (BufferJitterClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->set_property = BufferJitter_set_property;
	gobject_class->get_property = BufferJitter_get_property;

	gstelement_class->change_state = BufferJitter_change_state;

	g_object_class_install_property (gobject_class, PROP_SILENT,
		g_param_spec_boolean ("silent", "Silent", "Produce Output about stats on stdout", TRUE, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_SMALLEST_GAP,
		g_param_spec_int64 ("smallest_gap", "Smallest Gap", "Smallest Gap between frames in ms", 0, G_MAXINT64, G_MAXINT64, G_PARAM_READABLE));

	g_object_class_install_property (gobject_class, PROP_LARGEST_GAP,
		g_param_spec_int64 ("largest_gap", "Largest Gap", "Largest Gap between frames in ms", 0, G_MAXINT64, 0, G_PARAM_READABLE));

	g_object_class_install_property (gobject_class, PROP_AVERAGE_GAP,
		g_param_spec_int64 ("average_gap", "Average Gap", "Average Gap between frames in ms", 0, G_MAXINT64, 0, G_PARAM_READABLE));

	g_object_class_install_property (gobject_class, PROP_IGNORE_FIRST,
		g_param_spec_int64 ("ignore_first", "Ignore first n frames", "Ignore the first N frames", 0, G_MAXINT64, 0, G_PARAM_READWRITE));


	gst_element_class_set_details_simple(gstelement_class,
		"BufferJitter",
		"BufferJitter",
		"Messures Time differences between buffers",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
}

gboolean InitBufferJitter(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (bufferjitter_debug, "bufferjitter", 0, "BufferJitter");
	return gst_element_register (plugin, "bufferjitter", GST_RANK_NONE, GST_TYPE_BUFFERJITTER);
}


