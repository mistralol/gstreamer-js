
#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

#include "pipestats.h"
#include "internalcommon.h"
#include "internalsink.h"
#include "internalsrc.h"
#include "dropdeltas.h"
#include "drop2key.h"
#include "clockdrift.h"
#include "bufferjitter.h"
#include "bufferspike.h"
#include "dumpcaps.h"


static gboolean Register_init (GstPlugin *data)
{
	if (InitPipeStats(data) == FALSE)
		return FALSE;

	if (InitInternalSink(data) == FALSE)
		return FALSE;

	if (InitInternalSrc(data) == FALSE)
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
		
	if (InitDumpCaps(data) == FALSE)
		return FALSE;

	return TRUE;
}

#ifndef PACKAGE
#define PACKAGE "gstreamer-js"
#endif

GST_PLUGIN_DEFINE (
	GST_VERSION_MAJOR,
	GST_VERSION_MINOR,
	gstreamer-js,
	"Provide some extra gstreamer elements",
	Register_init,
	"1.0",
	"LGPL",
	"Stev",
	"http://www.stev.org/"
)


