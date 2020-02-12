#ifndef MAXIMILIAN_AUDIOSTREAMSTATUS_HPP
#define MAXIMILIAN_AUDIOSTREAMSTATUS_HPP

namespace Maximilian
{
	/*! \typedef typedef unsigned long RtAudioStreamStatus;
			\brief RtAudio stream status (over- or underflow) flags.

			Notification of a stream over- or underflow is indicated by a
			non-zero stream \c status argument in the RtAudioCallback function.
			The stream status can be one of the following two options,
			depending on whether the stream is open for output and/or input:

			- \e RTAUDIO_INPUT_OVERFLOW:   Input data was discarded because of an overflow condition at the driver.
			- \e RTAUDIO_OUTPUT_UNDERFLOW: The output buffer ran low, likely producing a break in the output sound.
		*/
	using AudioStreamStatus = unsigned int;

	/**
	 * Input data was discarded because of an overflow condition at the driver.
	 */
	static constexpr AudioStreamStatus RTAUDIO_INPUT_OVERFLOW = 0x1;

	/**
	 * The output buffer ran low, likely causing a gap in the output sound.
	 */
	static constexpr AudioStreamStatus RTAUDIO_OUTPUT_UNDERFLOW = 0x2;
}

#endif //MAXIMILIAN_AUDIOSTREAMSTATUS_HPP
