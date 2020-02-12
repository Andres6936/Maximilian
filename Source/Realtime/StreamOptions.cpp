#include "Realtime/StreamOptions.hpp"

Maximilian::AudioStreamFlags Maximilian::StreamOptions::getFlags() const
{
	return flags;
}

int Maximilian::StreamOptions::getPriority() const
{
	return priority;
}

unsigned int Maximilian::StreamOptions::getNumberOfBuffers() const
{
	return numberOfBuffers;
}

void Maximilian::StreamOptions::setNumberOfBuffers(unsigned int _numberOfBuffers)
{
	numberOfBuffers = _numberOfBuffers;
}
