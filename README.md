KinskiGL
========

kinskiGL is a set of portable C++ libraries useful for realtime graphics applications.

kinskiGL is currently known to compile and work for these platforms:

* MacOS 10.12
* Ubuntu Desktop
* iOS
* Rasbian Stretch (Rasberry Pi)
* Linaro on Asus Tinkerboard

the basic library-structure:

* core
{
    * image IO
    * asynchronous networking
    * serial ports
    * timers
    * property/component framework
    * serialization
    * threadpools
    * utilities
}

* gl
{
    * linear algebra utilities
    * geometric primitives, ray-casting
    * scenegraph, forward/deferred rendering paths
}

* app
{
    * lightweight application framework
}

TODO:

* dependencies, build instructions
* introduction to samples (media player, 3D viewer)
