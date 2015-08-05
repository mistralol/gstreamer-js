
#ifndef __GST_DROPDELTAS_H__
#define __GST_DROPDELTAS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_DROPDELTAS \
  (DropDeltas_get_type())

#define GST_DROPDELTAS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DROPDELTAS,DropDeltas))

#define GST_DROPDELTAS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DROPDELTAS,DropDeltasClass))

#define GST_IS_DROPDELTAS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DROPDELTAS))

#define GST_IS_DROPDELTAS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DROPDELTAS))

typedef struct _DropDeltas DropDeltas;
typedef struct _DropDeltasClass DropDeltasClass;

struct _DropDeltas
{
	GstElement element;
	GstPad *sinkpad;
	GstPad *srcpad;

};

struct _DropDeltasClass 
{
  GstElementClass parent_class;
};

GType DropDeltas_get_type (void);
gboolean InitDropDeltas(GstPlugin *plugin);

G_END_DECLS

#endif /* __GST_PIH264SINK_H__ */
