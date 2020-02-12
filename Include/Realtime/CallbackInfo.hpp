#ifndef MAXIMILIAN_CALLBACKINFO_HPP
#define MAXIMILIAN_CALLBACKINFO_HPP

#include <pthread.h>

namespace Maximilian
{

	/**
	 * This global structure type is used to pass callback
	 * information between the private RtAudio stream
	 * structure and global callback handling functions.
	 */
	class CallbackInfo
	{

	private:

	public:

		void* object = nullptr;    // Used as a "this" pointer.
		pthread_t thread;
		void* callback = nullptr;
		void* userData = nullptr;
		void* apiInfo = nullptr;   // void pointer for API specific callback information
		bool isRunning = false;

		// Default constructor.
		CallbackInfo() = default;

	};
}


#endif //MAXIMILIAN_CALLBACKINFO_HPP
