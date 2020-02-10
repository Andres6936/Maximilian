
#include "Maximilian.hpp"
#include "Realtime/Audio.hpp"
#include "Audio.hpp"

Maximilian::Oscilation mySine; // This is the oscillator we will use to generate the test tone
Maximilian::maxiClock myClock; // This will allow us to generate a clock signal and do things at specific times
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

	Maximilian::RtAudio dac(Maximilian::RtAudio::WINDOWS_DS);
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