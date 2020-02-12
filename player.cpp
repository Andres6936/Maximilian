/*
 *  player.cpp
 *  rtaudiotest
 *
 *  Created by Chris on 23/08/2011.
 *  Copyright 2011 Goldsmiths Creative Computing. All rights reserved.
 *
 */

#include "Maximilian.hpp"

#include <vector>
#include <iostream>

using namespace Maximilian;

void play(double* output);//run dac! Very very often. Too often in fact. er...

int routing(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
		double streamTime, AudioStreamStatus status, void* userData)
{
	unsigned int i, j;

	double* buffer = (double*)outputBuffer;

	double* lastValues = (double*)userData;
	//	double currentTime = (double) streamTime; Might come in handy for control
	if (status)
	{
		std::cout << "Stream underflow detected!" << std::endl;
	}
	for (i = 0; i < nBufferFrames; i++)
	{
	}
	// Write interleaved audio data.
	for (i = 0; i < nBufferFrames; i++)
	{
		play(lastValues);
		for (j = 0; j < Settings::CHANNELS; j++)
		{
			*buffer++ = lastValues[j];
		}
	}
	return 0;
}

int main()
{
	Audio dac(SupportedArchitectures::Windows_Ds);

	unsigned int sampleRate = Settings::SAMPLE_RATE;
	unsigned int bufferFrames = Settings::BUFFER_SIZE;

	std::vector <double> data(Settings::CHANNELS, 0);

	try
	{
		dac.openStream(RTAUDIO_FLOAT64,
				sampleRate, &bufferFrames, &routing, (void*)&(data[0]));

		dac.startStream();
	}
	catch (Exception& e)
	{
		e.printMessage();
		exit(0);
	}

	char input;
	std::cout << "\nMaximilian is playing ... press <enter> to quit.\n";
	std::cin.get(input);

	try
	{
		// Stop the stream
		dac.stopStream();
	}
	catch (Exception& e)
	{
		e.printMessage();
	}

	if (dac.isStreamOpen())
	{ dac.closeStream(); }

	return 0;
}
