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


		void* apiHandle = nullptr;          // void pointer for API specific stream handle information

		char* userBuffer[2] = { nullptr, nullptr };       // Playback and record, respectively.
		char* deviceBuffer = nullptr;

		bool doConvertBuffer[2] = { false, false };   // Playback and record, respectively.
		bool userInterleaved = true;
		bool deviceInterleaved[2] = { true, true }; // Playback and record, respectively.
		bool doByteSwap[2] = { false, false };        // Playback and record, respectively.

		unsigned int device[2] = { 11'111, 11'111 };    // Playback and record, respectively.
		unsigned int sampleRate = 0;
		unsigned int bufferSize = 0;
		unsigned int nBuffers = 0;
		unsigned int nUserChannels[2] = { 0, 0 };    // Playback and record, respectively.
		unsigned int nDeviceChannels[2] = { 0, 0 };  // Playback and record channels, respectively.
		unsigned int channelOffset[2] = { 0, 0 };    // Playback and record, respectively.

		unsigned long latency[2] = { 0, 0 };         // Playback and record, respectively.

		double streamTime = 0.0;         // Number of elapsed seconds since the stream started.

		pthread_mutex_t mutex;

		StreamMode mode = StreamMode::UNINITIALIZED;           // OUTPUT, INPUT, or DUPLEX.
		StreamState state = StreamState::STREAM_CLOSED;         // STOPPED, RUNNING, or CLOSED
		AudioFormat userFormat = AudioFormat::Float64;
		AudioFormat deviceFormat[2] = { AudioFormat::Float64,
										AudioFormat::Float64 };    // Playback and record, respectively.
		CallbackInfo callbackInfo;
		ConvertInfo convertInfo[2] = { };

		AudioStream() = default;

	};
}


#endif //MAXIMILIAN_AUDIOSTREAM_HPP
