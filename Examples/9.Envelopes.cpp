//Envelopes allow you to shape the sound. The basic idea is that a sound has the following shape
// Attack: This is how long it takes to fade up to maximum volume
// Decay: This is how long it takes to reach the sustain level.
// Sustain: This is the sustain level
// Release: This is how long it takes to fade out.

#include "Maximilian.hpp"

using namespace Maximilian;

Oscilation myCounter, mySwitchableOsc;//
int CurrentCount;//
double myOscOutput, myCurrentVolume;//
Env myEnvelope;


void setup()
{//some inits

	//Timing is in ms

	myEnvelope.setAttack(0);
	myEnvelope.setDecay(1);  // Needs to be at least 1
	myEnvelope.setSustain(1);
	myEnvelope.setRelease(1000);

}

void play(double* output)
{

	//notice that we feed in a value of 1. to create an envelope shape we can apply later.
	myCurrentVolume = myEnvelope.adsr(1., myEnvelope.trigger);

	CurrentCount = myCounter.phasor(1, 1, 9);//phasor can take three arguments; frequency, start value and end value.

	// You'll notice that these 'if' statements don't require curly braces "{}".
	// This is because there is only one outcome if the statement is true.

	if (CurrentCount == 1)
	{
		myEnvelope.trigger = 1; //trigger the envelope

	}
	else
	{ myEnvelope.trigger = 0; }//release the envelope to make it fade out only if it's been triggered

	if (CurrentCount < 5)
	{

		myOscOutput = mySwitchableOsc.sawn(CurrentCount * 100);
	}

	else if (CurrentCount >= 5)
	{//and the 'else' bit.

		myOscOutput = mySwitchableOsc.sinewave(
				CurrentCount * 50);
	}//one osc object can produce whichever waveform you want.


	output[0] = myOscOutput * myCurrentVolume;//left speaker
	output[1] = output[0];

}


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
	setup();

	Audio dac(SupportedArchitectures::Windows_Ds);

	StreamParameters parameters;
	parameters.deviceId = dac.getDefaultOutputDevice();
	parameters.nChannels = Settings::CHANNELS;
	parameters.firstChannel = 0;
	unsigned int sampleRate = Settings::SAMPLE_RATE;
	unsigned int bufferFrames = Settings::BUFFER_SIZE;
	//double data[maxiSettings::channels];
	vector <double> data(Settings::CHANNELS, 0);

	try
	{
		dac.openStream(parameters, RTAUDIO_FLOAT64,
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