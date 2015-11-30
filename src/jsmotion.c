
#include <gst/gst.h>
#include <gst/video/video.h>

#include "jsmotion.h"

GST_DEBUG_CATEGORY_STATIC (jsmotion_debug);
#define GST_CAT_DEFAULT jsmotion_debug

/* Filter signals and args */
enum
{
  PROP_SILENT = 1,
  LAST_SIGNAL
};

#define jsmotion_parent_class parent_class
G_DEFINE_TYPE (JSMotion, JSMotion, GST_TYPE_ELEMENT);


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

static void JSMotion_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
//	JSMotion *this = GST_JSMOTION (object);

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void JSMotion_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
//	JSMotion *this = GST_JSMOTION (object);

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GstFlowReturn JSMotion_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	JSMotion *this = GST_JSMOTION(parent);

	GstBuffer *output = gst_buffer_copy(buf);
	if (output == NULL)
		return GST_FLOW_ERROR;

	if (this->last_frame != NULL)
	{
		GstMapInfo outputinfo;
		if (gst_buffer_map(output, &outputinfo, GST_MAP_WRITE) == FALSE)
		{
			gst_buffer_unref(output);
			return GST_FLOW_ERROR;
		}

		GstMapInfo lastinfo;
		gst_buffer_map(this->last_frame, &lastinfo, GST_MAP_READ);

		if (lastinfo.size != outputinfo.size)
		{
			GST_ERROR("lastinfo.size != tmpinfo.size");
			return GST_FLOW_OK;
		}

		for(gsize x = 0;x < outputinfo.size;x += 3)
		{
			outputinfo.data[x] = ABS(outputinfo.data[x] - lastinfo.data[x]);
			outputinfo.data[x+1] = ABS(outputinfo.data[x+1] - lastinfo.data[x+1]);
			outputinfo.data[x+2] = ABS(outputinfo.data[x+2] - lastinfo.data[x+2]);

			guint sense = 32;
			if (outputinfo.data[x] > sense || outputinfo.data[x+1] > sense || outputinfo.data[x+2] > sense)
			{
				outputinfo.data[x] = 255;
				outputinfo.data[x+1] = 255;
				outputinfo.data[x+2] = 255;
			}
			else
			{
				outputinfo.data[x] = 0;
				outputinfo.data[x+1] = 0;
				outputinfo.data[x+2] = 0;
			}
		}

		gst_buffer_unmap(this->last_frame, &lastinfo);
		gst_buffer_unmap(output, &outputinfo);

		gst_buffer_unref(this->last_frame);
		this->last_frame = NULL;
	}

	this->last_frame = buf;

	return gst_pad_push (this->srcpad, output);
}

static gboolean JSMotion_event (GstPad *pad, GstObject *parent, GstEvent  *event)
{
	JSMotion* this = GST_JSMOTION(gst_pad_get_parent (pad));
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
			GST_INFO("New Width: %d Height: %d", info.width, info.height);
			this->width = info.width;
			this->height = info.height;

			GstCaps *ncaps = gst_caps_copy(caps);
			ret = gst_pad_set_caps(this->srcpad, ncaps);
			gst_caps_unref(ncaps);
			gst_event_unref(event);
			break;
		}
		default:
			ret = gst_pad_event_default (pad, parent, event);
			break;
	}

	return ret;
}

static void JSMotion_Finalize(GObject *object)
{
	JSMotion *this = GST_JSMOTION(object);
	
	if (this->last_frame)
		gst_buffer_unref(this->last_frame);

}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void JSMotion_init (JSMotion *this)
{
	this->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	this->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
	
	this->last_frame = NULL;

	GST_PAD_SET_PROXY_CAPS(this->sinkpad);
	GST_PAD_SET_PROXY_CAPS(this->srcpad);

	gst_pad_set_event_function (this->sinkpad, JSMotion_event);
	gst_pad_set_chain_function (this->sinkpad, JSMotion_chain);

	gst_element_add_pad (GST_ELEMENT (this), this->sinkpad);
	gst_element_add_pad (GST_ELEMENT (this), this->srcpad);
}


static void JSMotion_class_init (JSMotionClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->finalize = JSMotion_Finalize;
	gobject_class->set_property = JSMotion_set_property;
	gobject_class->get_property = JSMotion_get_property;

	gst_element_class_set_details_simple(gstelement_class,
		"JSMotion",
		"JSMotion",
		"Motion Detection",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
}

gboolean InitJSMotion(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (jsmotion_debug, "jsmotion", 0, "JSMotion");
	return gst_element_register (plugin, "jsmotion", GST_RANK_NONE, GST_TYPE_JSMOTION);
}


