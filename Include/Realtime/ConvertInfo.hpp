#ifndef MAXIMILIAN_CONVERTINFO_HPP
#define MAXIMILIAN_CONVERTINFO_HPP

#include "Definition/AudioFormat.hpp"

#include <vector>

namespace Maximilian
{

	/**
	 * A protected structure used for buffer conversion.
	 */
	class ConvertInfo
	{

	private:

	public:

		int channels;
		int inJump, outJump;
		RtAudioFormat inFormat, outFormat;
		std::vector <int> inOffset;
		std::vector <int> outOffset;

	};
}


#endif //MAXIMILIAN_CONVERTINFO_HPP
