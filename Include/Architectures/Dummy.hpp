// Joan Andrés (@Andres6936) Github.

#ifndef MAXIMILIAN_DUMMY_HPP
#define MAXIMILIAN_DUMMY_HPP

#include <Realtime/IAudioArchitecture.hpp>

namespace Maximilian::Architectures
{

	class Dummy : public IAudioArchitecture
	{

	public:

		bool probeDeviceOpen(std::uint32_t device, StreamMode mode, std::uint32_t channels,
				std::uint32_t firstChannel) noexcept override;

		SupportedArchitectures getCurrentArchitecture() const noexcept override;

		unsigned int getDeviceCount() const noexcept override;

		DeviceInfo getDeviceInfo(int device) override;

		void closeStream() noexcept override;

		void startStream() noexcept override;

		void stopStream() noexcept override;

		void abortStream() noexcept override;

	};

}

#endif //MAXIMILIAN_DUMMY_HPP
