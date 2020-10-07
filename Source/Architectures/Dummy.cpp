// Joan Andrés (@Andres6936) Github.

#include "Architectures/Dummy.hpp"

using namespace Maximilian;

unsigned int Architectures::Dummy::getDeviceCount() const noexcept
{
	return -1;
}

SupportedArchitectures Architectures::Dummy::getCurrentArchitecture() const noexcept
{
	return SupportedArchitectures::Audio_Dummy;
}

DeviceInfo Architectures::Dummy::getDeviceInfo(int device)
{
	return DeviceInfo();
}

void Architectures::Dummy::closeStream() noexcept
{

}

void Architectures::Dummy::startStream() noexcept
{

}

void Architectures::Dummy::stopStream() noexcept
{

}

void Architectures::Dummy::abortStream() noexcept
{

}
