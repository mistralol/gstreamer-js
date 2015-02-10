

#ifndef __GST_INTERNALCOMMON_H__
#define __GST_INTERNALCOMMON_H__



struct InternalWriter
{
	gchar *Name;
	GList *Readers;
	GMutex lock;
	int count;
};

struct ReaderOptions
{
	guint MaxQueue;
	guint64 Timeout;
};

struct InternalReader
{
	struct InternalWriter *Writer; //The current Writer struct this is a member of
	GAsyncQueue *Queue;
	struct ReaderOptions Options;
	guint Dropped;
};

extern struct InternalWriter *InternalWriterAttach(const gchar *Name, gboolean alloc);
extern void InternalWriterWrite(struct InternalWriter *Writer, GstSample *buf);
extern void InternalWriterFree(struct InternalWriter *Writer);

extern struct InternalReader *InternalReaderAttach(const gchar *Name, const struct ReaderOptions *Options);
extern void InternalReaderRead(struct InternalReader *Reader, GstSample **buf);
extern void InternalReaderFree(struct InternalReader *Reader);

#endif


