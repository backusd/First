cmake_minimum_required(VERSION 3.26)

project("First" VERSION 1.0)

# Create a 'compiler flags' interface that can be used by linked libraries
# - Force the use of C++23
add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_23)

# Create helper variables to determine which compiler we are using
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

# Enable compiler flags depending on which compiler we are using
target_compile_options(compiler_flags INTERFACE
  "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
  "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
)

# Generate the configuration header
configure_file("include/FirstConfig.h.in" "generated/FirstConfig.h")

# Add the Physics subdirectory so that the Physics library will be included
add_subdirectory(Physics)


# See here: https://stackoverflow.com/questions/6921695/how-can-i-build-a-c-project-with-multiple-interdependent-subdirectories
# 1. Recursively locate all .cpp & .h files within the src directory
# 2. Loop over all header files and identify all necessary include directories (keep only unique entries)
# 3. Create the executable with the list of sources
# 4. Include all include directories in the target
file (GLOB_RECURSE First_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file (GLOB_RECURSE First_HEADERS CONFIGURE_DEPENDS "include/*.h")

set (First_INCLUDE_DIRS "")
foreach (_headerFile ${First_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND First_INCLUDE_DIRS ${_dir})
endforeach()
list (REMOVE_DUPLICATES First_INCLUDE_DIRS)

add_executable(First ${First_SOURCES} ${First_HEADERS})
target_include_directories(First PRIVATE ${First_INCLUDE_DIRS})



# Link the Physics library
target_link_libraries(First PUBLIC Physics)

# Include the following directories:
#	* build (aka binary) - because that is where the 'FirstConfig.h' will be generated
#   * build/src - this is where the generated header files will be created
target_include_directories(First PUBLIC 
							"${PROJECT_BINARY_DIR}/generated")