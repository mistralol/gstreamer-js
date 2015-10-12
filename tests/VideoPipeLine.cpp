
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

VideoPipeLine::VideoPipeLine(const std::string strpipe)
{
	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		abort();
	}

	m_loop = false;
	pipeline = NULL;
	strpipeline = strpipe;
}


VideoPipeLine::~VideoPipeLine()
{
	pthread_mutex_destroy(&lock);
}

void VideoPipeLine::Start()
{
	m_loop = true;
	if( pthread_create(&m_thread, NULL, &Run, this))
	{
		abort();
	}
}

void VideoPipeLine::Stop()
{
	m_loop = false;
	void *retval = NULL;
	pthread_join(m_thread, &retval);
}

void VideoPipeLine::WaitForEos(GstBus *bus)
{
	bool loop = true;
			
	while(loop)
	{
		if (m_loop == false)
			return;

		GstClockTime timeout = 1000000000; //1 Seconds
		GstMessage *msg = gst_bus_timed_pop (bus, timeout);
		if (msg == NULL)
		{
			GstState state;
			if (gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE) != GST_STATE_CHANGE_SUCCESS)
				return;
			if (state != GST_STATE_PLAYING)
			{
				printf("Pipeline is not playing!\n");
				return;
			}
			continue;
		}


		switch (GST_MESSAGE_TYPE (msg)) {
			case GST_MESSAGE_EOS:
				//g_print ("EOS\n");
				loop = false;
				break;
			case GST_MESSAGE_ERROR:
				{
					GError *err = NULL; /* error to show to users                 */
					gchar *dbg = NULL;  /* additional debug string for developers */

					gst_message_parse_error (msg, &err, &dbg);
					if (err) {
						g_printerr ("ERROR: %s\n", err->message);
						g_error_free (err);
					}
					if (dbg) {
						g_printerr ("[Debug details: %s]\n", dbg);
						g_free (dbg);
					}
					loop = false;
					break;
				}
			//Suppress Noise
			case GST_MESSAGE_STREAM_STATUS:
			case GST_MESSAGE_ASYNC_START:
			case GST_MESSAGE_ASYNC_DONE:
			case GST_MESSAGE_STATE_CHANGED:
			case GST_MESSAGE_NEW_CLOCK:
				break;
			default:
				g_printerr ("Unexpected message of type %s\n", GST_MESSAGE_TYPE_NAME (msg));
				break;
		}
		gst_message_unref (msg);
	}
}

bool VideoPipeLine::SetState(GstState state)
{
	GstStateChangeReturn ret = gst_element_set_state (pipeline, state);
	if (ret == GST_STATE_CHANGE_SUCCESS)
		return true;
	if (ret == GST_STATE_CHANGE_FAILURE)
		return false;
	if (ret == GST_STATE_CHANGE_NO_PREROLL)
		abort(); //Not Yet Handled

	if (ret == GST_STATE_CHANGE_ASYNC)
	{
		GstState curstate;
		GstClockTime timeout = GST_SECOND * 15;
		int retries = 1;

		for(int i =0;i<retries;i++)
		{
			ret = gst_element_get_state(pipeline, &curstate, NULL, timeout);
			if (ret == GST_STATE_CHANGE_SUCCESS)
				return true;
			if (ret == GST_STATE_CHANGE_FAILURE)
				return false;
			printf("gst_element_get_state timeout: %s\n", gst_element_state_change_return_get_name(ret));
		}

		printf("Gstreamer Lockup detected\n");
		return false;
	}
	else
	{
		printf("Invalud Return from gst_element_set_state\n");
		abort(); //Accoridng to gstreamer documentation this is not reachable
	}
}


void *VideoPipeLine::Run(void *arg)
{
	VideoPipeLine *self = (class VideoPipeLine *) arg;
	std::string strpipe = self->strpipeline;

	pthread_mutex_lock(&self->lock);
	while(self->m_loop)
	{
		GstBus *bus = NULL;
		GError *error = NULL;
		self->pipeline = gst_parse_launch (strpipe.c_str(), &error);
		if (!self->pipeline) {
			g_print ("Parse error: %s\n", error->message);
			abort();
		}

		GstElement *sink = gst_bin_get_by_name((GstBin *) self->pipeline, "appsink0");
		if (sink != NULL)
		{
			g_signal_connect (sink, "new-sample",  G_CALLBACK (VideoPipeLine::OnNewBuffer), self);
		}

		bus = gst_element_get_bus (self->pipeline);
		
		g_signal_connect(self->pipeline, "deep-notify", G_CALLBACK (gst_object_default_deep_notify), NULL);

		printf("VideoPipeLine Start Playing\n");
		if (self->SetState(GST_STATE_PLAYING) == true)
		{
			//Unlock While we run
			pthread_mutex_unlock(&self->lock);
			self->WaitForEos(bus);
			pthread_mutex_lock(&self->lock);
		}
		else
		{
			printf("Failed To start pipeline\n");
		}

		if (self->SetState(GST_STATE_NULL) == false)
		{
			printf("Unable To Set Pipeline to NULL\n");
			abort(); //Otherwise we leak the pipeline
		}

		gst_object_unref (bus);
		gst_object_unref (self->pipeline);
		self->pipeline = NULL;
		printf("VideoPipeLine ShutDown Complete\n");
//		pthread_mutex_unlock(&self->lock);
//		sleep(5);
//		pthread_mutex_lock(&self->lock);
	}
	pthread_mutex_unlock(&self->lock);
	return NULL;
}

GstFlowReturn VideoPipeLine::OnNewBuffer(GstElement *object, gpointer user_data)
{
//	VideoPipeLine *self = (class VideoPipeLine *) user_data;
	GstAppSink* sink = (GstAppSink *) object;
	GstSample *sample = gst_app_sink_pull_sample(sink);
	if (sample == NULL)
		return GST_FLOW_ERROR;
	printf("OnNewBuffer\n");
	gst_sample_unref(sample);
	return GST_FLOW_OK;
}




