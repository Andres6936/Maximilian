#include "Maximilian.hpp"
#include "Realtime/Audio.hpp"
#include "Audio.hpp"

//This shows how the fundamental building block of digital audio - the sine wave.
//
Maximilian::Oscilation mySine;//One oscillator - can be called anything. Can be any of the available waveforms.
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

	Maximilian::RtAudio dac(Maximilian::RtAudio::Windows_Ds);
	if (dac.getDeviceCount() < 1)
	{
		std::cout << "\nNo audio devices found!\n";
		char input;
		std::cin.get(input);
		exit(0);
	}

	Maximilian::RtAudio::StreamParameters parameters;
	parameters.deviceId = dac.getDefaultOutputDevice();
	parameters.nChannels = Maximilian::Settings::CHANNELS;
	parameters.firstChannel = 0;
	unsigned int sampleRate = Maximilian::Settings::SAMPLE_RATE;
	unsigned int bufferFrames = Maximilian::Settings::BUFFER_SIZE;
	//double data[maxiSettings::channels];
	vector <double> data(Maximilian::Settings::CHANNELS, 0);

	try
	{
		dac.openStream(&parameters, NULL, RTAUDIO_FLOAT64,
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