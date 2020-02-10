//This examples shows another fundamental building block of digital audio - adding two sine waves together. When you add waves together they create a new wave whose amplitude at any time is computed by adding the current amplitudes of each wave together. So, if one wave has an amplitude of 1, and the other has an amplitude of 1, the new wave will be equal to 2 at that point in time. Whereas, later, if one wave has an amplitude of -1, and the other has an amplitude of 1, the new wave - the one you hear - will equal 0. This can create some interesting effects, including 'beating', when the waves interact to create a single wave that fades up and down based on the frequencies of the two interacting waves. The frequency of the 'beating' i.e. the fading in and out, is equal to the difference in frequency between the two waves.

#include "Maximilian.hpp"
#include "Realtime/Audio.hpp"

Maximilian::Oscilation mySine, myOtherSine;//Two oscillators with names.

void setup()
{//some inits
	//nothing to go here this time
}

void play(double* output)
{//this is where the magic happens. Very slow magic.

	//output[0] is the left output. output[1] is the right output
	output[0] = mySine.sinewave(440) +
				myOtherSine.sinewave(441);//these two sines will beat together. They're now a bit too loud though..
	output[1] = output[0];

}

int routing(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
		double streamTime, Maximilian::RtAudioStreamStatus status, void* userData)
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
		for (j = 0; j < Maximilian::Settings::CHANNELS; j++)
		{
			*buffer++ = lastValues[j];
		}
	}
	return 0;
}

int main()
{
	setup();

	Maximilian::Audio dac(Maximilian::Audio::SupportedArchitectures::Windows_Ds);
	if (dac.getDeviceCount() < 1)
	{
		std::cout << "\nNo audio devices found!\n";
		char input;
		std::cin.get(input);
		exit(0);
	}

	Maximilian::Audio::StreamParameters parameters;
	parameters.deviceId = dac.getDefaultOutputDevice();
	parameters.nChannels = Maximilian::Settings::CHANNELS;
	parameters.firstChannel = 0;
	unsigned int sampleRate = Maximilian::Settings::SAMPLE_RATE;
	unsigned int bufferFrames = Maximilian::Settings::BUFFER_SIZE;
	//double data[maxiSettings::channels];
	vector <double> data(Maximilian::Settings::CHANNELS, 0);

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