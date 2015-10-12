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
	VideoPipeLine *VSource = new VideoPipeLine("videotestsrc is-live=true ! video/x-raw,format=I420, width=(int)640, height=(int)480, framerate=(fraction)5/1 ! jpegenc ! internalsink streamname=vs1");
//	VideoPipeLine *VDest1 = new VideoPipeLine("internalsrc streamname=vs1 ! appsink name=appsink0 emit-signals=true");
	VideoPipeLine *VDest2 = new VideoPipeLine("internalsrc streamname=vs1 ! jpegdec ! fakesink silent=false");

	//VideoPipeLine *VDest = new VideoPipeLine("internalsrc streamname=vs1 ! appsink name=appsink0 emit-signals=true");


	printf("Starting VSource\n");
	VSource->Start();
	sleep(5);
	printf("Starting VDest\n");
//	VDest1->Start();
	VDest2->Start();

	sleep(30);

	printf("Stopping VDest\n");
//	VDest1->Stop();
	VDest2->Stop();
	printf("Stopping VScource\n");
	VSource->Stop();

	printf("Cleanup\n");
	delete VSource;
//	delete VDest1;
	delete VDest2;
	gst_deinit();
	return 0;
}

