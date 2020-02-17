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
