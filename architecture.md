# OpenStorm VR - Architecture and Data Structures

## Overview
OpenStorm VR is a weather and radar visualization tool built in Unreal Engine. It processes raw weather radar data (like NEXRAD level 2/3 and ODIM H5) and visualizes the volumetric storms in a 3D environment, with an immersive VR interface. 

The codebase heavily integrates C++ data processing and an immediate-mode GUI (Dear ImGui) for user interfaces.

## Directory Structure

The primary source code is located in `Source/OpenStorm/`:

*   **`Application/`**: Contains top-level application logic. Notable is `RadarDataDownloader.cpp`, which handles HTTP requests for pulling live radar data down from weather APIs.
*   **`Radar/`**: The core data processing powerhouse. 
    *   Parsers for various radar formats: `Nexrad.cpp`, `MiniRad.cpp`, `OdimH5.cpp`.
    *   `RadarCollection`, `RadarDataHolder`, `RadarData`: The core data structures that store sweeps, rays, and bins of radar moments (Reflectivity, Velocity, etc.) in memory.
    *   `Globe.cpp` / `SimpleVector3.cpp`: Handles geospatial math, projecting radar bins to 3D spherical coordinates.
    *   **`Deps/rsl/`**: A bundled third-party C library (NASA TRMM Radar Software Library) used for decoding raw WSR-88D archive files.
*   **`UI/`**: The user interface layer, built on top of UnrealImGui.
    *   `ImGuiUI.cpp` / `ImGuiController.cpp`: Manages the application's windows, settings menus, and floating 2D overlays.
    *   `Native.cpp`: Handles OS-level windowing hooks and console allocation for debugging.
*   **`Mapping/`**: Handles GIS mapping, shapefiles, and underlying base maps to contextualize the radar data.
*   **`Objects/`**: Contains in-engine actor representations (like MapMesh, RadarVolumeRender) that actually display the data visually in the UE5 world.

## Core Data Structures

1.  **Radar Volume (`volume.cpp` / `RadarDataHolder.h`)**: The top-level data structure representing a full 3D scan of the atmosphere. 
2.  **Sweeps**: A single 360-degree rotation of the radar dish at a specific elevation angle.
3.  **Rays**: An array of data bins stretching out from the radar dish at a specific azimuth.
4.  **Products**: Derived data visualizations, such as Base Reflectivity (Z), Base Velocity (V), or algorithms like Normalized Rotation.

## The UE 5.2 to UE 5.7 Upgrade Journey

Upgrading OpenStorm from Unreal Engine 5.2 to 5.7 involved several breaking API changes:

1.  **UObject MetaClass Paths**: Unreal 5.7 strictly enforces fully qualified paths for class paths. Fixed `ImGuiModuleSettings.h` to use standard object pathing.
2.  **ANY_PACKAGE Deprecation**: Removed `ANY_PACKAGE` calls in `ImGuiUI.cpp` and replaced them with `nullptr` as it is no longer supported in `FindObject`.
3.  **Windows API Conflicts**: Unreal 5.7 overhauled `MinimalWindowsApi.h`. We resolved strict redefinitions of `LoadLibraryW` and `FreeLibrary` in `Native.cpp`.
4.  **HTTP Module Updates**: Migrated `RadarDataDownloader.cpp` to use the new `OnRequestProgress64` callback signature (which tracks bytes as 64-bit integers instead of 32-bit). We also resolved the deprecated `Failed_ConnectionError` enum.
5.  **Strict Compiler Warnings (C4702 & C4701)**: UE 5.7 enabled MSVC warnings-as-errors by default. The legacy `rsl` C-library had hundreds of "Unreachable Code" and "Potentially uninitialized variable" warnings. We explicitly injected `#pragma warning(disable: 4702)` and `4701` at the top of the offending C++ files (`Globe.cpp`, `volume.cpp`, `wsr88d_get_site.cpp`, etc.) to force the compiler to ignore them without breaking the library.
