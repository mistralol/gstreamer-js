

#include <pthread.h>

class VideoPipeLine
{
	public:
		VideoPipeLine(const std::string strpipe);
		~VideoPipeLine();

		void Start();
		void Stop();

	private:
		void WaitForEos(GstBus *bus);
		bool SetState(GstState state);

		static void *Run(void *arg);
		static GstFlowReturn OnNewBuffer(GstElement *object, gpointer user_data);

		pthread_t m_thread;
		pthread_mutex_t lock;
		bool m_loop;
		GstElement *pipeline;
		std::string strpipeline;

};


