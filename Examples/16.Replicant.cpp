#include "Maximilian.hpp"

using namespace Maximilian;

//Bizarelly, this sounds a little bit like Kraftwerk's 'Metropolis', although it isn't. Funny that.

Oscilation sound, bass, timer, mod, lead, lead2, leadmod;//here are the synth bits
Env envelope, leadenvelope;//some envelopes
Filter filter, filter2;//some filters
DelayLine delay;//a delay
Convert mtof;//a method for converting midi notes to frequency
double bassout, leadout, delayout;//some variables to hold the data and pass it around
int trigger, trigger2, newnote;//some control variables
int currentCount, lastCount, playHead = 0, currentChord = 0;//some other control variables
int pitch[8] = { 57, 57, 59, 60 };//the bassline for the arpeggio
int chord[8] = { 0, 0, 7, 2, 5, 5, 0, 0 };//the root chords for the arpeggio
float currentPitch, leadPitch;//the final pitch variables

//here's the lead line trigger array, followed by the pitches
int leadLineTrigger[256] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
							 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
							 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
							 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
							 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int leadLinePitch[15] = { 69, 67, 65, 64, 67, 66, 64, 62, 65, 64, 62, 57, 55, 60, 57 };


void setup()
{//some inits

}

void play(std::vector <double>& output)
{//this is where the magic happens. Very slow magic.

	currentCount = (int)timer.phasor(9);//this sets up a metronome that ticks every so often

	if (lastCount not_eq currentCount)
	{//if we have a new timer int this sample, play the sound
		trigger = 1;//play the arpeggiator line
		trigger2 = leadLineTrigger[playHead % 256];//play the lead line
		if (trigger2 == 1)
		{//if we are going to play a note
			leadPitch = mtof.mtof(leadLinePitch[newnote]);//get the next pitch val
			newnote++;//and iterate
			if (newnote > 14)
			{
				newnote = 0;//make sure we don't go over the edge of the array
			}
		}
		currentPitch = mtof.mtof(
				pitch[(playHead % 4)] + chord[currentChord % 8]);//write the frequency val into currentPitch
		playHead++;//iterate the playhead
		if (playHead % 32 == 0)
		{//wrap every 4 bars
			currentChord++;//change the chord
		}
		//cout << "tick\n";//the clock ticks
		lastCount = 0;//set lastCount to 0
	}

	bassout = filter2.lores(
			envelope.adsr(bass.saw(currentPitch * 0.5) + sound.pulse(currentPitch * 0.5, mod.phasor(1)), 1, 0.9995,
					0.25, 0.9995, 1, trigger), 9250, 2);//new, simple ADSR.
	leadout = filter.lores(
			leadenvelope.ar(lead2.saw(leadPitch * 4) + lead.pulse(leadPitch + (leadmod.sinebuf(1.9) * 1.5), 0.6),
					0.00005, 0.999975, 50000, trigger2), 5900, 10);//leadline

	delayout = (leadout + (delay.dl(leadout, 14000, 0.8) * 0.5)) / 2;//add some delay

	if (trigger not_eq 0)
	{ trigger = 0; }//set the trigger to off if you want it to trigger immediately next time.


	output[0] = (bassout) / 2;//sum output
	output[1] = (bassout) / 2;

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