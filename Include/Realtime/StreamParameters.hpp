#ifndef MAXIMILIAN_STREAMPARAMETERS_HPP
#define MAXIMILIAN_STREAMPARAMETERS_HPP


namespace Maximilian
{
	/**
	 * The structure for specifying input or ouput stream parameters.
	 */
	class StreamParameters
	{

	private:

	public:

		unsigned int deviceId = 0;     /*!< Device index (0 to getDeviceCount() - 1). */
		unsigned int nChannels = 2;    /*!< Number of channels. */
		unsigned int firstChannel = 0; /*!< First channel index on device (default = 0). */

		// Default constructor.
		StreamParameters() = default;

		// Getters

		[[nodiscard]] unsigned int getDeviceId() const;

		[[nodiscard]] unsigned int getNChannels() const;

		[[nodiscard]] unsigned int getFirstChannel() const;

		// Setters

		void setDeviceId(unsigned int _deviceId);
	};
}


#endif //MAXIMILIAN_STREAMPARAMETERS_HPP
