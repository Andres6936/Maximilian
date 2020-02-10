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

		void* object;    // Used as a "this" pointer.
		pthread_t thread;
		void* callback;
		void* userData;
		void* apiInfo;   // void pointer for API specific callback information
		bool isRunning;

		// Default constructor.
		CallbackInfo()
				: object(0), callback(0), userData(0), apiInfo(0), isRunning(false)
		{
		}

	};
}


#endif //MAXIMILIAN_CALLBACKINFO_HPP
