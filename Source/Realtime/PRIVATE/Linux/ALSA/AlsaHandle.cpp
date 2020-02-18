#include "AlsaHandle.hpp"

bool Maximilian::AlsaHandle::isAvailableForCapture(snd_ctl_t& handle, snd_pcm_info_t& info)
{
	snd_pcm_info_set_stream(&info, SND_PCM_STREAM_CAPTURE);

	if (snd_ctl_pcm_info(&handle, &info) < 0)
	{
		return false;
	}

	return true;
}

void Maximilian::AlsaHandle::probeSupportedDateFormats(snd_pcm_t& handle, snd_pcm_hw_params_t& params, DeviceInfo& info)
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
