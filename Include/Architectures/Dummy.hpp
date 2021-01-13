// Joan Andrés (@Andres6936) Github.

#ifndef MAXIMILIAN_DUMMY_HPP
#define MAXIMILIAN_DUMMY_HPP

#include <Realtime/IAudioArchitecture.hpp>

namespace Maximilian::Architectures
{

	class Dummy : public IAudioArchitecture
	{

	public:

		bool probeDeviceOpen(const StreamMode mode,
				const StreamParameters& parameters) noexcept override;

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
