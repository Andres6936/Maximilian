// Joan Andrés (@Andres6936) Github.

#ifndef MAXIMILIAN_DUMMY_HPP
#define MAXIMILIAN_DUMMY_HPP

#include <Realtime/IAudioArchitecture.hpp>

namespace Maximilian::Architectures
{

	class Dummy : public IAudioArchitecture
	{

	public:

		unsigned int getDeviceCount() const noexcept override;

		SupportedArchitectures getCurrentArchitecture() const noexcept override;

		DeviceInfo getDeviceInfo(int device) override;

		void startStream() override;

		void stopStream() override;

		void abortStream() override;

	};

}

#endif //MAXIMILIAN_DUMMY_HPP
