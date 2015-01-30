
#include <glib.h>
#include <gst/gst.h>

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

#define InternalSrc_parent_class parent_class
G_DEFINE_TYPE (InternalSrc, InternalSrc, GST_TYPE_ELEMENT);


static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE (
	"src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
);

static void InternalSrc_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	InternalSrc *data = GST_INTERNALSRC (object);

	switch (prop_id)
	{
		case PROP_STREAMNAME:
			data->Name = g_value_dup_string(value);
			break;
		case PROP_MAXQUEUESIZE:
			data->MaxQueue = g_value_get_uint(value);
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
			g_value_set_uint(value, data->MaxQueue);
			break;
		case PROP_TIMEOUT:
			g_value_set_uint64(value, data->Timeout);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void InternalSrc_Task(gpointer user_data)
{
	InternalSrc *data = (InternalSrc *) user_data;
	GstBuffer *buf = NULL;

	g_print("Running\n");
	InternalReaderRead(data->Reader, &buf);
	if (buf == NULL)
	{
		g_print("WTF? No Buffer?");
		//FIXME: Send EOS
		return;
	}

	//FIXME: Compare and send caps
	//FIXME: Send buffer
	//FIXME: Free buffer
	gst_buffer_unref(buf);
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
		case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
			data->Reader = InternalReaderAttach(data->Name);
			if (!data->Reader)
				return GST_STATE_CHANGE_FAILURE;
			gst_task_start(data->task);
			break;
		default:
			break;
	}

	ret = GST_ELEMENT_CLASS(parent_class)->change_state (element, transition);
	if (ret == GST_STATE_CHANGE_FAILURE)
		return ret;

	switch(transition)
	{
		case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
			gst_task_stop(data->task);
			gst_task_join(data->task);
			InternalReaderFree(data->Reader);
			data->Reader = NULL;
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
	data->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

	gst_element_add_pad (GST_ELEMENT (data), data->srcpad);

	data->Name = NULL;
	data->Reader = NULL;
	data->MaxQueue = 15;
	data->Timeout = 5000;

	data->task = gst_task_new(InternalSrc_Task, data, NULL);
	
}

static void InternalSrc_Finalize(GObject *object)
{
	InternalSrc *data = GST_INTERNALSRC(object);

	if (data->Name)
	{
		g_free(data->Name);
	}
	gst_object_unref(data->task);
}

static void InternalSrc_class_init (InternalSrcClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;


	gobject_class->finalize = InternalSrc_Finalize;
	gobject_class->set_property = InternalSrc_set_property;
	gobject_class->get_property = InternalSrc_get_property;

	gstelement_class->change_state = InternalSrc_change_state;

	g_object_class_install_property (gobject_class, PROP_STREAMNAME,
		g_param_spec_string ("streamname", "streamname", "The stream name for the source element to connect to", "", G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_MAXQUEUESIZE,
		g_param_spec_uint ("maxqueue", "maxqueue", "Max backlog in the queue for this reader", 1, G_MAXUINT, 15, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_TIMEOUT,
		g_param_spec_uint64 ("timeout", "timeout", "Timeout for reading in ms", 0, G_MAXUINT64, 5000, G_PARAM_READWRITE));

	gst_element_class_set_details_simple(gstelement_class,
		"InternalSrc",
		"InternalSrc",
		"Provides a Src element for getting data from internalsink",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
}

gboolean InitInternalSrc(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (internalsrc_debug, "internalsrc", 0, "InternalSrc");
	return gst_element_register (plugin, "internalsrc", GST_RANK_NONE, GST_TYPE_INTERNALSRC);
}



