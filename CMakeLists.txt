CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(Maximilian)

SET(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(Include)

# Added the library Levin for purpose of log
INCLUDE_DIRECTORIES(Library)
# Here find the .a, .so, .dll, .lib
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/Library)

ADD_DEFINITIONS(-D__LINUX_ALSA__)

ADD_LIBRARY(Maximilian
        Source/main.cpp
        Source/maximilian.cpp
        Source/Realtime/Audio.cpp)

TARGET_LINK_LIBRARIES(Maximilian PRIVATE pthread asound)
# Link to Maximilian the library Levin
TARGET_LINK_LIBRARIES(Maximilian PRIVATE Levin)

ADD_EXECUTABLE(ExampleMain
        player.cpp)

TARGET_LINK_LIBRARIES(ExampleMain PRIVATE Maximilian)

ADD_EXECUTABLE(TestTone Examples/1.TestTone.cpp)
TARGET_LINK_LIBRARIES(TestTone PRIVATE Maximilian)

ADD_EXECUTABLE(TwoTones Examples/2.TwoTones.cpp)
TARGET_LINK_LIBRARIES(TwoTones PRIVATE Maximilian)

ADD_EXECUTABLE(AM1 Examples/3.AM1.cpp)
TARGET_LINK_LIBRARIES(AM1 PRIVATE Maximilian)

ADD_EXECUTABLE(Counting1 Examples/7.Counting1.cpp)
TARGET_LINK_LIBRARIES(Counting1 PRIVATE Maximilian)

ADD_EXECUTABLE(Envelopes Examples/9.Envelopes.cpp)
TARGET_LINK_LIBRARIES(Envelopes PRIVATE Maximilian)

ADD_EXECUTABLE(SamplePlayer Examples/12.SamplePlayer.cpp)
TARGET_LINK_LIBRARIES(SamplePlayer PRIVATE Maximilian)