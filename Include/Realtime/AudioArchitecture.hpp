#ifndef MAXIMILIAN_AUDIOARCHITECTURE_HPP
#define MAXIMILIAN_AUDIOARCHITECTURE_HPP

#include "Audio.hpp"

namespace Maximilian
{
	class AudioArchitecture
	{

	private:

		void assertThatStreamIsNotOpen();

		void assertThatTheFormatOfBytesIsGreaterThatZero(RtAudioFormat _format);

		void assertThatDeviceParameterIsNotInvalid(const Audio::StreamParameters& _parameters);

		static void assertThatChannelsAreGreaterThatOne(const Audio::StreamParameters& _parameters);

	public:

		AudioArchitecture();

		virtual ~AudioArchitecture();

		virtual unsigned int getDeviceCount() = 0;

		virtual unsigned int getDefaultInputDevice();

		virtual unsigned int getDefaultOutputDevice();

		virtual Audio::SupportedArchitectures getCurrentArchitecture() = 0;

		virtual Audio::DeviceInfo getDeviceInfo(unsigned int device) = 0;

		void openStream(Audio::StreamParameters& oParams,
				RtAudioFormat format, unsigned int sampleRate,
				unsigned int* bufferFrames, RtAudioCallback callback,
				void* userData, Audio::StreamOptions* options);

		void openStream(Audio::StreamParameters& oParams,
				Audio::StreamParameters& iParams,
				RtAudioFormat format, unsigned int sampleRate,
				unsigned int* bufferFrames, RtAudioCallback callback,
				void* userData, Audio::StreamOptions* options);

		virtual void closeStream();

		virtual void startStream() = 0;

		virtual void stopStream() = 0;

		virtual void abortStream() = 0;

		long getStreamLatency();

		unsigned int getStreamSampleRate();

		virtual double getStreamTime();

		bool isStreamOpen() const
		{
			return stream_.state != STREAM_CLOSED;
		};

		bool isStreamRunning() const
		{
			return stream_.state == STREAM_RUNNING;
		};

		void showWarnings(bool value)
		{
			showWarnings_ = value;
		};


	protected:

		static const unsigned int MAX_SAMPLE_RATES;
		static const unsigned int SAMPLE_RATES[];

		enum
		{
			FAILURE, SUCCESS
		};

		enum StreamState
		{
			STREAM_STOPPED,
			STREAM_RUNNING,
			STREAM_CLOSED = -50
		};

		enum StreamMode
		{
			OUTPUT,
			INPUT,
			DUPLEX,
			UNINITIALIZED = -75
		};

		// A protected structure used for buffer conversion.
		struct ConvertInfo
		{
			int channels;
			int inJump, outJump;
			RtAudioFormat inFormat, outFormat;
			std::vector <int> inOffset;
			std::vector <int> outOffset;
		};

		// A protected structure for audio streams.
		struct RtApiStream
		{
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
			RtAudioFormat userFormat;
			RtAudioFormat deviceFormat[2];    // Playback and record, respectively.
			StreamMutex mutex;
			CallbackInfo callbackInfo;
			ConvertInfo convertInfo[2];
			double streamTime;         // Number of elapsed seconds since the stream started.

#if defined(HAVE_GETTIMEOFDAY)
			struct timeval lastTickTimestamp;
#endif

			RtApiStream()
					: apiHandle(0), deviceBuffer(0)
			{
				device[0] = 11111;
				device[1] = 11111;
			}
		};

		typedef signed short Int16;
		typedef signed int Int32;
		typedef float Float32;
		typedef double Float64;

		std::ostringstream errorStream_;
		std::string errorText_;
		bool showWarnings_;
		RtApiStream stream_;

		/*!
		  Protected, api-specific method that attempts to open a device
		  with the given parameters.  This function MUST be implemented by
		  all subclasses.  If an error is encountered during the probe, a
		  "warning" message is reported and FAILURE is returned. A
		  successful probe is indicated by a return value of SUCCESS.
		*/
		virtual bool probeDeviceOpen(unsigned int device, StreamMode mode, unsigned int channels,
				unsigned int firstChannel, unsigned int sampleRate,
				RtAudioFormat format, unsigned int* bufferSize,
				Audio::StreamOptions* options);

		//! A protected function used to increment the stream time.
		void tickStreamTime();

		//! Protected common method to clear an RtApiStream structure.
		void clearStreamInfo();

		/*!
		  Protected common method that throws an Exception (type =
		  INVALID_USE) if a stream is not open.
		*/
		void verifyStream();

		//! Protected common error method to allow global control over error handling.
		void error(Exception::Type type);

		/*!
		  Protected method used to perform format, channel number, and/or interleaving
		  conversions between the user and device buffers.
		*/
		void convertBuffer(char* outBuffer, char* inBuffer, ConvertInfo& info);

		//! Protected common method used to perform byte-swapping on buffers.
		void byteSwapBuffer(char* buffer, unsigned int samples, RtAudioFormat format);

		//! Protected common method that returns the number of bytes for a given format.
		unsigned int formatBytes(RtAudioFormat format);

		//! Protected common method that sets up the parameters for buffer conversion.
		void setConvertInfo(StreamMode mode, unsigned int firstChannel);
	};
}


#endif //MAXIMILIAN_AUDIOARCHITECTURE_HPP
