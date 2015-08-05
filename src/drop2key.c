
#include <gst/gst.h>
#include <stdlib.h>

#include "drop2key.h"

GST_DEBUG_CATEGORY_STATIC (drop2key_debug);
#define GST_CAT_DEFAULT drop2key_debug

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

#define drop2key_parent_class parent_class
G_DEFINE_TYPE (Drop2Key, Drop2Key, GST_TYPE_ELEMENT);


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

static void Drop2Key_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
//	Drop2Key *this = GST_DROP2KEY (object);

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void Drop2Key_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
//	Drop2Key *this = GST_Drop2Key (object);

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GstFlowReturn Drop2Key_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	Drop2Key *this = GST_DROP2KEY(parent);

	if (this->NeedKeyFrame)
	{
		if (GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT))
		{
			gst_buffer_unref(buf);
			return GST_FLOW_OK;
		}
		this->NeedKeyFrame = FALSE;
	}

	return gst_pad_push (this->srcpad, buf);
}

static gboolean Drop2Key_event (GstPad *pad, GstObject *parent, GstEvent  *event)
{
	Drop2Key* this = GST_DROP2KEY(gst_pad_get_parent (pad));
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
static void Drop2Key_init (Drop2Key *this)
{
	this->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	this->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

	gst_pad_set_event_function (this->sinkpad, Drop2Key_event);

	gst_pad_set_chain_function (this->sinkpad, Drop2Key_chain);

	gst_element_add_pad (GST_ELEMENT (this), this->sinkpad);
	gst_element_add_pad (GST_ELEMENT (this), this->srcpad);

	this->NeedKeyFrame = TRUE;
}


static void Drop2Key_class_init (Drop2KeyClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->set_property = Drop2Key_set_property;
	gobject_class->get_property = Drop2Key_get_property;

	gst_element_class_set_details_simple(gstelement_class,
		"Drop2Key",
		"Drop2Key",
		"Drops all delta frames until a key frame arrives",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
}

gboolean InitDrop2Key(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (drop2key_debug, "drop2key", 0, "Drop2Key");
	return gst_element_register (plugin, "drop2key", GST_RANK_NONE, GST_TYPE_DROP2KEY);
}


