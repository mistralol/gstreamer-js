
#include <math.h>

#include <gst/gst.h>
#include <gst/video/video.h>

#include "convolution.h"

GST_DEBUG_CATEGORY_STATIC (convolution_debug);
#define GST_CAT_DEFAULT convolution_debug

/* Filter signals and args */
enum
{
  PROP_SILENT = 1,
  PROP_KERNEL,
  PROP_CUSTOMKERNEL,
  LAST_SIGNAL
};

#define convolution_parent_class parent_class
G_DEFINE_TYPE (Convolution, Convolution, GST_TYPE_ELEMENT);

//Define our enum type for normal kernels.
#define ConvolutionKernelType (ConvolutionKernelTypeGetType())
static GType ConvolutionKernelTypeGetType()
{
	static GType type = 0;
	static const GEnumValue KernelType[] = {
		{Kernel_Identity, "Identity {1}", "Identity"},
		{Kernel_BoxBlur, "BoxBlur {1, 1, 1} {1, 1, 1} {1, 1, 1}", "BoxBlur"},
		{Kernel_GaussianBlur, "GaussianBlur {1, 2, 1} {2, 4, 2} {1, 2, 1}", "GaussianBlur"},
		{Kernel_Sharpen, "Sharpen {0, -1, 0} {-1, 5, -1} {0, -1, 0}", "Sharpen"},
		{Kernel_EdgeDetect, "Edge Detect {0, 1, 0} {1, -4, 1} {0, 1, 0}", "EdgeDetect"},
		{Kernel_Emboss, "Emboss {-2, -1, 0} {-1, 1, 1} {0, 1, 2}", "Emboss"},
		{Kernel_Custom, "Custom Kernel property is used", "Custom"},
		{0, NULL, NULL}
	};

	if (!type)
	{
		type = g_enum_register_static("ConvolutionKernelType", KernelType);
	}
	return type;
}


static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE (
	"sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE("{ RGB, GRAY8 }"))
);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE (
	"src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE("{ RGB, GRAY8 }"))
);

static void ConvolutionKernelParse(Convolution *this, const gchar *kernel)
{
	//Free existing Kernel.
	g_free(this->kernel.data);
	this->kernel.data = 0;
	this->kernel.width = 0;
	this->kernel.height = 0;
	this->kernel.div = 0;
	this->KernelValid = FALSE;

	GST_INFO("Parsing New Kernel %s", kernel);

	//Parse data
	gchar *str = g_strdup(kernel);

	//Check the string
	gchar *tmp = str;
	guint commas = 0;

	while(*tmp != 0)
	{
		if (!((*tmp >= '0' && *tmp <= '9') || *tmp == ',' || *tmp == '.' || *tmp == '-'))
		{
			g_free(str);
			GST_ERROR("Kernel has invalid char %s", tmp);
			return;
		}
		if (*tmp == ',')
		{
			commas++;
		}
		tmp++;
	}

	this->kernel.data = g_malloc(sizeof(float) * (commas + 1));
	guint idx = 0; //Marks current item we are paring

	//Search the string and find each ',' then set it to null
	//Then move base pointer forward.
	//If we don't find the comma then we are on the last item

	gchar *base = str;
	tmp = str;

	for(guint i=0;i<commas + 1;i++)
	{
		while(*tmp != ',' && *tmp != 0)
			tmp++;
		*tmp = 0;
		this->kernel.data[idx] = (gfloat) g_ascii_strtod(base, NULL);
		idx++;
		tmp++;
		base = tmp;
	}

	//Figure our width / height. They should be the same!
	int z = sqrt(commas + 1);
	if (z * z != commas + 1)
	{
		g_free(str);
		GST_ERROR("Kernel has invalid width / height %dx%d", z, z);
		return;
	}

	if (z != 1)
	{
		if ((z % 2) != 1)
		{
			GST_WARNING("Kernel is not odd sized %d", z);
			g_free(str);
			return;
		}
	}

	this->kernel.width = z;
	this->kernel.height = z;

	//Sum The items in the kernel for our divide
	gfloat sum = 0.0f;
	for(guint i = 0; i < this->kernel.width * this->kernel.height; i++)
	{
		sum += this->kernel.data[i];
	}
	this->kernel.div = sum;

	GST_INFO("New Kernel %d %d Divide: %f", this->kernel.width, this->kernel.height, this->kernel.div);

	this->KernelValid = TRUE;
	g_free(str);
}

static void ConvolutionKernelSetup(Convolution *this)
{
	switch(this->KernelType)
	{
		case Kernel_Identity:
			ConvolutionKernelParse(this, "1");
			break;
		case Kernel_BoxBlur:
			ConvolutionKernelParse(this, "1,1,1,1,1,1,1,1,1");
			break;
		case Kernel_GaussianBlur:
			ConvolutionKernelParse(this, "1,2,1,2,4,2,1,2,1");
			break;
		case Kernel_Sharpen:
			ConvolutionKernelParse(this, "0,-1,0,-1,5,-1,0,-1,0");
			break;
		case Kernel_EdgeDetect:
			ConvolutionKernelParse(this, "0,1,0,1,-4,1,0,1,0");
			break;
		case Kernel_Emboss:
			ConvolutionKernelParse(this, "-2,-1,0,-1,1,1,0,1,2");
			break;
		case Kernel_Custom:
			ConvolutionKernelParse(this, this->custom);
			break;
	}
}

static void Convolution_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	Convolution *this = GST_CONVOLUTION (object);

	switch (prop_id) {
		case PROP_KERNEL:
			this->KernelType = g_value_get_enum(value);
			ConvolutionKernelSetup(this);
			break;
		case PROP_CUSTOMKERNEL:
			g_free(this->custom);
			this->custom = g_strdup(g_value_get_string(value));
			ConvolutionKernelSetup(this);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void Convolution_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	Convolution *this = GST_CONVOLUTION (object);

	switch (prop_id) {
		case PROP_KERNEL:
			g_value_set_enum(value, this->KernelType);
			break;
		case PROP_CUSTOMKERNEL:
			g_value_set_string(value, this->custom);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static gboolean Convolution_GRAY8(Convolution *this, GstBuffer *buffer, GstBuffer *output, const ConvolutionKernel *kernel)
{
	GstMapInfo infobuf, infoout;
	
	if (gst_buffer_map(buffer, &infobuf, GST_MAP_READ) == FALSE)
		return FALSE;

	if (gst_buffer_map(output, &infoout, GST_MAP_WRITE) == FALSE)
	{
		gst_buffer_unmap(buffer, &infobuf);
		return FALSE;
	}

	int hkwidth = kernel->width / 2;
	int hkheight = kernel->height / 2;

	for(int y = hkheight; y < this->height - hkheight; y++)
	{
		for(int x = hkwidth; x < this->width - hkwidth; x++)
		{
			float sum = 0.0f;

			for(int i = 0; i < kernel->height; i++)
			{
				for(int j=0; j < kernel->width;j++)
				{
					int py = y - hkheight + i; //Figure out pixel locations
					int px = x - hkwidth + j;
					
					sum += infobuf.data[(py * this->width) + (px)] * kernel->data[(i * kernel->width) + j];
				}
			}
			if (kernel->div > 0)
			{
				sum /= kernel->div;
			}

			infoout.data[(y * this->width) + x] = (guint8) fabs(sum);
		}
	}
	
	gst_buffer_unmap(buffer, &infobuf);
	gst_buffer_unmap(output, &infoout);
	
	return TRUE;
}

static gboolean Convolution_RGB(Convolution *this, GstBuffer *buffer, GstBuffer *output, const ConvolutionKernel *kernel)
{
	GstMapInfo infobuf, infoout;
	
	if (gst_buffer_map(buffer, &infobuf, GST_MAP_READ) == FALSE)
		return FALSE;

	if (gst_buffer_map(output, &infoout, GST_MAP_WRITE) == FALSE)
	{
		gst_buffer_unmap(buffer, &infobuf);
		return FALSE;
	}

	int hkwidth = kernel->width / 2;
	int hkheight = kernel->height / 2;

	for(int y = hkheight; y < this->height - hkheight; y++)
	{
		for(int x = hkwidth; x < this->width - hkwidth; x++)
		{
			float sum1 = 0.0f, sum2 = 0.0f, sum3 = 0.0f;

			for(int i = 0; i < kernel->height; i++)
			{
				for(int j=0; j < kernel->width;j++)
				{
					int py = y - hkheight + i; //Figure out pixel locations
					int px = x - hkwidth + j;
					
					sum1 += infobuf.data[(py * this->width * 3) + (px * 3)] * kernel->data[(i * kernel->width) + j];
					sum2 += infobuf.data[(py * this->width * 3) + (px * 3) + 1] * kernel->data[(i * kernel->width) + j];
					sum3 += infobuf.data[(py * this->width * 3) + (px * 3) + 2] * kernel->data[(i * kernel->width) + j];
				}
			}
			if (kernel->div > 0)
			{
				sum1 /= kernel->div;
				sum2 /= kernel->div;
				sum3 /= kernel->div;
			}

			infoout.data[(y * this->width * 3) + (x * 3)] = (guint8) fabs(sum1);
			infoout.data[(y * this->width * 3) + (x * 3) + 1] = (guint8) fabs(sum2);
			infoout.data[(y * this->width * 3) + (x * 3) + 2] = (guint8) fabs(sum3);
		}
	}
	
	gst_buffer_unmap(buffer, &infobuf);
	gst_buffer_unmap(output, &infoout);
	
	return TRUE;
}

static GstFlowReturn Convolution_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	Convolution *this = GST_CONVOLUTION(parent);

	if (this->KernelValid == FALSE)
	{
		GST_ERROR("Invalid convolution kernel");
		gst_buffer_unref(buf);
		return GST_FLOW_ERROR;
	}

	GstBuffer *output = gst_buffer_copy(buf);
	if (output == NULL)
	{
		gst_buffer_unref(buf);
		return GST_FLOW_ERROR;
	}

	switch (this->Mode)
	{
		case Unknown:
			gst_buffer_unref(output);
			gst_buffer_unref(buf);
			return GST_FLOW_ERROR;
			break;
		case RGB:
			if (Convolution_RGB(this, buf, output, &this->kernel) == FALSE)
			{
				gst_buffer_unref(buf);
				gst_buffer_unref(output);
				return GST_FLOW_ERROR;
			}
			break;
		case GRAY8:
			if (Convolution_GRAY8(this, buf, output, &this->kernel) == FALSE)
			{
				gst_buffer_unref(buf);
				gst_buffer_unref(output);
				return GST_FLOW_ERROR;
			}
			break;
		default:
			gst_buffer_unref(output);
			gst_buffer_unref(buf);
			return GST_FLOW_ERROR;
	}

	gst_buffer_unref(buf);
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
			
			switch(info.finfo->format)
			{
				case GST_VIDEO_FORMAT_RGB:
					this->Mode = RGB;
					break;
				case GST_VIDEO_FORMAT_GRAY8:
					this->Mode = GRAY8;
					break;
				default:
					this->Mode = Unknown;
					break;
			}
			
			GST_INFO("New Width: %d Height: %d", info.width, info.height);
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

static void Convolution_Finalize(GObject *object)
{
	Convolution *this = GST_CONVOLUTION(object);
	
	g_free(this->kernel.data);
	g_free(this->custom);
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

	this->Mode = Unknown;
	this->KernelType = Kernel_Identity;
	this->KernelValid = FALSE; //Will be set to TRUE when we first setup

	//Basic simple kernel	
	this->kernel.data = NULL;
	this->kernel.div = 0;
	this->kernel.width = 0;
	this->kernel.height = 0;
	this->custom = g_strdup("1");

	ConvolutionKernelSetup(this);

	//Width / Height is buffer size setup when we get caps
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

	g_object_class_install_property(gobject_class, PROP_CUSTOMKERNEL,
		g_param_spec_string("custom_kernel", "Custom Kernel", "Setup a custom convolution kernel (matrix) with comma seperated values it must be square and have odd width / height", NULL, G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class, PROP_KERNEL,
		g_param_spec_enum("kernel", "Kernel Type", "Set the convolution kernel type", ConvolutionKernelType, Kernel_Identity, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

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


