
#include <glib.h>
#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

#include "internalcommon.h"
#include "internalsrc.h"

GST_DEBUG_CATEGORY_STATIC (internalsrc_debug);
#define GST_CAT_DEFAULT internalsrc_debug

/* Filter signals and args */
enum
{
	LAST_SIGNAL
};

enum
{
	PROP_STREAMNAME = 1,
	PROP_MAXQUEUESIZE,
	PROP_TIMEOUT
};

/* Define src and src pad capabilities. */
static GstStaticPadTemplate src = GST_STATIC_PAD_TEMPLATE(
	"src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("ANY")
);

#define InternalSrc_parent_class parent_class
G_DEFINE_TYPE (InternalSrc, InternalSrc, GST_TYPE_PUSH_SRC);

static void InternalSrc_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	InternalSrc *data = GST_INTERNALSRC (object);

	switch (prop_id)
	{
		case PROP_STREAMNAME:
			data->Name = g_value_dup_string(value);
			break;
		case PROP_MAXQUEUESIZE:
			data->MaxQueue = g_value_get_int(value);
			break;
		case PROP_TIMEOUT:
			data->Timeout = g_value_get_uint64(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void InternalSrc_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	InternalSrc *data = GST_INTERNALSRC (object);

	switch (prop_id)
	{
		case PROP_STREAMNAME:
			g_value_set_string(value, data->Name);
			break;
		case PROP_MAXQUEUESIZE:
			g_value_set_int(value, data->MaxQueue);
			break;
		case PROP_TIMEOUT:
			g_value_set_uint64(value, data->Timeout);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

/* ask the subclass to create a buffer, the default implementation
 * uses alloc and fill */
GstFlowReturn InternalSrcCreate(GstPushSrc *src, GstBuffer **buf)
{
	InternalSrc *data = GST_INTERNALSRC(src);
	GstBaseSrc *base = &src->parent;

	if (data->Reader == NULL)
	{
		data->Reader = InternalReaderAttach(data->Name);
		if (data->Reader == NULL)
		{
			return GST_FLOW_ERROR;
		}
	}

	GstSample *sample = NULL;
	InternalReaderRead(data->Reader, &sample);
	if (sample == NULL)
	{
		return GST_FLOW_ERROR;
	}

	GstBuffer *tmp = gst_sample_get_buffer(sample); //unref of gstbuffer is not required
	GstCaps *caps = gst_sample_get_caps(sample);
	*buf = tmp;
	gst_buffer_ref(tmp);

	if (caps == NULL)
	{
		gst_sample_unref(sample);
		return GST_FLOW_ERROR;
	}

	//Compare and send new caps if required
	GstCaps *ccaps = gst_pad_get_current_caps(GST_BASE_SRC_PAD(base));
	if (ccaps == NULL)
	{

		if (gst_base_src_set_caps(base, caps) == FALSE)
		{
			gst_sample_unref(sample);
			return GST_FLOW_ERROR;
		}
		GstEvent *event = gst_event_new_caps(caps);
		if (gst_pad_push_event(GST_BASE_SRC_PAD(base), event) == FALSE)
		{
			gst_sample_unref(sample);
			gst_event_unref(event);
			return GST_FLOW_ERROR;
		}
	}
	else
	{
		if (gst_caps_is_equal(ccaps, caps) == FALSE)
		{
			if (gst_base_src_set_caps(base, caps) == FALSE)
			{
				gst_caps_unref(ccaps);
				gst_sample_unref(sample);
				return GST_FLOW_ERROR;
			}
			GstEvent *event = gst_event_new_caps(caps);
			if (gst_pad_push_event(GST_BASE_SRC_PAD(base), event) == FALSE)
			{
				gst_event_unref(event);
				gst_caps_unref(ccaps);
				gst_sample_unref(sample);
				return GST_FLOW_ERROR;
			}
			gst_caps_unref(ccaps);
		}
	}

	gst_sample_unref(sample);
	return GST_FLOW_OK;
}

static GstStateChangeReturn InternalSrc_change_state(GstElement *element, GstStateChange transition)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	InternalSrc *data = GST_INTERNALSRC(element);

	if (GST_IS_INTERNALSRC(element) == FALSE)
		return GST_FLOW_ERROR;

	if (data->Name == NULL || g_strcmp0(data->Name, "") == 0)
	{
		GST_ERROR("streamname property is not set");
		return GST_FLOW_ERROR;
	}

	switch(transition)
	{
		case GST_STATE_CHANGE_NULL_TO_READY:
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
			if (data->Reader != NULL)
			{
				InternalReaderFree(data->Reader);
				data->Reader = NULL;
			}
			break;
		default:
			break;
	}

	return ret;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void InternalSrc_init (InternalSrc *data)
{
	GstPushSrc *src = &data->parent;
	GstBaseSrc *base = &src->parent;

	data->Name = NULL;
	data->Reader = NULL;
	data->MaxQueue = 15;
	data->Timeout = 5000;

	gst_base_src_set_live(base, FALSE);
}

static void InternalSrc_Finalize(GObject *object)
{
	InternalSrc *data = GST_INTERNALSRC(object);

	if (data->Name)
	{
		g_free(data->Name);
	}
}

static void InternalSrc_class_init (InternalSrcClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *element_class;
	GstPushSrcClass *pushsrc_class;
	//GstBaseSrcClass *basesrc_class;

	gobject_class = G_OBJECT_CLASS(klass);
	element_class = GST_ELEMENT_CLASS(klass);
	pushsrc_class = GST_PUSH_SRC_CLASS(klass);
	//basesrc_class = GST_BASE_SRC_CLASS(klass);

	gobject_class->finalize = InternalSrc_Finalize;
	gobject_class->set_property = InternalSrc_set_property;
	gobject_class->get_property = InternalSrc_get_property;

	element_class->change_state = InternalSrc_change_state;

	pushsrc_class->create = InternalSrcCreate;

	g_object_class_install_property (gobject_class, PROP_STREAMNAME,
		g_param_spec_string ("streamname", "streamname", "The stream name for the source element to connect to", "", G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_MAXQUEUESIZE,
		g_param_spec_int ("maxqueue", "maxqueue", "Max backlog in the queue for this reader", 1, G_MAXINT, 15, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_TIMEOUT,
		g_param_spec_uint64 ("timeout", "timeout", "Timeout for reading in ms", 0, G_MAXUINT64, 5000, G_PARAM_READWRITE));

	gst_element_class_set_details_simple(element_class,
		"InternalSrc",
		"InternalSrc",
		"Provides a Src element for getting data from internalsink",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&src));
}

gboolean InitInternalSrc(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (internalsrc_debug, "internalsrc", 0, "InternalSrc");
	return gst_element_register (plugin, "internalsrc", GST_RANK_NONE, GST_TYPE_INTERNALSRC);
}

