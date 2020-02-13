#ifndef MAXIMILIAN_AUDIOSTREAMSTATUS_HPP
#define MAXIMILIAN_AUDIOSTREAMSTATUS_HPP

namespace Maximilian
{
	/**
	 * @brief Stream status (over- or underflow) flags.

			Notification of a stream over- or underflow is indicated by a
			non-zero stream \c status argument in the RtAudioCallback function.
			The stream status can be one of the following two options,
			depending on whether the stream is open for output and/or input:

			- \e RTAUDIO_INPUT_OVERFLOW:   Input data was discarded because of an overflow condition at the driver.
			- \e RTAUDIO_OUTPUT_UNDERFLOW: The output buffer ran low, likely producing a break in the output sound.
		*/
	enum class AudioStreamStatus : unsigned int
	{
		None,
		// Input data was discarded because of an overflow condition at the driver.
				RTAUDIO_INPUT_OVERFLOW,
		// The output buffer ran low, likely causing a gap in the output sound.
				RTAUDIO_OUTPUT_UNDERFLOW,
	};
}

#endif //MAXIMILIAN_AUDIOSTREAMSTATUS_HPP
