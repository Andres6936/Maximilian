#ifndef MAXIMILIAN_LINUXALSA_HPP
#define MAXIMILIAN_LINUXALSA_HPP

#include "Audio.hpp"
#include "AudioArchitecture.hpp"

namespace Maximilian
{
	class LinuxAlsa : public AudioArchitecture
	{
	public:

		LinuxAlsa();

		~LinuxAlsa();

		Audio::SupportedArchitectures getCurrentApi()
		{
			return Audio::Linux_Alsa;
		};

		unsigned int getDeviceCount();

		Audio::DeviceInfo getDeviceInfo(unsigned int device);

		void closeStream();

		void startStream();

		void stopStream();

		void abortStream();

		// This function is intended for internal use only.  It must be
		// public because it is called by the internal callback handler,
		// which is not a member of RtAudio.  External use of this function
		// will most likely produce highly undesireable results!
		void callbackEvent();

	private:

		std::vector <Audio::DeviceInfo> devices_;

		void saveDeviceInfo();

		bool probeDeviceOpen(unsigned int device, StreamMode mode, unsigned int channels,
				unsigned int firstChannel, unsigned int sampleRate,
				RtAudioFormat format, unsigned int* bufferSize,
				Audio::StreamOptions* options);
	};
}


#endif //MAXIMILIAN_LINUXALSA_HPP
