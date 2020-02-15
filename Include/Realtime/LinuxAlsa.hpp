#ifndef MAXIMILIAN_LINUXALSA_HPP
#define MAXIMILIAN_LINUXALSA_HPP

#include "Audio.hpp"
#include "AudioArchitecture.hpp"

#include <vector>

namespace Maximilian
{
	class LinuxAlsa : public AudioArchitecture
	{

	public:

		LinuxAlsa() = default;

		~LinuxAlsa() override;

		SupportedArchitectures getCurrentArchitecture() override
		{
			return SupportedArchitectures::Linux_Alsa;
		};

		unsigned int getDeviceCount() override;

		DeviceInfo getDeviceInfo(int device) override;

		void closeStream() override;

		void startStream() override;

		void stopStream() override;

		void abortStream() override;

		// This function is intended for internal use only.  It must be
		// public because it is called by the internal callback handler,
		// which is not a member of RtAudio.  External use of this function
		// will most likely produce highly undesireable results!
		void callbackEvent();

	private:

		std::vector <DeviceInfo> devices_;

		void saveDeviceInfo();

		bool probeDeviceOpen(
				unsigned int device,
				StreamMode mode,
				unsigned int channels,
				unsigned int firstChannel) override;

		void startCallbackFunction();

		template <class Handle, class Info>
		void tryInput(Handle _handle, Info apiInfo);

		template <class Handle, class Info>
		void tryOutput(Handle _handle, Info apiInfo);

		template <class Device>
		void prepareStateOfDevice(Device _device);

		template <class Handle>
		void verifyUnderRunOrError(Handle _handle, int result);

		void unlockMutex();

		void unlockMutexOfAPIHandle();
	};
}


#endif //MAXIMILIAN_LINUXALSA_HPP
