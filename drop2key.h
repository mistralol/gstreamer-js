
#ifndef __GST_DROP2KEY_H__
#define __GST_DROP2KEY_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_DROP2KEY \
  (Drop2Key_get_type())

#define GST_DROP2KEY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DROP2KEY,Drop2Key))

#define GST_DROP2KEY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DROP2KEY,Drop2KeyClass))

#define GST_IS_DROP2KEY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DROP2KEY))

#define GST_IS_DROP2KEY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DROP2KEY))

typedef struct _Drop2Key Drop2Key;
typedef struct _Drop2KeyClass Drop2KeyClass;

struct _Drop2Key
{
	GstElement element;
	GstPad *sinkpad;
	GstPad *srcpad;

	gboolean NeedKeyFrame;
};

struct _Drop2KeyClass 
{
  GstElementClass parent_class;
};

GType Drop2Key_get_type (void);
gboolean InitDrop2Key(GstPlugin *plugin);

G_END_DECLS

#endif /* __GST_PIH264SINK_H__ */
