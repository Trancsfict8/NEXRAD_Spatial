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
6.  **VR Interactive Probe / Inspector Tool**: 
    *   **Feature**: Implemented an interactive sphere attached to the VR controller that probes the 3D volume rendering and extracts the exact DbZ value that it intersects in 3D space. 
    *   **Stumble - Text Scaling**: The dynamic floating text on the probe became unreadable at a distance or blended into the bright storms. 
    *   **Fix**: Migrated away from deprecated static `SetXScale` to dynamically updating `SetWorldSize` tied to controller distance. Added a dual-layered `UTextRenderComponent` approach to create a solid black text drop-shadow for contrast.
    *   **Stumble - Probe Axis & Extraction Mapping**: The right joystick allowed unintended Z/Y translation that broke immersion, and data extraction occasionally returned -2.0 or clear-air readings in storm cores.
    *   **Fix**: Isolated rotation controls strictly to the Yaw axis. Mapped extraction data cleanly by fixing mirrored polar coordinate math (reversing the Y polarity and utilizing proper atan2 logic for Azimuth matching) so the physical UE5 space matches the spatial layout of the DBZ array.
7.  **VR Input Bindings & Oculus Support**:
    *   **Feature**: Added a "Hold Left X + Right Joystick" mechanic to dynamically push/pull the inspector tool distance.
    *   **Stumble - Unresponsive Inputs**: Attempting to directly bind `EKeys::Gamepad_FaceButton_Left` via C++ was silently failing to fire on standard VR headsets.
    *   **Fix**: Overhauled input by adding explicitly defined `ActionMappings` in `DefaultInput.ini` (e.g. `OculusTouch_Left_X_Click`, `ValveIndex_Left_A_Click`) and binding the action by name via `BindAction()`.
8.  **Engine & API Deprecation Cleansing**:
    *   **Stumble - Deprecation Pileup**: During upgrades, UE5.4+ audio APIs and standard library updates triggered dozens of severe deprecation warnings that threatened future compatibility.
    *   **Fix**: Migrated audio code in `Joke.cpp` to `FOnAudioCaptureFunction` and `OpenAudioCaptureStream` (and fixed a subtle compiler error by correctly swapping to `const void*` buffers). Resolved C++17 `std::codecvt_utf8` deprecations, and patched `UnrealImGui` plugin array deprecations (replacing `false` with `EAllowShrinking::No`).
9.  **National Weather Service (NWS) API Integration**:
    *   **Feature**: Implemented `WarningManager.cpp` to fetch live weather warnings (Tornado, Severe Thunderstorm, Flash Flood, etc.) from `api.weather.gov`.
    *   **Architecture**: Added HTTP JSON requests that poll NWS for warnings around the camera's location. The warnings are mapped from latitude/longitude to the 3D spherical globe and rendered as hollow 40m thick outlines (`GISPolyline`) overlaid perfectly on the map. Features automatic LOD distance culling and a settings UI to toggle alert types on and off.
10. **Persistent State & UX Refinements**:
    *   **Feature**: `SettingsSaver.cpp` was updated to persistently save new settings like Imperial/Metric units, Max FPS limits, map brightness, and warning type toggles to `settings.json`.
    *   **Feature**: Mapped specific VR controllers (Left Trigger to move Up, Left Grip to move Down) directly via dynamic API mapping and `DefaultInput.ini` fallback explicitly to prevent Oculus OpenXR mapping issues. Added a dynamic "LOADING DATA..." tag above active radar volumes to improve user feedback.
