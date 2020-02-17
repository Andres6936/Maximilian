#include "Realtime/DeviceInfo.hpp"

void Maximilian::DeviceInfo::determineChannelsForDuplexMode()
{
	if (this->outputChannels > 0 and this->inputChannels > 0)
	{
		if (this->outputChannels > this->inputChannels)
		{
			this->duplexChannels = this->inputChannels;
		}
		else
		{
			this->duplexChannels = this->outputChannels;
		}
	}
}

void Maximilian::DeviceInfo::determineChannelsForDefaultByDevice(int device)
{
	if (device == 0 and this->outputChannels > 0)
	{
		this->isDefaultOutput = true;
	}
	if (device == 0 and this->inputChannels > 0)
	{
		this->isDefaultInput = true;
	}
}
