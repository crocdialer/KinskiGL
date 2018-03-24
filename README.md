KinskiGL
========

![Preview](http://crocdialer.com/wp-content/uploads/2018/03/kinski_cover.jpg)

kinskiGL is a set of portable C++ libraries useful for cross-platform OpenGL applications.

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
    * output warping

* app

    * lightweight application framework
    * rpc-interface
    * UI components

* samples

    * empty template
    * media player with support for network-syncing and warping
    * 3D viewer with forward/deferred rendering paths.

* modules

  a list of optional modules can be used, offering a variety of functionality.
  including but not not limited to  

    * blocking- / non-blocking http, using libcurl
    * integration of scenegraph elements with the Bullet Physics Library, offering rigid- and softbody physics
    * integration of the Asset Import Library (assimp) for loading 3D models and scenes
    * an L-System implementation for generating procedural meshes
    * OpenCL-based particlesystem, using CL/GL interop

TODO:

* dependencies, build instructions
* introduction to samples (media player, 3D viewer)
