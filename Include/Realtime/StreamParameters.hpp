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

		/**
		 * Device index (0 to getDeviceCount() - 1).
		 */
		unsigned int deviceId = 0;

		/**
		 * Number of channels.
		 */
		unsigned int nChannels = 2;

		/**
		 * First channel index on device (default = 0).
		 */
		unsigned int firstChannel = 0;

	public:

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
