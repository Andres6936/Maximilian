/************************************************************************/
/*! \class Exception
    \brief Exception handling class for RtAudio & RtMidi.

    The Exception class is quite simple but it does allow errors to be
    "caught" by Exception::Type. See the RtAudio and RtMidi
    documentation to know which methods can throw an Exception.

*/
/************************************************************************/

#ifndef RTERROR_H
#define RTERROR_H

#include <exception>
#include <string_view>

#include <Levin/Log.h>

class Exception : public std::exception
{

public:
	//! Defined Exception types.
	enum Type
	{
		WARNING,           /*!< A non-critical error. */
		DEBUG_WARNING,     /*!< A non-critical error which might be useful for debugging. */
		UNSPECIFIED,       /*!< The default, unspecified error type. */
		NO_DEVICES_FOUND,  /*!< No devices found on system. */
		INVALID_DEVICE,    /*!< An invalid device ID was specified. */
		MEMORY_ERROR,      /*!< An error occured during memory allocation. */
		INVALID_PARAMETER, /*!< An invalid parameter was specified to a function. */
		INVALID_USE,       /*!< The function was called incorrectly. */
		DRIVER_ERROR,      /*!< A system driver error occured. */
		SYSTEM_ERROR,      /*!< A system error occured. */
		THREAD_ERROR       /*!< A thread error occured. */
	};


private:

	std::string_view message;

public:

	explicit Exception(std::string_view _message)
	{
		message = _message;
	}

	~Exception() override = default;

	virtual void printMessage() const
	{
		Levin::Error() << "Exception: " << message.data() << Levin::endl;
	}

	//! Returns the thrown error message string.
	[[nodiscard]] virtual std::string_view getMessage() const
	{
		return message;
	}

	//! Returns the thrown error message as a c-style string.
	[[nodiscard]] const char* what() const noexcept override
	{
		return message.data();
	}
};

#endif
