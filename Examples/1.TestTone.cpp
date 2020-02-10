#include "Maximilian.hpp"
#include "Realtime/Audio.hpp"

using namespace Maximilian;

//This shows how the fundamental building block of digital audio - the sine wave.
//
Oscilation mySine;//One oscillator - can be called anything. Can be any of the available waveforms.

void setup()
{//some inits
	//nothing to go here this time
}

void play(double* output)
{

	output[0] = mySine.sinewave(440);
	output[1] = output[0];

}

int routing(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
		double streamTime, RtAudioStreamStatus status, void* userData)
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

	Audio audio(Audio::SupportedArchitectures::Windows_Ds);

	Audio::StreamParameters parameters;
	parameters.deviceId = audio.getDefaultOutputDevice();
	parameters.nChannels = Settings::CHANNELS;
	parameters.firstChannel = 0;
	unsigned int sampleRate = Settings::SAMPLE_RATE;
	unsigned int bufferFrames = Settings::BUFFER_SIZE;
	//double data[maxiSettings::channels];
	std::vector <double> data(Settings::CHANNELS, 0);

	try
	{
		audio.openStream(parameters, RTAUDIO_FLOAT64,
				sampleRate, &bufferFrames, &routing, (void*)&(data[0]));

		audio.startStream();
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
		audio.stopStream();
	}
	catch (Exception& e)
	{
		e.printMessage();
	}

	if (audio.isStreamOpen())
	{ audio.closeStream(); }

	return 0;
}