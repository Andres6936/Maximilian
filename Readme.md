## What's Maximilian?

Maximilian is an audio synthesis and signal processing library written in C++.
It's cross-platform compatible with MacOS, Windows, Linux and IOS systems. The
main features are:

- sample playback, recording and looping
- read from WAV and OGG files.
- a selection of oscillators and filters
- enveloping
- multichannel mixing for 1, 2, 4 and 8 channel setups
- controller mapping functions
- effects including delay, distortion, chorus, flanging
- granular synthesis, including time and pitch stretching
- atom synthesis
- realtime music information retrieval functions: spectrum analysis, spectral
  features, octave analysis, Bark scale analysis, and MFCCs
- example projects for Windows and MacOS, using command line and OpenFrameworks
  environments

### Basic Examples

You can choose between using RTAudio and PortAudio drivers in player.h by
uncommenting the appropriate line. To use PortAudio, you will need to compile
the portAudio library from http://http://www.portaudio.com/ and link it with
your executable.

Examples demonstrating different features can be found in the
maximilian_examples folder. To try them, replace the contents of main.cpp with
the contents of a tutorial file and compile.

#### MAC OSX XCode Project

You can run the examples using the 'maximilianTest' XCode 3 project provided.

#### Windows Visual Studio 2010 Project

This is in the maximilianTestWindowsVS2010 folder. You will need to install the
DirectX SDK, so that the program can use DirectSound.

#### COMMAND LINE COMPILATION IN MACOSX

> g++ -Wall -D__MACOSX_CORE__ -o maximilian main.cpp RtAudio.cpp player.cpp maximilian.cpp -framework CoreAudio -framework CoreFoundation -lpthread

> ./maximilian

#### COMMAND LINE COMPILATION IN LINUX

With OSS:
> g++ -Wall -D__LINUX_OSS__ -o maximilian main.cpp RtAudio.cpp player.cpp maximilian.cpp -lpthread

With ALSA:
> g++ -Wall -D__LINUX_ALSA__ -o maximilian main.cpp RtAudio.cpp player.cpp maximilian.cpp -lasound -lpthread

With Jack:
> g++ -Wall -D__UNIX_JACK__ -o maximilian main.cpp RtAudio.cpp player.cpp maximilian.cpp `pkg-config --cflags --libs jack` -lpthread

then:
> ./maximilian

