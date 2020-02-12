#ifndef MAXIMILIAN_AUDIOARCHITECTURE_HPP
#define MAXIMILIAN_AUDIOARCHITECTURE_HPP

#include "Audio.hpp"
#include "AudioStream.hpp"
#include "ConvertInfo.hpp"
#include "Enum/SupportedArchitectures.hpp"

namespace Maximilian
{

	/**
	 * AudioArchitecture is a "controller" used to select an available audio i/o
	 * interface.  It presents a common API for the user to call but
	 * all functionality is implemented by the class RtApi and its
	 * subclasses.  RtAudio creates an instance of an RtApi subclass
	 * based on the user's API choice.  If no choice is made, RtAudio
	 * attempts to make a "logical" API selection.
	 */
	class AudioArchitecture
	{

	private:

		/**
		 * Specifies output stream parameters to use when opening a
		 * stream, including a device ID, number of channels, and
		 * starting channel number.  For input-only streams, this
		 * argument should be NULL.
		 *
		 * The device ID is an index value between 0 and getDeviceCount() - 1.
		 */
		StreamParameters outputParameters;

		/**
		 * Specifies input stream parameters to use when opening a
		 * stream, including a device ID, number of channels, and
		 * starting channel number.  For output-only streams, this
		 * argument should be NULL.
		 *
		 * The device ID is an index value between 0 and getDeviceCount() - 1.
		 */
		StreamParameters inputParameters;

		void assertThatStreamIsNotOpen();

		void assertThatTheFormatOfBytesIsGreaterThatZero(AudioFormat _format);

	public:

		AudioArchitecture();

		virtual ~AudioArchitecture();

		virtual unsigned int getDeviceCount() = 0;

		unsigned int getDefaultInputDevice();

		unsigned int getDefaultOutputDevice();

		virtual SupportedArchitectures getCurrentArchitecture() = 0;

		virtual DeviceInfo getDeviceInfo(int device) = 0;

		void openStream(AudioFormat format, unsigned int sampleRate,
				unsigned int* bufferFrames, RtAudioCallback callback,
				void* userData, StreamOptions* options);

		virtual void closeStream();

		virtual void startStream() = 0;

		virtual void stopStream() = 0;

		virtual void abortStream() = 0;

		long getStreamLatency();

		unsigned int getStreamSampleRate();

		virtual double getStreamTime();

		bool isStreamOpen() const
		{
			return stream_.state != StreamState::STREAM_CLOSED;
		};

		bool isStreamRunning() const
		{
			return stream_.state == StreamState::STREAM_RUNNING;
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

		typedef signed short Int16;
		typedef signed int Int32;
		typedef float Float32;
		typedef double Float64;

		std::ostringstream errorStream_;
		std::string errorText_;
		bool showWarnings_;
		AudioStream stream_;

		/*!
		  Protected, api-specific method that attempts to open a device
		  with the given parameters.  This function MUST be implemented by
		  all subclasses.  If an error is encountered during the probe, a
		  "warning" message is reported and FAILURE is returned. A
		  successful probe is indicated by a return value of SUCCESS.
		*/
		virtual bool probeDeviceOpen(unsigned int device, StreamMode mode, unsigned int channels,
				unsigned int firstChannel, unsigned int sampleRate,
				AudioFormat format, unsigned int* bufferSize,
				StreamOptions* options);

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
		void byteSwapBuffer(char* buffer, unsigned int samples, AudioFormat format);

		//! Protected common method that returns the number of bytes for a given format.
		unsigned int formatBytes(AudioFormat format);

		//! Protected common method that sets up the parameters for buffer conversion.
		void setConvertInfo(StreamMode mode, unsigned int firstChannel);
	};
}


#endif //MAXIMILIAN_AUDIOARCHITECTURE_HPP
