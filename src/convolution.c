
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <gst/gst.h>
#include <gst/video/video.h>

#include "convolution.h"

GST_DEBUG_CATEGORY_STATIC (convolution_debug);
#define GST_CAT_DEFAULT convolution_debug

/* Filter signals and args */
enum
{
  PROP_SILENT = 1,
  LAST_SIGNAL
};

#define convolution_parent_class parent_class
G_DEFINE_TYPE (Convolution, Convolution, GST_TYPE_ELEMENT);


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

static void Convolution_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
//	Convolution *this = GST_CONVOLUTION (object);

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void Convolution_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
//	Convolution *this = GST_CONVOLUTION (object);

	switch (prop_id) {
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

struct CKernel
{
	float *data;
	float div;
	int width;
	int height;
};

static gboolean Convolution_RGB(Convolution *this, GstBuffer *buffer, const struct CKernel *kernel)
{
	GstMapInfo info;
	
	if (gst_buffer_map(buffer, &info, GST_MAP_WRITE) == FALSE)
		return FALSE;

	for(int y = kernel->height; y < this->height - kernel->height; y++)
	{
		for(int x = kernel->width; x < this->width - kernel->width; x++)
		{
			float sum1 = 0.0f, sum2 = 0.0f, sum3 = 0.0f;

			for(int i = 0; i < kernel->height; i++)
			{
				for(int j=0;j<kernel->width;j++)
				{
					int py = y;// - kernel->height + i; //Figure out pixel locations
					int px = x;// - kernel->width + j;
					
					sum1 += info.data[(py * this->width * 3) + (px * 3)] * kernel->data[(i * kernel->width) + j];
					sum2 += info.data[(py * this->width * 3) + (px * 3) + 1] * kernel->data[(i * kernel->width) + j];
					sum3 += info.data[(py * this->width * 3) + (px * 3) + 2] * kernel->data[(i * kernel->width) + j];					
				}
			}
			sum1 /= kernel->div;
			sum2 /= kernel->div;
			sum3 /= kernel->div;
			info.data[(y * this->width * 3) + (x * 3)] = ABS((int) sum1);
			info.data[(y * this->width * 3) + (x * 3) + 1] = ABS((int) sum2);
			info.data[(y * this->width * 3) + (x * 3) + 2] = ABS((int) sum3);
		}
	}
	
	gst_buffer_unmap(buffer, &info);
	
	return TRUE;
}

static GstFlowReturn Convolution_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	Convolution *this = GST_CONVOLUTION(parent);

	GstBuffer *output = gst_buffer_make_writable(buf);
	if (output == NULL)
		return GST_FLOW_ERROR;

	//float data[] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};
	float data[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
	//float data[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	//float data[] = {0, 0, 0, -1, 1, 0, 0, 0, 0};
	struct CKernel kernel;
	kernel.data = data;
	kernel.div = 9;
	kernel.width = 3;
	kernel.height = 3;

	if (Convolution_RGB(this, output, &kernel) == FALSE)
	{
		gst_buffer_unref(output);
		return GST_FLOW_ERROR;
	}

	return gst_pad_push (this->srcpad, output);
}

static gboolean Convolution_event (GstPad *pad, GstObject *parent, GstEvent  *event)
{
	Convolution* this = GST_CONVOLUTION(gst_pad_get_parent (pad));
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

static void Convolution_Finalize(GObject *object)
{
//	Convolution *this = GST_CONVOLUTION(object);
	
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void Convolution_init (Convolution *this)
{
	this->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	this->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
	
	this->width = 0;
	this->height = 0;
	
	GST_PAD_SET_PROXY_CAPS(this->sinkpad);
	GST_PAD_SET_PROXY_CAPS(this->srcpad);

	gst_pad_set_event_function (this->sinkpad, Convolution_event);
	gst_pad_set_chain_function (this->sinkpad, Convolution_chain);

	gst_element_add_pad (GST_ELEMENT (this), this->sinkpad);
	gst_element_add_pad (GST_ELEMENT (this), this->srcpad);
}


static void Convolution_class_init (ConvolutionClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->finalize = Convolution_Finalize;
	gobject_class->set_property = Convolution_set_property;
	gobject_class->get_property = Convolution_get_property;

	gst_element_class_set_details_simple(gstelement_class,
		"Convolution",
		"Convolution",
		"Perform a 2d convolution on the image",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
}

gboolean InitConvolution(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (convolution_debug, "convolution", 0, "Convolution");
	return gst_element_register (plugin, "convolution", GST_RANK_NONE, GST_TYPE_CONVOLUTION);
}


