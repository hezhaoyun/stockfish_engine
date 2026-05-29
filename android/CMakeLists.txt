cmake_minimum_required(VERSION 3.4.1)
project(stockfish)

# Enable 16KB page size support for Android (required for Google Play)
add_link_options(-Wl,-z,max-page-size=16384)
add_link_options(-Wl,-z,common-page-size=16384)

file(GLOB_RECURSE cppPaths CONFIGURE_DEPENDS "../ios/stockfish/Sources/stockfish/Engine/src/*.cpp")
add_library(
  stockfish
  SHARED
  ../ios/stockfish/Sources/stockfish/FlutterStockfish/ffi.cpp
  ${cppPaths}
)

if(ANDROID_ABI STREQUAL arm64-v8a)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(stockfish PRIVATE -DUSE_PTHREADS -DIS_64BIT -DUSE_POPCNT -DUSE_NEON=8)
  else()
    target_compile_options(stockfish PRIVATE -fno-exceptions -DUSE_PTHREADS -DNDEBUG -O3 -DIS_64BIT -DUSE_POPCNT -DUSE_NEON=8)
  endif()
else()
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(stockfish PRIVATE -DUSE_PTHREADS)
  else()
    target_compile_options(stockfish PRIVATE -fno-exceptions -DUSE_PTHREADS -DNDEBUG -O3)
  endif()
endif()

# Disable NNUE embedding. The app copies NNUE files from Flutter assets and
# configures them at runtime with UCI EvalFile options.
target_compile_definitions(stockfish PRIVATE NNUE_EMBEDDING_OFF)
