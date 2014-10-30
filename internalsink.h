
#ifndef __GST_INTERNALSINK_H__
#define __GST_INTERNALSINK_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_INTERNALSINK \
  (InternalSink_get_type())

#define GST_INTERNALSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_INTERNALSINK,InternalSink))

#define GST_INTERNALSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_INTERNALSINK,InternalSinkClass))

#define GST_IS_INTERNALSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_INTERNALSINK))

#define GST_IS_INTERNALSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_INTERNALSINK))

typedef struct _InternalSink InternalSink;
typedef struct _InternalSinkClass InternalSinkClass;

struct _InternalSink
{
	GstElement element;
	GstPad *sinkpad;

	gchar *Name;
	struct InternalWriter *Writer;
	gint maxqueuesize;
};

struct _InternalSinkClass 
{
	GstElementClass parent_class;
};

GType InternalSink_get_type (void);
gboolean InitInternalSink(GstPlugin *plugin);

G_END_DECLS

#endif

