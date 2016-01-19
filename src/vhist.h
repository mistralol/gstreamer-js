
#ifndef __GST_VHIST_H__
#define __GST_VHIST_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_VHIST \
  (VHist_get_type())

#define GST_VHIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VHIST,VHist))

#define GST_VHIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VHIST,VHistClass))

#define GST_IS_VHIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VHIST))

#define GST_IS_VHIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VHIST))

typedef struct _VHist VHist;
typedef struct _VHistClass VHistClass;

struct _VHist
{
	GstElement element;
	GstPad *sinkpad;
	GstPad *srcpad;

	guint width;
	guint height;
	gboolean send_events;
	guint last;
};

struct _VHistClass 
{
  GstElementClass parent_class;
};

GType VHist_get_type (void);
gboolean InitVHist(GstPlugin *plugin);

G_END_DECLS

#endif 
