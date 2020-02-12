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

		int channels = 0;
		int inJump = 0;
		int outJump = 0;

		AudioFormat outFormat = AudioFormat::Float64;
		AudioFormat inFormat = AudioFormat::Float64;

		std::vector <int> inOffset;
		std::vector <int> outOffset;

	};
}


#endif //MAXIMILIAN_CONVERTINFO_HPP
