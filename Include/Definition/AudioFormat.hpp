#ifndef MAXIMILIAN_AUDIOFORMAT_HPP
#define MAXIMILIAN_AUDIOFORMAT_HPP


namespace Maximilian
{

	/**
	 * @brief RtAudio data format type.
	 *
	 * Support for signed integers and floats.  Audio data fed to/from an
	 * RtAudio stream is assumed to ALWAYS be in host byte order.  The
	 * internal routines will automatically take care of any necessary
   	 * byte-swapping between the host format and the soundcard.  Thus,
	 * endian-ness is not a concern in the following format definitions.
	 * Note that 24-bit data is expected to be encapsulated in a 32-bit
	 * format.

	 * - \e RTAUDIO_SINT8:   8-bit signed integer.
	 * - \e RTAUDIO_SINT16:  16-bit signed integer.
	 * - \e RTAUDIO_SINT24:  Lower 3 bytes of 32-bit signed integer.
	 * - \e RTAUDIO_SINT32:  32-bit signed integer.
	 * - \e RTAUDIO_FLOAT32: Normalized between plus/minus 1.0.
	 * - \e RTAUDIO_FLOAT64: Normalized between plus/minus 1.0.
	 */
	using AudioFormat = unsigned long;

	static constexpr AudioFormat RTAUDIO_SINT8 = 0x1;    // 8-bit signed integer.
	static constexpr AudioFormat RTAUDIO_SINT16 = 0x2;   // 16-bit signed integer.
	static constexpr AudioFormat RTAUDIO_SINT24 = 0x4;   // Lower 3 bytes of 32-bit signed integer.
	static constexpr AudioFormat RTAUDIO_SINT32 = 0x8;   // 32-bit signed integer.
	static constexpr AudioFormat RTAUDIO_FLOAT32 = 0x10; // Normalized between plus/minus 1.0.
	static constexpr AudioFormat RTAUDIO_FLOAT64 = 0x20; // Normalized between plus/minus 1.0.
}


#endif //MAXIMILIAN_AUDIOFORMAT_HPP
