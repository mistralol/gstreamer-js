
#include <glib.h>
#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>

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
	PROP_TIMEOUT,
	PROP_ALLOW_CAPS_CHANGE,
	PROP_DROPPED
};

/* Define src and src pad capabilities. */
static GstStaticPadTemplate src = GST_STATIC_PAD_TEMPLATE(
	"src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("ANY")
);

#define InternalSrc_parent_class parent_class
G_DEFINE_TYPE (InternalSrc, InternalSrc, GST_TYPE_BASE_SRC);

static void InternalSrc_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	InternalSrc *data = GST_INTERNALSRC (object);

	switch (prop_id)
	{
		case PROP_STREAMNAME:
			data->Name = g_value_dup_string(value);
			break;
		case PROP_MAXQUEUESIZE:
			data->Options.MaxQueue = g_value_get_int(value);
			break;
		case PROP_TIMEOUT:
			data->Options.Timeout = g_value_get_uint64(value);
			break;
		case PROP_ALLOW_CAPS_CHANGE:
			data->AllowCapsChange = g_value_get_boolean(value);
			break;

		case PROP_DROPPED: //Read Only...
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
			g_value_set_int(value, data->Options.MaxQueue);
			break;
		case PROP_TIMEOUT:
			g_value_set_uint64(value, data->Options.Timeout);
			break;
		case PROP_ALLOW_CAPS_CHANGE:
			g_value_set_boolean(value, data->AllowCapsChange);
			break;
		case PROP_DROPPED:
			if (data->Reader)
			{
				g_value_set_int(value, data->Reader->Dropped);
			}
			else
			{
				g_value_set_int(value, 0);
			}
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

/* ask the subclass to create a buffer, the default implementation
 * uses alloc and fill */
GstFlowReturn InternalSrcCreate(GstBaseSrc *base, guint64 offset, guint size, GstBuffer **buf)
{
	InternalSrc *data = GST_INTERNALSRC(base);

	if (data->Reader == NULL)
	{
		GST_DEBUG("Attempting to connect to: %s", data->Name);
		data->Reader = InternalReaderAttach(data->Name, &data->Options);
		if (data->Reader == NULL)
		{
			GST_ERROR("Failed to connect to stream '%s'", data->Name);
			return GST_FLOW_ERROR;
		}
		GST_DEBUG("Connected to: %s", data->Name);
	}

	GstSample *sample = NULL;
	GST_DEBUG("Reading buffer from %s", data->Name);
	InternalReaderRead(data->Reader, &sample);
	if (sample == NULL)
	{
		GST_ERROR("Failed to read buffer from streamname '%s'", data->Name);
		return GST_FLOW_ERROR;
	}
	GST_DEBUG("Read buffer from %s", data->Name);

	GstBuffer *tmp = gst_sample_get_buffer(sample);
	GstCaps *caps = gst_sample_get_caps(sample);
	*buf = gst_buffer_copy(tmp); //We need to take a copy of the buffer
	if (!buf)
	{
		gst_sample_unref(sample);
		GST_ERROR("Failed to make buffer writeable");
		return GST_FLOW_ERROR;
	}
	//gst_buffer_ref(*buf); 

	if (caps == NULL)
	{
		gst_sample_unref(sample);
		gst_buffer_unref(*buf);
		return GST_FLOW_ERROR;
	}

	//Compare and send new caps if required
	GstCaps *oldcaps = gst_pad_get_current_caps(GST_BASE_SRC_PAD(base));
	if (oldcaps == NULL)
	{
		//No caps then we set them
		if (gst_base_src_set_caps(base, caps) == FALSE)
		{
			gst_sample_unref(sample);
			gst_buffer_unref(*buf);
			return GST_FLOW_ERROR;
		}
	}
	else
	{
		//Find out if caps have changed
		if (gst_caps_is_equal(oldcaps, caps) == FALSE)
		{
			if (data->AllowCapsChange == FALSE)
			{
				gst_caps_unref(oldcaps);
				gst_sample_unref(sample);
				gst_buffer_unref(*buf);
				return GST_FLOW_ERROR;
			}

			if (gst_base_src_set_caps(base, caps) == FALSE)
			{
				gst_caps_unref(oldcaps);
				gst_sample_unref(sample);
				gst_buffer_unref(*buf);
				return GST_FLOW_ERROR;
			}
			
			gst_caps_unref(oldcaps);
		}
	}

	//Put timestampts to the time of the first buffer we saw
	if (data->first_dts == 0  && GST_BUFFER_DTS(*buf) != GST_CLOCK_TIME_NONE)
	{
		data->first_dts = GST_BUFFER_DTS(*buf);
	}
	GST_BUFFER_DTS(*buf) -= data->first_dts;
	
	//Put timestampts to the time of the first buffer we saw
	if (data->first_pts == 0  && GST_BUFFER_PTS(*buf) != GST_CLOCK_TIME_NONE)
	{
		data->first_pts = GST_BUFFER_PTS(*buf);
	}
	GST_BUFFER_PTS(*buf) -= data->first_pts;
	
	gst_sample_unref(sample);
	return GST_FLOW_OK;
}

static gboolean InternalSrcStart(GstBaseSrc *basesrc)
{
	GST_INFO("InternalSrcStart");
	InternalSrc *data = GST_INTERNALSRC(basesrc);
	
	if (data->Name == NULL || g_strcmp0(data->Name, "") == 0)
	{
		GST_ERROR("streamname property is not set");
		return FALSE;
	}
	
	return TRUE;
}

static gboolean InternalSrcStop(GstBaseSrc *basesrc)
{
	GST_DEBUG("InternalSrcStop");
	InternalSrc *data = GST_INTERNALSRC(basesrc);
	
	if (data->Reader)
	{
		InternalReaderFree(data->Reader);
		data->Reader = NULL;
	}
	
	return TRUE;
}

static gboolean InternalSrcQuery(GstBaseSrc *src, GstQuery *query)
{
	GST_DEBUG("InternalSrcQuery");
	gboolean ret = FALSE;
	switch (GST_QUERY_TYPE (query)) {
		case GST_QUERY_SCHEDULING:
			GST_INFO("Set scheduling mode");
			gst_query_set_scheduling (query, GST_SCHEDULING_FLAG_SEQUENTIAL, 1, -1, 0);
			gst_query_add_scheduling_mode (query, GST_PAD_MODE_PUSH);
			ret = TRUE;
			break;
		default:
			ret =  GST_BASE_SRC_CLASS (parent_class)->query (src, query);
			break;
	}
	return ret;
}

static void InternalSrcGetTimes(GstBaseSrc *basesrc, GstBuffer *buffer, GstClockTime *start, GstClockTime *end)
{
	GST_DEBUG("InternalSrcGetTimes");
	*start = -1;
	*end = -1;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void InternalSrc_init (InternalSrc *data)
{
	GstBaseSrc *basesrc = &data->parent;

	data->Name = NULL;
	data->Reader = NULL;
	data->Options.MaxQueue = 15;
	data->Options.Timeout = 5000;
	data->AllowCapsChange = TRUE;
	data->first_dts = 0;
	data->first_pts = 0;
	
	gst_base_src_set_live(basesrc, TRUE);
	gst_base_src_set_format(basesrc, GST_FORMAT_TIME);
}

static void InternalSrc_Finalize(GObject *object)
{
	InternalSrc *data = GST_INTERNALSRC(object);

	if (data->Name)
		g_free(data->Name);

}

static void InternalSrc_class_init (InternalSrcClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *element_class;
	GstBaseSrcClass *basesrc_class;

	gobject_class = G_OBJECT_CLASS(klass);
	element_class = GST_ELEMENT_CLASS(klass);
	basesrc_class = GST_BASE_SRC_CLASS(klass);

	gobject_class->finalize = InternalSrc_Finalize;
	gobject_class->set_property = InternalSrc_set_property;
	gobject_class->get_property = InternalSrc_get_property;

	basesrc_class->create = InternalSrcCreate;
	basesrc_class->start = InternalSrcStart;
	basesrc_class->stop = InternalSrcStop;
	basesrc_class->query = InternalSrcQuery;
	basesrc_class->get_times = InternalSrcGetTimes;


	g_object_class_install_property (gobject_class, PROP_STREAMNAME,
		g_param_spec_string ("streamname", "streamname", "The stream name for the source element to connect to", "", G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_MAXQUEUESIZE,
		g_param_spec_int ("maxqueue", "maxqueue", "Max backlog in the queue for this reader", 1, G_MAXINT, 15, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_TIMEOUT,
		g_param_spec_uint64 ("timeout", "timeout", "Timeout for reading in ms", 0, G_MAXUINT64, 5000, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_ALLOW_CAPS_CHANGE,
		g_param_spec_boolean ("allowcapschange", "allowcaoschange", "Allow element to change caps", TRUE, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_DROPPED,
		g_param_spec_int ("dropped", "dropped", "Number of frames dropped in the queue", 0, G_MAXINT, 0, G_PARAM_READABLE));

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

