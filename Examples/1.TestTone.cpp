#include "Maximilian.hpp"

using namespace Maximilian;

//This shows how the fundamental building block of digital audio - the sine wave.
//
Oscilation mySine;//One oscillator - can be called anything. Can be any of the available waveforms.

void play(std::vector <double>& output)
{

	output[0] = mySine.sinewave(440);
	output[1] = output[0];

}

int main()
{
	Audio audio;
	audio.openStream(play);
	audio.startStream();

	char input;
	std::cout << "\nMaximilian is playing ... press <enter> to quit.\n";
	std::cin.get(input);

	// Stop the stream
	audio.stopStream();

	if (audio.isStreamOpen())
	{ audio.closeStream(); }
}