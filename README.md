# longweekend

A simple CPU (for now) path tracer based mainly on the Ray Tracing in One Weekend series of books, but continued and extended in other directions.

<p align="center">
  <img src="./.images/cover.jpg">
</p>

<p align="center">
  <img src="./.images/dragon.jpg">
</p>

## Building

- Visual Studio 2022, C++
- vcpkg

Before building make sure to add a precompiled OIDN version to `external/` like so:
```plaintext
external
├───bin
│       OpenImageDenoise.dll
│       OpenImageDenoise_core.dll
│       ...
├───include
│   └───OpenImageDenoise
│           config.h
│           oidn.h
│           oidn.hpp
└───lib
        OpenImageDenoise.lib
        OpenImageDenoise_core.lib
```
The 2.3.0 release is available [here](https://github.com/RenderKit/oidn/releases/tag/v2.3.0).

The project uses vcpkg for all other dependencies (vcpkg.json), Visual Studio should install them for you automagically.
