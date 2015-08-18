
#include <gst/gst.h>
#include <stdlib.h>

#include "dumpcaps.h"

GST_DEBUG_CATEGORY_STATIC (dumpcaps_debug);
#define GST_CAT_DEFAULT dumpcaps_debug

/* Filter signals and args */
enum
{
  PROP_SILENT = 1,
  LAST_SIGNAL
};

#define dumpcaps_parent_class parent_class
G_DEFINE_TYPE (DumpCaps, DumpCaps, GST_TYPE_ELEMENT);


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

static void DumpCaps_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	DumpCaps *this = GST_DUMPCAPS (object);

	switch (prop_id) {
		case PROP_SILENT:
			this->silent = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void DumpCaps_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	DumpCaps *this = GST_DUMPCAPS (object);

	switch (prop_id) {
		case PROP_SILENT:
			g_value_set_boolean (value, this->silent);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GstFlowReturn DumpCaps_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	DumpCaps *this = GST_DUMPCAPS(parent);

	if (this->caps == NULL)
	{
		GST_ERROR("CAPS IS NULL");
		
	}
	else
	{
		gchar *str = gst_caps_to_string(this->caps);
		if (this->silent == FALSE)
			g_print("CAPS: %s\n", str);
		GST_INFO("CAPS: %s", str);
		g_free(str);
	}


	return gst_pad_push (this->srcpad, buf);
}

static gboolean DumpCaps_event (GstPad *pad, GstObject *parent, GstEvent  *event)
{
	DumpCaps* this = GST_DUMPCAPS(gst_pad_get_parent (pad));
	gboolean ret = FALSE;

	switch (GST_EVENT_TYPE (event)) {
		case GST_EVENT_CAPS: /* Simply forward caps */
		{
			GstCaps *caps = NULL;

			gst_event_parse_caps (event, &caps);
			GstCaps *ncaps = gst_caps_copy(caps);
			ret = gst_pad_set_caps(this->srcpad, ncaps);
			gst_caps_unref(ncaps);
			
			if (this->caps)
			{
				gst_caps_unref(this->caps);
				this->caps = NULL;
			}
			this->caps = gst_caps_copy(caps);
				
			gchar *str = gst_caps_to_string(this->caps);
			if (this->silent == FALSE)
				g_print("NEW CAPS: %s\n", str);
			GST_INFO("NEW CAPS: %s", str);
			g_free(str);
			break;
		}
		default:
			ret = gst_pad_event_default (pad, parent, event);
			break;
	}

	return ret;
}

static void DumpCaps_Finalize(GObject *object)
{
	DumpCaps *this = GST_DUMPCAPS(object);

	if (this->caps)
	{
		gst_caps_unref(this->caps);
		this->caps = NULL;
	}
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void DumpCaps_init (DumpCaps *this)
{
	this->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	this->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

	gst_pad_set_event_function (this->sinkpad, DumpCaps_event);
	gst_pad_set_chain_function (this->sinkpad, DumpCaps_chain);

	gst_element_add_pad (GST_ELEMENT (this), this->sinkpad);
	gst_element_add_pad (GST_ELEMENT (this), this->srcpad);
}


static void DumpCaps_class_init (DumpCapsClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->finalize = DumpCaps_Finalize;
	gobject_class->set_property = DumpCaps_set_property;
	gobject_class->get_property = DumpCaps_get_property;

	g_object_class_install_property (gobject_class, PROP_SILENT,
		g_param_spec_boolean ("silent", "Silent", "Produce Output about caps on stdout", FALSE, G_PARAM_READWRITE));


	gst_element_class_set_details_simple(gstelement_class,
		"DumpCaps",
		"DumpCaps",
		"Drops all delta frames",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
}

gboolean InitDumpCaps(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (dumpcaps_debug, "dumpcaps", 0, "DumpCaps");
	return gst_element_register (plugin, "dumpcaps", GST_RANK_NONE, GST_TYPE_DUMPCAPS);
}


