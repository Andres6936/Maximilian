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

		bool isRunning = false;

		// Default constructor.
		CallbackInfo() = default;

	};
}


#endif //MAXIMILIAN_CALLBACKINFO_HPP
