# SDL experiments
Repo for proof of concept using SDL2 with jivelite.

SDL2 textures are used to render visualisers.
By virtue of using textures, the GPU is used to render visualisers.

Textures are loaded into a least recently used (LRU) cache to facilitate capping the number of bytes used to store textures.

The size of the LRU cache is configurable at runtime.
The LRU cache is implemented as a fixed size hash table (4093 entries).

This feature is particularly useful on memory restricted platforms like the raspberry Pi Zero 2W, which have a total of 512 MiB RAM. The current assumption is the on such a platform, 64 MiB can be spared for GPU.
To this end the scripts launching the application set the size of texture cache to 50000 bytes.

The application is multithreaded in such a manner such that all SDL operations using textures (including rendering) are performed on the main thread.

Other operations like input handling are delegated to other threads. The objectives are
 * run the rendering thread to with minimal stalling.
 * behave as much as possible - like a single threaded SDL application. The should improve stability when using the SDL software stack.

3 other threads exist
* input thread: Input events are despatched from the main thread to another queue from consumption by the this thread which handles the events, by invoking call handlers associated with widgets sensitive to the event screen location or input key value.
* player poll thread:  This thread polls the Lyrion Media Server (LMS) for the player meta data. It sets up data for consumption by widgets
* controller thread: Serves as a general purpose controller agent. For example it reads JSON files to setup the list of widgets to be displayed.

Inter thread synchronisation is largely achieved using atomics, especially when the render thread is involved.
The objective is to reduce stalling of the render thread.
Mutexes may be used for synchronisation of other threads.

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
Scripts in directory pcp support building on piCorePlayer
* download-sdl2.sh : Downloads the packages (tczs) required to build on piCorePlayer
  * packages are only downloaded but not loaded into memory and are not usable.
  * typically required just once or when updated
* load-sdl2-dev.sh : Loads the packages required to build on piCorePlayer into memory
  * usable after download-sdl2.sh has been invoked at least once
  * this is a subset of the packages loaded by load-sdl2-dev.sh
* load-sdl2-run.sh : Loads the packages required to run on piCorePLayer into memory
  * usable after download-sdl2.sh has been invoked at least once
  * this is a subset of the packages loaded by load-sdl2-dev.sh
  * this set of packages is insufficient to build the executables


## Design
### Texture Cache
### Rotation
### Render optimisation
During development satisfactory frame rates were not achievable when using 1920x1080 screen resolution, 
using the classic robust render of 
 - clear the texture
 - render all widgets in z-order

A significant drop in frame rates was observed fro VU meters with identical dynamic components,
but different backgrounds ( none vs dial).

Based on this observation an optimisation was introduced:

Rendering is split into 2 parts
* background
* foreground

An additional texture was introduced named "backdrop".

The target of background renders is the backdrop texture.

The target of foreground renders is the final texture ( see rotation ).

For each frame the backdrop is rendered and then the widget foregrounds.

Each widget sets a flag to indicate that it's background must be rendered afresh.
This is usually when content changes and also on widget creation.

The render loop queries all widgets for this flag,
then if any instance of the flag is set:
* clears the backdrop texture
* invokes the background render function on all widgets in order

This reduces repeated rendering of the same pixels.

Disadavantages: 
The z-order of widgets rendering is implicit in the order of declaration.

The split in rendering violates this implicit rule.
Foreground renders are always now above background renders.
Implicit z-order is respected w.r.t widget foreground renders.

Whilst this is undesirable, performance considerations are considered more
important.

With the current algorithm, to reap the performance benefits, there must be
fewer calls to update the backdrop.

This is only possible if widgets associated with frequently changing values
like track time (progress bar) and fps reporting are promoted to
foreground rendering.

Widgets which are hotspots are rendered as foreground - this usually is not
a violations as hotspots are overlaid and typically do not render unless
focussed on/ hovered over.

However this does result in further violation of z-order.

Again this is deemed acceptable:
* Typically widgets are not overlaid
* When they are then usually just one is rendering as foreground
  * examples progress bar over controls
  * VU meter over other controls
