cmake_minimum_required(VERSION 3.0)
include(ExternalProject)

# Testing library
FetchContent_Declare(
  catch
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v2.13.6)
FetchContent_MakeAvailable(catch)
# Adds Catch2::Catch2

# Add llama.cpp so we have access to a tokenizer

set(LLAMACPP_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/llamacpp)

ExternalProject_Add(llamacpp
    GIT_REPOSITORY https://github.com/ggerganov/llama.cpp
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${LLAMACPP_INSTALL_LOCATION} -DLLAMA_STATIC=Off -DBUILD_SHARED_LIBS=On -DLLAMA_BUILD_TESTS=Off -DLLAMA_BUILD_EXAMPLES=Off -DLLAMA_BUILD_SERVER=Off
)

include_directories(${LLAMACPP_INSTALL_LOCATION}/include)
link_directories(${LLAMACPP_INSTALL_LOCATION}/lib)

# Download the phi2 model
if (EXISTS "${CMAKE_BINARY_DIR}/tests/phi2.gguf")
    message("phi2.gguf Exists")
else()
    message("Downloading phi2.gguf")
    file(DOWNLOAD https://huggingface.co/TheBloke/phi-2-GGUF/resolve/main/phi-2.Q2_K.gguf ${CMAKE_BINARY_DIR}/tests/phi2.gguf)
endif()

# Tests need to be added as executables first
add_executable(testlmfe lmfetests.cpp jsonschemaparsertests.cpp testutils.cpp)

# I'm using C++17 in the test
target_compile_features(testlmfe PRIVATE cxx_std_17)

# Should be linked to the main library, as well as the Catch2 testing library
target_link_libraries(testlmfe PRIVATE lmfe_library Catch2::Catch2 llama ggml_shared)

# If you register a test, then ctest and make test will run it.
# You can also run examples and check the output, as well.
add_test(NAME testlmfetest COMMAND testlmfe) # Command can be a target
