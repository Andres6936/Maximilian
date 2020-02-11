#include "Realtime/LinuxAlsa.hpp"

#include <alsa/asoundlib.h>

#include <array>
#include <climits>

using namespace Maximilian;

// A structure to hold various information related to the ALSA API
// implementation.
struct AlsaHandle
{
	snd_pcm_t* handles[2];
	bool synchronized;
	bool xrun[2];
	pthread_cond_t runnable_cv;
	bool runnable;

	AlsaHandle()
			: synchronized(false), runnable(false)
	{
		xrun[0] = false;
		xrun[1] = false;
	}
};


extern "C" void* alsaCallbackHandler(void* ptr)
{
	CallbackInfo* info = (CallbackInfo*)ptr;
	LinuxAlsa* object = (LinuxAlsa*)info->object;
	bool* isRunning = &info->isRunning;

	while (*isRunning == true)
	{
		pthread_testcancel();
		object->callbackEvent();
	}

	pthread_exit(nullptr);
}


LinuxAlsa::~LinuxAlsa()
{
	if (stream_.state != StreamState::STREAM_CLOSED)
	{ closeStream(); }
}

unsigned int LinuxAlsa::getDeviceCount()
{
	unsigned numberOfDevices = 0;

	// Count cards and devices
	int card = -1;
	snd_card_next(&card);

	snd_ctl_t* handle = nullptr;

	if (card >= 0)
	{
		std::array <char, 64> name{ };

		sprintf(name.data(), "hw:%d", card);

		int result = snd_ctl_open(&handle, name.data(), 0);

		if (result < 0)
		{
			Levin::Warn() << "Linux Alsa: getDeviceCount, control open, card = " << card
						  << ", " << snd_strerror(result) << "." << Levin::endl;

			snd_ctl_close(handle);
			// Free the cache for avoid memory leak
			snd_config_update_free_global();
			snd_card_next(&card);

			// Return zero device in this point
			return numberOfDevices;
		}

		int subDevice = -1;

		while (true)
		{
			result = snd_ctl_pcm_next_device(handle, &subDevice);

			if (result < 0)
			{
				Levin::Warn() << "Linux Alsa: getDeviceCount, control next device, card = " << card
							  << ", " << snd_strerror(result) << "." << Levin::endl;
				break;
			}

			if (subDevice < 0)
			{
				break;
			}

			numberOfDevices++;
		}
	}

	if (handle != nullptr)
	{
		snd_ctl_close(handle);
		// Free the cache for avoid memory leak
		snd_config_update_free_global();
	}

	return numberOfDevices;
}

Audio::DeviceInfo LinuxAlsa::getDeviceInfo(int device)
{
	if (device >= getDeviceCount())
	{
		throw Exception("DeviceInvalidException");
	}

	Audio::DeviceInfo info;
	info.probed = false;

	int result;
	int subDevice = device;

	std::array <char, 64> name{ };

	snd_ctl_t* handle;

	// Count cards and devices
	int card = -1;
	snd_card_next(&card);

	if (card >= 0)
	{
		sprintf(name.data(), "hw:%d", card);
		snd_ctl_open(&handle, name.data(), SND_CTL_NONBLOCK);
		snd_ctl_pcm_next_device(handle, &subDevice);
		sprintf(name.data(), "hw:%d,%d", card, subDevice);
	}

	int openMode = SND_PCM_ASYNC;
	snd_pcm_stream_t stream;
	snd_pcm_info_t* pcminfo;
	snd_pcm_info_alloca(&pcminfo);
	snd_pcm_t* phandle;
	snd_pcm_hw_params_t* params;
	snd_pcm_hw_params_alloca(&params);

	// First try for playback
	stream = SND_PCM_STREAM_PLAYBACK;
	snd_pcm_info_set_device(pcminfo, subDevice);
	snd_pcm_info_set_subdevice(pcminfo, 0);
	snd_pcm_info_set_stream(pcminfo, stream);

	result = snd_ctl_pcm_info(handle, pcminfo);
	if (result < 0)
	{
		// Device probably doesn't support playback.
		goto captureProbe;
	}

	result = snd_pcm_open(&phandle, name.data(), stream, openMode | SND_PCM_NONBLOCK);
	if (result < 0)
	{
		errorStream_ << "RtApiAlsa::getDeviceInfo: snd_pcm_open error for device (" << name.data() << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		goto captureProbe;
	}

	// The device is open ... fill the parameter structure.
	result = snd_pcm_hw_params_any(phandle, params);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::getDeviceInfo: snd_pcm_hw_params error for device (" << name.data() << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		goto captureProbe;
	}

	// Get output channel information.
	unsigned int value;
	result = snd_pcm_hw_params_get_channels_max(params, &value);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::getDeviceInfo: error getting device (" << name.data() << ") output channels, "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		goto captureProbe;
	}
	info.outputChannels = value;
	snd_pcm_close(phandle);

captureProbe:
	// Now try for capture
	stream = SND_PCM_STREAM_CAPTURE;
	snd_pcm_info_set_stream(pcminfo, stream);

	result = snd_ctl_pcm_info(handle, pcminfo);
	snd_ctl_close(handle);
	if (result < 0)
	{
		// Device probably doesn't support capture.
		if (info.outputChannels == 0)
		{ return info; }
		goto probeParameters;
	}

	result = snd_pcm_open(&phandle, name.data(), stream, openMode | SND_PCM_NONBLOCK);
	if (result < 0)
	{
		errorStream_ << "RtApiAlsa::getDeviceInfo: snd_pcm_open error for device (" << name.data() << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		if (info.outputChannels == 0)
		{ return info; }
		goto probeParameters;
	}

	// The device is open ... fill the parameter structure.
	result = snd_pcm_hw_params_any(phandle, params);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::getDeviceInfo: snd_pcm_hw_params error for device (" << name.data() << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		if (info.outputChannels == 0)
		{ return info; }
		goto probeParameters;
	}

	result = snd_pcm_hw_params_get_channels_max(params, &value);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::getDeviceInfo: error getting device (" << name.data() << ") input channels, "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		if (info.outputChannels == 0)
		{ return info; }
		goto probeParameters;
	}
	info.inputChannels = value;
	snd_pcm_close(phandle);

	// If device opens for both playback and capture, we determine the channels.
	if (info.outputChannels > 0 && info.inputChannels > 0)
	{
		info.duplexChannels = (info.outputChannels > info.inputChannels) ? info.inputChannels : info.outputChannels;
	}

	// ALSA doesn't provide default devices so we'll use the first available one.
	if (device == 0 && info.outputChannels > 0)
	{
		info.isDefaultOutput = true;
	}
	if (device == 0 && info.inputChannels > 0)
	{
		info.isDefaultInput = true;
	}

probeParameters:
	// At this point, we just need to figure out the supported data
	// formats and sample rates.  We'll proceed by opening the device in
	// the direction with the maximum number of channels, or playback if
	// they are equal.  This might limit our sample rate options, but so
	// be it.

	if (info.outputChannels >= info.inputChannels)
	{
		stream = SND_PCM_STREAM_PLAYBACK;
	}
	else
	{
		stream = SND_PCM_STREAM_CAPTURE;
	}
	snd_pcm_info_set_stream(pcminfo, stream);

	result = snd_pcm_open(&phandle, name.data(), stream, openMode | SND_PCM_NONBLOCK);
	if (result < 0)
	{
		errorStream_ << "RtApiAlsa::getDeviceInfo: snd_pcm_open error for device (" << name.data() << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		return info;
	}

	// The device is open ... fill the parameter structure.
	result = snd_pcm_hw_params_any(phandle, params);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::getDeviceInfo: snd_pcm_hw_params error for device (" << name.data() << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		return info;
	}

	// Test our discrete set of sample rate values.
	info.sampleRates.clear();
	for (unsigned int i = 0; i < MAX_SAMPLE_RATES; i++)
	{
		if (snd_pcm_hw_params_test_rate(phandle, params, SAMPLE_RATES[i], 0) == 0)
		{
			info.sampleRates.push_back(SAMPLE_RATES[i]);
		}
	}
	if (info.sampleRates.size() == 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::getDeviceInfo: no supported sample rates found for device (" << name.data() << ").";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		return info;
	}

	// Probe the supported data formats ... we don't care about endian-ness just yet
	snd_pcm_format_t format;
	info.nativeFormats = 0;
	format = SND_PCM_FORMAT_S8;
	if (snd_pcm_hw_params_test_format(phandle, params, format) == 0)
	{
		info.nativeFormats |= RTAUDIO_SINT8;
	}
	format = SND_PCM_FORMAT_S16;
	if (snd_pcm_hw_params_test_format(phandle, params, format) == 0)
	{
		info.nativeFormats |= RTAUDIO_SINT16;
	}
	format = SND_PCM_FORMAT_S24;
	if (snd_pcm_hw_params_test_format(phandle, params, format) == 0)
	{
		info.nativeFormats |= RTAUDIO_SINT24;
	}
	format = SND_PCM_FORMAT_S32;
	if (snd_pcm_hw_params_test_format(phandle, params, format) == 0)
	{
		info.nativeFormats |= RTAUDIO_SINT32;
	}
	format = SND_PCM_FORMAT_FLOAT;
	if (snd_pcm_hw_params_test_format(phandle, params, format) == 0)
	{
		info.nativeFormats |= RTAUDIO_FLOAT32;
	}
	format = SND_PCM_FORMAT_FLOAT64;
	if (snd_pcm_hw_params_test_format(phandle, params, format) == 0)
	{
		info.nativeFormats |= RTAUDIO_FLOAT64;
	}

	// Check that we have at least one supported format
	if (info.nativeFormats == 0)
	{
		errorStream_ << "RtApiAlsa::getDeviceInfo: pcm device (" << name.data()
					 << ") data format not supported by RtAudio.";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		return info;
	}

	// Get the device name
	char* cardname;
	result = snd_card_get_name(card, &cardname);
	if (result >= 0)
	{
		sprintf(name.data(), "hw:%s,%d", cardname, subDevice);
	}
	info.name = name.data();

	// That's all ... close the device and return
	snd_pcm_close(phandle);
	info.probed = true;
	return info;
}

void LinuxAlsa::saveDeviceInfo()
{
	devices_.clear();

	unsigned int nDevices = getDeviceCount();
	devices_.resize(nDevices);
	for (unsigned int i = 0; i < nDevices; i++)
	{
		devices_[i] = getDeviceInfo(i);
	}
}

bool LinuxAlsa::probeDeviceOpen(unsigned int device, StreamMode mode, unsigned int channels,
		unsigned int firstChannel, unsigned int sampleRate,
		AudioFormat format, unsigned int* bufferSize,
		Audio::StreamOptions* options)
{
#if defined(__RTAUDIO_DEBUG__)
	snd_output_t *out;
  snd_output_stdio_attach(&out, stderr, 0);
#endif

	// Convert the StreamMode enum to int for use in arrays
	int index = (int)mode;

	// I'm not using the "plug" interface ... too much inconsistent behavior.

	unsigned nDevices = 0;
	int result, subdevice, card;
	char name[64];
	snd_ctl_t* chandle;

	if (options && options->flags & RTAUDIO_ALSA_USE_DEFAULT)
	{
		snprintf(name, sizeof(name), "%s", "default");
	}
	else
	{
		// Count cards and devices
		card = -1;
		snd_card_next(&card);
		while (card >= 0)
		{
			sprintf(name, "hw:%d", card);
			result = snd_ctl_open(&chandle, name, SND_CTL_NONBLOCK);
			if (result < 0)
			{
				errorStream_ << "RtApiAlsa::probeDeviceOpen: control open, card = " << card << ", "
							 << snd_strerror(result) << ".";
				errorText_ = errorStream_.str();
				return FAILURE;
			}
			subdevice = -1;
			while (1)
			{
				result = snd_ctl_pcm_next_device(chandle, &subdevice);
				if (result < 0)
				{ break; }
				if (subdevice < 0)
				{ break; }
				if (nDevices == device)
				{
					sprintf(name, "hw:%d,%d", card, subdevice);
					snd_ctl_close(chandle);
					goto foundDevice;
				}
				nDevices++;
			}
			snd_ctl_close(chandle);
			snd_card_next(&card);
		}

		if (nDevices == 0)
		{
			// This should not happen because a check is made before this function is called.
			errorText_ = "RtApiAlsa::probeDeviceOpen: no devices found!";
			return FAILURE;
		}

		if (device >= nDevices)
		{
			// This should not happen because a check is made before this function is called.
			errorText_ = "RtApiAlsa::probeDeviceOpen: device ID is invalid!";
			return FAILURE;
		}
	}

foundDevice:

	// The getDeviceInfo() function will not work for a device that is
	// already open.  Thus, we'll probe the system before opening a
	// stream and save the results for use by getDeviceInfo().
	if (mode == StreamMode::OUTPUT || (mode == StreamMode::INPUT && stream_.mode != StreamMode::OUTPUT))
	{ // only do once
		this->saveDeviceInfo();
	}

	snd_pcm_stream_t stream;
	if (mode == StreamMode::OUTPUT)
	{
		stream = SND_PCM_STREAM_PLAYBACK;
	}
	else
	{
		stream = SND_PCM_STREAM_CAPTURE;
	}

	snd_pcm_t* phandle;
	int openMode = SND_PCM_ASYNC;
	result = snd_pcm_open(&phandle, name, stream, openMode);
	if (result < 0)
	{
		if (mode == StreamMode::OUTPUT)
		{
			errorStream_ << "RtApiAlsa::probeDeviceOpen: pcm device (" << name << ") won't open for output.";
		}
		else
		{
			errorStream_ << "RtApiAlsa::probeDeviceOpen: pcm device (" << name << ") won't open for input.";
		}
		errorText_ = errorStream_.str();
		return FAILURE;
	}

	// Fill the parameter structure.
	snd_pcm_hw_params_t* hw_params;
	snd_pcm_hw_params_alloca(&hw_params);
	result = snd_pcm_hw_params_any(phandle, hw_params);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error getting pcm device (" << name << ") parameters, "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

#if defined(__RTAUDIO_DEBUG__)
	fprintf( stderr, "\nRtApiAlsa: dump hardware params just after device open:\n\n" );
  snd_pcm_hw_params_dump( hw_params, out );
#endif

	// Set access ... check user preference.
	if (options && options->flags & RTAUDIO_NONINTERLEAVED)
	{
		stream_.userInterleaved = false;
		result = snd_pcm_hw_params_set_access(phandle, hw_params, SND_PCM_ACCESS_RW_NONINTERLEAVED);
		if (result < 0)
		{
			result = snd_pcm_hw_params_set_access(phandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
			stream_.deviceInterleaved[index] = true;
		}
		else
		{
			stream_.deviceInterleaved[index] = false;
		}
	}
	else
	{
		stream_.userInterleaved = true;
		result = snd_pcm_hw_params_set_access(phandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
		if (result < 0)
		{
			result = snd_pcm_hw_params_set_access(phandle, hw_params, SND_PCM_ACCESS_RW_NONINTERLEAVED);
			stream_.deviceInterleaved[index] = false;
		}
		else
		{
			stream_.deviceInterleaved[index] = true;
		}
	}

	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting pcm device (" << name << ") access, "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

	// Determine how to set the device format.
	stream_.userFormat = format;
	snd_pcm_format_t deviceFormat = SND_PCM_FORMAT_UNKNOWN;

	if (format == RTAUDIO_SINT8)
	{
		deviceFormat = SND_PCM_FORMAT_S8;
	}
	else if (format == RTAUDIO_SINT16)
	{
		deviceFormat = SND_PCM_FORMAT_S16;
	}
	else if (format == RTAUDIO_SINT24)
	{
		deviceFormat = SND_PCM_FORMAT_S24;
	}
	else if (format == RTAUDIO_SINT32)
	{
		deviceFormat = SND_PCM_FORMAT_S32;
	}
	else if (format == RTAUDIO_FLOAT32)
	{
		deviceFormat = SND_PCM_FORMAT_FLOAT;
	}
	else if (format == RTAUDIO_FLOAT64)
	{
		deviceFormat = SND_PCM_FORMAT_FLOAT64;
	}

	if (snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat) == 0)
	{
		stream_.deviceFormat[index] = format;
		goto setFormat;
	}

	// The user requested format is not natively supported by the device.
	deviceFormat = SND_PCM_FORMAT_FLOAT64;
	if (snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat) == 0)
	{
		stream_.deviceFormat[index] = RTAUDIO_FLOAT64;
		goto setFormat;
	}

	deviceFormat = SND_PCM_FORMAT_FLOAT;
	if (snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat) == 0)
	{
		stream_.deviceFormat[index] = RTAUDIO_FLOAT32;
		goto setFormat;
	}

	deviceFormat = SND_PCM_FORMAT_S32;
	if (snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat) == 0)
	{
		stream_.deviceFormat[index] = RTAUDIO_SINT32;
		goto setFormat;
	}

	deviceFormat = SND_PCM_FORMAT_S24;
	if (snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat) == 0)
	{
		stream_.deviceFormat[index] = RTAUDIO_SINT24;
		goto setFormat;
	}

	deviceFormat = SND_PCM_FORMAT_S16;
	if (snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat) == 0)
	{
		stream_.deviceFormat[index] = RTAUDIO_SINT16;
		goto setFormat;
	}

	deviceFormat = SND_PCM_FORMAT_S8;
	if (snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat) == 0)
	{
		stream_.deviceFormat[index] = RTAUDIO_SINT8;
		goto setFormat;
	}

	// If we get here, no supported format was found.
	errorStream_ << "RtApiAlsa::probeDeviceOpen: pcm device " << device << " data format not supported by RtAudio.";
	errorText_ = errorStream_.str();
	return FAILURE;

setFormat:
	result = snd_pcm_hw_params_set_format(phandle, hw_params, deviceFormat);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting pcm device (" << name << ") data format, "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

	// Determine whether byte-swaping is necessary.
	stream_.doByteSwap[index] = false;
	if (deviceFormat != SND_PCM_FORMAT_S8)
	{
		result = snd_pcm_format_cpu_endian(deviceFormat);
		if (result == 0)
		{
			stream_.doByteSwap[index] = true;
		}
		else if (result < 0)
		{
			snd_pcm_close(phandle);
			errorStream_ << "RtApiAlsa::probeDeviceOpen: error getting pcm device (" << name << ") endian-ness, "
						 << snd_strerror(result) << ".";
			errorText_ = errorStream_.str();
			return FAILURE;
		}
	}

	// Set the sample rate.
	result = snd_pcm_hw_params_set_rate_near(phandle, hw_params, (unsigned int*)&sampleRate, 0);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting sample rate on device (" << name << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

	// Determine the number of channels for this device.  We support a possible
	// minimum device channel number > than the value requested by the user.
	stream_.nUserChannels[index] = channels;
	unsigned int value;
	result = snd_pcm_hw_params_get_channels_max(hw_params, &value);
	unsigned int deviceChannels = value;
	if (result < 0 || deviceChannels < channels + firstChannel)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: requested channel parameters not supported by device (" << name
					 << "), " << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

	result = snd_pcm_hw_params_get_channels_min(hw_params, &value);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error getting minimum channels for device (" << name << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}
	deviceChannels = value;
	if (deviceChannels < channels + firstChannel)
	{ deviceChannels = channels + firstChannel; }
	stream_.nDeviceChannels[index] = deviceChannels;

	// Set the device channels.
	result = snd_pcm_hw_params_set_channels(phandle, hw_params, deviceChannels);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting channels for device (" << name << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

	// Set the buffer (or period) size.
	int dir = 0;
	snd_pcm_uframes_t periodSize = *bufferSize;
	result = snd_pcm_hw_params_set_period_size_near(phandle, hw_params, &periodSize, &dir);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting period size for device (" << name << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}
	*bufferSize = periodSize;

	// Set the buffer number, which in ALSA is referred to as the "period".
	unsigned int periods = 0;
	if (options && options->flags & RTAUDIO_MINIMIZE_LATENCY)
	{ periods = 2; }
	if (options && options->numberOfBuffers > 0)
	{ periods = options->numberOfBuffers; }
	if (periods < 2)
	{ periods = 4; } // a fairly safe default value
	result = snd_pcm_hw_params_set_periods_near(phandle, hw_params, &periods, &dir);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting periods for device (" << name << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

	// If attempting to setup a duplex stream, the bufferSize parameter
	// MUST be the same in both directions!
	if (stream_.mode == StreamMode::OUTPUT && mode == StreamMode::INPUT && *bufferSize != stream_.bufferSize)
	{
		errorStream_ << "RtApiAlsa::probeDeviceOpen: system error setting buffer size for duplex stream on device ("
					 << name << ").";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

	stream_.bufferSize = *bufferSize;

	// Install the hardware configuration
	result = snd_pcm_hw_params(phandle, hw_params);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error installing hardware configuration on device (" << name
					 << "), " << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

#if defined(__RTAUDIO_DEBUG__)
	fprintf(stderr, "\nRtApiAlsa: dump hardware params after installation:\n\n");
  snd_pcm_hw_params_dump( hw_params, out );
#endif

	// Set the software configuration to fill buffers with zeros and prevent device stopping on xruns.
	snd_pcm_sw_params_t* sw_params = NULL;
	snd_pcm_sw_params_alloca(&sw_params);
	snd_pcm_sw_params_current(phandle, sw_params);
	snd_pcm_sw_params_set_start_threshold(phandle, sw_params, *bufferSize);
	snd_pcm_sw_params_set_stop_threshold(phandle, sw_params, ULONG_MAX);
	snd_pcm_sw_params_set_silence_threshold(phandle, sw_params, 0);

	// The following two settings were suggested by Theo Veenker
	//snd_pcm_sw_params_set_avail_min( phandle, sw_params, *bufferSize );
	//snd_pcm_sw_params_set_xfer_align( phandle, sw_params, 1 );

	// here are two options for a fix
	//snd_pcm_sw_params_set_silence_size( phandle, sw_params, ULONG_MAX );
	snd_pcm_uframes_t val;
	snd_pcm_sw_params_get_boundary(sw_params, &val);
	snd_pcm_sw_params_set_silence_size(phandle, sw_params, val);

	result = snd_pcm_sw_params(phandle, sw_params);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error installing software configuration on device (" << name
					 << "), " << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

#if defined(__RTAUDIO_DEBUG__)
	fprintf(stderr, "\nRtApiAlsa: dump software params after installation:\n\n");
  snd_pcm_sw_params_dump( sw_params, out );
#endif

	// Set flags for buffer conversion
	stream_.doConvertBuffer[index] = false;
	if (stream_.userFormat != stream_.deviceFormat[index])
	{
		stream_.doConvertBuffer[index] = true;
	}
	if (stream_.nUserChannels[index] < stream_.nDeviceChannels[index])
	{
		stream_.doConvertBuffer[index] = true;
	}
	if (stream_.userInterleaved != stream_.deviceInterleaved[index] &&
		stream_.nUserChannels[index] > 1)
	{
		stream_.doConvertBuffer[index] = true;
	}

	// Allocate the ApiHandle if necessary and then save.
	AlsaHandle* apiInfo = 0;
	if (stream_.apiHandle == 0)
	{
		try
		{
			apiInfo = (AlsaHandle*)new AlsaHandle;
		}
		catch (std::bad_alloc&)
		{
			errorText_ = "RtApiAlsa::probeDeviceOpen: error allocating AlsaHandle memory.";
			goto error;
		}

		if (pthread_cond_init(&apiInfo->runnable_cv, NULL))
		{
			errorText_ = "RtApiAlsa::probeDeviceOpen: error initializing pthread condition variable.";
			goto error;
		}

		stream_.apiHandle = (void*)apiInfo;
		apiInfo->handles[0] = 0;
		apiInfo->handles[1] = 0;
	}
	else
	{
		apiInfo = (AlsaHandle*)stream_.apiHandle;
	}
	apiInfo->handles[index] = phandle;

	// Allocate necessary internal buffers.
	unsigned long bufferBytes;
	bufferBytes = stream_.nUserChannels[index] * *bufferSize * formatBytes(stream_.userFormat);
	stream_.userBuffer[index] = (char*)calloc(bufferBytes, 1);
	if (stream_.userBuffer[index] == NULL)
	{
		errorText_ = "RtApiAlsa::probeDeviceOpen: error allocating user buffer memory.";
		goto error;
	}

	if (stream_.doConvertBuffer[index])
	{

		bool makeBuffer = true;
		bufferBytes = stream_.nDeviceChannels[index] * formatBytes(stream_.deviceFormat[index]);
		if (mode == StreamMode::INPUT)
		{
			if (stream_.mode == StreamMode::OUTPUT && stream_.deviceBuffer)
			{
				unsigned long bytesOut = stream_.nDeviceChannels[0] * formatBytes(stream_.deviceFormat[0]);
				if (bufferBytes <= bytesOut)
				{ makeBuffer = false; }
			}
		}

		if (makeBuffer)
		{
			bufferBytes *= *bufferSize;
			if (stream_.deviceBuffer)
			{ free(stream_.deviceBuffer); }
			stream_.deviceBuffer = (char*)calloc(bufferBytes, 1);
			if (stream_.deviceBuffer == NULL)
			{
				errorText_ = "RtApiAlsa::probeDeviceOpen: error allocating device buffer memory.";
				goto error;
			}
		}
	}

	stream_.sampleRate = sampleRate;
	stream_.nBuffers = periods;
	stream_.device[index] = device;
	stream_.state = StreamState::STREAM_STOPPED;

	// Setup the buffer conversion information structure.
	if (stream_.doConvertBuffer[index])
	{ setConvertInfo(mode, firstChannel); }

	// Setup thread if necessary.
	if (stream_.mode == StreamMode::OUTPUT && mode == StreamMode::INPUT)
	{
		// We had already set up an output stream.
		stream_.mode = StreamMode::DUPLEX;
		// Link the streams if possible.
		apiInfo->synchronized = false;
		if (snd_pcm_link(apiInfo->handles[0], apiInfo->handles[1]) == 0)
		{
			apiInfo->synchronized = true;
		}
		else
		{
			errorText_ = "RtApiAlsa::probeDeviceOpen: unable to synchronize input and output devices.";
			error(Exception::WARNING);
		}
	}
	else
	{
		stream_.mode = mode;

		// Setup callback thread.
		stream_.callbackInfo.object = (void*)this;

		// Set the thread attributes for joinable and realtime scheduling
		// priority (optional).  The higher priority will only take affect
		// if the program is run as root or suid. Note, under Linux
		// processes with CAP_SYS_NICE privilege, a user can change
		// scheduling policy and priority (thus need not be root). See
		// POSIX "capabilities".
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
#ifdef SCHED_RR // Undefined with some OSes (eg: NetBSD 1.6.x with GNU Pthread)
		if (options && options->flags & RTAUDIO_SCHEDULE_REALTIME)
		{
			struct sched_param param;
			int priority = options->priority;
			int min = sched_get_priority_min(SCHED_RR);
			int max = sched_get_priority_max(SCHED_RR);
			if (priority < min)
			{ priority = min; }
			else if (priority > max)
			{ priority = max; }
			param.sched_priority = priority;
			pthread_attr_setschedparam(&attr, &param);
			pthread_attr_setschedpolicy(&attr, SCHED_RR);
		}
		else
		{
			pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
		}
#else
		pthread_attr_setschedpolicy( &attr, SCHED_OTHER );
#endif

		stream_.callbackInfo.isRunning = true;
		result = pthread_create(&stream_.callbackInfo.thread, &attr, alsaCallbackHandler, &stream_.callbackInfo);
		pthread_attr_destroy(&attr);
		if (result)
		{
			stream_.callbackInfo.isRunning = false;
			errorText_ = "RtApiAlsa::error creating callback thread!";
			goto error;
		}
	}

	return SUCCESS;

error:
	if (apiInfo)
	{
		pthread_cond_destroy(&apiInfo->runnable_cv);
		if (apiInfo->handles[0])
		{ snd_pcm_close(apiInfo->handles[0]); }
		if (apiInfo->handles[1])
		{ snd_pcm_close(apiInfo->handles[1]); }
		delete apiInfo;
		stream_.apiHandle = 0;
	}

	for (auto& i : stream_.userBuffer)
	{
		if (i)
		{
			free(i);
		}
	}

	if (stream_.deviceBuffer)
	{
		free(stream_.deviceBuffer);
		stream_.deviceBuffer = 0;
	}

	return FAILURE;
}

void LinuxAlsa::closeStream()
{
	if (stream_.state == StreamState::STREAM_CLOSED)
	{
		errorText_ = "RtApiAlsa::closeStream(): no open stream to close!";
		error(Exception::WARNING);
		return;
	}

	AlsaHandle* apiInfo = (AlsaHandle*)stream_.apiHandle;
	stream_.callbackInfo.isRunning = false;
	pthread_mutex_lock(&stream_.mutex);
	if (stream_.state == StreamState::STREAM_STOPPED)
	{
		apiInfo->runnable = true;
		pthread_cond_signal(&apiInfo->runnable_cv);
	}
	pthread_mutex_unlock(&stream_.mutex);
	pthread_join(stream_.callbackInfo.thread, NULL);

	if (stream_.state == StreamState::STREAM_RUNNING)
	{
		stream_.state = StreamState::STREAM_STOPPED;
		if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
		{
			snd_pcm_drop(apiInfo->handles[0]);
		}
		if (stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX)
		{
			snd_pcm_drop(apiInfo->handles[1]);
		}
	}

	if (apiInfo)
	{
		pthread_cond_destroy(&apiInfo->runnable_cv);
		if (apiInfo->handles[0])
		{ snd_pcm_close(apiInfo->handles[0]); }
		if (apiInfo->handles[1])
		{ snd_pcm_close(apiInfo->handles[1]); }
		delete apiInfo;
		stream_.apiHandle = 0;
	}

	for (auto& i : stream_.userBuffer)
	{
		if (i)
		{
			free(i);
		}
	}

	if (stream_.deviceBuffer)
	{
		free(stream_.deviceBuffer);
		stream_.deviceBuffer = 0;
	}

	stream_.mode = StreamMode::UNINITIALIZED;
	stream_.state = StreamState::STREAM_CLOSED;
}

void LinuxAlsa::startStream()
{
	// This method calls snd_pcm_prepare if the device isn't already in that state.

	verifyStream();
	if (stream_.state == StreamState::STREAM_RUNNING)
	{
		errorText_ = "RtApiAlsa::startStream(): the stream is already running!";
		error(Exception::WARNING);
		return;
	}

	pthread_mutex_lock(&stream_.mutex);

	int result = 0;
	snd_pcm_state_t state;
	AlsaHandle* apiInfo = (AlsaHandle*)stream_.apiHandle;
	snd_pcm_t** handle = (snd_pcm_t**)apiInfo->handles;
	if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
	{
		state = snd_pcm_state(handle[0]);
		if (state != SND_PCM_STATE_PREPARED)
		{
			result = snd_pcm_prepare(handle[0]);
			if (result < 0)
			{
				errorStream_ << "RtApiAlsa::startStream: error preparing output pcm device, " << snd_strerror(result)
							 << ".";
				errorText_ = errorStream_.str();
				goto unlock;
			}
		}
	}

	if ((stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX) && !apiInfo->synchronized)
	{
		state = snd_pcm_state(handle[1]);
		if (state != SND_PCM_STATE_PREPARED)
		{
			result = snd_pcm_prepare(handle[1]);
			if (result < 0)
			{
				errorStream_ << "RtApiAlsa::startStream: error preparing input pcm device, " << snd_strerror(result)
							 << ".";
				errorText_ = errorStream_.str();
				goto unlock;
			}
		}
	}

	stream_.state = StreamState::STREAM_RUNNING;

unlock:
	apiInfo->runnable = true;
	pthread_cond_signal(&apiInfo->runnable_cv);
	pthread_mutex_unlock(&stream_.mutex);

	if (result >= 0)
	{ return; }
	error(Exception::SYSTEM_ERROR);
}

void LinuxAlsa::stopStream()
{
	verifyStream();
	if (stream_.state == StreamState::STREAM_STOPPED)
	{
		errorText_ = "RtApiAlsa::stopStream(): the stream is already stopped!";
		error(Exception::WARNING);
		return;
	}

	stream_.state = StreamState::STREAM_STOPPED;
	pthread_mutex_lock(&stream_.mutex);

	//if ( stream_.state == STREAM_STOPPED ) {
	//  MUTEX_UNLOCK( &stream_.mutex );
	//  return;
	//}

	int result = 0;
	AlsaHandle* apiInfo = (AlsaHandle*)stream_.apiHandle;
	snd_pcm_t** handle = (snd_pcm_t**)apiInfo->handles;
	if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
	{
		if (apiInfo->synchronized)
		{
			result = snd_pcm_drop(handle[0]);
		}
		else
		{
			result = snd_pcm_drain(handle[0]);
		}
		if (result < 0)
		{
			errorStream_ << "RtApiAlsa::stopStream: error draining output pcm device, " << snd_strerror(result) << ".";
			errorText_ = errorStream_.str();
			goto unlock;
		}
	}

	if ((stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX) && !apiInfo->synchronized)
	{
		result = snd_pcm_drop(handle[1]);
		if (result < 0)
		{
			errorStream_ << "RtApiAlsa::stopStream: error stopping input pcm device, " << snd_strerror(result) << ".";
			errorText_ = errorStream_.str();
			goto unlock;
		}
	}

unlock:
	stream_.state = StreamState::STREAM_STOPPED;
	pthread_mutex_unlock(&stream_.mutex);

	if (result >= 0)
	{ return; }
	error(Exception::SYSTEM_ERROR);
}

void LinuxAlsa::abortStream()
{
	verifyStream();
	if (stream_.state == StreamState::STREAM_STOPPED)
	{
		errorText_ = "RtApiAlsa::abortStream(): the stream is already stopped!";
		error(Exception::WARNING);
		return;
	}

	stream_.state = StreamState::STREAM_STOPPED;
	pthread_mutex_lock(&stream_.mutex);

	//if ( stream_.state == STREAM_STOPPED ) {
	//  MUTEX_UNLOCK( &stream_.mutex );
	//  return;
	//}

	int result = 0;
	AlsaHandle* apiInfo = (AlsaHandle*)stream_.apiHandle;
	snd_pcm_t** handle = (snd_pcm_t**)apiInfo->handles;
	if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
	{
		result = snd_pcm_drop(handle[0]);
		if (result < 0)
		{
			errorStream_ << "RtApiAlsa::abortStream: error aborting output pcm device, " << snd_strerror(result) << ".";
			errorText_ = errorStream_.str();
			goto unlock;
		}
	}

	if ((stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX) && !apiInfo->synchronized)
	{
		result = snd_pcm_drop(handle[1]);
		if (result < 0)
		{
			errorStream_ << "RtApiAlsa::abortStream: error aborting input pcm device, " << snd_strerror(result) << ".";
			errorText_ = errorStream_.str();
			goto unlock;
		}
	}

unlock:
	stream_.state = StreamState::STREAM_STOPPED;
	pthread_mutex_unlock(&stream_.mutex);

	if (result >= 0)
	{ return; }
	error(Exception::SYSTEM_ERROR);
}

void LinuxAlsa::callbackEvent()
{
	AlsaHandle* apiInfo = (AlsaHandle*)stream_.apiHandle;
	if (stream_.state == StreamState::STREAM_STOPPED)
	{
		pthread_mutex_lock(&stream_.mutex);
		while (!apiInfo->runnable)
		{
			pthread_cond_wait(&apiInfo->runnable_cv, &stream_.mutex);
		}

		if (stream_.state != StreamState::STREAM_RUNNING)
		{
			pthread_mutex_unlock(&stream_.mutex);
			return;
		}
		pthread_mutex_unlock(&stream_.mutex);
	}

	if (stream_.state == StreamState::STREAM_CLOSED)
	{
		errorText_ = "RtApiAlsa::callbackEvent(): the stream is closed ... this shouldn't happen!";
		error(Exception::WARNING);
		return;
	}

	int doStopStream = 0;
	RtAudioCallback callback = (RtAudioCallback)stream_.callbackInfo.callback;
	double streamTime = getStreamTime();
	RtAudioStreamStatus status = 0;
	if (stream_.mode != StreamMode::INPUT && apiInfo->xrun[0] == true)
	{
		status |= RTAUDIO_OUTPUT_UNDERFLOW;
		apiInfo->xrun[0] = false;
	}
	if (stream_.mode != StreamMode::OUTPUT && apiInfo->xrun[1] == true)
	{
		status |= RTAUDIO_INPUT_OVERFLOW;
		apiInfo->xrun[1] = false;
	}
	doStopStream = callback(stream_.userBuffer[0], stream_.userBuffer[1],
			stream_.bufferSize, streamTime, status, stream_.callbackInfo.userData);

	if (doStopStream == 2)
	{
		abortStream();
		return;
	}

	pthread_mutex_lock(&stream_.mutex);

	// The state might change while waiting on a mutex.
	if (stream_.state == StreamState::STREAM_STOPPED)
	{ goto unlock; }

	int result;
	char* buffer;
	int channels;
	snd_pcm_t** handle;
	snd_pcm_sframes_t frames;
	AudioFormat format;
	handle = (snd_pcm_t**)apiInfo->handles;

	if (stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX)
	{

		// Setup parameters.
		if (stream_.doConvertBuffer[1])
		{
			buffer = stream_.deviceBuffer;
			channels = stream_.nDeviceChannels[1];
			format = stream_.deviceFormat[1];
		}
		else
		{
			buffer = stream_.userBuffer[1];
			channels = stream_.nUserChannels[1];
			format = stream_.userFormat;
		}

		// Read samples from device in interleaved/non-interleaved format.
		if (stream_.deviceInterleaved[1])
		{
			result = snd_pcm_readi(handle[1], buffer, stream_.bufferSize);
		}
		else
		{
			void* bufs[channels];
			size_t offset = stream_.bufferSize * formatBytes(format);
			for (int i = 0; i < channels; i++)
			{
				bufs[i] = (void*)(buffer + (i * offset));
			}
			result = snd_pcm_readn(handle[1], bufs, stream_.bufferSize);
		}

		if (result < (int)stream_.bufferSize)
		{
			// Either an error or overrun occured.
			if (result == -EPIPE)
			{
				snd_pcm_state_t state = snd_pcm_state(handle[1]);
				if (state == SND_PCM_STATE_XRUN)
				{
					apiInfo->xrun[1] = true;
					result = snd_pcm_prepare(handle[1]);
					if (result < 0)
					{
						errorStream_ << "RtApiAlsa::callbackEvent: error preparing device after overrun, "
									 << snd_strerror(result) << ".";
						errorText_ = errorStream_.str();
					}
				}
				else
				{
					errorStream_ << "RtApiAlsa::callbackEvent: error, current state is " << snd_pcm_state_name(state)
								 << ", " << snd_strerror(result) << ".";
					errorText_ = errorStream_.str();
				}
			}
			else
			{
				errorStream_ << "RtApiAlsa::callbackEvent: audio read error, " << snd_strerror(result) << ".";
				errorText_ = errorStream_.str();
			}
			error(Exception::WARNING);
			goto tryOutput;
		}

		// Do byte swapping if necessary.
		if (stream_.doByteSwap[1])
		{
			byteSwapBuffer(buffer, stream_.bufferSize * channels, format);
		}

		// Do buffer conversion if necessary.
		if (stream_.doConvertBuffer[1])
		{
			convertBuffer(stream_.userBuffer[1], stream_.deviceBuffer, stream_.convertInfo[1]);
		}

		// Check stream latency
		result = snd_pcm_delay(handle[1], &frames);
		if (result == 0 && frames > 0)
		{ stream_.latency[1] = frames; }
	}

tryOutput:

	if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
	{

		// Setup parameters and do buffer conversion if necessary.
		if (stream_.doConvertBuffer[0])
		{
			buffer = stream_.deviceBuffer;
			convertBuffer(buffer, stream_.userBuffer[0], stream_.convertInfo[0]);
			channels = stream_.nDeviceChannels[0];
			format = stream_.deviceFormat[0];
		}
		else
		{
			buffer = stream_.userBuffer[0];
			channels = stream_.nUserChannels[0];
			format = stream_.userFormat;
		}

		// Do byte swapping if necessary.
		if (stream_.doByteSwap[0])
		{
			byteSwapBuffer(buffer, stream_.bufferSize * channels, format);
		}

		// Write samples to device in interleaved/non-interleaved format.
		if (stream_.deviceInterleaved[0])
		{
			result = snd_pcm_writei(handle[0], buffer, stream_.bufferSize);
		}
		else
		{
			void* bufs[channels];
			size_t offset = stream_.bufferSize * formatBytes(format);
			for (int i = 0; i < channels; i++)
			{
				bufs[i] = (void*)(buffer + (i * offset));
			}
			result = snd_pcm_writen(handle[0], bufs, stream_.bufferSize);
		}

		if (result < (int)stream_.bufferSize)
		{
			// Either an error or underrun occured.
			if (result == -EPIPE)
			{
				snd_pcm_state_t state = snd_pcm_state(handle[0]);
				if (state == SND_PCM_STATE_XRUN)
				{
					apiInfo->xrun[0] = true;
					result = snd_pcm_prepare(handle[0]);
					if (result < 0)
					{
						errorStream_ << "RtApiAlsa::callbackEvent: error preparing device after underrun, "
									 << snd_strerror(result) << ".";
						errorText_ = errorStream_.str();
					}
				}
				else
				{
					errorStream_ << "RtApiAlsa::callbackEvent: error, current state is " << snd_pcm_state_name(state)
								 << ", " << snd_strerror(result) << ".";
					errorText_ = errorStream_.str();
				}
			}
			else
			{
				errorStream_ << "RtApiAlsa::callbackEvent: audio write error, " << snd_strerror(result) << ".";
				errorText_ = errorStream_.str();
			}
			error(Exception::WARNING);
			goto unlock;
		}

		// Check stream latency
		result = snd_pcm_delay(handle[0], &frames);
		if (result == 0 && frames > 0)
		{ stream_.latency[0] = frames; }
	}

unlock:
	pthread_mutex_unlock(&stream_.mutex);

	AudioArchitecture::tickStreamTime();
	if (doStopStream == 1)
	{ this->stopStream(); }
}