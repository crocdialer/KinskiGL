KinskiGL
========

![Preview](http://crocdialer.com/kinskiGL/kinski_cover.jpg)

kinskiGL is a set of portable C++ libraries useful for realtime-graphic applications.

currently known to compile and work for these platforms:

* MacOS 10.12
* Ubuntu Desktop 16.04.3 LTS
* iOS
* Rasbian Stretch on Rasberry Pi
* Linaro Strech on Asus Tinkerboard

the basic library-structure:

* core

    * image IO
    * asynchronous networking
    * serial ports
    * timers
    * property/component framework
    * serialization
    * threadpools
    * utilities

* gl

    * linear algebra utilities
    * drawing utilities
    * geometric primitives, ray-casting
    * scenegraph, forward/deferred rendering paths
    * lights and materials
    * post-fx

* app

    * lightweight application framework
    * rpc-interface
    * UI components

* samples

    * empty template
    * media player with support for network-syncing and warping
    * 3D viewer

TODO:

* dependencies, build instructions
* introduction to samples (media player, 3D viewer)
