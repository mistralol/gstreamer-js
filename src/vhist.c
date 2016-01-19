
#include <gst/gst.h>
#include <gst/video/video.h>

#include "vhist.h"

GST_DEBUG_CATEGORY_STATIC (vhist_debug);
#define GST_CAT_DEFAULT vhist_debug

/* Filter signals and args */
enum
{
  PROP_SILENT = 1,
  PROP_SEND_EVENT,
  PROP_LAST_VALUE,
  LAST_SIGNAL
};

#define vhist_parent_class parent_class
G_DEFINE_TYPE (VHist, VHist, GST_TYPE_ELEMENT);


static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE (
	"sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE("RGB"))
);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE (
	"src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE("RGB"))
);

static void VHist_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	VHist *this = GST_VHIST (object);

	switch (prop_id) {
		case PROP_SEND_EVENT:
			this->send_events = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void VHist_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	VHist *this = GST_VHIST (object);

	switch (prop_id) {
		case PROP_SEND_EVENT:
			g_value_set_boolean (value, this->send_events);
			break;
		case PROP_LAST_VALUE:
			g_value_set_uint(value, this->last);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GstFlowReturn VHist_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	VHist *this = GST_VHIST(parent);

	GstMapInfo info;
	if (gst_buffer_map(buf, &info, GST_MAP_READ) == FALSE)
	{
		gst_buffer_unref(buf);
		return GST_FLOW_ERROR;
	}

	guint size = this->width * this->height * 3;
	guint64 sum = 0;
	for(guint i = 0;i<size;i++)
	{
		sum += info.data[i];
	}
	this->last = sum / size;
	
	gst_buffer_unmap(buf, &info);
	
	if (this->send_events)
	{
		GstStructure *data = gst_structure_new("brightness", "value", G_TYPE_UINT, this->last, NULL);
		GstMessage *msg = gst_message_new_element(GST_OBJECT(this), data);
		gst_element_post_message(GST_ELEMENT_CAST(this), msg);
	}

	return gst_pad_push (this->srcpad, buf);
}

static gboolean VHist_event (GstPad *pad, GstObject *parent, GstEvent  *event)
{
	VHist* this = GST_VHIST(gst_pad_get_parent (pad));
	gboolean ret = FALSE;

	switch (GST_EVENT_TYPE (event)) {
		case GST_EVENT_CAPS: /* Simply forward caps */
		{
			GstCaps *caps = NULL;

			gst_event_parse_caps (event, &caps);

			GstVideoInfo info;
			if (gst_video_info_from_caps (&info, caps) == FALSE)
			{
				gst_event_unref(event);
				return FALSE;
			}
			GST_INFO("New Width: %u Height: %u", info.width, info.height);
			this->width = info.width;
			this->height = info.height;

			ret = gst_pad_set_caps(this->srcpad, caps);
			gst_event_unref(event);
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
static void VHist_init (VHist *this)
{
	this->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	this->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
	
	gst_pad_set_event_function (this->sinkpad, VHist_event);
	gst_pad_set_chain_function (this->sinkpad, VHist_chain);

	this->send_events = TRUE;

	gst_element_add_pad (GST_ELEMENT (this), this->sinkpad);
	gst_element_add_pad (GST_ELEMENT (this), this->srcpad);
}

static void VHist_class_init (VHistClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->set_property = VHist_set_property;
	gobject_class->get_property = VHist_get_property;

	g_object_class_install_property (gobject_class, PROP_SEND_EVENT,
		g_param_spec_boolean ("sendevents", "sendevents", "Send brightness event on bus", FALSE, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_LAST_VALUE,
		g_param_spec_uint ("last", "last", "Read last brightness value", 0, 255, 0, G_PARAM_READABLE));


	gst_element_class_set_details_simple(gstelement_class,
		"VHist",
		"VHist",
		"Motion Detection",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
}

gboolean InitVHist(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (vhist_debug, "vhist", 0, "VHist");
	return gst_element_register (plugin, "vhist", GST_RANK_NONE, GST_TYPE_VHIST);
}


