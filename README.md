### What is Kii Thing SDK?
SDK developed for IoT Cloud powered by Kii.<br>
https://developer.kii.com

Written in C and focusing embedded linux.

### How to install Kii Thing SDK in your environment
Generate source package by following command.
```sh
cd KiiThingSDK
make archive
```
Source package libkii.tar.gz would be generated.
Copy this file in your development environement
and extract codes in your project.

kii\_cloud.h is public APIs header file.
include this file from your application.
### Build and install Kii Thing SDK as shared library using CMAKE
##### Dependency
Bellow is the dependency to sucessfully build with `CMAKE`
1. CMAKE (required)
2. libtool (required)
3. libcurl (required)
4. libjansson (optional, you can build and install `jansson` as shared library by adding `-DBUILD_JANSSON` on CMAKE execution)
5. automake (required if you wants to build jansson)

##### Build with cmake for unix native environment

Create `build` directory
```bash
mkdir build 
cd build
```
###### Mac OSX and Linux x86
Without building `jansson`
```bash
mkdir build 
cd build
cmake ..
make
```
With building `jansson`
```bash
mkdir build 
cd build
cmake .. -DBUILD_JANSSON=true
sudo make
```

###### Raspberry Pi
Without building `jansson`
```bash
mkdir build 
cd build
cmake .. -DNO_INLINE=true
make
```
With building `jansson`
```bash
mkdir build 
cd build
cmake .. -DBUILD_JANSSON=true -DNO_INLINE=true
sudo make
```

### How to use it?
Please refer to
https://github.com/KiiCorp/KiiThingSDK/blob/master/KiiThingSDKTests/ThingSDKBasicTests.m



[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/KiiCorp/kiithingsdk/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

