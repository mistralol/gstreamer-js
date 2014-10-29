
#include <glib.h>
#include <gst/gst.h>

#include "internalcommon.h"
#include "internalsink.h"

GST_DEBUG_CATEGORY_STATIC (internalsink_debug);
#define GST_CAT_DEFAULT internalsink_debug

/* Filter signals and args */
enum
{
	LAST_SIGNAL
};

enum
{
	PROP_STREAMNAME = 1
};

#define InternalSink_parent_class parent_class
G_DEFINE_TYPE (InternalSink, InternalSink, GST_TYPE_ELEMENT);


GList *WriterList = NULL;
G_LOCK_DEFINE(WriterList);
volatile int WriterCount = 0;


static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE (
	"sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
);

static struct InternalWriter *WriterAdd(InternalSink *data)
{
	struct InternalWriter *tmp = NULL;
	G_LOCK(WriterList);
	if (WriterCount == 0)
	{
		WriterList = g_list_alloc();
	}

	GList *it = WriterList;
	while(it && it->data)
	{
		struct InternalWriter *tmp = it->data;
		if (g_strcmp0(data->Name, tmp->Name) == 0)
		{
			G_UNLOCK(WriterList);
			return NULL; //Fail here because we have an item by that name already added
		}
		it = it->next;
	}

	tmp = g_new(struct InternalWriter, 1);
	if (!tmp)
	{
		G_UNLOCK(WriterList);
		return NULL;
	}
	
	//Init Struct
	tmp->Name = g_strdup(data->Name);
	tmp->Readers = g_list_alloc();
	g_mutex_init(&tmp->lock);
	tmp->count = 1; //We count as 1

	//Append to List
	WriterList = g_list_append(WriterList, tmp);
	WriterCount++;
	G_UNLOCK(WriterList);
	return tmp;
};

static void WriterRemove(InternalSink *data)
{
	struct InternalWriter *tmp = NULL;
	gboolean found = FALSE;
	G_LOCK(WriterList);

	GList *it = WriterList;
	while(it && it->data)
	{
		tmp = it->data;
		if (g_strcmp0(data->Name, tmp->Name) == 0)
		{
			WriterList = g_list_remove(WriterList, tmp);
			WriterCount--;

			if (WriterCount == 0)
			{
				g_list_free(WriterList);
				WriterList = NULL;
			}

			found = TRUE;
			break;
		}
		it = it->next;
	}

	if (found == FALSE)
	{
		g_print("Fail to Find %s during Remove\n", data->Name);
		GST_ERROR("Failed to remove InternalSink from List because it does not currently exist. This is a bug");
		G_UNLOCK(WriterList);
		return;
	}

	//Note: nothing should be able to get a reference to increament tmp->count while we hold WriterList lock
	tmp->count--;
	if (tmp->count == 0)
	{
		g_free(tmp->Name);
		g_list_free(tmp->Readers);
		g_mutex_clear(&tmp->lock);
		g_free(tmp);
	}

	G_UNLOCK(WriterList);
};

static gboolean WriterWrite(struct InternalWriter *Writer, GstBuffer *buf)
{
	g_mutex_lock(&Writer->lock);
	GList *it = Writer->Readers;
	while(it && it->data)
	{
		struct InternalReader *Reader = it->data;
		if (g_async_queue_length(Reader->Queue) < 30)	//Assume overflow FIXME: Dead with this better
			g_async_queue_push(Reader->Queue, buf);
		it = it->next;
	}
	g_mutex_unlock(&Writer->lock);

	return FALSE;
};


static void InternalSink_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	InternalSink *data = GST_INTERNALSINK (object);

	switch (prop_id)
	{
		case PROP_STREAMNAME:
			data->Name = g_value_dup_string(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void InternalSink_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	InternalSink *data = GST_INTERNALSINK (object);

	switch (prop_id)
	{
		case PROP_STREAMNAME:
			g_value_set_string(value, data->Name);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GstFlowReturn InternalSink_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{
	InternalSink *data = GST_INTERNALSINK(parent);

	if (data->Writer == NULL)
	{
		GST_ERROR("data->Writer is NULL");
		gst_buffer_unref(buf);
		return GST_FLOW_ERROR;
	}

	WriterWrite(data->Writer, buf);

	gst_buffer_unref(buf);
	return GST_FLOW_OK;
}

static GstStateChangeReturn InternalSink_change_state(GstElement *element, GstStateChange transition)
{
	GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
	InternalSink *data = GST_INTERNALSINK(element);

	if (GST_IS_INTERNALSINK(element) == FALSE)
		return GST_FLOW_ERROR;

	if (data->Name == NULL || g_strcmp0(data->Name, "") == 0)
	{
		GST_ERROR("streamname property is not set");
		return GST_FLOW_ERROR;
	}

	switch(transition)
	{
		case GST_STATE_CHANGE_NULL_TO_READY:
			{
				struct InternalWriter *tmp = WriterAdd(data);
				if (tmp == NULL)
				{
					GST_ERROR("Failed To Add Writer to InternalWriter List");
					return GST_STATE_CHANGE_FAILURE;
				}
				data->Writer = tmp;
			}
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
			WriterRemove(data);
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
static void InternalSink_init (InternalSink *data)
{
	data->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");

	gst_pad_set_chain_function (data->sinkpad, InternalSink_chain);

	gst_element_add_pad (GST_ELEMENT (data), data->sinkpad);

	data->Writer = NULL;
}

static void InternalSink_Finalize(GObject *object)
{
	InternalSink *data = GST_INTERNALSINK(object);

	if (data->Name)
	{
		g_free(data->Name);
	}

}

static void InternalSink_class_init (InternalSinkClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;


	gobject_class->finalize = InternalSink_Finalize;
	gobject_class->set_property = InternalSink_set_property;
	gobject_class->get_property = InternalSink_get_property;

	gstelement_class->change_state = InternalSink_change_state;

	g_object_class_install_property (gobject_class, PROP_STREAMNAME,
		g_param_spec_string ("streamname", "streamname", "The stream name for the source element to connect to", "", G_PARAM_READWRITE));


	gst_element_class_set_details_simple(gstelement_class,
		"InternalSink",
		"InternalSink",
		"Provides a Sink element for providing data to InternalSrc",
		"James Stevenson <james@stev.org>"
	);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
}

gboolean InitInternalSink(GstPlugin *plugin)
{
	GST_DEBUG_CATEGORY_INIT (internalsink_debug, "internalsink", 0, "InternalSink");
	return gst_element_register (plugin, "internalsink", GST_RANK_NONE, GST_TYPE_INTERNALSINK);
}



