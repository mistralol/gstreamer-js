
#ifndef __GST_CONVOLUTION_H__
#define __GST_CONVOLUTION_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_CONVOLUTION \
  (Convolution_get_type())

#define GST_CONVOLUTION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CONVOLUTION,Convolution))

#define GST_CONVOLUTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CONVOLUTION,ConvolutionClass))

#define GST_IS_CONVOLUTION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CONVOLUTION))

#define GST_IS_CONVOLUTION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CONVOLUTION))

typedef struct _ConvolutionKernel ConvolutionKernel;

struct _ConvolutionKernel
{
	gfloat *data;
	gfloat div;
	guint width;
	guint height;
};

typedef enum
{
	Kernel_Identity,
	Kernel_BoxBlur,
	Kernel_GaussianBlur,
	Kernel_Sharpen,
	Kernel_EdgeDetect,
	Kernel_Emboss,
	Kernel_Custom
} ConvolutionKernelType;

typedef struct _Convolution Convolution;
typedef struct _ConvolutionClass ConvolutionClass;

struct _Convolution
{
	GstElement element;
	GstPad *sinkpad;
	GstPad *srcpad;

	ConvolutionKernelType KernelType;
	ConvolutionKernel kernel;
	gboolean KernelValid;

	gchar *custom;

	guint width;
	guint height;
};

struct _ConvolutionClass 
{
  GstElementClass parent_class;
};

GType Convolution_get_type (void);
gboolean InitConvolution(GstPlugin *plugin);

G_END_DECLS

#endif 
