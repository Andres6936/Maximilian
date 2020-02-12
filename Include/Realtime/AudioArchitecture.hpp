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
		 * The desired sample rate (sample frames per second).
		 */
		unsigned int sampleRate = 44'100;

		/*
		 * The desired internal buffer size in sample frames.
		 */
		unsigned int bufferFrames = 1'024;

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

		/**
		 *  Enum that content various global stream options, including
		 *  a list of OR'ed RtAudioStreamFlags and a suggested number
		 *  of stream buffers that can be used to control stream latency.
		 *  More buffers typically result in more robust performance,
		 *  though at a cost of greater latency.
		 *
		 *  If a value of zero is specified, a system-specific median
		 *  value is chosen.
		 *
		 *  If the RTAUDIO_MINIMIZE_LATENCY flag bit is set, the lowest
		 *  allowable value is used.
		 *
		 *  The actual value used is returned via the structure argument.
		 *
		 *  The parameter is API dependent.
		 */
		StreamOptions options;

		/**
		 * Specifying the desired sample data format.
		 */
		AudioFormat format = AudioFormat::RTAUDIO_FLOAT64;

		void assertThatStreamIsNotOpen();

	public:

		AudioArchitecture();

		virtual ~AudioArchitecture();

		// Static Methods

		static unsigned int getDefaultInputDevice();

		static unsigned int getDefaultOutputDevice();

		// Virtual Methods

		virtual unsigned int getDeviceCount() = 0;

		virtual SupportedArchitectures getCurrentArchitecture() = 0;

		virtual DeviceInfo getDeviceInfo(int device) = 0;

		void openStream(RtAudioCallback callback, void* userData);

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

		// Getters

		int getOptionsPriority() const;

		unsigned int getSampleRate() const;

		unsigned int getBufferFrames() const;

		unsigned int getNumberOfBuffersOptions() const;

		AudioFormat getAudioFormat() const;

		AudioStreamFlags getOptionsFlags() const;

		// Setters

		void setSampleRate(unsigned int _sampleRate);

		void setBufferFrames(unsigned int _bufferFrames);

		void setNumberOfBuffersOptions(unsigned int _numberOfBuffers);

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
		virtual bool probeDeviceOpen(
				unsigned int device,
				StreamMode mode,
				unsigned int channels,
				unsigned int firstChannel);

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
