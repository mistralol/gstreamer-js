

#ifndef __GST_INTERNALCOMMON_H__
#define __GST_INTERNALCOMMON_H__



struct InternalWriter
{
	gchar *Name;
	GList *Readers;
	GMutex lock;
	int count;
};

struct InternalReader
{
	struct InternalWriter *Writer; //The current Writer struct this is a member of
	GAsyncQueue *Queue;
	guint MaxQueue;
	guint Dropped;
};

extern struct InternalWriter *InternalWriterAttach(const gchar *Name, gboolean alloc);
extern void InternalWriterWrite(struct InternalWriter *Writer, GstBuffer *buf);
extern void InternalWriterFree(struct InternalWriter *Writer);

#endif


