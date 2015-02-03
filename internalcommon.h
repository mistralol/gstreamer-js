

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
	guint64 Timeout;
};

extern struct InternalWriter *InternalWriterAttach(const gchar *Name, gboolean alloc);
extern void InternalWriterWrite(struct InternalWriter *Writer, GstSample *buf);
extern void InternalWriterFree(struct InternalWriter *Writer);

extern struct InternalReader *InternalReaderAttach(const gchar *Name);
extern void InternalReaderRead(struct InternalReader *Reader, GstSample **buf);
extern void InternalReaderFree(struct InternalReader *Reader);

#endif


