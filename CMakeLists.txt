cmake_minimum_required(VERSION 3.30)
project(w32-pong)

add_executable(w32-pong)
target_sources(w32-pong PRIVATE src/main.cpp)
target_compile_features(w32-pong PRIVATE cxx_std_23)

if(MSVC)
   target_compile_options(w32-pong PRIVATE -permissive- -W4 -WX -wd4100 -wd4189)
   target_link_options(w32-pong PRIVATE -SUBSYSTEM:WINDOWS Winmm.lib)
else()
   target_compile_options(w32-pong PRIVATE -pedantic -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter)

   if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      target_compile_options(w32-pong PRIVATE -Wno-missing-designated-field-initializers)
   endif()

endif()
