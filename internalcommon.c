
#include <glib.h>
#include <gst/gst.h>

#include "internalcommon.h"

GList *WriterList = NULL;
G_LOCK_DEFINE(WriterLock);


struct InternalWriter *InternalWriterAttach(const gchar *name, gboolean alloc)
{
	struct InternalWriter *Writer = NULL;
	G_LOCK(WriterLock);
	if (WriterList == NULL)
	{
		WriterList = g_list_alloc();
		if (!WriterList)
		{
			G_UNLOCK(WriterLock);
			return NULL;
		}
	}

	//Find an Item
	GList *it = WriterList;
	while(it && it->data)
	{
		struct InternalWriter *tmp = it->data;
		if (g_strcmp0(name, tmp->Name) == 0)
		{
			g_mutex_lock(&tmp->lock);
			tmp->count++;
			g_mutex_unlock(&tmp->lock);
			G_UNLOCK(WriterLock);
			return tmp; //Success!
		}
		it = it->next;
	}

	if (alloc == FALSE)
	{
		G_UNLOCK(WriterLock);
		return NULL;
	}
	

	Writer = g_new(struct InternalWriter, 1);
	if (!Writer)
	{
		G_UNLOCK(WriterLock);
		return NULL; /* Failure */
	}

	//Init Struct
	Writer->Name = g_strdup(name);
	Writer->Readers = g_list_alloc();
	g_mutex_init(&Writer->lock);
	Writer->count = 1; //We count as 1

	WriterList = g_list_append(WriterList, Writer);

	G_UNLOCK(WriterLock);
	return Writer; //Sucess!
}

void InternalWriterWrite(struct InternalWriter *Writer, GstBuffer *buf)
{
	g_mutex_lock(&Writer->lock);
	GList *it = Writer->Readers;
	while(it && it->data)
	{
		struct InternalReader *Reader = it->data;
		if (g_async_queue_length(Reader->Queue) < Reader->MaxQueue)	//Assume overflow FIXME: Deal with this better
		{
			gst_buffer_ref(buf);
			g_async_queue_push(Reader->Queue, buf);
		}
		else
		{
			Reader->Dropped++;
		}
		it = it->next;
	}
	g_mutex_unlock(&Writer->lock);
}

void InternalWriterFree(struct InternalWriter *Writer)
{
	G_LOCK(WriterLock);
	g_mutex_lock(&Writer->lock);
	Writer->count--;
	if (Writer->count == 0)
	{
		WriterList = g_list_remove(WriterList, Writer);
		g_free(Writer->Name);
		g_list_free(Writer->Readers);
		g_mutex_unlock(&Writer->lock);
		g_mutex_clear(&Writer->lock);
		g_free(Writer);

		if (g_list_length(WriterList) == 0)
		{
			g_list_free(WriterList);
			WriterList = NULL;
		}
	}
	
	G_UNLOCK(WriterLock);
}

struct InternalReader *InternalReaderAttach(const gchar *Name)
{
	struct InternalWriter *Writer = InternalWriterAttach(Name, FALSE);

	if (Writer == NULL)
	{
		return NULL;
	}

	struct InternalReader *Reader = g_new(struct InternalReader, 1);
	if (Reader == NULL)
	{
		InternalWriterFree(Writer);
		return NULL;
	}


	Reader->Writer = Writer;
	Reader->Queue = g_async_queue_new();
	Reader->MaxQueue = 15; //FIXME: Get Limits from Element
	Reader->Dropped = 0;
	Reader->Timeout = 5000; //FIXME: Get Timeout from Element

	g_mutex_lock(&Writer->lock);
	Writer->Readers = g_list_append(Writer->Readers, Reader);
	g_mutex_unlock(&Writer->lock);

	return Reader;
}

void InternalReaderRead(struct InternalReader *Reader, GstBuffer **buf)
{
	*buf = g_async_queue_timeout_pop(Reader->Queue, Reader->Timeout);
}

void InternalReaderFree(struct InternalReader *Reader)
{
	//Unregister
	g_mutex_lock(&Reader->Writer->lock);
	Reader->Writer->Readers = g_list_remove(Reader->Writer->Readers, Reader);
	g_mutex_unlock(&Reader->Writer->lock);

	//Pop All and Free Buffers
	while(g_async_queue_length(Reader->Queue))
	{
		GstBuffer *buf = g_async_queue_pop(Reader->Queue);
		gst_buffer_unref(buf);
	}

	g_async_queue_unref(Reader->Queue);
	g_free(Reader);
}



