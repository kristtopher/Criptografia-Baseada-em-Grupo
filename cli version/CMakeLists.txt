cmake_minimum_required(VERSION 3.14)

project(iot_key_core LANGUAGES CXX)

# Find includes in the build directories
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(QT_VERSION 5)
set(REQUIRED_LIBS Core Mqtt)
set(REQUIRED_LIBS_QUALIFIED Qt5::Core Qt5::Mqtt)

add_library(iot_key_core SHARED
        device.h
        device.cpp
        FFT.cpp
        FFT.h
        mqttserver.h
        mqttserver.cpp
        trevor.cpp
        trevor.h
        iot-key-agreement-core_global.h
        experiments.cpp
        experiments.h
        MyApp.h)

if (NOT CMAKE_PREFIX_PATH)
    message(WARNING "CMAKE_PREFIX_PATH is not defined, you may need to set it "
            "(-DCMAKE_PREFIX_PATH=\"path/to/Qt/lib/cmake\" or -DCMAKE_PREFIX_PATH=/usr/include/{host}/qt{version}/ on Ubuntu)")
endif ()

add_executable(experiment main.cpp)
FIND_PACKAGE( Boost COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
find_package(Qt${QT_VERSION} COMPONENTS ${REQUIRED_LIBS} REQUIRED)
target_link_libraries(iot_key_core ${REQUIRED_LIBS_QUALIFIED} ${Boost_LIBRARIES} -lgmp -lgmpxx)
target_link_libraries(experiment iot_key_core ${REQUIRED_LIBS_QUALIFIED}  )
