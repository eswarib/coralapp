cmake_minimum_required(VERSION 3.10)
project(VoiceTyper)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
pkg_check_modules(SNDFILE REQUIRED sndfile)

include_directories(${PORTAUDIO_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS} whispercpp/include whispercpp/ggml/include)
link_directories(${PORTAUDIO_LIBRARY_DIRS} ${SNDFILE_LIBRARY_DIRS})

add_executable(vocal
    main.cpp
    recorder.cpp
    transcriber.cpp
    textInjector.cpp
)

target_link_libraries(vocal
    ${PORTAUDIO_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${CMAKE_CURRENT_SOURCE_DIR}/whispercpp/build/src/libwhisper.so
)

