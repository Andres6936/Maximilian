CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(Maximilian VERSION 2020.0211 LANGUAGES CXX)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

INCLUDE_DIRECTORIES(Include)
INCLUDE_DIRECTORIES(Source/Realtime/PRIVATE)

# Added the library Levin for purpose of log
ADD_SUBDIRECTORY(Library/Levin/)

ADD_DEFINITIONS(-D__LINUX_ALSA__)

ADD_LIBRARY(Maximilian STATIC
        Source/main.cpp
        Source/Architectures/Dummy.cpp
        Source/maximilian.cpp
        Source/Realtime/Audio.cpp
        Source/Realtime/IAudioArchitecture.cpp
        Source/Realtime/LinuxAlsa.cpp
        Source/Realtime/AudioStream.cpp
        Source/Realtime/ConvertInfo.cpp
        Source/Realtime/DeviceInfo.cpp
        Source/Realtime/StreamParameters.cpp
        Source/Realtime/StreamOptions.cpp
        Source/Realtime/PRIVATE/Linux/ALSA/AlsaHandle.cpp)

# Define a macro for use in 'if constexpr' for print debug messages.
TARGET_COMPILE_DEFINITIONS(Maximilian PRIVATE $<$<CONFIG:DEBUG>:MAXIMILIAN_DEBUG>)

TARGET_LINK_LIBRARIES(Maximilian PRIVATE pthread asound)
# Link to Maximilian the library Levin
TARGET_LINK_LIBRARIES(Maximilian PUBLIC Levin::Framework)

ADD_EXECUTABLE(ExampleMain player.cpp)
TARGET_LINK_LIBRARIES(ExampleMain PRIVATE Maximilian)

ADD_EXECUTABLE(TestTone Examples/1.TestTone.cpp)
TARGET_LINK_LIBRARIES(TestTone PRIVATE Maximilian)

ADD_EXECUTABLE(TwoTones Examples/2.TwoTones.cpp)
TARGET_LINK_LIBRARIES(TwoTones PRIVATE Maximilian)

ADD_EXECUTABLE(AM1 Examples/3.AM1.cpp Include/Definition/AudioFormat.hpp)
TARGET_LINK_LIBRARIES(AM1 PRIVATE Maximilian)

ADD_EXECUTABLE(Counting1 Examples/7.Counting1.cpp)
TARGET_LINK_LIBRARIES(Counting1 PRIVATE Maximilian)

ADD_EXECUTABLE(Envelopes Examples/9.Envelopes.cpp)
TARGET_LINK_LIBRARIES(Envelopes PRIVATE Maximilian)

ADD_EXECUTABLE(SamplePlayer Examples/12.SamplePlayer.cpp)
TARGET_LINK_LIBRARIES(SamplePlayer PRIVATE Maximilian)

ADD_EXECUTABLE(Replicant Examples/16.Replicant.cpp)
TARGET_LINK_LIBRARIES(Replicant PRIVATE Maximilian)

ADD_EXECUTABLE(Polysynth Examples/15.polysynth.cpp)
TARGET_LINK_LIBRARIES(Polysynth PRIVATE Maximilian)