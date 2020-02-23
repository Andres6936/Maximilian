#include "Realtime/LinuxAlsa.hpp"
#include "PRIVATE/Linux/ALSA/AlsaHandle.hpp"

#include <alsa/asoundlib.h>

#include <array>
#include <climits>

using namespace Maximilian;

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
	return alsaHandle.getNumberOfDevices();
}

DeviceInfo LinuxAlsa::getDeviceInfo(int device)
{
	if (device >= getDeviceCount())
	{
		throw Exception("DeviceInvalidException");
	}

	DeviceInfo info;

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
		snd_config_update_free_global();
		snd_ctl_pcm_next_device(handle, &subDevice);
		sprintf(name.data(), "hw:%d,%d", card, subDevice);
	}

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

	if (snd_ctl_pcm_info(handle, pcminfo) < 0)
	{
		// Device probably doesn't support playback.
		goto captureProbe;
	}

	// Feature C++17, assigment operator in if-else
	if (int e = snd_pcm_open(&phandle, name.data(), stream, SND_PCM_ASYNC | SND_PCM_NONBLOCK) < 0)
	{
		snd_config_update_free_global();
		Levin::Warn() << "Linux Alsa: getDeviceInfo, snd_pcm_open error for device ("
					  << name.data() << "), " << snd_strerror(e) << "." << Levin::endl;

		goto captureProbe;
	}

	snd_config_update_free_global();

	// The device is open ... fill the parameter structure.
	if (int e = snd_pcm_hw_params_any(phandle, params) < 0)
	{
		snd_pcm_close(phandle);

		Levin::Warn() << "Linux Alsa: getDeviceInfo, snd_pcm_hw_params error for device ("
					  << name.data() << "), " << snd_strerror(e) << "." << Levin::endl;

		goto captureProbe;
	}

	// Get output channel information.
	unsigned int value;
	if (int e = snd_pcm_hw_params_get_channels_max(params, &value) < 0)
	{
		snd_pcm_close(phandle);

		Levin::Warn() << "Linux Alsa: getDeviceInfo, error getting device ("
					  << name.data() << ") output channels, " << snd_strerror(e) << "." << Levin::endl;

		goto captureProbe;
	}

	info.outputChannels = value;
	snd_pcm_close(phandle);

captureProbe:

	if (not AlsaHandle::isAvailableForCapture(*handle, *pcminfo))
	{
		Levin::Debug() << "Not support for capture found in the device: " << name.data() << Levin::endl;
		// Close the handle
		snd_ctl_close(handle);

		// Device probably doesn't support capture.
		if (info.outputChannels == 0) return info;
		goto probeParameters;
	}

	snd_ctl_close(handle);

	if (int e = snd_pcm_open(&phandle, name.data(), SND_PCM_STREAM_CAPTURE, SND_PCM_ASYNC | SND_PCM_NONBLOCK) < 0)
	{
		snd_config_update_free_global();
		Levin::Warn() << "Linux Alsa: getDeviceInfo, snd_pcm_open error for device ("
					  << name.data() << "), " << snd_strerror(e) << "." << Levin::endl;

		if (info.outputChannels == 0) return info;
		goto probeParameters;
	}

	snd_config_update_free_global();

	// The device is open ... fill the parameter structure.
	if (int e = snd_pcm_hw_params_any(phandle, params) < 0)
	{
		snd_pcm_close(phandle);

		Levin::Warn() << "Linux Alsa: getDeviceInfo, snd_pcm_hw_params error for device ("
					  << name.data() << "), " << snd_strerror(e) << "." << Levin::endl;

		if (info.outputChannels == 0)
		{ return info; }
		goto probeParameters;
	}

	if (int e = snd_pcm_hw_params_get_channels_max(params, &value) < 0)
	{
		snd_pcm_close(phandle);

		Levin::Warn() << "Linux Alsa: getDeviceInfo, error getting device ("
					  << name.data() << ") input channels, " << snd_strerror(e) << "." << Levin::endl;

		if (info.outputChannels == 0)
		{ return info; }
		goto probeParameters;
	}

	info.inputChannels = value;
	snd_pcm_close(phandle);

	// If device opens for both playback and capture, we determine the channels.
	info.determineChannelsForDuplexMode();

	// ALSA doesn't provide default devices so we'll use the first available one.
	info.determineChannelsForDefaultByDevice(device);

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

	if (int e = snd_pcm_open(&phandle, name.data(), stream, SND_PCM_ASYNC | SND_PCM_NONBLOCK) < 0)
	{
		Levin::Warn() << "Linux Alsa: getDeviceInfo, snd_pcm_open error for device ("
					  << name.data() << "), " << snd_strerror(e) << "." << Levin::endl;

		return info;
	}

	// The device is open ... fill the parameter structure.
	if (int e = snd_pcm_hw_params_any(phandle, params) < 0)
	{
		snd_pcm_close(phandle);

		Levin::Warn() << "Linux Alsa: getDeviceInfo, snd_pcm_hw_params error for device ("
					  << name.data() << "), " << snd_strerror(e) << "." << Levin::endl;

		return info;
	}

	// Test our discrete set of sample rate values.
	AlsaHandle::testSupportedDateFormats(*phandle, *params, SAMPLE_RATES, info);

	if (info.sampleRates.empty())
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::getDeviceInfo: no supported sample rates found for device (" << name.data() << ").";
		errorText_ = errorStream_.str();
		error(Exception::WARNING);
		return info;
	}

	// Probe the supported data formats ... we don't care about endian-ness just yet
	AlsaHandle::setSupportedDateFormats(*phandle, *params, info);

	// That's all ... close the device and return
	snd_pcm_close(phandle);
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

bool LinuxAlsa::probeDeviceOpen(
		unsigned int device,
		StreamMode mode,
		unsigned int channels,
		unsigned int firstChannel)
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

	if (getOptionsFlags() == AudioStreamFlags::Alsa_Use_Default)
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
			snd_config_update_free_global();

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
	snd_config_update_free_global();

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
	if (getOptionsFlags() == AudioStreamFlags::Non_Interleaved)
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
	stream_.userFormat = getAudioFormat();
	snd_pcm_format_t deviceFormat;

	const std::vector <std::pair <snd_pcm_format_t, AudioFormat>> equivalentFormats = {

			{ SND_PCM_FORMAT_FLOAT64, AudioFormat::Float64 },
			{ SND_PCM_FORMAT_FLOAT,   AudioFormat::Float32 },
			{ SND_PCM_FORMAT_S32,     AudioFormat::SInt32 },
			{ SND_PCM_FORMAT_S24,     AudioFormat::SInt24 },
			{ SND_PCM_FORMAT_S16,     AudioFormat::SInt16 },
			{ SND_PCM_FORMAT_S8,      AudioFormat::SInt8 }
	};

	for (auto& format : equivalentFormats)
	{
		if (snd_pcm_hw_params_test_format(phandle, hw_params, format.first) == 0)
		{
			deviceFormat = format.first;
			stream_.deviceFormat[index] = format.second;
			break;
		}
	}

	if (deviceFormat == SND_PCM_FORMAT_UNKNOWN)
	{
		Levin::Severe() << "Linux Alsa: Data format not supported." << Levin::endl;
		throw Exception("DataFormatNotSupportedException");
	}

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

	// Is needed the pointer for pass for argument to
	// function { snd_pcm_hw_params_set_rate_near }
	auto sampleRate = std::make_unique <unsigned int>(getSampleRate());

	// Set the sample rate.
	if (snd_pcm_hw_params_set_rate_near(phandle, hw_params, sampleRate.get(), nullptr) < 0)
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
	snd_pcm_uframes_t periodSize = getBufferFrames();
	result = snd_pcm_hw_params_set_period_size_near(phandle, hw_params, &periodSize, &dir);
	if (result < 0)
	{
		snd_pcm_close(phandle);
		errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting period size for device (" << name << "), "
					 << snd_strerror(result) << ".";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

	setBufferFrames(periodSize);

	// Set the buffer number, which in ALSA is referred to as the "period".
	unsigned int periods = 0;
	if (getOptionsFlags() == AudioStreamFlags::Minimize_Latency)
	{ periods = 2; }

	if (getNumberOfBuffersOptions() > 0)
	{ periods = getNumberOfBuffersOptions(); }

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
	if (stream_.mode == StreamMode::OUTPUT && mode == StreamMode::INPUT && getBufferFrames() != stream_.bufferSize)
	{
		errorStream_ << "RtApiAlsa::probeDeviceOpen: system error setting buffer size for duplex stream on device ("
					 << name << ").";
		errorText_ = errorStream_.str();
		return FAILURE;
	}

	stream_.bufferSize = getBufferFrames();

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
	snd_pcm_sw_params_set_start_threshold(phandle, sw_params, getBufferFrames());
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

	if (index == 0)
	{
		alsaHandle.setTheHandleForPlayback(phandle);
	}
	else
	{
		alsaHandle.setTheHandleForRecord(phandle);
	}

	// Allocate necessary internal buffers.
	unsigned long bufferBytes;
	bufferBytes = stream_.nUserChannels[index] * getBufferFrames() * formatBytes(stream_.userFormat);

	if (index == 0)
	{
		stream_.userBuffer.first.resize(bufferBytes);
	}
	else if (index == 1)
	{
		stream_.userBuffer.second.resize(bufferBytes);
	}
	else
	{
		Levin::Error() << "Linux Alsa: probeDeviceOpen, The index is invalid." << Levin::endl;

		throw Exception("IndexInvalidException");
	}

	if (stream_.doConvertBuffer[index])
	{

		bool makeBuffer = true;
		bufferBytes = stream_.nDeviceChannels[index] * formatBytes(stream_.deviceFormat[index]);
		if (mode == StreamMode::INPUT)
		{
			if (stream_.mode == StreamMode::OUTPUT and not stream_.deviceBuffer.empty())
			{
				unsigned long bytesOut = stream_.nDeviceChannels[0] * formatBytes(stream_.deviceFormat[0]);
				if (bufferBytes <= bytesOut)
				{ makeBuffer = false; }
			}
		}

		if (makeBuffer)
		{
			bufferBytes *= getBufferFrames();

			stream_.deviceBuffer.clear();
			stream_.deviceBuffer.resize(bufferBytes);
		}
	}

	stream_.sampleRate = getSampleRate();
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
		if (snd_pcm_link(alsaHandle.getHandleForPlayback(), alsaHandle.getHandleForRecord()) == 0)
		{
			alsaHandle.setSynchronized(true);
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
		if (getOptionsFlags() == AudioStreamFlags::Schedule_Realtime)
		{
			struct sched_param param;
			int priority = getOptionsPriority();
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

		stream_.apiHandle = alsaHandle;

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

	stream_.deviceBuffer.clear();

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

	auto* apiInfo = std::any_cast <AlsaHandle>(&stream_.apiHandle);

	stream_.callbackInfo.isRunning = false;
	pthread_mutex_lock(&stream_.mutex);
	if (stream_.state == StreamState::STREAM_STOPPED)
	{
		alsaHandle.setRunnable(true);
		pthread_cond_signal(&apiInfo->runnable_cv);
	}
	pthread_mutex_unlock(&stream_.mutex);
	pthread_join(stream_.callbackInfo.thread, NULL);

	if (stream_.state == StreamState::STREAM_RUNNING)
	{
		stream_.state = StreamState::STREAM_STOPPED;
		if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
		{
			snd_pcm_drop(alsaHandle.handles[0]);
		}
		if (stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX)
		{
			snd_pcm_drop(alsaHandle.handles[1]);
		}
	}

	stream_.userBuffer.first.clear();
	stream_.userBuffer.second.clear();

	stream_.deviceBuffer.clear();

	stream_.mode = StreamMode::UNINITIALIZED;
	stream_.state = StreamState::STREAM_CLOSED;
}

void LinuxAlsa::startStream()
{
	// This method calls snd_pcm_prepare if the device isn't already in that state.

	verifyStream();

	if (stream_.state == StreamState::STREAM_RUNNING)
	{
		Levin::Warn() << "Linux Alsa: startStream(): the stream is already running." << Levin::endl;
		// Exit function
		return;
	}

	pthread_mutex_lock(&stream_.mutex);

	if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
	{
		prepareStateOfDevice(alsaHandle.handles[0]);
	}

	if ((stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX) && !alsaHandle.isSynchronized())
	{
		prepareStateOfDevice(alsaHandle.handles[1]);
	}

	stream_.state = StreamState::STREAM_RUNNING;
	unlockMutexOfAPIHandle();
}

template <class Device>
void LinuxAlsa::prepareStateOfDevice(Device _device)
{
	snd_pcm_state_t state = snd_pcm_state(_device);

	if (state != SND_PCM_STATE_PREPARED)
	{
		if (int e = snd_pcm_prepare(_device) < 0)
		{
			Levin::Error() << "Linux Alsa: error preparing pcm device, "
						   << snd_strerror(e) << "." << Levin::endl;

			unlockMutexOfAPIHandle();

			throw Exception("InvalidUseException");
		}
	}
}

void LinuxAlsa::unlockMutexOfAPIHandle()
{
	auto* apiInfo = std::any_cast <AlsaHandle>(&stream_.apiHandle);

	alsaHandle.setRunnable(true);
	pthread_cond_signal(&apiInfo->runnable_cv);
	pthread_mutex_unlock(&stream_.mutex);
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

	if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
	{
		int result = 0;

		if (alsaHandle.isSynchronized())
		{
			result = snd_pcm_drop(alsaHandle.handles[0]);
		}
		else
		{
			result = snd_pcm_drain(alsaHandle.handles[0]);
		}

		if (result < 0)
		{
			Levin::Error() << "Linux Alsa: stopStream, error draining output pcm device, "
						   << snd_strerror(result) << "." << Levin::endl;

			pthread_mutex_unlock(&stream_.mutex);
			throw Exception("ErrorDropHandleException");
		}
	}

	if ((stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX) && !alsaHandle.isSynchronized())
	{
		dropHandle(alsaHandle.handles[1]);
	}

	pthread_mutex_unlock(&stream_.mutex);
}

void LinuxAlsa::abortStream()
{
	verifyStream();

	if (stream_.state == StreamState::STREAM_STOPPED)
	{
		Levin::Warn() << "Linux Alsa: abortStream, the stream is already stopped." << Levin::endl;
		return;
	}

	stream_.state = StreamState::STREAM_STOPPED;
	pthread_mutex_lock(&stream_.mutex);

	if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
	{
		dropHandle(alsaHandle.handles[0]);
	}

	if ((stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX) && !alsaHandle.isSynchronized())
	{
		dropHandle(alsaHandle.handles[1]);
	}

	pthread_mutex_unlock(&stream_.mutex);
}

template <class Handle>
void LinuxAlsa::dropHandle(Handle _handle)
{
	if (int e = snd_pcm_drop(_handle) < 0)
	{
		Levin::Error() << "Linux Alsa: error stopping stream in pcm device, "
					   << snd_strerror(e) << "." << Levin::endl;

		pthread_mutex_unlock(&stream_.mutex);
		throw Exception("ErrorDropHandleException");
	}
}

void LinuxAlsa::callbackEvent()
{
	auto* apiInfo = std::any_cast <AlsaHandle>(&stream_.apiHandle);

	if (stream_.state == StreamState::STREAM_STOPPED)
	{
		pthread_mutex_lock(&stream_.mutex);

		while (!alsaHandle.isRunnable())
		{
			alsaHandle.waitThreadForCondition(stream_.mutex);
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

	AudioStreamStatus status = AudioStreamStatus::None;

	if (stream_.mode != StreamMode::INPUT && alsaHandle.isXRunPlayback() == true)
	{
		status = AudioStreamStatus::Underflow;
		alsaHandle.setXRunPlayback(false);
	}
	if (stream_.mode != StreamMode::OUTPUT && alsaHandle.isXRunRecord() == true)
	{
		status = AudioStreamStatus::Overflow;
		alsaHandle.setXRunRecord(false);
	}

	if (status != AudioStreamStatus::None)
	{
		Levin::Error() << "An Underflow or Overflow has been produced." << Levin::endl;

		throw Exception("UnderflowOrOverflowException");
	}

	startCallbackFunction();

	pthread_mutex_lock(&stream_.mutex);

	// The state might change while waiting on a mutex.
	if (stream_.state == StreamState::STREAM_STOPPED)
	{
		unlockMutex();
		return;
	}

	if (stream_.mode == StreamMode::INPUT || stream_.mode == StreamMode::DUPLEX)
	{
		tryInput(alsaHandle.handles[1]);
	}

	if (stream_.mode == StreamMode::OUTPUT || stream_.mode == StreamMode::DUPLEX)
	{
		tryOutput(alsaHandle.handles[0]);
	}

	unlockMutex();
}

void LinuxAlsa::startCallbackFunction()
{
	// Left and Right channel
	std::vector <double> data(2, 0);

	std::vector <double> bufferConvert;
	bufferConvert.resize(stream_.userBuffer.first.size());

	int indexOfBuffer = 0;

	// Write interleaved audio data.
	for (int i = 0; i < stream_.bufferSize; i++)
	{
		audioCallback(data);

		for (int j = 0; j < 2; j++)
		{
			bufferConvert[indexOfBuffer] = data[j];
			indexOfBuffer += 1;
		}
	}

	char* test = (char*)bufferConvert.data();

	for (int k = 0; k < bufferConvert.size(); ++k)
	{
		stream_.userBuffer.first[k] = test[k];
	}
}

void LinuxAlsa::unlockMutex()
{
	pthread_mutex_unlock(&stream_.mutex);

	AudioArchitecture::tickStreamTime();
}

template <class Device>
void LinuxAlsa::tryInput(Device _handle)
{
	int channels = 0;
	int result = 0;

	AudioFormat format;
	std::vector <char> buffer;

	// Setup parameters.
	if (stream_.doConvertBuffer[1])
	{
		buffer = stream_.deviceBuffer;
		channels = stream_.nDeviceChannels[1];
		format = stream_.deviceFormat[1];
	}
	else
	{
		buffer = stream_.userBuffer.second;
		channels = stream_.nUserChannels[1];
		format = stream_.userFormat;
	}

	// Read samples from device in interleaved/non-interleaved format.
	if (stream_.deviceInterleaved[1])
	{
		result = snd_pcm_readi(_handle, buffer.data(), stream_.bufferSize);
	}

	verifyUnderRunOrError(_handle, 1, result);

	// Do byte swapping if necessary.
	if (stream_.doByteSwap[1])
	{
		byteSwapBuffer(buffer.data(), stream_.bufferSize * channels, format);
	}

	// Do buffer conversion if necessary.
	if (stream_.doConvertBuffer[1])
	{
		convertBuffer(stream_.userBuffer.second.data(), stream_.deviceBuffer.data(), stream_.convertInfo[1]);
	}

	// Check stream latency
	checkStreamLatencyOf(_handle, 1);
}

template <class Handle>
void LinuxAlsa::tryOutput(Handle _handle)
{
	int channels = 0;
	int result = 0;

	AudioFormat format;
	std::vector <char> buffer;

	// Setup parameters and do buffer conversion if necessary.
	if (stream_.doConvertBuffer[0])
	{
		buffer = stream_.deviceBuffer;
		convertBuffer(buffer.data(), stream_.userBuffer.first.data(), stream_.convertInfo[0]);
		channels = stream_.nDeviceChannels[0];
		format = stream_.deviceFormat[0];
	}
	else
	{
		buffer = stream_.userBuffer.first;
		channels = stream_.nUserChannels[0];
		format = stream_.userFormat;
	}

	// Do byte swapping if necessary.
	if (stream_.doByteSwap[0])
	{
		byteSwapBuffer(buffer.data(), stream_.bufferSize * channels, format);
	}

	// Write samples to device in interleaved/non-interleaved format.
	if (stream_.deviceInterleaved[0])
	{
		result = snd_pcm_writei(_handle, buffer.data(), stream_.bufferSize);
	}

	verifyUnderRunOrError(_handle, 0, result);

	// Check stream latency
	checkStreamLatencyOf(_handle, 0);
}

template <class Handle>
void LinuxAlsa::verifyUnderRunOrError(Handle _handle, int index, int result)
{
	if (result < (int)stream_.bufferSize)
	{
		// Either an error or under-run occurred.
		if (result == -EPIPE)
		{
			snd_pcm_state_t state = snd_pcm_state(_handle);
			if (state == SND_PCM_STATE_XRUN)
			{
				if (index == 0)
				{
					alsaHandle.setXRunPlayback(true);
				}
				else
				{
					alsaHandle.setXRunRecord(true);
				}

				if (int e = snd_pcm_prepare(_handle) < 0)
				{
					Levin::Error() << "Linux Alsa: error preparing device after overrun, "
								   << snd_strerror(e) << "." << Levin::endl;
				}
			}
			else
			{
				Levin::Error() << "Linux Alsa: error, current state is " << snd_pcm_state_name(state)
							   << ", " << snd_strerror(result) << "." << Levin::endl;
			}
		}
		else
		{
			Levin::Error() << "Linux Alsa: audio write/read error, " << snd_strerror(result) << "." << Levin::endl;
		}
	}
}

template <class Handle>
void LinuxAlsa::checkStreamLatencyOf(Handle _handle, int index)
{
	long frames = 0;
	int result = snd_pcm_delay(_handle, &frames);

	if (result == 0 && frames > 0)
	{
		stream_.latency[index] = frames;
	}
}
