
#ifndef __GST_INTERNALSRC_H__
#define __GST_INTERNALSRC_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_INTERNALSRC \
  (InternalSrc_get_type())

#define GST_INTERNALSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_INTERNALSRC,InternalSrc))

#define GST_INTERNALSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_INTERNALSRC,InternalSrcClass))

#define GST_IS_INTERNALSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_INTERNALSRC))

#define GST_IS_INTERNALSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_INTERNALSRC))

typedef struct _InternalSrc InternalSrc;
typedef struct _InternalSrcClass InternalSrcClass;

struct _InternalSrc
{
	GstBaseSrc parent;

	gchar *Name;
	struct InternalReader *Reader;
	struct ReaderOptions Options;
	GstClockTimeDiff time_offset;
	gboolean AllowCapsChange;
};

struct _InternalSrcClass 
{
	GstBaseSrcClass parent_class;
};

GType InternalSrc_get_type (void);
gboolean InitInternalSrc(GstPlugin *plugin);

G_END_DECLS

#endif

