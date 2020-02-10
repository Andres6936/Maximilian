#ifndef MAXIMILIAN_AUDIOSTREAM_HPP
#define MAXIMILIAN_AUDIOSTREAM_HPP

#include "ConvertInfo.hpp"
#include "CallbackInfo.hpp"
#include "Definition/AudioFormat.hpp"

namespace Maximilian
{

	enum class StreamState : char
	{
		STREAM_STOPPED,
		STREAM_RUNNING,
		STREAM_CLOSED = -50
	};

	enum class StreamMode : char
	{
		OUTPUT,
		INPUT,
		DUPLEX,
		UNINITIALIZED = -75
	};

	/*
	 * A protected structure for audio streams.
	 */
	class AudioStream
	{

	private:

	public:

		unsigned int device[2];    // Playback and record, respectively.
		void* apiHandle;           // void pointer for API specific stream handle information
		StreamMode mode;           // OUTPUT, INPUT, or DUPLEX.
		StreamState state;         // STOPPED, RUNNING, or CLOSED
		char* userBuffer[2];       // Playback and record, respectively.
		char* deviceBuffer;
		bool doConvertBuffer[2];   // Playback and record, respectively.
		bool userInterleaved;
		bool deviceInterleaved[2]; // Playback and record, respectively.
		bool doByteSwap[2];        // Playback and record, respectively.
		unsigned int sampleRate;
		unsigned int bufferSize;
		unsigned int nBuffers;
		unsigned int nUserChannels[2];    // Playback and record, respectively.
		unsigned int nDeviceChannels[2];  // Playback and record channels, respectively.
		unsigned int channelOffset[2];    // Playback and record, respectively.
		unsigned long latency[2];         // Playback and record, respectively.
		AudioFormat userFormat;
		AudioFormat deviceFormat[2];    // Playback and record, respectively.
		pthread_mutex_t mutex;
		CallbackInfo callbackInfo;
		ConvertInfo convertInfo[2];
		double streamTime;         // Number of elapsed seconds since the stream started.

		AudioStream()
				: apiHandle(0), deviceBuffer(0)
		{
			device[0] = 11111;
			device[1] = 11111;
		}

	};
}


#endif //MAXIMILIAN_AUDIOSTREAM_HPP
