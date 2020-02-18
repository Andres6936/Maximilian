#ifndef MAXIMILIAN_ALSAHANDLE_HPP
#define MAXIMILIAN_ALSAHANDLE_HPP

#include "Realtime/DeviceInfo.hpp"

#include <array>
#include <alsa/asoundlib.h>

namespace Maximilian
{
	// A structure to hold various information related to the ALSA API
	// implementation.
	class AlsaHandle
	{

	public:

		bool runnable = false;
		bool synchronized = false;

		snd_pcm_t* handles[2] = { nullptr, nullptr };
		pthread_cond_t runnable_cv;

		std::array <bool, 2> xrun{ false, false };

		AlsaHandle() = default;

		static bool isAvailableForCapture(snd_ctl_t& handle, snd_pcm_info_t& info);

		static void testSupportedDateFormats(snd_pcm_t& handle, snd_pcm_hw_params_t& params,
				const std::array <unsigned int, 14>& rates, DeviceInfo& info);

		static void setSupportedDateFormats(snd_pcm_t& handle, snd_pcm_hw_params_t& params, DeviceInfo& info);
	};
}


#endif //MAXIMILIAN_ALSAHANDLE_HPP
