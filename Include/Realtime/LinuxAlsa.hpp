#ifndef MAXIMILIAN_LINUXALSA_HPP
#define MAXIMILIAN_LINUXALSA_HPP

#include "Audio.hpp"
#include "IAudioArchitecture.hpp"

#include <vector>
#include <thread>
#include <atomic>
#include <alsa/asoundlib.h>

namespace Maximilian
{
	class LinuxAlsa : public IAudioArchitecture
	{

	private:

		std::vector<DeviceInfo> devices_;

		std::vector<std::thread> threads;

		std::atomic_bool isStreamRunning = false;

		snd_pcm_t* phandle = nullptr;

		snd_pcm_hw_params_t* hw_params = nullptr;

	public:

		LinuxAlsa() noexcept = default;

		~LinuxAlsa() override;

		// This function is intended for internal use only.  It must be
		// public because it is called by the internal callback handler,
		// which is not a member of RtAudio.  External use of this function
		// will most likely produce highly undesireable results!
		void callbackEvent();

		void closeStream() noexcept override;

		void startStream() noexcept override;

		void stopStream() noexcept override;

		void abortStream() noexcept override;

		unsigned int getDeviceCount() const noexcept override;

		DeviceInfo getDeviceInfo(int device) override;

		SupportedArchitectures getCurrentArchitecture() const noexcept override
		{
			return SupportedArchitectures::Linux_Alsa;
		};

	private:

		LinuxAlsa& buildHW();

		LinuxAlsa& allocateHW();

		LinuxAlsa& getPCMDevice();

		LinuxAlsa& setHWSampleRate();

		LinuxAlsa& setHWFormat(const std::int32_t index);

		LinuxAlsa& setHWPeriodSize(const StreamMode mode);

		LinuxAlsa& setHWInterleaved(const std::int32_t index);

		LinuxAlsa& setHWChannels(const StreamParameters& parameters, const std::int32_t index);

		bool probeDeviceOpen(const StreamMode mode,
				const StreamParameters& parameters) noexcept override;

		void unlockMutex();

		void saveDeviceInfo();

		void startCallbackFunction();

		void unlockMutexOfAPIHandle();

		template<class Handle>
		void tryInput(Handle _handle);

		template<class Handle>
		void tryOutput(Handle _handle);

		template<class Handle>
		void dropHandle(Handle _handle);

		template<class Device>
		void prepareStateOfDevice(Device _device);

		template<class Handle>
		void checkStreamLatencyOf(Handle _handle, int index);

		template<class Handle>
		void verifyUnderRunOrError(Handle _handle, int index, int result);
	};
}


#endif //MAXIMILIAN_LINUXALSA_HPP
