#ifndef MAXIMILIAN_AUDIOSTREAMFLAGS_HPP
#define MAXIMILIAN_AUDIOSTREAMFLAGS_HPP

namespace Maximilian
{

	/*! \typedef typedef unsigned long RtAudioStreamFlags;
			\brief RtAudio stream option flags.

			The following flags can be OR'ed together to allow a client to
			make changes to the default stream behavior:

			- \e RTAUDIO_NONINTERLEAVED:   Use non-interleaved buffers (default = interleaved).
			- \e RTAUDIO_MINIMIZE_LATENCY: Attempt to set stream parameters for lowest possible latency.
			- \e RTAUDIO_HOG_DEVICE:       Attempt grab device for exclusive use.
			- \e RTAUDIO_ALSA_USE_DEFAULT: Use the "default" PCM device (ALSA only).

			By default, RtAudio streams pass and receive audio data from the
			client in an interleaved format.  By passing the
			RTAUDIO_NONINTERLEAVED flag to the openStream() function, audio
			data will instead be presented in non-interleaved buffers.  In
			this case, each buffer argument in the RtAudioCallback function
			will point to a single array of data, with \c nFrames samples for
			each channel concatenated back-to-back.  For example, the first
			sample of data for the second channel would be located at index \c
			nFrames (assuming the \c buffer pointer was recast to the correct
			data type for the stream).

			Certain audio APIs offer a number of parameters that influence the
			I/O latency of a stream.  By default, RtAudio will attempt to set
			these parameters internally for robust (glitch-free) performance
			(though some APIs, like Windows Direct Sound, make this difficult).
			By passing the RTAUDIO_MINIMIZE_LATENCY flag to the openStream()
			function, internal stream settings will be influenced in an attempt
			to minimize stream latency, though possibly at the expense of stream
			performance.

			If the RTAUDIO_HOG_DEVICE flag is set, RtAudio will attempt to
			open the input and/or output stream device(s) for exclusive use.
			Note that this is not possible with all supported audio APIs.

			If the RTAUDIO_SCHEDULE_REALTIME flag is set, RtAudio will attempt
			to select realtime scheduling (round-robin) for the callback thread.

			If the RTAUDIO_ALSA_USE_DEFAULT flag is set, RtAudio will attempt to
			open the "default" PCM device when using the ALSA API. Note that this
			will override any specified input or output device id.
		*/
	using AudioStreamFlags = unsigned int;

	static constexpr AudioStreamFlags RTAUDIO_NONINTERLEAVED = 0x1;    // Use non-interleaved buffers (default = interleaved).
	static constexpr AudioStreamFlags RTAUDIO_MINIMIZE_LATENCY = 0x2;  // Attempt to set stream parameters for lowest possible latency.
	static constexpr AudioStreamFlags RTAUDIO_HOG_DEVICE = 0x4;        // Attempt grab device and prevent use by others.
	static constexpr AudioStreamFlags RTAUDIO_SCHEDULE_REALTIME = 0x8; // Try to select realtime scheduling for callback thread.
	static constexpr AudioStreamFlags RTAUDIO_ALSA_USE_DEFAULT = 0x10; // Use the "default" PCM device (ALSA only).

}

#endif //MAXIMILIAN_AUDIOSTREAMFLAGS_HPP
