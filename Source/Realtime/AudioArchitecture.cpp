#include "Realtime/AudioArchitecture.hpp"

#include <Levin/Log.h>
#include <cstring>

using namespace Maximilian;

// Static variable definitions.
const unsigned int AudioArchitecture::MAX_SAMPLE_RATES = 14;

const unsigned int AudioArchitecture::SAMPLE_RATES[] = {
		4000, 5512, 8000, 9600, 11025, 16000, 22050,
		32000, 44100, 48000, 88200, 96000, 176400, 192000
};

Maximilian::AudioArchitecture::AudioArchitecture()
{
	stream_.state = STREAM_CLOSED;
	stream_.mode = UNINITIALIZED;
	stream_.apiHandle = 0;
	stream_.userBuffer[0] = 0;
	stream_.userBuffer[1] = 0;
	pthread_mutex_init(&stream_.mutex, nullptr);
	showWarnings_ = true;
}

Maximilian::AudioArchitecture::~AudioArchitecture()
{
	pthread_mutex_destroy(&stream_.mutex);
}

void AudioArchitecture::assertThatStreamIsNotOpen()
{
	if (stream_.state != STREAM_CLOSED)
	{
		Levin::Error() << "Assert: OpenStream, a stream is already open!" << Levin::endl;
		throw "StreamAlreadyOpenException";
	}
}


void AudioArchitecture::assertThatDeviceParameterIsNotInvalid(const Audio::StreamParameters& _parameters)
{
	if (_parameters.deviceId >= getDeviceCount())
	{
		Levin::Error() << "Assert: OpenStream, the device parameter value"
						  " is invalid." << Levin::endl;
		throw "DeviceParameterIsInvalidException";
	}
}

void AudioArchitecture::assertThatChannelsAreGreaterThatOne(const Audio::StreamParameters& _parameters)
{
	if (_parameters.nChannels < 1)
	{
		Levin::Error() << "Assert: OpenStream, StreamParameters structure cannot "
						  "have an nChannels value less than one." << Levin::endl;
		throw "StreamChannelsIsLessThatOneException";
	}
}

void AudioArchitecture::assertThatTheFormatOfBytesIsGreaterThatZero(const RtAudioFormat _format)
{
	if (formatBytes(_format) == 0)
	{
		Levin::Error() << "Assert: OpenStream, 'format' parameter "
						  "value is undefined." << Levin::endl;
		throw "FormatValueIsUndefinedException";
	}
}

void AudioArchitecture::openStream(Audio::StreamParameters& oParams, RtAudioFormat format, unsigned int sampleRate,
		unsigned int* bufferFrames, RtAudioCallback callback, void* userData, Audio::StreamOptions* options)
{
	assertThatStreamIsNotOpen();
	assertThatChannelsAreGreaterThatOne(oParams);
	assertThatDeviceParameterIsNotInvalid(oParams);
	assertThatTheFormatOfBytesIsGreaterThatZero(format);

	clearStreamInfo();

	bool result;

	result = probeDeviceOpen(oParams.deviceId, OUTPUT, oParams.nChannels, oParams.firstChannel,
			sampleRate, format, bufferFrames, options);

	if (result == false)
	{ error(Exception::SYSTEM_ERROR); }

	stream_.callbackInfo.callback = (void*)callback;
	stream_.callbackInfo.userData = userData;

	if (options)
	{ options->numberOfBuffers = stream_.nBuffers; }
	stream_.state = STREAM_STOPPED;
}

void AudioArchitecture::openStream(
		Audio::StreamParameters& oParams,
		Audio::StreamParameters& iParams,
		Maximilian::RtAudioFormat format, unsigned int sampleRate,
		unsigned int* bufferFrames,
		Maximilian::RtAudioCallback callback, void* userData,
		Audio::StreamOptions* options)
{
	assertThatStreamIsNotOpen();
	assertThatChannelsAreGreaterThatOne(oParams);
	assertThatDeviceParameterIsNotInvalid(oParams);
	assertThatChannelsAreGreaterThatOne(iParams);
	assertThatDeviceParameterIsNotInvalid(iParams);
	assertThatTheFormatOfBytesIsGreaterThatZero(format);

	clearStreamInfo();

	bool result;

	result = probeDeviceOpen(oParams.deviceId, OUTPUT, oParams.nChannels, oParams.firstChannel,
			sampleRate, format, bufferFrames, options);

	if (result == false)
	{ error(Exception::SYSTEM_ERROR); }


	result = probeDeviceOpen(iParams.deviceId, INPUT, iParams.nChannels, iParams.firstChannel,
			sampleRate, format, bufferFrames, options);

	if (result == false)
	{
		if (oParams.nChannels > 0)
		{ closeStream(); }
		error(Exception::SYSTEM_ERROR);
	}

	stream_.callbackInfo.callback = (void*)callback;
	stream_.callbackInfo.userData = userData;

	if (options)
	{ options->numberOfBuffers = stream_.nBuffers; }
	stream_.state = STREAM_STOPPED;
}

unsigned int AudioArchitecture::getDefaultInputDevice(void)
{
	// Should be implemented in subclasses if possible.
	return 0;
}

unsigned int AudioArchitecture::getDefaultOutputDevice(void)
{
	// Should be implemented in subclasses if possible.
	return 0;
}

void AudioArchitecture::closeStream(void)
{
	// MUST be implemented in subclasses!
	return;
}

bool AudioArchitecture::probeDeviceOpen(unsigned int device, StreamMode mode, unsigned int channels,
		unsigned int firstChannel, unsigned int sampleRate,
		Maximilian::RtAudioFormat format, unsigned int* bufferSize,
		Audio::StreamOptions* options)
{
	// MUST be implemented in subclasses!
	return FAILURE;
}

void AudioArchitecture::tickStreamTime(void)
{
	// Subclasses that do not provide their own implementation of
	// getStreamTime should call this function once per buffer I/O to
	// provide basic stream time support.

	stream_.streamTime += (stream_.bufferSize * 1.0 / stream_.sampleRate);

#if defined( HAVE_GETTIMEOFDAY )
	gettimeofday( &stream_.lastTickTimestamp, NULL );
#endif
}

long AudioArchitecture::getStreamLatency(void)
{
	verifyStream();

	long totalLatency = 0;
	if (stream_.mode == OUTPUT || stream_.mode == DUPLEX)
	{
		totalLatency = stream_.latency[0];
	}
	if (stream_.mode == INPUT || stream_.mode == DUPLEX)
	{
		totalLatency += stream_.latency[1];
	}

	return totalLatency;
}

double AudioArchitecture::getStreamTime(void)
{
	verifyStream();

#if defined( HAVE_GETTIMEOFDAY )
	// Return a very accurate estimate of the stream time by
  // adding in the elapsed time since the last tick.
  struct timeval then;
  struct timeval now;

  if ( stream_.state != STREAM_RUNNING || stream_.streamTime == 0.0 )
	return stream_.streamTime;

  gettimeofday( &now, NULL );
  then = stream_.lastTickTimestamp;
  return stream_.streamTime +
	((now.tv_sec + 0.000001 * now.tv_usec) -
	 (then.tv_sec + 0.000001 * then.tv_usec));
#else
	return stream_.streamTime;
#endif
}

unsigned int AudioArchitecture::getStreamSampleRate(void)
{
	verifyStream();

	return stream_.sampleRate;
}


// This method can be modified to control the behavior of error
// message printing.
void AudioArchitecture::error(Exception::Type type)
{
	errorStream_.str(""); // clear the ostringstream
	if (type == Exception::WARNING && showWarnings_ == true)
	{
		std::cerr << '\n' << errorText_ << "\n\n";
	}
	else if (type != Exception::WARNING)
	{
		throw (Exception(errorText_, type));
	}
}

void AudioArchitecture::verifyStream()
{
	if (stream_.state == STREAM_CLOSED)
	{
		errorText_ = "RtApi:: a stream is not open!";
		error(Exception::INVALID_USE);
	}
}

void AudioArchitecture::clearStreamInfo()
{
	stream_.mode = UNINITIALIZED;
	stream_.state = STREAM_CLOSED;
	stream_.sampleRate = 0;
	stream_.bufferSize = 0;
	stream_.nBuffers = 0;
	stream_.userFormat = 0;
	stream_.userInterleaved = true;
	stream_.streamTime = 0.0;
	stream_.apiHandle = 0;
	stream_.deviceBuffer = 0;
	stream_.callbackInfo.callback = 0;
	stream_.callbackInfo.userData = 0;
	stream_.callbackInfo.isRunning = false;
	for (int i = 0; i < 2; i++)
	{
		stream_.device[i] = 11111;
		stream_.doConvertBuffer[i] = false;
		stream_.deviceInterleaved[i] = true;
		stream_.doByteSwap[i] = false;
		stream_.nUserChannels[i] = 0;
		stream_.nDeviceChannels[i] = 0;
		stream_.channelOffset[i] = 0;
		stream_.deviceFormat[i] = 0;
		stream_.latency[i] = 0;
		stream_.userBuffer[i] = 0;
		stream_.convertInfo[i].channels = 0;
		stream_.convertInfo[i].inJump = 0;
		stream_.convertInfo[i].outJump = 0;
		stream_.convertInfo[i].inFormat = 0;
		stream_.convertInfo[i].outFormat = 0;
		stream_.convertInfo[i].inOffset.clear();
		stream_.convertInfo[i].outOffset.clear();
	}
}

unsigned int AudioArchitecture::formatBytes(Maximilian::RtAudioFormat format)
{
	if (format == RTAUDIO_SINT16)
	{
		return 2;
	}
	else if (format == RTAUDIO_SINT24 || format == RTAUDIO_SINT32 ||
			 format == RTAUDIO_FLOAT32)
	{
		return 4;
	}
	else if (format == RTAUDIO_FLOAT64)
	{
		return 8;
	}
	else if (format == RTAUDIO_SINT8)
	{
		return 1;
	}

	errorText_ = "RtApi::formatBytes: undefined format.";
	error(Exception::WARNING);

	return 0;
}

void AudioArchitecture::setConvertInfo(StreamMode mode, unsigned int firstChannel)
{
	if (mode == INPUT)
	{ // convert device to user buffer
		stream_.convertInfo[mode].inJump = stream_.nDeviceChannels[1];
		stream_.convertInfo[mode].outJump = stream_.nUserChannels[1];
		stream_.convertInfo[mode].inFormat = stream_.deviceFormat[1];
		stream_.convertInfo[mode].outFormat = stream_.userFormat;
	}
	else
	{ // convert user to device buffer
		stream_.convertInfo[mode].inJump = stream_.nUserChannels[0];
		stream_.convertInfo[mode].outJump = stream_.nDeviceChannels[0];
		stream_.convertInfo[mode].inFormat = stream_.userFormat;
		stream_.convertInfo[mode].outFormat = stream_.deviceFormat[0];
	}

	if (stream_.convertInfo[mode].inJump < stream_.convertInfo[mode].outJump)
	{
		stream_.convertInfo[mode].channels = stream_.convertInfo[mode].inJump;
	}
	else
	{
		stream_.convertInfo[mode].channels = stream_.convertInfo[mode].outJump;
	}

	// Set up the interleave/deinterleave offsets.
	if (stream_.deviceInterleaved[mode] != stream_.userInterleaved)
	{
		if ((mode == OUTPUT && stream_.deviceInterleaved[mode]) ||
			(mode == INPUT && stream_.userInterleaved))
		{
			for (int k = 0; k < stream_.convertInfo[mode].channels; k++)
			{
				stream_.convertInfo[mode].inOffset.push_back(k * stream_.bufferSize);
				stream_.convertInfo[mode].outOffset.push_back(k);
				stream_.convertInfo[mode].inJump = 1;
			}
		}
		else
		{
			for (int k = 0; k < stream_.convertInfo[mode].channels; k++)
			{
				stream_.convertInfo[mode].inOffset.push_back(k);
				stream_.convertInfo[mode].outOffset.push_back(k * stream_.bufferSize);
				stream_.convertInfo[mode].outJump = 1;
			}
		}
	}
	else
	{ // no (de)interleaving
		if (stream_.userInterleaved)
		{
			for (int k = 0; k < stream_.convertInfo[mode].channels; k++)
			{
				stream_.convertInfo[mode].inOffset.push_back(k);
				stream_.convertInfo[mode].outOffset.push_back(k);
			}
		}
		else
		{
			for (int k = 0; k < stream_.convertInfo[mode].channels; k++)
			{
				stream_.convertInfo[mode].inOffset.push_back(k * stream_.bufferSize);
				stream_.convertInfo[mode].outOffset.push_back(k * stream_.bufferSize);
				stream_.convertInfo[mode].inJump = 1;
				stream_.convertInfo[mode].outJump = 1;
			}
		}
	}

	// Add channel offset.
	if (firstChannel > 0)
	{
		if (stream_.deviceInterleaved[mode])
		{
			if (mode == OUTPUT)
			{
				for (int k = 0; k < stream_.convertInfo[mode].channels; k++)
				{
					stream_.convertInfo[mode].outOffset[k] += firstChannel;
				}
			}
			else
			{
				for (int k = 0; k < stream_.convertInfo[mode].channels; k++)
				{
					stream_.convertInfo[mode].inOffset[k] += firstChannel;
				}
			}
		}
		else
		{
			if (mode == OUTPUT)
			{
				for (int k = 0; k < stream_.convertInfo[mode].channels; k++)
				{
					stream_.convertInfo[mode].outOffset[k] += (firstChannel * stream_.bufferSize);
				}
			}
			else
			{
				for (int k = 0; k < stream_.convertInfo[mode].channels; k++)
				{
					stream_.convertInfo[mode].inOffset[k] += (firstChannel * stream_.bufferSize);
				}
			}
		}
	}
}

void AudioArchitecture::convertBuffer(char* outBuffer, char* inBuffer, ConvertInfo& info)
{
	// This function does format conversion, input/output channel compensation, and
	// data interleaving/deinterleaving.  24-bit integers are assumed to occupy
	// the lower three bytes of a 32-bit integer.

	// Clear our device buffer when in/out duplex device channels are different
	if (outBuffer == stream_.deviceBuffer && stream_.mode == DUPLEX &&
		(stream_.nDeviceChannels[0] < stream_.nDeviceChannels[1]))
	{
		memset(outBuffer, 0, stream_.bufferSize * info.outJump * formatBytes(info.outFormat));
	}

	int j;
	if (info.outFormat == RTAUDIO_FLOAT64)
	{
		Float64 scale;
		Float64* out = (Float64*)outBuffer;

		if (info.inFormat == RTAUDIO_SINT8)
		{
			signed char* in = (signed char*)inBuffer;
			scale = 1.0 / 127.5;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float64)in[info.inOffset[j]];
					out[info.outOffset[j]] += 0.5;
					out[info.outOffset[j]] *= scale;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT16)
		{
			Int16* in = (Int16*)inBuffer;
			scale = 1.0 / 32767.5;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float64)in[info.inOffset[j]];
					out[info.outOffset[j]] += 0.5;
					out[info.outOffset[j]] *= scale;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT24)
		{
			Int32* in = (Int32*)inBuffer;
			scale = 1.0 / 8388607.5;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float64)(in[info.inOffset[j]] & 0x00ffffff);
					out[info.outOffset[j]] += 0.5;
					out[info.outOffset[j]] *= scale;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT32)
		{
			Int32* in = (Int32*)inBuffer;
			scale = 1.0 / 2147483647.5;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float64)in[info.inOffset[j]];
					out[info.outOffset[j]] += 0.5;
					out[info.outOffset[j]] *= scale;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT32)
		{
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float64)in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT64)
		{
			// Channel compensation and/or (de)interleaving only.
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
	else if (info.outFormat == RTAUDIO_FLOAT32)
	{
		Float32 scale;
		Float32* out = (Float32*)outBuffer;

		if (info.inFormat == RTAUDIO_SINT8)
		{
			signed char* in = (signed char*)inBuffer;
			scale = (Float32)(1.0 / 127.5);
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float32)in[info.inOffset[j]];
					out[info.outOffset[j]] += 0.5;
					out[info.outOffset[j]] *= scale;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT16)
		{
			Int16* in = (Int16*)inBuffer;
			scale = (Float32)(1.0 / 32767.5);
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float32)in[info.inOffset[j]];
					out[info.outOffset[j]] += 0.5;
					out[info.outOffset[j]] *= scale;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT24)
		{
			Int32* in = (Int32*)inBuffer;
			scale = (Float32)(1.0 / 8388607.5);
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float32)(in[info.inOffset[j]] & 0x00ffffff);
					out[info.outOffset[j]] += 0.5;
					out[info.outOffset[j]] *= scale;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT32)
		{
			Int32* in = (Int32*)inBuffer;
			scale = (Float32)(1.0 / 2147483647.5);
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float32)in[info.inOffset[j]];
					out[info.outOffset[j]] += 0.5;
					out[info.outOffset[j]] *= scale;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT32)
		{
			// Channel compensation and/or (de)interleaving only.
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT64)
		{
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float32)in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
	else if (info.outFormat == RTAUDIO_SINT32)
	{
		Int32* out = (Int32*)outBuffer;
		if (info.inFormat == RTAUDIO_SINT8)
		{
			signed char* in = (signed char*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 24;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT16)
		{
			Int16* in = (Int16*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 16;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT24)
		{ // Hmmm ... we could just leave it in the lower 3 bytes
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 8;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT32)
		{
			// Channel compensation and/or (de)interleaving only.
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT32)
		{
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)(in[info.inOffset[j]] * 2147483647.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT64)
		{
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)(in[info.inOffset[j]] * 2147483647.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
	else if (info.outFormat == RTAUDIO_SINT24)
	{
		Int32* out = (Int32*)outBuffer;
		if (info.inFormat == RTAUDIO_SINT8)
		{
			signed char* in = (signed char*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 16;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT16)
		{
			Int16* in = (Int16*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 8;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT24)
		{
			// Channel compensation and/or (de)interleaving only.
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT32)
		{
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] >>= 8;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT32)
		{
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)(in[info.inOffset[j]] * 8388607.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT64)
		{
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)(in[info.inOffset[j]] * 8388607.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
	else if (info.outFormat == RTAUDIO_SINT16)
	{
		Int16* out = (Int16*)outBuffer;
		if (info.inFormat == RTAUDIO_SINT8)
		{
			signed char* in = (signed char*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int16)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 8;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT16)
		{
			// Channel compensation and/or (de)interleaving only.
			Int16* in = (Int16*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT24)
		{
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int16)((in[info.inOffset[j]] >> 8) & 0x0000ffff);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT32)
		{
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int16)((in[info.inOffset[j]] >> 16) & 0x0000ffff);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT32)
		{
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int16)(in[info.inOffset[j]] * 32767.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT64)
		{
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int16)(in[info.inOffset[j]] * 32767.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
	else if (info.outFormat == RTAUDIO_SINT8)
	{
		signed char* out = (signed char*)outBuffer;
		if (info.inFormat == RTAUDIO_SINT8)
		{
			// Channel compensation and/or (de)interleaving only.
			signed char* in = (signed char*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		if (info.inFormat == RTAUDIO_SINT16)
		{
			Int16* in = (Int16*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (signed char)((in[info.inOffset[j]] >> 8) & 0x00ff);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT24)
		{
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (signed char)((in[info.inOffset[j]] >> 16) & 0x000000ff);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_SINT32)
		{
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (signed char)((in[info.inOffset[j]] >> 24) & 0x000000ff);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT32)
		{
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (signed char)(in[info.inOffset[j]] * 127.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == RTAUDIO_FLOAT64)
		{
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (signed char)(in[info.inOffset[j]] * 127.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
}

//static inline uint16_t bswap_16(uint16_t x) { return (x>>8) | (x<<8); }
//static inline uint32_t bswap_32(uint32_t x) { return (bswap_16(x&0xffff)<<16) | (bswap_16(x>>16)); }
//static inline uint64_t bswap_64(uint64_t x) { return (((unsigned long long)bswap_32(x&0xffffffffull))<<32) | (bswap_32(x>>32)); }

void AudioArchitecture::byteSwapBuffer(char* buffer, unsigned int samples, RtAudioFormat format)
{
	register char val;
	register char* ptr;

	ptr = buffer;
	if (format == RTAUDIO_SINT16)
	{
		for (unsigned int i = 0; i < samples; i++)
		{
			// Swap 1st and 2nd bytes.
			val = *(ptr);
			*(ptr) = *(ptr + 1);
			*(ptr + 1) = val;

			// Increment 2 bytes.
			ptr += 2;
		}
	}
	else if (format == RTAUDIO_SINT24 ||
			 format == RTAUDIO_SINT32 ||
			 format == RTAUDIO_FLOAT32)
	{
		for (unsigned int i = 0; i < samples; i++)
		{
			// Swap 1st and 4th bytes.
			val = *(ptr);
			*(ptr) = *(ptr + 3);
			*(ptr + 3) = val;

			// Swap 2nd and 3rd bytes.
			ptr += 1;
			val = *(ptr);
			*(ptr) = *(ptr + 1);
			*(ptr + 1) = val;

			// Increment 3 more bytes.
			ptr += 3;
		}
	}
	else if (format == RTAUDIO_FLOAT64)
	{
		for (unsigned int i = 0; i < samples; i++)
		{
			// Swap 1st and 8th bytes
			val = *(ptr);
			*(ptr) = *(ptr + 7);
			*(ptr + 7) = val;

			// Swap 2nd and 7th bytes
			ptr += 1;
			val = *(ptr);
			*(ptr) = *(ptr + 5);
			*(ptr + 5) = val;

			// Swap 3rd and 6th bytes
			ptr += 1;
			val = *(ptr);
			*(ptr) = *(ptr + 3);
			*(ptr + 3) = val;

			// Swap 4th and 5th bytes
			ptr += 1;
			val = *(ptr);
			*(ptr) = *(ptr + 1);
			*(ptr + 1) = val;

			// Increment 5 more bytes.
			ptr += 5;
		}
	}
}