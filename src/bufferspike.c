
#include <gst/gst.h>
#include <stdlib.h>

#include "bufferspike.h"

GST_DEBUG_CATEGORY_STATIC (bufferspike_debug);
#define GST_CAT_DEFAULT bufferspike_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
	PROP_SILENT = 1,
	PROP_RANGE_START,
	PROP_RANGE_END,
	PROP_PROBABILITY,
	PROP_VALUE
};

#define bufferspike_parent_class parent_class
G_DEFINE_TYPE (BufferSpike, BufferSpike, GST_TYPE_ELEMENT);


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

static void BufferSpike_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	BufferSpike *this = GST_BUFFERSPIKE (object);

	switch (prop_id) {
		case PROP_RANGE_START:
			this->range_start = g_value_get_uint(value);
			break;
		case PROP_RANGE_END:
			this->range_end = g_value_get_uint(value);
			break;
		case PROP_PROBABILITY:
			this->probability = g_value_get_uint(value);
			break;
		case PROP_VALUE:
			this->value = g_value_get_uint(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void BufferSpike_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	BufferSpike *this = GST_BUFFERSPIKE (object);

	switch (prop_id) {
		case PROP_RANGE_START:
			g_value_set_uint(value, this->range_start);
			break;
		case PROP_RANGE_END:
			g_value_set_uint(value, this->range_end);
			break;
		case PROP_PROBABILITY:
			g_value_set_uint(value, this->probability);
			break;
		case PROP_VALUE:
			g_value_set_uint(value, this->value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GstFlowReturn BufferSpike_chain (GstPad *pad, GstObject *parent, GstBuffer *origbuf)
{
	BufferSpike *this = GST_BUFFERSPIKE(parent);
	GstBuffer *buf = origbuf;

	guint32 number = g_rand_int_range(this->rand, 1, 100);
	if (number < this->probability)
	{
		if (!gst_buffer_is_writable(buf))
		{
			buf = gst_buffer_make_writable(buf);
			gst_buffer_unref(origbuf);
			if (!buf)
				return GST_FLOW_ERROR;
		}
		gsize bufsize = gst_buffer_get_size(buf);
		gsize start = 0;
		gsize end = 0;

		gsize end_size = this->range_end;
		if (end_size > bufsize)
			end_size = bufsize;

		start = g_rand_int_range(this->rand, this->range_start, bufsize);
		end_size += start;

		end = g_rand_int_range(this->rand, start, end_size);

		if (start > bufsize)
			start = 0;
		if (end > bufsize)
			end = bufsize;
		if (start + end > bufsize)
			end = bufsize - start;

		if (end > start)
		{
			GST_WARNING("Spiking Buffer");
			gst_buffer_memset(buf, start, 0, end - start);
		}
	}


	return gst_pad_push (this->srcpad, buf);
}

static gboolean BufferSpike_event (GstPad *pad, GstObject *parent, GstEvent  *event)
{
	BufferSpike* this = GST_BUFFERSPIKE(gst_pad_get_parent (pad));
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
static void BufferSpike_init (BufferSpike *this)
{
	this->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	this->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

	gst_pad_set_event_function (this->sinkpad, BufferSpike_event);

	gst_pad_set_chain_function (this->sinkpad, BufferSpike_chain);

	gst_element_add_pad (GST_ELEMENT (this), this->sinkpad);
	gst_element_add_pad (GST_ELEMENT (this), this->srcpad);

	this->rand = g_rand_new();
	
	this->range_start = 0;
	this->range_end = G_MAXUINT;
	this->probability = 0;
	this->value = 0;
}

static void BufferSpike_Finalize(GObject *object)
{
	BufferSpike *this = GST_BUFFERSPIKE(object);

	g_rand_free(this->rand);

}

static void BufferSpike_class_init (BufferSpikeClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->finalize = BufferSpike_Finalize;
	gobject_class->set_property = BufferSpike_set_property;
	gobject_class->get_property = BufferSpike_get_property;

	g_object_class_install_property (gobject_class, PROP_RANGE_START,
		g_param_spec_uint ("range_start", "range_start", "Start position to spike the buffer from", 0, G_MAXUINT, 0, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_RANGE_END,
		g_param_spec_uint ("range_end", "range_end", "End position to spike the buffer from", 0, G_MAXUINT, G_MAXUINT, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_PROBABILITY,
		g_param_spec_uint ("probability", "probability", "Chance of a buffer being spiked", 0, 100, 0, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_VALUE,
		g_param_spec_uint ("value", "value", "Value to spike buffer with", 0, 255, 0, G_PARAM_READWRITE));


	gst_element_class_set_details_simple(gstelement_class,
		"BufferSpike",
		"BufferSpike",
		"Spikes buffers with random data values",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
}

gboolean InitBufferSpike(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (bufferspike_debug, "bufferspike", 0, "BufferSpike");
	return gst_element_register (plugin, "bufferspike", GST_RANK_NONE, GST_TYPE_BUFFERSPIKE);
}


