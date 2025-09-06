cmake_minimum_required(VERSION 3.20)
project(kagamiz_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set library paths for Homebrew on macOS
if(APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew" ${CMAKE_PREFIX_PATH})
    set(CMAKE_LIBRARY_PATH "/opt/homebrew/lib" ${CMAKE_LIBRARY_PATH})
    set(CMAKE_INCLUDE_PATH "/opt/homebrew/include" ${CMAKE_INCLUDE_PATH})
endif()

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(yaml-cpp REQUIRED)

# Add cpp-httplib
include(FetchContent)
FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(cpp-httplib)

# Add cxxopts
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.1.1
)
FetchContent_MakeAvailable(cxxopts)

# Include directories
include_directories(${OpenSSL_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/main.cpp
    src/schema.cpp
    src/solver.cpp
    src/api_handler.cpp
)

# Headers
set(HEADERS
    src/schema.hpp
    src/solver.hpp
    src/api_handler.hpp
    src/nlohmann_json.hpp
)

# Create executable
add_executable(kagamiz_cpp ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(kagamiz_cpp
    OpenSSL::SSL
    OpenSSL::Crypto
    yaml-cpp::yaml-cpp
    httplib::httplib
    cxxopts::cxxopts
)

# Copy config.yaml.sample to build directory as config.yaml
configure_file(config.yaml config.yaml COPYONLY)

# Install target
install(TARGETS kagamiz_cpp DESTINATION bin)