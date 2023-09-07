# See here: https://stackoverflow.com/questions/6921695/how-can-i-build-a-c-project-with-multiple-interdependent-subdirectories
# 1. Recursively locate all .cpp & .h files within the src directory
# 2. Loop over all header files and identify all necessary include directories (keep only unique entries)
# 3. Create the library with the list of sources
# 4. Add all the include directories as an INTERFACE so that anything linking it must as include those directories
file (GLOB_RECURSE Physics_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file (GLOB_RECURSE Physics_HEADERS CONFIGURE_DEPENDS "include/*.h")

set (Physics_INCLUDE_DIRS "")
foreach (_headerFile ${Physics_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND Physics_INCLUDE_DIRS ${_dir})
endforeach()
list (REMOVE_DUPLICATES Physics_INCLUDE_DIRS)

add_library(Physics ${Physics_SOURCES} ${Physics_HEADERS})
target_include_directories(Physics PUBLIC ${Physics_INCLUDE_DIRS})

# Add eigen as a private include directory
target_include_directories(Physics PRIVATE vendor/eigen)

# link to the compiler flags declared in the parent project
target_link_libraries(Physics PUBLIC compiler_flags)