# DarkPlate - C++ demo of a license plate reader.
# Copyright 2021 Stephane Charette <stephanecharette@gmail.com>
# MIT license applies.  See "license.txt" for details.

ADD_EXECUTABLE (DarkPlate main.cpp)

target_include_directories(DarkPlate PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../main.h")
include_directories(DarkPlate PUBLIC "C:/DarknetOpenCV/DarkHelp/src-lib")

#set(DARKHELP "C:/DarknetOpenCV/DarkHelp/build/src-lib/Release")
TARGET_LINK_LIBRARIES (DarkPlate PRIVATE Threads::Threads ${DARKHELP} ${DARKNET} ${OpenCV_LIBS})

