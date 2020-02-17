#ifndef MAXIMILIAN_ALSAHANDLE_HPP
#define MAXIMILIAN_ALSAHANDLE_HPP

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
	};
}


#endif //MAXIMILIAN_ALSAHANDLE_HPP
