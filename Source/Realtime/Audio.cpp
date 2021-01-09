/************************************************************************/
/*! \class RtAudio
    \brief Realtime audio i/o C++ classes.

    RtAudio provides a common API (Application Programming Interface)
    for realtime audio input/output across Linux (native ALSA, Jack,
    and OSS), Macintosh OS X (CoreAudio and Jack), and Windows
    (DirectSound and ASIO) operating systems.

    RtAudio WWW site: http://www.music.mcgill.ca/~gary/rtaudio/

    RtAudio: realtime audio i/o C++ classes
    Copyright (c) 2001-2011 Gary P. Scavone

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation files
    (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    Any person wishing to distribute modifications to the Software is
    asked to send the modifications to the original developer so that
    they can be incorporated into the canonical version.  This is,
    however, not a binding provision of this license.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/************************************************************************/

// RtAudio: Version 4.0.9

#include "Architectures/Dummy.hpp"
#include "Realtime/Audio.hpp"
#include "Realtime/LinuxAlsa.hpp"

#include <memory>
#include <iostream>
#include <exception>

#include <Levin/Log.h>
#include <Levin/Logger.h>

using namespace Maximilian;

std::vector <SupportedArchitectures> Audio::getArchitecturesCompiled() noexcept
{
	std::vector <SupportedArchitectures> architectures;

	// The order here will control the order of RtAudio's API search in
	// the constructor.
#if defined(__UNIX_JACK__)
	apis.push_back( UNIX_JACK );
#endif
#if defined(__LINUX_ALSA__)
	architectures.push_back(SupportedArchitectures::Linux_Alsa);
#endif
#if defined(__LINUX_OSS__)
	apis.push_back( LINUX_OSS );
#endif
#if defined(__WINDOWS_ASIO__)
	apis.push_back( WINDOWS_ASIO );
#endif
#if defined(__WINDOWS_DS__)
	apis.push_back( WINDOWS_DS );
#endif
#if defined(__MACOSX_CORE__)
	apis.push_back( MACOSX_CORE );
#endif
#if defined(__RTAUDIO_DUMMY__)
	apis.push_back( RTAUDIO_DUMMY );
#endif

	return architectures;
}

void Audio::tryInitializeInstanceOfArchitecture(SupportedArchitectures _architecture) noexcept
{
	// This methods is design for be called several times of way consecutive
	// to found an instance that content an device.

	// Is necessary deleted the pointer several times if this it initialized
	// in any called, and that no have an device valid.

	// Deleted an nullptr no have effect or consequences.
	audioArchitecture.reset(nullptr);


#if defined(__UNIX_JACK__)
																															if ( api == UNIX_JACK )
    rtapi_ = new RtApiJack();
#endif
#if defined(__LINUX_ALSA__)
	if (_architecture == SupportedArchitectures::Linux_Alsa)
	{
		audioArchitecture = std::make_unique<LinuxAlsa>();
		// Exit methods
		return;
	}
#endif
#if defined(__LINUX_OSS__)
																															if ( api == LINUX_OSS )
    rtapi_ = new RtApiOss();
#endif
#if defined(__WINDOWS_ASIO__)
																															if ( api == WINDOWS_ASIO )
    rtapi_ = new RtApiAsio();
#endif
#if defined(__WINDOWS_DS__)
																															if ( api == WINDOWS_DS )
    rtapi_ = new RtApiDs();
#endif
#if defined(__MACOSX_CORE__)
																															if ( api == MACOSX_CORE )
    rtapi_ = new RtApiCore();
#endif
#if defined(__RTAUDIO_DUMMY__)
																															if ( api == RTAUDIO_DUMMY )
    rtapi_ = new RtApiDummy();
#endif

	// No compiled support for specified API value.  Issue a debug
	// warning and continue as if no API was specified.
	Levin::Warn() << "No compiled support for specified API argument." << Levin::endl;
}

void Audio::assertThatAudioArchitectureHaveMinimumAnDevice() noexcept
{
	if (audioArchitecture == nullptr)
	{
		Levin::Error() << "No compiled API support found... Use of Dummy Audio (Not functional)."
		<< Levin::endl;

		audioArchitecture = std::make_unique<Architectures::Dummy>();
	}

	if (audioArchitecture->getDeviceCount() < 0)
	{
		Levin::Error() << "No Audio Devices Found" << Levin::endl;
	}
}

Audio::Audio(SupportedArchitectures _architecture) noexcept
{
	// Initialize Levin for use of Log
	Levin::LOGGER = std::make_unique <Levin::ColoredLogger>(std::wcout);

	if (_architecture != SupportedArchitectures::Unspecified)
	{
		// Attempt to open the specified API.
		tryInitializeInstanceOfArchitecture(_architecture);

		// If the instance ha been correctly initialized
		if (audioArchitecture != nullptr)
		{
			assertThatAudioArchitectureHaveMinimumAnDevice();
			return;
		}
	}

	// If the instance of Audio Architect is null,
	// try use an architect available

	// Iterate through the compiled APIs and return as soon as we find
	// one with at least one device or we reach the end of the list.
	std::vector <SupportedArchitectures> supportedArchitectures = getArchitecturesCompiled();

	for (SupportedArchitectures architecture : supportedArchitectures)
	{
		tryInitializeInstanceOfArchitecture(architecture);

		if (audioArchitecture->getDeviceCount() >= 1)
		{
			// Only need of an Audio Architecture with a number
			// of device greater to one, when we found it architecture
			// and initialize, we can exit of for loop.
			break;
		}
	}

	// If the for-loop finalize and the instance not have an device, throw exception
	assertThatAudioArchitectureHaveMinimumAnDevice();
}

void Audio::openStream(void _functionUser(std::vector <double>&)) noexcept
{
	return audioArchitecture->openStream(_functionUser);
}

SupportedArchitectures Audio::getCurrentApi() const noexcept
{
	return audioArchitecture->getCurrentArchitecture();
}

unsigned int Audio::getDeviceCount() const noexcept
{
	return audioArchitecture->getDeviceCount();
}

DeviceInfo Audio::getDeviceInfo(unsigned int device)
{
	return audioArchitecture->getDeviceInfo(device);
}

unsigned int Audio::getDefaultInputDevice() noexcept
{
	return IAudioArchitecture::getDefaultInputDevice();
}

unsigned int Audio::getDefaultOutputDevice() noexcept
{
	return IAudioArchitecture::getDefaultOutputDevice();
}

void Audio::closeStream() noexcept
{
	return audioArchitecture->closeStream();
}

void Audio::startStream() noexcept
{
	return audioArchitecture->startStream();
}

void Audio::stopStream() noexcept
{
	return audioArchitecture->stopStream();
}

void Audio::abortStream() noexcept
{
	return audioArchitecture->abortStream();
}

bool Audio::isStreamOpen() const noexcept
{
	return audioArchitecture->isStreamOpen();
}

bool Audio::isStreamRunning() const noexcept
{
	return audioArchitecture->isStreamRunning();
}

long Audio::getStreamLatency()
{
	return audioArchitecture->getStreamLatency();
}

unsigned int Audio::getStreamSampleRate()
{
	return audioArchitecture->getStreamSampleRate();
};

double Audio::getStreamTime()
{
	return audioArchitecture->getStreamTime();
}

void Audio::showWarnings(bool value) noexcept
{
	audioArchitecture->showWarnings(value);
}