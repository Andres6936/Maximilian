#include "Realtime/AudioArchitecture.hpp"

#include <Levin/Log.h>
#include <cstring>

using namespace Maximilian;

Maximilian::AudioArchitecture::AudioArchitecture()
{
	pthread_mutex_init(&stream_.mutex, nullptr);
	outputParameters.setDeviceId(getDefaultOutputDevice());
}

Maximilian::AudioArchitecture::~AudioArchitecture()
{
	pthread_mutex_destroy(&stream_.mutex);
}

void AudioArchitecture::assertThatStreamIsNotOpen()
{
	if (stream_.state != StreamState::STREAM_CLOSED)
	{
		Levin::Error() << "Assert: OpenStream, a stream is already open!" << Levin::endl;
		throw Exception("StreamAlreadyOpenException");
	}
}


void AudioArchitecture::openStream(void _functionUser(std::vector <double>&))
{
	assertThatStreamIsNotOpen();

	bool result = probeDeviceOpen(
			outputParameters.getDeviceId(),
			StreamMode::OUTPUT,
			outputParameters.getNChannels(),
			outputParameters.getFirstChannel());

	audioCallback = _functionUser;

	if (result == false)
	{ error(Exception::SYSTEM_ERROR); }

	if (getOptionsFlags() != AudioStreamFlags::None)
	{ options.setNumberOfBuffers(stream_.nBuffers); }
	stream_.state = StreamState::STREAM_STOPPED;
}

unsigned int AudioArchitecture::getDefaultInputDevice()
{
	// Should be implemented in subclasses if possible.
	return 0;
}

unsigned int AudioArchitecture::getDefaultOutputDevice()
{
	// Should be implemented in subclasses if possible.
	return 0;
}

void AudioArchitecture::closeStream()
{
	// MUST be implemented in subclasses!
	throw Exception("NotImplementedException");
}

bool AudioArchitecture::probeDeviceOpen(
		unsigned int device,
		StreamMode mode,
		unsigned int channels,
		unsigned int firstChannel)
{
	// MUST be implemented in subclasses!
	throw Exception("NotImplementedException");
}

void AudioArchitecture::tickStreamTime()
{
	// Subclasses that do not provide their own implementation of
	// getStreamTime should call this function once per buffer I/O to
	// provide basic stream time support.

	stream_.streamTime += (stream_.bufferSize * 1.0 / stream_.sampleRate);
}

long AudioArchitecture::getStreamLatency()
{
	verifyStream();

	long totalLatency = 0;
	if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
	{
		totalLatency = stream_.latency[0];
	}
	if (stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX)
	{
		totalLatency += stream_.latency[1];
	}

	return totalLatency;
}

double AudioArchitecture::getStreamTime()
{
	verifyStream();
	return stream_.streamTime;

}

unsigned int AudioArchitecture::getStreamSampleRate()
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
		Levin::Warn() << errorText_ << Levin::endl;
	}
	else if (type != Exception::WARNING)
	{
		throw Exception(errorText_);
	}
}

void AudioArchitecture::verifyStream()
{
	if (stream_.state == StreamState::STREAM_CLOSED)
	{
		Levin::Error() << "Audio Architecture: a stream is not open." << Levin::endl;

		throw Exception("StreamIsNotOpenException");
	}
}

unsigned int AudioArchitecture::formatBytes(AudioFormat _audioFormat)
{
	switch (_audioFormat)
	{

	case AudioFormat::SInt8:
		return 1;

	case AudioFormat::SInt16:
		return 2;

	case AudioFormat::SInt24:
	case AudioFormat::SInt32:
	case AudioFormat::Float32:
		return 4;

	case AudioFormat::Float64:
		return 8;
	}

	// Code inaccessible
	return 0;
}

void AudioArchitecture::setConvertInfo(StreamMode mode, unsigned int firstChannel)
{
	int index = (int)mode;

	if (mode == StreamMode::INPUT)
	{ // convert device to user buffer
		stream_.convertInfo[index].inJump = stream_.nDeviceChannels[1];
		stream_.convertInfo[index].outJump = stream_.nUserChannels[1];
		stream_.convertInfo[index].inFormat = stream_.deviceFormat[1];
		stream_.convertInfo[index].outFormat = stream_.userFormat;
	}
	else
	{ // convert user to device buffer
		stream_.convertInfo[index].inJump = stream_.nUserChannels[0];
		stream_.convertInfo[index].outJump = stream_.nDeviceChannels[0];
		stream_.convertInfo[index].inFormat = stream_.userFormat;
		stream_.convertInfo[index].outFormat = stream_.deviceFormat[0];
	}

	if (stream_.convertInfo[index].inJump < stream_.convertInfo[index].outJump)
	{
		stream_.convertInfo[index].channels = stream_.convertInfo[index].inJump;
	}
	else
	{
		stream_.convertInfo[index].channels = stream_.convertInfo[index].outJump;
	}

	// Set up the interleave/deinterleave offsets.
	if (stream_.deviceInterleaved[index] != stream_.userInterleaved)
	{
		if ((mode == StreamMode::OUTPUT && stream_.deviceInterleaved[index]) ||
			(mode == StreamMode::INPUT && stream_.userInterleaved))
		{
			for (int k = 0; k < stream_.convertInfo[index].channels; k++)
			{
				stream_.convertInfo[index].inOffset.push_back(k * stream_.bufferSize);
				stream_.convertInfo[index].outOffset.push_back(k);
				stream_.convertInfo[index].inJump = 1;
			}
		}
		else
		{
			for (int k = 0; k < stream_.convertInfo[index].channels; k++)
			{
				stream_.convertInfo[index].inOffset.push_back(k);
				stream_.convertInfo[index].outOffset.push_back(k * stream_.bufferSize);
				stream_.convertInfo[index].outJump = 1;
			}
		}
	}
	else
	{ // no (de)interleaving
		if (stream_.userInterleaved)
		{
			for (int k = 0; k < stream_.convertInfo[index].channels; k++)
			{
				stream_.convertInfo[index].inOffset.push_back(k);
				stream_.convertInfo[index].outOffset.push_back(k);
			}
		}
		else
		{
			for (int k = 0; k < stream_.convertInfo[index].channels; k++)
			{
				stream_.convertInfo[index].inOffset.push_back(k * stream_.bufferSize);
				stream_.convertInfo[index].outOffset.push_back(k * stream_.bufferSize);
				stream_.convertInfo[index].inJump = 1;
				stream_.convertInfo[index].outJump = 1;
			}
		}
	}

	// Add channel offset.
	if (firstChannel > 0)
	{
		if (stream_.deviceInterleaved[index])
		{
			if (mode == StreamMode::OUTPUT)
			{
				for (int k = 0; k < stream_.convertInfo[index].channels; k++)
				{
					stream_.convertInfo[index].outOffset[k] += firstChannel;
				}
			}
			else
			{
				for (int k = 0; k < stream_.convertInfo[index].channels; k++)
				{
					stream_.convertInfo[index].inOffset[k] += firstChannel;
				}
			}
		}
		else
		{
			if (mode == StreamMode::OUTPUT)
			{
				for (int k = 0; k < stream_.convertInfo[index].channels; k++)
				{
					stream_.convertInfo[index].outOffset[k] += (firstChannel * stream_.bufferSize);
				}
			}
			else
			{
				for (int k = 0; k < stream_.convertInfo[index].channels; k++)
				{
					stream_.convertInfo[index].inOffset[k] += (firstChannel * stream_.bufferSize);
				}
			}
		}
	}
}


template <typename T, typename O, typename I>
void AudioArchitecture::formatBufferTo(T _scale, O* outBuffer, I* inBuffer, ConvertInfo& info)
{
	for (unsigned int i = 0; i < stream_.bufferSize; i++)
	{
		for (unsigned int j = 0; j < info.channels; j++)
		{
			outBuffer[info.outOffset[j]] = (T)inBuffer[info.inOffset[j]];
			outBuffer[info.outOffset[j]] += 0.5;
			outBuffer[info.outOffset[j]] *= _scale;
		}

		inBuffer += info.inJump;
		outBuffer += info.outJump;
	}
}

template <typename T, typename O, typename I>
void AudioArchitecture::formatBufferOf24BitsTo(T _scale, O* outBuffer, I* inBuffer, ConvertInfo& info)
{
	for (unsigned int i = 0; i < stream_.bufferSize; i++)
	{
		for (unsigned int j = 0; j < info.channels; j++)
		{
			outBuffer[info.outOffset[j]] = (T)(inBuffer[info.inOffset[j]] & 0x00FFFFFF);
			outBuffer[info.outOffset[j]] += 0.5;
			outBuffer[info.outOffset[j]] *= _scale;
		}

		inBuffer += info.inJump;
		outBuffer += info.outJump;
	}
}

void AudioArchitecture::convertBuffer(char* outBuffer, char* inBuffer, ConvertInfo& info)
{
	// This function does format conversion, input/output channel compensation, and
	// data interleaving/deinterleaving.  24-bit integers are assumed to occupy
	// the lower three bytes of a 32-bit integer.

	// Clear our device buffer when in/out duplex device channels are different
	if (outBuffer == stream_.deviceBuffer.data() && stream_.mode == StreamMode::DUPLEX &&
		(stream_.nDeviceChannels[0] < stream_.nDeviceChannels[1]))
	{
		memset(outBuffer, 0, stream_.bufferSize * info.outJump * formatBytes(info.outFormat));
	}

	if (info.outFormat == AudioFormat::Float64)
	{
		auto* out = (Float64*)outBuffer;

		if (info.inFormat == AudioFormat::SInt8)
		{
			auto* in = (signed char*)inBuffer;
			Float64 scale = 1.0 / 127.5;

			formatBufferTo(scale, out, in, info);
		}
		else if (info.inFormat == AudioFormat::SInt16)
		{
			auto* in = (Int16*)inBuffer;
			Float64 scale = 1.0 / 32767.5;

			formatBufferTo(scale, out, in, info);
		}
		else if (info.inFormat == AudioFormat::SInt24)
		{
			Int32* in = (Int32*)inBuffer;
			Float64 scale = 1.0 / 8388607.5;

			formatBufferOf24BitsTo(scale, out, in, info);
		}
		else if (info.inFormat == AudioFormat::SInt32)
		{
			Int32* in = (Int32*)inBuffer;
			Float64 scale = 1.0 / 2147483647.5;

			formatBufferTo(scale, out, in, info);
		}
		else if (info.inFormat == AudioFormat::Float32)
		{
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float64)in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::Float64)
		{
			// Channel compensation and/or (de)interleaving only.
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
	else if (info.outFormat == AudioFormat::Float32)
	{
		Float32* out = (Float32*)outBuffer;

		if (info.inFormat == AudioFormat::SInt8)
		{
			signed char* in = (signed char*)inBuffer;
			Float32 scale = (Float32)(1.0 / 127.5);

			formatBufferTo(scale, out, in, info);
		}
		else if (info.inFormat == AudioFormat::SInt16)
		{
			Int16* in = (Int16*)inBuffer;
			Float32 scale = (Float32)(1.0 / 32767.5);

			formatBufferTo(scale, out, in, info);
		}
		else if (info.inFormat == AudioFormat::SInt24)
		{
			Int32* in = (Int32*)inBuffer;
			Float32 scale = (Float32)(1.0 / 8388607.5);

			formatBufferOf24BitsTo(scale, out, in, info);
		}
		else if (info.inFormat == AudioFormat::SInt32)
		{
			Int32* in = (Int32*)inBuffer;
			Float32 scale = (Float32)(1.0 / 2147483647.5);

			formatBufferTo(scale, out, in, info);
		}
		else if (info.inFormat == AudioFormat::Float32)
		{
			// Channel compensation and/or (de)interleaving only.
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::Float64)
		{
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Float32)in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
	else if (info.outFormat == AudioFormat::SInt32)
	{
		Int32* out = (Int32*)outBuffer;
		if (info.inFormat == AudioFormat::SInt8)
		{
			signed char* in = (signed char*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 24;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt16)
		{
			Int16* in = (Int16*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 16;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt24)
		{ // Hmmm ... we could just leave it in the lower 3 bytes
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 8;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt32)
		{
			// Channel compensation and/or (de)interleaving only.
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::Float32)
		{
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)(in[info.inOffset[j]] * 2147483647.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::Float64)
		{
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)(in[info.inOffset[j]] * 2147483647.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
	else if (info.outFormat == AudioFormat::SInt24)
	{
		Int32* out = (Int32*)outBuffer;
		if (info.inFormat == AudioFormat::SInt8)
		{
			signed char* in = (signed char*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 16;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt16)
		{
			Int16* in = (Int16*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 8;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt24)
		{
			// Channel compensation and/or (de)interleaving only.
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt32)
		{
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)in[info.inOffset[j]];
					out[info.outOffset[j]] >>= 8;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::Float32)
		{
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)(in[info.inOffset[j]] * 8388607.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::Float64)
		{
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int32)(in[info.inOffset[j]] * 8388607.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
	else if (info.outFormat == AudioFormat::SInt16)
	{
		Int16* out = (Int16*)outBuffer;
		if (info.inFormat == AudioFormat::SInt8)
		{
			signed char* in = (signed char*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int16)in[info.inOffset[j]];
					out[info.outOffset[j]] <<= 8;
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt16)
		{
			// Channel compensation and/or (de)interleaving only.
			Int16* in = (Int16*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt24)
		{
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int16)((in[info.inOffset[j]] >> 8) & 0x0000ffff);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt32)
		{
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int16)((in[info.inOffset[j]] >> 16) & 0x0000ffff);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::Float32)
		{
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int16)(in[info.inOffset[j]] * 32767.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::Float64)
		{
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (Int16)(in[info.inOffset[j]] * 32767.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
	else if (info.outFormat == AudioFormat::SInt8)
	{
		signed char* out = (signed char*)outBuffer;
		if (info.inFormat == AudioFormat::SInt8)
		{
			// Channel compensation and/or (de)interleaving only.
			signed char* in = (signed char*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = in[info.inOffset[j]];
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		if (info.inFormat == AudioFormat::SInt16)
		{
			Int16* in = (Int16*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (signed char)((in[info.inOffset[j]] >> 8) & 0x00ff);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt24)
		{
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (signed char)((in[info.inOffset[j]] >> 16) & 0x000000ff);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::SInt32)
		{
			Int32* in = (Int32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (signed char)((in[info.inOffset[j]] >> 24) & 0x000000ff);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::Float32)
		{
			Float32* in = (Float32*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (signed char)(in[info.inOffset[j]] * 127.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
		else if (info.inFormat == AudioFormat::Float64)
		{
			Float64* in = (Float64*)inBuffer;
			for (unsigned int i = 0; i < stream_.bufferSize; i++)
			{
				for (unsigned int j = 0; j < info.channels; j++)
				{
					out[info.outOffset[j]] = (signed char)(in[info.inOffset[j]] * 127.5 - 0.5);
				}
				in += info.inJump;
				out += info.outJump;
			}
		}
	}
}

void AudioArchitecture::byteSwapBuffer(char* buffer, unsigned int samples, AudioFormat format)
{
	char val;
	char* ptr;

	ptr = buffer;
	if (format == AudioFormat::SInt16)
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
	else if (format == AudioFormat::SInt24 ||
			 format == AudioFormat::SInt32 ||
			 format == AudioFormat::Float32)
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
	else if (format == AudioFormat::Float64)
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

// Getters

unsigned int AudioArchitecture::getSampleRate() const
{
	return sampleRate;
}

unsigned int AudioArchitecture::getBufferFrames() const
{
	return bufferFrames;
}

// Setters

void AudioArchitecture::setBufferFrames(unsigned int _bufferFrames)
{
	bufferFrames = _bufferFrames;
}

AudioStreamFlags AudioArchitecture::getOptionsFlags() const
{
	return options.getFlags();
}

int AudioArchitecture::getOptionsPriority() const
{
	return options.getPriority();
}

unsigned int AudioArchitecture::getNumberOfBuffersOptions() const
{
	return options.getNumberOfBuffers();
}

AudioFormat AudioArchitecture::getAudioFormat() const
{
	return format;
}