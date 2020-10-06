#ifndef MAXIMILIAN_AUDIOSTREAM_HPP
#define MAXIMILIAN_AUDIOSTREAM_HPP

#include "ConvertInfo.hpp"
#include "CallbackInfo.hpp"
#include "Definition/AudioFormat.hpp"

#include <array>
#include <vector>

namespace Maximilian
{

	using Buffer = std::vector <char>;

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

		Buffer deviceBuffer;

		bool userInterleaved = true;

		unsigned int nBuffers = 0;

		unsigned int sampleRate = 0;
		unsigned int bufferSize = 0;
		double streamTime = 0.0;         // Number of elapsed seconds since the stream started.

		/**
		 * Playback and record, respectively.
		 */
		std::pair <Buffer, Buffer> userBuffer;

		/**
		 * Playback and record, respectively.
		 */
		std::array <bool, 2> doConvertBuffer = { false, false };

		/**
		 * Playback and record, respectively.
		 */
		std::array <bool, 2> deviceInterleaved = { true, true };

		/**
		 * Playback and record, respectively.
		 */
		std::array<bool, 2> doByteSwap = { false, false };

		/**
		 * Playback and record, respectively.
		 */
		std::array<unsigned int, 2> nUserChannels = { 0, 0 };

		/**
		 * Playback and record, respectively.
		 */
		std::array<unsigned int, 2> nDeviceChannels = { 0, 0 };

		/**
		 * Playback and record, respectively.
		 */
		std::array<unsigned int, 2> channelOffset = { 0, 0 };

		/**
		 * Playback and record, respectively.
		 */
		std::array<unsigned int, 2> device = { 11'111, 11'111 };

		/**
		 * Playback and record, respectively.
		 */
		std::array<unsigned long, 2> latency = { 0, 0 };

		/**
		 * Playback and record, respectively.
		 */
		std::array<AudioFormat, 2> deviceFormat = { AudioFormat::Float64,
													AudioFormat::Float64 };

		pthread_mutex_t mutex;

		StreamMode mode = StreamMode::UNINITIALIZED;           // OUTPUT, INPUT, or DUPLEX.

		StreamState state = StreamState::STREAM_CLOSED;         // STOPPED, RUNNING, or CLOSED

		AudioFormat userFormat = AudioFormat::Float64;

		ConvertInfo convertInfo[2] = { };

		AudioStream() = default;

	};
}


#endif //MAXIMILIAN_AUDIOSTREAM_HPP
