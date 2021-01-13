# Device

Under Linux, a **device** is considered to be any object (physical or virtual)
that is attached to a bus (also either physical or virtual). The Linux kernel
accesses devices that are represented in the '/dev/' filesystem structure and
their specification is usually a filename within that directory (for example a
CD-ROM drive might be '/dev/hdc', a camera might be '/dev/video0', and a
soundcard might be '/dev/dsp').

An ALSA device is more specifically a physical or virtual object for which ALSA
provides device drivers to access and control its functionality. With ALSA
providing the interface to the sound hardware, it is no longer necessary for
users (and application programmers) to worry about the kernel devices as
presented in the '/dev/' filesystem; ALSA presents the user with its own set of
devices which are more standardized, flexible, and sophisticated than the kernel
audio devices. ALSA devices are specified by a string following the format "
interface:card,device" where:

* interface is a description of an ALSA protocol for accesses. Currently, the
  two main interfaces are "hw", which provides direct communication to the
  kernel device, and "plughw", which might provide translation from a
  standardized protocol to one which is supported by the kernel device (for
  example, changing an unsupported frame rate to one which the kernel device can
  handle).

* card is a number ("0", "1", "2", etc.) which specifies to which kernel audio
  device the ALSA device belongs. The kernel device might be a physical card (
  such as a SoundBlaster or Ensoniq) or a virtual device (such as the "virmidi"
  virtual MIDI device).

* device is the number of the ALSA device on the specified card. There are three
  main types of ALSA devices: digital audio devices (such as PCM capture and
  playback devices), control devices (such as mixers or equalizers), MIDI
  devices (such as sequencers or sound generators). The device numbers as
  presented in the specification string ("interface:card,device") are not
  unique, device #0 might be a control device, an audio device, or a MIDI
  device; which device is actually being specified is determined by the ALSA
  library function that is called.