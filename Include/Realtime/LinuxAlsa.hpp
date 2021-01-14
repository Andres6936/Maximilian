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

		/**
		 * Handle for the PCM device.
		 */
		snd_pcm_t* phandle = nullptr;

		/**
		 * This structure contains information about the hardware and can be
		 * used to specify the configuration to be used for the PCM stream.
		 */
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

		/**
		 * Verify the precondition general for several methods.
		 *
		 * This set of preconditions are:
		 * 	1. The PCM device has been initialized.
		 * 	2. The Hardware parameters has been initialized.
		 */
		void verifyGeneralPrecondition();

		/**
		 * Install the hardware configuration.
		 *
		 * The hardware parameters are not actually made active until the call to this function.
		 *
		 * Precondition:
		 * 	1. The PCM device has been initialized.
		 * 	2. The Hardware parameters has been initialized.
		 */
		void buildHW();

		/**
		 * Fill the Hardware parameters with default values.
		 *
		 * Precondition:
		 * 	1. The PCM device has been initialized.
		 * 	2. The Hardware parameters has been initialized.
		 */
		void getPCMDevice();

		/**
		 * Set the sample data rate. Default 44100 bits/second sampling rate (CD quality).
		 *
		 * Precondition:
		 * 	1. The PCM device has been initialized.
		 * 	2. The Hardware parameters has been initialized.
		 */
		void setHWSampleRate();

		/**
		 * Set the sample data format. Default Float (64 bits).
		 *
		 * Precondition:
		 * 	1. The PCM device has been initialized.
		 * 	2. The Hardware parameters has been initialized.
		 *
		 * @param index Value 0 for OUTPUT, value 1 for INPUT, value 2 for DUPLEX.
		 */
		void setHWFormat(const std::int32_t index);

		/**
		 * Set the period size or buffer size. For default the period size is 1,024.
		 *
		 * Precondition:
		 * 	1. The PCM device has been initialized.
		 * 	2. The Hardware parameters has been initialized.
		 *
		 * @param mode Possibles values: OUTPUT, INPUT and DUPLEX.
		 */
		void setHWPeriodSize(const StreamMode mode);

		/**
		 * Set the Interleaved mode.
		 *
		 * Precondition:
		 * 	1. The PCM device has been initialized.
		 * 	2. The Hardware parameters has been initialized.
		 *
		 * @param index Value 0 for OUTPUT, value 1 for INPUT, value 2 for DUPLEX.
		 */
		void setHWInterleaved(const std::int32_t index);

		/**
		 * Set the channel. For default Two channels (stereo).
		 *
		 * Precondition:
		 * 	1. The PCM device has been initialized.
		 * 	2. The Hardware parameters has been initialized.
		 *
		 * @param parameters Stream parameters. Determine the channels for the
		 *  stream and the first channel.
		 * @param index Value 0 for OUTPUT, value 1 for INPUT, value 2 for DUPLEX.
		 */
		void setHWChannels(const StreamParameters& parameters, const std::int32_t index);

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

		/**
		 * Verify condition of buffer underrun or overrun.
		 *
		 * @tparam Handle Type allow: snd_pcm_t
		 * @param _handle Handle for the PCM device.
		 * @param index Value 0 for Playback, any other value for Record.
		 * @param result Number of frames actually written in the PCM device.
		 */
		template<class Handle>
		void verifyUnderRunOrError(Handle _handle, int index, const std::int64_t result);
	};
}


#endif //MAXIMILIAN_LINUXALSA_HPP
