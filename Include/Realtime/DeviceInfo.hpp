#ifndef MAXIMILIAN_DEVICEINFO_HPP
#define MAXIMILIAN_DEVICEINFO_HPP

#include "AudioStream.hpp"

#include <string>

namespace Maximilian
{

	/**
	 * The public device information structure for returning queried values.
	 */
	class DeviceInfo
	{

	private:

	public:

		bool probed = false;                  /*!< true if the device capabilities were successfully probed. */
		unsigned int outputChannels = 0;  /*!< Maximum output channels supported by device. */
		unsigned int inputChannels = 0;   /*!< Maximum input channels supported by device. */
		unsigned int duplexChannels = 0;  /*!< Maximum simultaneous input/output channels supported by device. */
		bool isDefaultOutput = false;         /*!< true if this is the default output device. */
		bool isDefaultInput = false;          /*!< true if this is the default input device. */
		std::vector <unsigned int> sampleRates; /*!< Supported sample rates (queried from list of standard rates). */
		AudioFormat nativeFormats = AudioFormat::Float64;  /*!< Bit mask of supported data formats. */

		// Default constructor.
		DeviceInfo() = default;

	};
}


#endif //MAXIMILIAN_DEVICEINFO_HPP
