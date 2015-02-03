#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <string>
#include <list>

#include <glib.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include "VideoPipeLine.h"


int main(int argc, char **argv)
{
	gst_init(&argc, &argv);
	printf("Init\n");
	VideoPipeLine *VSource = new VideoPipeLine("videotestsrc is-live=true ! video/x-raw,format=I420, width=(int)640, height=(int)480, framerate=(fraction)5/1 ! x264enc key-int-max=30 intra-refresh=true ! internalsink streamname=vs1");
	//VideoPipeLine *VDest = new VideoPipeLine("internalsrc streamname=vs1 ! appsink name=appsink0 emit-signals=true");
	VideoPipeLine *VDest = new VideoPipeLine("internalsrc streamname=vs1 ! avdec_h264 ! appsink name=appsink0 emit-signals=true");

	printf("Starting VSource\n");
	VSource->Start();
	sleep(5);
	printf("Starting VDest\n");
	VDest->Start();

	for(int i =0;i<15;i++)
	{
		sleep(1);
	}

	printf("Stopping VDest\n");
	VDest->Stop();
	printf("Stopping VScource\n");
	VSource->Stop();

	printf("Cleanup\n");
	delete VSource;
	delete VDest;
	gst_deinit();
	return 0;
}

