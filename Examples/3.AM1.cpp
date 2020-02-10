#include "Maximilian.hpp"
#include "Realtime/Audio.hpp"

//This shows how to use maximilian to do basic amplitude modulation. Amplitude modulation is when you multiply waves together. In maximilian you just use the * inbetween the two waveforms.

Maximilian::Oscilation mySine, myOtherSine;//Two oscillators. They can be called anything. They can be any of the available waveforms. These ones will be sinewaves

void setup()
{//some inits
	//nothing to go here this time
}

void play(double* output)
{

	// This form of amplitude modulation is straightforward multiplication of two waveforms.
	// Notice that the maths is different to when you add waves.
	// The waves aren't 'beating'. Instead, the amplitude of one is modulating the amplitude of the other
	// Remember that the sine wave has positive and negative sections as it oscillates.
	// When you multiply something by -1, its phase is inverted but it retains its amplitude.
	// So you hear 2 waves per second, not 1, even though the frequency is 1.
	output[0] = mySine.sinewave(440) * myOtherSine.sinewave(1);
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

	RtAudio dac(RtAudio::WINDOWS_DS);
	if (dac.getDeviceCount() < 1)
	{
		std::cout << "\nNo audio devices found!\n";
		char input;
		std::cin.get(input);
		exit(0);
	}

	RtAudio::StreamParameters parameters;
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