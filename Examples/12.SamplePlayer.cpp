#include "Maximilian.hpp"

#include <vector>

using namespace Maximilian;

Sample beats; //We give our sample a name. It's called beats this time. We could have loads of them, but they have to have different names.

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

	std::vector <double> data(Settings::CHANNELS, 0);

	try
	{
		audio.openStream(&routing, (void*)&(data[0]));

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