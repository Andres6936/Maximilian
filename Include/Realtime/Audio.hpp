/*
    @brief Realtime audio i/o C++ classes.

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

// RtAudio: Version 4.0.9

#ifndef __RTAUDIO_H
#define __RTAUDIO_H

#include <string>
#include <vector>
#include <sstream>

#include "Exception.h"
#include "DeviceInfo.hpp"
#include "StreamOptions.hpp"
#include "StreamParameters.hpp"
#include "Definition/AudioFormat.hpp"
#include "Definition/AudioStreamFlags.hpp"
#include "Definition/AudioStreamStatus.hpp"
#include "Enum/SupportedArchitectures.hpp"

namespace Maximilian
{
	class AudioArchitecture;

	class Audio
	{

	protected:

		AudioArchitecture* audioArchitecture = nullptr;

	private:

		void assertThatAudioArchitectureHaveMinimumAnDevice();

		void tryInitializeInstanceOfArchitecture(SupportedArchitectures _architecture);

	public:

		/*!
		  The constructor performs minor initialization tasks.  No exceptions
		  can be thrown.

		  If no API argument is specified and multiple API support has been
		  compiled, the default order of use is JACK, ALSA, OSS (Linux
		  systems) and ASIO, DS (Windows systems).
		*/
		explicit Audio(SupportedArchitectures _architecture = SupportedArchitectures::Unspecified);

		static std::vector <SupportedArchitectures> getArchitecturesCompiled();


		//! The destructor.
		/*!
		  If a stream is running or open, it will be stopped and closed
		  automatically.
		*/
		~Audio() throw();

		//! Returns the audio API specifier for the current instance of RtAudio.
		SupportedArchitectures getCurrentApi() throw();

		//! A public function that queries for the number of audio devices available.
		/*!
		  This function performs a system query of available devices each time it
		  is called, thus supporting devices connected \e after instantiation. If
		  a system error occurs during processing, a warning will be issued.
		*/
		unsigned int getDeviceCount() throw();

		//! Return an RtDeviceInfo structure for a specified device number.
		/*!

		  Any device integer between 0 and getDeviceCount() - 1 is valid.
		  If an invalid argument is provided, an Exception (type = INVALID_USE)
		  will be thrown.  If a device is busy or otherwise unavailable, the
		  structure member "probed" will have a value of "false" and all
		  other members are undefined.  If the specified device is the
		  current default input or output device, the corresponding
		  "isDefault" member will have a value of "true".
		*/
		DeviceInfo getDeviceInfo(unsigned int device);

		//! A function that returns the index of the default output device.
		/*!
		  If the underlying audio API does not provide a "default
		  device", or if no devices are available, the return value will be
		  0.  Note that this is a valid device identifier and it is the
		  client's responsibility to verify that a device is available
		  before attempting to open a stream.
		*/
		static unsigned int getDefaultOutputDevice() throw();

		//! A function that returns the index of the default input device.
		/*!
		  If the underlying audio API does not provide a "default
		  device", or if no devices are available, the return value will be
		  0.  Note that this is a valid device identifier and it is the
		  client's responsibility to verify that a device is available
		  before attempting to open a stream.
		*/
		static unsigned int getDefaultInputDevice() throw();

		//! A public function for opening a stream with the specified parameters.
		/*!
		  An Exception (type = SYSTEM_ERROR) is thrown if a stream cannot be
		  opened with the specified parameters or an error occurs during
		  processing.  An Exception (type = INVALID_USE) is thrown if any
		  invalid device ID or channel number parameters are specified.

		  \param outputParameters Specifies output stream parameters to use
				 when opening a stream, including a device ID, number of channels,
				 and starting channel number.  For input-only streams, this
				 argument should be NULL.  The device ID is an index value between
				 0 and getDeviceCount() - 1.
		  \param inputParameters Specifies input stream parameters to use
				 when opening a stream, including a device ID, number of channels,
				 and starting channel number.  For output-only streams, this
				 argument should be NULL.  The device ID is an index value between
				 0 and getDeviceCount() - 1.
		  \param format An RtAudioFormat specifying the desired sample data format.
		  \param sampleRate The desired sample rate (sample frames per second).
		  \param *bufferFrames A pointer to a value indicating the desired
				 internal buffer size in sample frames.  The actual value
				 used by the device is returned via the same pointer.  A
				 value of zero can be specified, in which case the lowest
				 allowable value is determined.
		  \param callback A client-defined function that will be invoked
				 when input data is available and/or output data is needed.
		  \param userData An optional pointer to data that can be accessed
				 from within the callback function.
		  \param options An optional pointer to a structure containing various
				 global stream options, including a list of OR'ed RtAudioStreamFlags
				 and a suggested number of stream buffers that can be used to
				 control stream latency.  More buffers typically result in more
				 robust performance, though at a cost of greater latency.  If a
				 value of zero is specified, a system-specific median value is
				 chosen.  If the RTAUDIO_MINIMIZE_LATENCY flag bit is set, the
				 lowest allowable value is used.  The actual value used is
				 returned via the structure argument.  The parameter is API dependent.
		*/
		void openStream(void _functionUser(std::vector <double>&));

		//! A function that closes a stream and frees any associated stream memory.
		/*!
		  If a stream is not open, this function issues a warning and
		  returns (no exception is thrown).
		*/
		void closeStream() throw();

		//! A function that starts a stream.
		/*!
		  An Exception (type = SYSTEM_ERROR) is thrown if an error occurs
		  during processing.  An Exception (type = INVALID_USE) is thrown if a
		  stream is not open.  A warning is issued if the stream is already
		  running.
		*/
		void startStream();

		//! Stop a stream, allowing any samples remaining in the output queue to be played.
		/*!
		  An Exception (type = SYSTEM_ERROR) is thrown if an error occurs
		  during processing.  An Exception (type = INVALID_USE) is thrown if a
		  stream is not open.  A warning is issued if the stream is already
		  stopped.
		*/
		void stopStream();

		//! Stop a stream, discarding any samples remaining in the input/output queue.
		/*!
		  An Exception (type = SYSTEM_ERROR) is thrown if an error occurs
		  during processing.  An Exception (type = INVALID_USE) is thrown if a
		  stream is not open.  A warning is issued if the stream is already
		  stopped.
		*/
		void abortStream();

		//! Returns true if a stream is open and false if not.
		bool isStreamOpen() const throw();

		//! Returns true if the stream is running and false if it is stopped or not open.
		bool isStreamRunning() const throw();

		//! Returns the number of elapsed seconds since the stream was started.
		/*!
		  If a stream is not open, an Exception (type = INVALID_USE) will be thrown.
		*/
		double getStreamTime();

		//! Returns the internal stream latency in sample frames.
		/*!
		  The stream latency refers to delay in audio input and/or output
		  caused by internal buffering by the audio system and/or hardware.
		  For duplex streams, the returned value will represent the sum of
		  the input and output latencies.  If a stream is not open, an
		  Exception (type = INVALID_USE) will be thrown.  If the API does not
		  report latency, the return value will be zero.
		*/
		long getStreamLatency();

		//! Returns actual sample rate in use by the stream.
		/*!
		  On some systems, the sample rate used may be slightly different
		  than that specified in the stream parameters.  If a stream is not
		  open, an Exception (type = INVALID_USE) will be thrown.
		*/
		unsigned int getStreamSampleRate();

		//! Specify whether warning messages should be printed to stderr.
		void showWarnings(bool value = true) throw();
	};
}

// Operating system dependent thread functionality.
#if defined(__WINDOWS_DS__) || defined(__WINDOWS_ASIO__)
#include <windows.h>
#include <process.h>

typedef unsigned long ThreadHandle;
typedef CRITICAL_SECTION StreamMutex;

#elif defined(__LINUX_ALSA__) || defined(__UNIX_JACK__) || defined(__LINUX_OSS__) || defined(__MACOSX_CORE__)
// Using pthread library for various flavors of unix.
#include <pthread.h>

#else // Setup for "dummy" behavior

#define __RTAUDIO_DUMMY__
typedef int ThreadHandle;
typedef int StreamMutex;

#endif

// **************************************************************** //
//
// RtApi class declaration.
//
// Subclasses of RtApi contain all API- and OS-specific code necessary
// to fully implement the RtAudio API.
//
// Note that RtApi is an abstract base class and cannot be
// explicitly instantiated.  The class RtAudio will create an
// instance of an RtApi subclass (RtApiOss, RtApiAlsa,
// RtApiJack, RtApiCore, RtApiAl, RtApiDs, or RtApiAsio).
//
// **************************************************************** //

#include <sstream>

// RtApi Subclass prototypes.

#if defined(__MACOSX_CORE__)

#include <CoreAudio/AudioHardware.h>

class RtApiCore: public RtApi
{
public:

  RtApiCore();
  ~RtApiCore();
  RtAudio::Api getCurrentApi( void ) { return RtAudio::MACOSX_CORE; };
  unsigned int getDeviceCount( void );
  RtDeviceInfo getDeviceInfo( unsigned int device );
  unsigned int getDefaultOutputDevice( void );
  unsigned int getDefaultInputDevice( void );
  void closeStream( void );
  void startStream( void );
  void stopStream( void );
  void abortStream( void );
  long getStreamLatency( void );

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesireable results!
  bool callbackEvent( AudioDeviceID deviceId,
					  const AudioBufferList *inBufferList,
					  const AudioBufferList *outBufferList );

  private:

  bool probeDeviceOpen( unsigned int device, StreamMode mode, unsigned int channels,
						unsigned int firstChannel, unsigned int sampleRate,
						RtAudioFormat format, unsigned int *bufferSize,
						RtAudio::StreamOptions *options );
  static const char* getErrorCode( OSStatus code );
};

#endif

#if defined(__UNIX_JACK__)

class RtApiJack: public RtApi
{
public:

  RtApiJack();
  ~RtApiJack();
  RtAudio::Api getCurrentApi( void ) { return RtAudio::UNIX_JACK; };
  unsigned int getDeviceCount( void );
  RtDeviceInfo getDeviceInfo( unsigned int device );
  void closeStream( void );
  void startStream( void );
  void stopStream( void );
  void abortStream( void );
  long getStreamLatency( void );

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesireable results!
  bool callbackEvent( unsigned long nframes );

  private:

  bool probeDeviceOpen( unsigned int device, StreamMode mode, unsigned int channels, 
						unsigned int firstChannel, unsigned int sampleRate,
						RtAudioFormat format, unsigned int *bufferSize,
						RtAudio::StreamOptions *options );
};

#endif

#if defined(__WINDOWS_ASIO__)

class RtApiAsio: public RtApi
{
public:

  RtApiAsio();
  ~RtApiAsio();
  RtAudio::Api getCurrentApi( void ) { return RtAudio::WINDOWS_ASIO; };
  unsigned int getDeviceCount( void );
  RtDeviceInfo getDeviceInfo( unsigned int device );
  void closeStream( void );
  void startStream( void );
  void stopStream( void );
  void abortStream( void );
  long getStreamLatency( void );

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesireable results!
  bool callbackEvent( long bufferIndex );

  private:

  std::vector<RtDeviceInfo> devices_;
  void saveDeviceInfo( void );
  bool coInitialized_;
  bool probeDeviceOpen( unsigned int device, StreamMode mode, unsigned int channels, 
						unsigned int firstChannel, unsigned int sampleRate,
						RtAudioFormat format, unsigned int *bufferSize,
						RtAudio::StreamOptions *options );
};

#endif

#if defined(__WINDOWS_DS__)

class RtApiDs: public RtApi
{
public:

  RtApiDs();
  ~RtApiDs();
  RtAudio::Api getCurrentApi( void ) { return RtAudio::WINDOWS_DS; };
  unsigned int getDeviceCount( void );
  unsigned int getDefaultOutputDevice( void );
  unsigned int getDefaultInputDevice( void );
  RtDeviceInfo getDeviceInfo( unsigned int device );
  void closeStream( void );
  void startStream( void );
  void stopStream( void );
  void abortStream( void );
  long getStreamLatency( void );

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesireable results!
  void callbackEvent( void );

  private:

  bool coInitialized_;
  bool buffersRolling;
  long duplexPrerollBytes;
  bool probeDeviceOpen( unsigned int device, StreamMode mode, unsigned int channels, 
						unsigned int firstChannel, unsigned int sampleRate,
						RtAudioFormat format, unsigned int *bufferSize,
						RtAudio::StreamOptions *options );
};

#endif

#if defined(__LINUX_ALSA__)

#endif

#if defined(__LINUX_OSS__)

class RtApiOss: public RtApi
{
public:

  RtApiOss();
  ~RtApiOss();
  RtAudio::Api getCurrentApi() { return RtAudio::LINUX_OSS; };
  unsigned int getDeviceCount( void );
  RtDeviceInfo getDeviceInfo( unsigned int device );
  void closeStream( void );
  void startStream( void );
  void stopStream( void );
  void abortStream( void );

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesireable results!
  void callbackEvent( void );

  private:

  bool probeDeviceOpen( unsigned int device, StreamMode mode, unsigned int channels, 
						unsigned int firstChannel, unsigned int sampleRate,
						RtAudioFormat format, unsigned int *bufferSize,
						RtAudio::StreamOptions *options );
};

#endif

#if defined(__RTAUDIO_DUMMY__)

class RtApiDummy: public RtApi
{
public:

  RtApiDummy() { errorText_ = "RtApiDummy: This class provides no functionality."; error( Exception::WARNING ); };
  RtAudio::Api getCurrentApi( void ) { return RtAudio::RTAUDIO_DUMMY; };
  unsigned int getDeviceCount( void ) { return 0; };
  RtDeviceInfo getDeviceInfo( unsigned int device ) { RtDeviceInfo info; return info; };
  void closeStream( void ) {};
  void startStream( void ) {};
  void stopStream( void ) {};
  void abortStream( void ) {};

  private:

  bool probeDeviceOpen( unsigned int device, StreamMode mode, unsigned int channels, 
						unsigned int firstChannel, unsigned int sampleRate,
						RtAudioFormat format, unsigned int *bufferSize,
						RtAudio::StreamOptions *options ) { return false; };
};

#endif

#endif