cmake_minimum_required(VERSION 3.11)
project(rand_exif)

include_directories( /usr/local/include "${PROJECT_BINARY_DIR}" )

# compile libjpeg
file (GLOB LIBJPEG libjpeg/*.c)
add_library (jpeg ${LIBJPEG})
SET_TARGET_PROPERTIES( jpeg PROPERTIES COMPILE_FLAGS "-fPIC")

# compile rand_exif
#add_executable (rand_exif rand_exif.c)
#link_directories (/usr/local/include/ /usr/local/opt/libexif)
#target_link_libraries (rand_exif jpeg exif)
