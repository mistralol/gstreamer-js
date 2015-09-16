
#include <gst/gst.h>
#include <stdlib.h>

#include <time.h>

#include "clockdrift.h"

GST_DEBUG_CATEGORY_STATIC (clockdrift_debug);
#define GST_CAT_DEFAULT clockdrift_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
	PROP_SILENT = 1,
	PROP_DRIFT,
	PROP_MAX_DRIFT,
	PROP_MIN_DRIFT,
	PROP_ERROR_DRIFT
};

#define clockdrift_parent_class parent_class
G_DEFINE_TYPE (ClockDrift, ClockDrift, GST_TYPE_ELEMENT);


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

static void ClockDrift_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	ClockDrift *this = GST_CLOCKDRIFT (object);

	switch (prop_id) {
		case PROP_SILENT:
			this->silent = g_value_get_boolean(value);
			break;
		case PROP_ERROR_DRIFT:
			this->error_drift = g_value_get_int(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void ClockDrift_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	ClockDrift *this = GST_CLOCKDRIFT (object);

	switch (prop_id) {
		case PROP_SILENT:
			g_value_set_boolean(value, this->silent);
			break;
		case PROP_DRIFT:
			g_value_set_int(value, this->last_drift);
			break;
		case PROP_MAX_DRIFT:
			g_value_set_int(value, this->max_drift);
			break;
		case PROP_MIN_DRIFT:
			g_value_set_int(value, this->min_drift);
			break;
		case PROP_ERROR_DRIFT:
			g_value_set_int(value, this->error_drift);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GstFlowReturn ClockDrift_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	ClockDrift *this = GST_CLOCKDRIFT(parent);
	struct timespec ts;
	GstClockTime mono = 0;
	GstClockTime tsbuf = 0;
	gint diff = 0;

	if (GST_BUFFER_TIMESTAMP(buf) == GST_CLOCK_TIME_NONE)
		return gst_pad_push (this->srcpad, buf);

	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
	{
		gst_buffer_unref(buf);
		GST_ERROR("clock_gettime failed: %d", errno);
		return GST_FLOW_ERROR;
	}
	mono = GST_TIMESPEC_TO_TIME(ts);

	if (this->first_buffer == 0)
	{
		this->first_buffer = GST_BUFFER_TIMESTAMP(buf);
		this->first_mono = mono;
		return gst_pad_push (this->srcpad, buf);
	}
	mono -= this->first_mono;
	tsbuf = GST_BUFFER_TIMESTAMP(buf) - this->first_buffer;

	gint buffer_timestamp = GST_TIME_AS_MSECONDS(tsbuf);
	gint mono_timestamp = GST_TIME_AS_MSECONDS(mono);

	diff = buffer_timestamp - mono_timestamp;

	if (diff > this->max_drift)
		this->max_drift = diff;
	if (diff < this->min_drift)
		this->min_drift = diff;

	if (diff > this->error_drift)
	{
		GST_ERROR("ERROR_DRIFT1 Exceeded: %d > %d", diff, this->error_drift);
		gst_buffer_unref(buf);
		return GST_FLOW_ERROR;
	}

	//Be aware that this->error_drift + diff is really always a subtraction since we are adding a negitive number
	if (diff < 0 && this->error_drift + diff < 0)
	{
		GST_ERROR("ERROR_DRIFT2 Exceeded: %d > %d", -diff, this->error_drift);
		gst_buffer_unref(buf);
		return GST_FLOW_ERROR;
	}


	if (this->silent == FALSE)
	{
		g_print("Drift: %d Min: %d Max: %d\n", diff, this->min_drift, this->max_drift);
	}

	
	this->last_drift = diff;
	return gst_pad_push (this->srcpad, buf);
}

static gboolean ClockDrift_event (GstPad *pad, GstObject *parent, GstEvent  *event)
{
	ClockDrift* this = GST_CLOCKDRIFT(gst_pad_get_parent (pad));
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
static void ClockDrift_init (ClockDrift *this)
{
	this->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	this->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

	GST_PAD_SET_PROXY_CAPS(this->sinkpad);
	GST_PAD_SET_PROXY_CAPS(this->srcpad);

	gst_pad_set_event_function (this->sinkpad, ClockDrift_event);
	gst_pad_set_chain_function (this->sinkpad, ClockDrift_chain);

	gst_element_add_pad (GST_ELEMENT (this), this->sinkpad);
	gst_element_add_pad (GST_ELEMENT (this), this->srcpad);

	this->first_buffer = 0;
	this->first_mono = 0;
	this->last_drift = 0;
	this->min_drift = 0;
	this->max_drift = 0;
	this->error_drift = G_MAXINT;
	this->silent = TRUE;
}


static void ClockDrift_class_init (ClockDriftClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->set_property = ClockDrift_set_property;
	gobject_class->get_property = ClockDrift_get_property;

	g_object_class_install_property (gobject_class, PROP_SILENT,
		g_param_spec_boolean ("silent", "Silent", "Produce Output on stdout", FALSE, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_DRIFT,
		g_param_spec_int ("drift", "drift", "Get the last drift value seen", 0, G_MAXINT, G_MAXINT, G_PARAM_READABLE));

	g_object_class_install_property (gobject_class, PROP_MIN_DRIFT,
		g_param_spec_int ("min_drift", "min_drift", "Get the mininimum drift value seen in ms", 0, G_MAXINT, G_MAXINT, G_PARAM_READABLE));

	g_object_class_install_property (gobject_class, PROP_MAX_DRIFT,
		g_param_spec_int ("max_drift", "max_drift", "Get the maximum drift value seen in ms", 0, G_MAXINT, G_MAXINT, G_PARAM_READABLE));

	g_object_class_install_property (gobject_class, PROP_ERROR_DRIFT,
		g_param_spec_int ("error_drift", "error_drift", "Fail if the drift is more than this value in ms", G_MAXINT, G_MAXINT, G_MAXINT, G_PARAM_READABLE));

	gst_element_class_set_details_simple(gstelement_class,
		"ClockDrift",
		"ClockDrift",
		"Messure clock drift on gstreamer buffers from live sources",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
}

gboolean InitClockDrift(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (clockdrift_debug, "clockdrift", 0, "ClockDrift");
	return gst_element_register (plugin, "clockdrift", GST_RANK_NONE, GST_TYPE_CLOCKDRIFT);
}


