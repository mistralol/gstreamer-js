

#ifndef __GST_INTERNALCOMMON_H__
#define __GST_INTERNALCOMMON_H__


//Mutex For Global List
//global List
//Global Count

//extern glist *InternalList;

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
};

#endif


