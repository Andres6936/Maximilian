#ifndef MAXIMILIAN_LINUXALSA_HPP
#define MAXIMILIAN_LINUXALSA_HPP

#include "Audio.hpp"
#include "IAudioArchitecture.hpp"

#include <vector>
#include <thread>
#include <atomic>

namespace Maximilian
{
	class LinuxAlsa : public IAudioArchitecture
	{

	private:

		std::vector<std::thread> threads;

		std::atomic_bool isStreamRunning = false;

	public:

		LinuxAlsa() noexcept = default;

		~LinuxAlsa() override;

		SupportedArchitectures getCurrentArchitecture() const noexcept override
		{
			return SupportedArchitectures::Linux_Alsa;
		};

		unsigned int getDeviceCount() const noexcept override;

		DeviceInfo getDeviceInfo(int device) override;

		void closeStream() noexcept override;

		void startStream() noexcept override;

		void stopStream() noexcept override;

		void abortStream() noexcept override;

		// This function is intended for internal use only.  It must be
		// public because it is called by the internal callback handler,
		// which is not a member of RtAudio.  External use of this function
		// will most likely produce highly undesireable results!
		void callbackEvent();

	private:

		std::vector <DeviceInfo> devices_;

		bool probeDeviceOpen(
				unsigned int device,
				StreamMode mode,
				unsigned int channels,
				unsigned int firstChannel) override;

		void saveDeviceInfo();

		void startCallbackFunction();

		void unlockMutex();

		void unlockMutexOfAPIHandle();

		template <class Handle>
		void dropHandle(Handle _handle);

		template <class Handle>
		void checkStreamLatencyOf(Handle _handle, int index);

		template <class Handle>
		void tryInput(Handle _handle);

		template <class Handle>
		void tryOutput(Handle _handle);

		template <class Device>
		void prepareStateOfDevice(Device _device);

		template <class Handle>
		void verifyUnderRunOrError(Handle _handle, int index, int result);
	};
}


#endif //MAXIMILIAN_LINUXALSA_HPP
