
#ifndef __GST_JSMOTION_H__
#define __GST_JSMOTION_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_JSMOTION \
  (JSMotion_get_type())

#define GST_JSMOTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_JSMOTION,JSMotion))

#define GST_JSMOTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_JSMOTION,JSMotionClass))

#define GST_IS_JSMOTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_JSMOTION))

#define GST_IS_JSMOTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_JSMOTION))

typedef struct _JSMotion JSMotion;
typedef struct _JSMotionClass JSMotionClass;

struct _JSMotion
{
	GstElement element;
	GstPad *sinkpad;
	GstPad *srcpad;

	GstBuffer *last_frame;
	int width;
	int height;
};

struct _JSMotionClass 
{
  GstElementClass parent_class;
};

GType JSMotion_get_type (void);
gboolean InitJSMotion(GstPlugin *plugin);

G_END_DECLS

#endif 
