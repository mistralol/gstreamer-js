
#include <glib.h>
#include <gst/gst.h>
#include <string.h>

#include "internalcommon.h"

GList *WriterList = NULL;
G_LOCK_DEFINE(WriterLock);


struct InternalWriter *InternalWriterAttach(const gchar *name, gboolean alloc)
{
	struct InternalWriter *Writer = NULL;
	G_LOCK(WriterLock);

	if (WriterList == NULL && alloc == FALSE)
	{
		G_UNLOCK(WriterLock);
		return NULL;
	}

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
	GList *it = g_list_first(WriterList);
	while(it)
	{
		//GLIB WTF? some of the list has NULL entries.
		if (it->data == NULL)
		{
			it = it->next;
			continue;
		}

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

void InternalWriterWrite(struct InternalWriter *Writer, GstSample *buf)
{
	g_mutex_lock(&Writer->lock);
	GList *it = Writer->Readers;
	while(it)
	{
		if (it->data == NULL)
		{
			it = it->next;
			continue;
		}

		struct InternalReader *Reader = it->data;
		gint QLength = g_async_queue_length(Reader->Queue);
		if (QLength < 0 || QLength < Reader->Options.MaxQueue)
		{
			gst_sample_ref(buf);
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

struct InternalReader *InternalReaderAttach(const gchar *Name, const struct ReaderOptions *Options)
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
	memcpy(&Reader->Options, Options, sizeof(*Options));
	Reader->Dropped = 0;

	g_mutex_lock(&Writer->lock);
	Writer->Readers = g_list_append(Writer->Readers, Reader);
	g_mutex_unlock(&Writer->lock);

	return Reader;
}

void InternalReaderRead(struct InternalReader *Reader, GstSample **buf)
{
	*buf = g_async_queue_timeout_pop(Reader->Queue, Reader->Options.Timeout * 1000);
}

void InternalReaderFree(struct InternalReader *Reader)
{
	//Unregister
	g_mutex_lock(&Reader->Writer->lock);
	Reader->Writer->Readers = g_list_remove(Reader->Writer->Readers, Reader);
	g_mutex_unlock(&Reader->Writer->lock);

	//Pop All and Free Buffers
	g_async_queue_lock(Reader->Queue);
	while(g_async_queue_length_unlocked(Reader->Queue))
	{
		GstSample *buf = g_async_queue_pop_unlocked(Reader->Queue);
		gst_sample_unref(buf);
	}
	g_async_queue_unlock(Reader->Queue);

	g_async_queue_unref(Reader->Queue);
	g_free(Reader);
}



