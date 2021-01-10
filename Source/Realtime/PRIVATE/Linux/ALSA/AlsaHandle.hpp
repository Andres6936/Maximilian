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

	private:

		std::uint8_t numberOfDevices = 0;

		bool runnable = false;

		bool synchronized = false;

		void determineTheNumberOfDevices();

	public:

		/**
		 * Handle for Playback and Record, respectively.
		 */
		snd_pcm_t* handles[2] = { nullptr, nullptr };

		pthread_cond_t runnable_cv;

		std::array <bool, 2> xrun{ false, false };

		AlsaHandle() noexcept;

		virtual ~AlsaHandle();

		// Methods Static

		static bool isAvailableForCapture(snd_ctl_t& handle, snd_pcm_info_t& info);

		static void testSupportedDateFormats(snd_pcm_t& handle, snd_pcm_hw_params_t& params,
				const std::array <unsigned int, 14>& rates, DeviceInfo& info);

		static void
		setSupportedDateFormats(snd_pcm_t& handle, snd_pcm_hw_params_t& params, DeviceInfo& info);

		// Methods

		void waitThreadForSignal();

		void waitThreadForCondition(pthread_mutex_t& _mutex);

		// Getters

		[[nodiscard]] std::uint8_t getNumberOfDevices() const noexcept;

		[[nodiscard]] bool isRunnable() const;

		[[nodiscard]] bool isXRunRecord() const;

		[[nodiscard]] bool isXRunPlayback() const;

		[[nodiscard]] bool isSynchronized() const;

		[[nodiscard]] snd_pcm_t* getHandleForPlayback() const;

		[[nodiscard]] snd_pcm_t* getHandleForRecord() const;

		// Setters

		void setRunnable(bool _runnable);

		void setXRunRecord(bool _run);

		void setXRunPlayback(bool _run);

		void setSynchronized(bool _synchronized);

		void setTheHandleForPlayback(snd_pcm_t* _handle);

		void setTheHandleForRecord(snd_pcm_t* _handle);
	};
}

/**
 * Feature of C++17, global object with keyword inline,
 * this object is equal a have an instance of an
 * Singleton Object.
 */
inline Maximilian::AlsaHandle alsaHandle;

#endif //MAXIMILIAN_ALSAHANDLE_HPP
