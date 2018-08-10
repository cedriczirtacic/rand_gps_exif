# rand_gps_exif
Randomize GPS EXIF data of JPEG images.

## Build:
```bash
$ ./build.sh
-- The C compiler identification is AppleClang 9.1.0.9020039
-- The CXX compiler identification is AppleClang 9.1.0.9020039
-- Check for working C compiler: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc
-- Check for working C compiler: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++
-- Check for working CXX compiler: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ -- works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done
-- Generating done
-- Build files have been written to: /Users/cedriczirtacic/Development/rand_gps_exif
Scanning dependencies of target jpeg
[ 33%] Building C object CMakeFiles/jpeg.dir/libjpeg/jpeg-data.c.o
[ 66%] Building C object CMakeFiles/jpeg.dir/libjpeg/jpeg-marker.c.o
[100%] Linking C static library libjpeg.a
[100%] Built target jpeg
```

### Requirements:
 * libexif

## Usage:
`rand_gps_exif` takes more than one JPEG file and will randomize:
 * GPSLatitude
 * GPSLatitudeRef
 * GPSLongitude
 * GPSLongitudeRef

```bash
$ ./rand_gps_exif dickbutt.jpg && exiftool -GPS* dickbutt.jpg
GPS Version ID                  : 2.3.0.0
GPS Latitude Ref                : South
GPS Longitude Ref               : East
GPS Latitude                    : 52 deg 52' 48.40" S
GPS Longitude                   : 35 deg 1' 11.30" E
GPS Position                    : 52 deg 52' 48.40" S, 35 deg 1' 11.30" E
$ ./rand_gps_exif dickbutt.jpg && exiftool -GPS* dickbutt.jpg
GPS Version ID                  : 2.3.0.0
GPS Latitude Ref                : North
GPS Longitude Ref               : West
GPS Latitude                    : 4 deg 39' 28.70" N
GPS Longitude                   : 62 deg 33' 29.80" W
GPS Position                    : 4 deg 39' 28.70" N, 62 deg 33' 29.80" W
```
And can delete those fields if needed.
```bash
$ ./rand_gps_exif -d dickbutt.jpg && exiftool -GPS* dickbutt.jpg
GPS Version ID                  : 2.3.0.0
```

More info: [GPS tag information](https://sno.phy.queensu.ca/~phil/exiftool/TagNames/GPS.html)
