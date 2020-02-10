#include "Maximilian.hpp"
#include "Realtime/Audio.hpp"
#include "Audio.hpp"

Maximilian::Sample beats; //We give our sample a name. It's called beats this time. We could have loads of them, but they have to have different names.

void setup()
{//some inits

	beats.load(
			"/home/andres6936/CLionProjects/Maximilian/beat2.wav");//load in your samples. Provide the full path to a wav file.
	printf("Summary:\n%s", beats.getSummary());//get info on samples if you like.

}

void play(double* output)
{//this is where the magic happens. Very slow magic.

	//output[0]=beats.play();//just play the file. Looping is default for all play functions.
	output[0] = beats.play(0.68);//play the file with a speed setting. 1. is normal speed.
	//output[0]=beats.play(0.5,0,44100);//linear interpolationplay with a frequency input, start point and end point. Useful for syncing.
	//output[0]=beats.play4(0.5,0,44100);//cubic interpolation play with a frequency input, start point and end point. Useful for syncing.

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