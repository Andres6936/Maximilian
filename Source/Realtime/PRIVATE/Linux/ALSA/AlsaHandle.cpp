#include <Exception.h>
#include "AlsaHandle.hpp"

#include "Levin/Log.h"

using namespace Maximilian;

AlsaHandle::AlsaHandle() noexcept
{
	determineTheNumberOfDevices();

	if (pthread_cond_init(&runnable_cv, nullptr))
	{
		Levin::Severe() << "Construct of Linux Alsa: ";
		Levin::Severe() << "Error initializing pthread condition variable." << Levin::endl;
	}
}

AlsaHandle::~AlsaHandle()
{
	pthread_cond_destroy(&runnable_cv);

	if (handles[0]) snd_pcm_close(handles[0]);
	if (handles[1]) snd_pcm_close(handles[1]);

	// Clear the cache for handle, avoid memory leak.
	snd_config_update_free_global();
}

bool AlsaHandle::isAvailableForCapture(snd_ctl_t& handle, snd_pcm_info_t& info)
{
	snd_pcm_info_set_stream(&info, SND_PCM_STREAM_CAPTURE);

	if (snd_ctl_pcm_info(&handle, &info) < 0)
	{
		return false;
	}

	return true;
}

void AlsaHandle::testSupportedDateFormats(snd_pcm_t& handle, snd_pcm_hw_params_t& params,
		const std::array <unsigned int, 14>& rates, DeviceInfo& info)
{
	for (unsigned int rate : rates)
	{
		if (snd_pcm_hw_params_test_rate(&handle, &params, rate, 0) == 0)
		{
			info.sampleRates.push_back(rate);
		}
	}
}

void AlsaHandle::setSupportedDateFormats(snd_pcm_t& handle, snd_pcm_hw_params_t& params, DeviceInfo& info)
{
	if (snd_pcm_hw_params_test_format(&handle, &params, SND_PCM_FORMAT_S8) == 0)
	{
		info.nativeFormats = AudioFormat::SInt8;
	}
	else if (snd_pcm_hw_params_test_format(&handle, &params, SND_PCM_FORMAT_S16) == 0)
	{
		info.nativeFormats = AudioFormat::SInt16;
	}
	else if (snd_pcm_hw_params_test_format(&handle, &params, SND_PCM_FORMAT_S24) == 0)
	{
		info.nativeFormats = AudioFormat::SInt24;
	}
	else if (snd_pcm_hw_params_test_format(&handle, &params, SND_PCM_FORMAT_S32) == 0)
	{
		info.nativeFormats = AudioFormat::SInt32;
	}
	else if (snd_pcm_hw_params_test_format(&handle, &params, SND_PCM_FORMAT_FLOAT) == 0)
	{
		info.nativeFormats = AudioFormat::Float32;
	}
	else if (snd_pcm_hw_params_test_format(&handle, &params, SND_PCM_FORMAT_FLOAT64) == 0)
	{
		info.nativeFormats = AudioFormat::Float64;
	}
}

void AlsaHandle::determineTheNumberOfDevices()
{
	// Count cards and devices
	int card = -1;
	snd_card_next(&card);

	// Can't use smart point in a abstract data type, aka: incomplete type.
	snd_ctl_t* handle = nullptr;

	if (card >= 0)
	{
		std::array <char, 64> name{ };

		std::sprintf(name.data(), "hw:%d", card);

		if (int e = snd_ctl_open(&handle, name.data(), 0) < 0)
		{
			Levin::Warn() << "Linux Alsa: getDeviceCount(): ";
			Levin::Warn() << "Control open, card = " << card << ", ";
			Levin::Warn() << snd_strerror(e) << "." << Levin::endl;

			snd_ctl_close(handle);
		}

		int subDevice = -1;

		while (true)
		{
			if (int e = snd_ctl_pcm_next_device(handle, &subDevice) < 0)
			{
				Levin::Warn() << "Linux Alsa: getDeviceCount(): ";
				Levin::Warn() << "Control next device, card = " << card;
				Levin::Warn() << ", " << snd_strerror(e) << "." << Levin::endl;
				break;
			}

			if (subDevice < 0) break;

			this->numberOfDevices += 1;
		}
	}
	else
	{
		Levin::Error() << "Linux Alsa: determineTheNumberOfDevices(): ";
		Levin::Error() << "Can't determine the number of devices." << Levin::endl;
	}

	if (handle != nullptr)
	{
		snd_ctl_close(handle);
		// Free the cache for avoid memory leak
		snd_config_update_free_global();
	}
}

UInt8 AlsaHandle::getNumberOfDevices() const
{
	return numberOfDevices;
}

void AlsaHandle::setTheHandleForPlayback(snd_pcm_t* _handle)
{
	handles[0] = _handle;
}

void AlsaHandle::setTheHandleForRecord(snd_pcm_t* _handle)
{
	handles[1] = _handle;
}

snd_pcm_t* AlsaHandle::getHandleForPlayback() const
{
	return handles[0];
}

snd_pcm_t* AlsaHandle::getHandleForRecord() const
{
	return handles[1];
}

void AlsaHandle::setSynchronized(bool _synchronized)
{
	synchronized = _synchronized;
}

void AlsaHandle::setRunnable(bool _runnable)
{
	runnable = _runnable;
}

bool AlsaHandle::isSynchronized() const
{
	return synchronized;
}

bool AlsaHandle::isRunnable() const
{
	return runnable;
}

void AlsaHandle::waitThreadForCondition(pthread_mutex_t& _mutex)
{
	pthread_cond_wait(&runnable_cv, &_mutex);
}

bool AlsaHandle::isXRunPlayback() const
{
	return xrun[0];
}

bool AlsaHandle::isXRunRecord() const
{
	return xrun[1];
}

void AlsaHandle::setXRunPlayback(bool _run)
{
	xrun[0] = _run;
}

void AlsaHandle::setXRunRecord(bool _run)
{
	xrun[1] = _run;
}