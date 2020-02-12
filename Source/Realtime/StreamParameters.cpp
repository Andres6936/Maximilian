#include "Realtime/StreamParameters.hpp"

// Getters

unsigned int Maximilian::StreamParameters::getDeviceId() const
{
	return deviceId;
}

unsigned int Maximilian::StreamParameters::getNChannels() const
{
	return nChannels;
}

unsigned int Maximilian::StreamParameters::getFirstChannel() const
{
	return firstChannel;
}

// Setters

void Maximilian::StreamParameters::setDeviceId(unsigned int _deviceId)
{
	deviceId = _deviceId;
}
