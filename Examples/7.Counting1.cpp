#include "Maximilian.hpp"

using namespace Maximilian;

Oscilation mySine; // This is the oscillator we will use to generate the test tone
maxiClock myClock; // This will allow us to generate a clock signal and do things at specific times
double freq; // This is a variable that we will use to hold and set the current frequency of the oscillator

void setup()
{

	myClock.setTicksPerBeat(1);//This sets the number of ticks per beat
	myClock.setTempo(120);// This sets the tempo in Beats Per Minute
	freq = 20; // Here we initialise the variable
}

void play(double* output)
{

	myClock.ticker(); // This makes the clock object count at the current samplerate

	//This is a 'conditional'. It does a test and then does something if the test is true

	if (myClock.tick)
	{ // If there is an actual tick at this time, this will be true.

		freq += 100; // DO SOMETHING

	} // The curly braces close the conditional

	//output[0] is the left output. output[1] is the right output

	output[0] = mySine.sinewave(freq);//simple as that!
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

	Audio audio(SupportedArchitectures::Windows_Ds);

	StreamParameters parameters;
	parameters.deviceId = audio.getDefaultOutputDevice();
	parameters.nChannels = Settings::CHANNELS;
	parameters.firstChannel = 0;
	unsigned int sampleRate = Settings::SAMPLE_RATE;
	unsigned int bufferFrames = Settings::BUFFER_SIZE;
	//double data[maxiSettings::channels];
	vector <double> data(Settings::CHANNELS, 0);

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