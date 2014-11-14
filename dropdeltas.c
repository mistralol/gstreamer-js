
#include <gst/gst.h>
#include <stdlib.h>

#include "dropdeltas.h"

GST_DEBUG_CATEGORY_STATIC (dropdeltas_debug);
#define GST_CAT_DEFAULT dropdeltas_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
	PROP_SILENT = 1,

};

#define dropdeltas_parent_class parent_class
G_DEFINE_TYPE (DropDeltas, DropDeltas, GST_TYPE_ELEMENT);


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

static void DropDeltas_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
//	DropDeltas *this = GST_DROPDELTAS (object);

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void DropDeltas_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
//	DropDeltas *this = GST_DROPDELTAS (object);

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GstFlowReturn DropDeltas_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	DropDeltas *this = GST_DROPDELTAS(parent);

	if (GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT))
	{
		gst_buffer_unref(buf);
		return GST_FLOW_OK;
	}

	return gst_pad_push (this->srcpad, buf);
}

static gboolean DropDeltas_event (GstPad *pad, GstObject *parent, GstEvent  *event)
{
	DropDeltas* this = GST_DROPDELTAS(gst_pad_get_parent (pad));
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
static void DropDeltas_init (DropDeltas *this)
{
	this->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	this->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

	gst_pad_set_event_function (this->sinkpad, DropDeltas_event);

	gst_pad_set_chain_function (this->sinkpad, DropDeltas_chain);

	gst_element_add_pad (GST_ELEMENT (this), this->sinkpad);
	gst_element_add_pad (GST_ELEMENT (this), this->srcpad);
}


static void DropDeltas_class_init (DropDeltasClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->set_property = DropDeltas_set_property;
	gobject_class->get_property = DropDeltas_get_property;

	gst_element_class_set_details_simple(gstelement_class,
		"DropDeltas",
		"DropDeltas",
		"Drops all delta frames",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
}

gboolean InitDropDeltas(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (dropdeltas_debug, "dropdeltas", 0, "DropDeltas");
	return gst_element_register (plugin, "dropdeltas", GST_RANK_NONE, GST_TYPE_DROPDELTAS);
}


