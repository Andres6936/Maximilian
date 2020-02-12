#ifndef MAXIMILIAN_SUPPORTEDARCHITECTURES_HPP
#define MAXIMILIAN_SUPPORTEDARCHITECTURES_HPP

namespace Maximilian
{
	//! Audio API specifier arguments.
	enum class SupportedArchitectures : unsigned char
	{
		Unspecified,    /*!< Search for a working compiled API. */
		Linux_Alsa,     /*!< The Advanced Linux Sound Architecture API. */
		Linux_Oss,      /*!< The Linux Open Sound System API. */
		Unix_Jack,      /*!< The Jack Low-Latency Audio Server API. */
		MacOs_Core,    /*!< Macintosh OS-X Core Audio API. */
		Windows_Asio,   /*!< The Steinberg Audio Stream I/O API. */
		Windows_Ds,     /*!< The Microsoft Direct Sound API. */
		Audio_Dummy   /*!< A compilable but non-functional API. */
	};
}

#endif //MAXIMILIAN_SUPPORTEDARCHITECTURES_HPP
