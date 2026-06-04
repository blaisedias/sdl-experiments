# SDL experiments
Repo for proof of concept using SDL2 with jivelite.

SDL2 textures are used to render visualisers.
By virtue of using textures, the GPU is used to render visualisers.

Textures are loaded into an LRU cache which makes it possible to cap the number of bytes used to store textures.


The size of the LRU cache is configurable at runtime.
The LRU cache is implemented as a fixed size hash table (4093 entries).

This feature is particularly useful on memory restricted platforms like the raspberry Pi Zero 2W, which have a total of 512 MiB RAM. The current assumption is the on sucha a platform 64 MiB can be spared for GPU. To this end the scripts launching the application set the size of texture cache to 50000 bytes.

The application is multithreaded in such a manner such that all SDL operations related to textures (including rendering) are performed on the main thread.

Other operations like input handling are delegated to other threads. The objectives are
 * run the rendering thread to with minimal stalling.
 * behave as much as possible - like a single threaded SDL application. The should improve stability when using the SDL software stack.

3 other threads exist
* input thread: Input events are despatched from the main thread to another queue from consumption by the this thread which handles the events, by invoking call handlers associated with widgets sensitive to the event screen location or input key value.
* player poll thread:  This thread polls the Lyrion Media Server (LMS) for the player meta data. It sets up data for consumption by widgets
* controller thread: Serves as a general purpose controller agent. For example it reads JSON files to setup the list of widgets to be displayed.

Inter thread synchronisation is largely achieved using atomics, especially when the render thread is involved. This is to avoid stalling of the render thread. Mutexes may be used for synchronisation of other threads.

## Interface with Lyrion Media Server
At the moment this uses sockets and the CLI interface to communicate with LMS and not the JSON RPC interface.

See https://lyrion.org/reference/cli/introduction/

In the future this may be replaced with a JSON RPC implementation.

## Requirements
* git
* make
* python3
* gcc
* SDL2
  * SDL2_image
  * SDL2_ttf

optional:
* tslib

## Git
```
git clone https://github.com/blaisedias/sdl-experiments.git
cd sdl-experiments/
git submodule update --init --recursive
```

## Build
```
    make
```

## Run
```
    ./scripts/runsq.sh
```
help
```
    ./scripts/runsq.sh -h
```

## Test executables
TBD

# piCorePlayer
Scripts in direcrtory pcp support building on piCorePlayer
* download-sdl2.sh : Downloads the packages (tczs) required to build on piCorePlayer
  * packages are only dowloaded but not loaded into memory and are not usable.
  * typically required just once or when updated
* load-sdl2-dev.sh : Loads the packages required to build on piCorePlayer into memory
  * usable after download-sdl2.sh has been invoked at least once
  * this is a subset of the packages loaded by load-sdl2-dev.sh
* load-sdl2-run.sh : Loads the packages required to run on piCorePLayer into memory
  * usable after download-sdl2.sh has been invoked at least once
  * this is a subset of the packages loaded by load-sdl2-dev.sh
  * this set of packages is insufficient to build the executables

