
#include <gst/gst.h>

#include "pipestats.h"
#include "internalsink.h"
#include "dropdeltas.h"
#include "drop2key.h"
#include "clockdrift.h"
#include "bufferjitter.h"
#include "bufferspike.h"


static gboolean Register_init (GstPlugin *data)
{
	if (InitPipeStats(data) == FALSE)
		return FALSE;

	if (InitInternalSink(data) == FALSE)
		return FALSE;

	if (InitDropDeltas(data) == FALSE)
		return FALSE;

	if (InitDrop2Key(data) == FALSE)
		return FALSE;

	if (InitClockDrift(data) == FALSE)
		return FALSE;

	if (InitBufferJitter(data) == FALSE)
		return FALSE;

	if (InitBufferSpike(data) == FALSE)
		return FALSE;

	return TRUE;
}

#define PACKAGE "Custom"
GST_PLUGIN_DEFINE (
	GST_VERSION_MAJOR,
	GST_VERSION_MINOR,
	Custom,
	"Provide some extra gstreamer elements",
	Register_init,
	"0.1",
	"LGPL",
	"Stev",
	"http://www.stev.org/"
)


