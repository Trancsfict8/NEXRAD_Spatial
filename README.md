# Nexrad Spatial

![Nexrad Spatial Logo](NexradSpatial.png)

Nexrad Spatial is an immersive, fully 3D volumetric weather radar and data visualization application built on top of Unreal Engine 5.7. It allows you to step inside extreme weather events, view raw radar sweeps from across the globe in breathtaking 3D detail, and analyze live atmospheric data dynamically in Virtual Reality.

For more information, visit our website at [NexradSpatial.com](https://NexradSpatial.com) or join the community on the [Nexrad Spatial Discord Server](https://discord.gg/nexradspatial).

This project is a heavily expanded derivative work of the open-source **OpenStorm Radar** project (originally developed by Jordan Schlick). It utilizes the powerful underlying volumetric rendering algorithms and data decoders of OpenStorm, while introducing a suite of specialized spatial computing features, real-time alerting, and an upgraded immersive user experience.

## Demo Video

*[Demo video coming soon]*

## Features
### Core Features (Inherited from OpenStorm)
* Full 3D level 2 radar volumetric rendering using custom ray marching shaders.
* Real-time multithreaded data loading and parsing without limits.
* Broad support for base radar products (Reflectivity, Radial Velocity, Spectrum Width, Correlation Coefficient, Differential Reflectivity).
* Real-time live data downloading and Historical dataset support.
* Windows and Linux support.
* NEXRAD and European ODIM H5 radar data processing.
* Built-in 3D global topological map with elevation and GIS boundaries.

### New Spatial & VR Features (Nexrad Spatial)
* **Immersive VR Inspector Tool:** Reach out with your VR controller to probe the center of a storm. Dynamically extracts the exact dBZ, velocity, or specific radar moment of the voxel you are interacting with, displaying the data and your exact altitude AGL dynamically on your controller.
* **Live NWS Warning Polygons:** Seamless integration with the National Weather Service API. Real-time Tornado, Severe Thunderstorm, Flash Flood, and Special Marine warnings are downloaded and mapped as procedural mesh outlines perfectly to the 3D globe's surface in your environment. You can interact with and click these warning polygons in VR to instantly view the detailed alert text and information directly from the NWS.
* **Level 3 Storm Tracks & 3D Markers:** Process live Level 3 storm tracking data, generating dynamic visual Hail and TVS (Tornado Vortex Signature) icons right at the exact storm cell echotop altitude.
* **Spatial Weather Audio:** Experience the storms with immersive 3D spatialized rain and hail audio that responds to your position within the radar volume.
* **Intelligent LOD & Culling System:** Performance-first architecture automatically distance-culls detailed GIS data and warning shapes when they aren't relevant to your camera view. 
* **Data-Driven Spatial UI:** Beautiful floating spatial UI built natively using Slate. Toggle Imperial/Metric unit conversions dynamically across the app, constrain frame rates, modify warning visibilities, and customize VR locomotion properties.
* **Custom Color Tables:** Full support for custom radar color tables. Personalize how radar products like reflectivity and velocity are visualized by easily dropping `.pal` configuration files into the project.
* **Overhauled Locomotion:** Extended OpenXR VR locomotion mapping (Trigger and Grip vertical movement bindings) across modern hardware including Meta Quest, Valve Index, and Windows Mixed Reality headsets. 

## Getting Data
NEXRAD and European ODIM H5 radar data are supported out of the box.
* Simply click on a physical radar dish on the 3D map inside the app to instantly fetch live data.
* Retrieve historical archive data manually via [Amazon AWS Unidata NEXRAD Archives](https://s3.amazonaws.com/unidata-nexrad-level2/index.html).

## Building
1. Install **Unreal Engine 5.7** (or the latest 5.x version) and its C++ development dependencies.
2. Clone this repository: `git clone [YOUR_REPO_URL_HERE]`
3. Initialize submodules (required for RSL): `git submodule update --init --recursive`
4. Right-click the `.uproject` file and select **Generate Visual Studio project files**.
5. Open the resulting Visual Studio solution file (`.sln`).
6. Build the project (e.g., `Ctrl+Shift+B`).
7. Open the project in Unreal Engine.

*Note: Nexrad Spatial relies heavily on strict compiler compliance in modern Unreal Engine. Warnings-as-errors (such as C4702) have been deliberately suppressed for the legacy NASA TRMM RSL C-library to allow successful builds.*

To build a standalone executable, select **Package Project** under the Platforms dropdown within the Unreal Editor.

### Map & GIS Data Add-ons
Some of the heavier geospatial data files are excluded from the repository. You can copy the entire `Content/Data` folder from the latest release into your project to populate the map. 
Key paths include:
* Elevation data: `Content/Data/Map/elevation.bin.gz`
* GIS shapefiles and configs: `Content/Data/Map/GIS/`
*(Tip: You can easily add your own high-detail road/boundary ESRI `.shp` files to the GIS folder alongside a `.cnf` configuration file to automatically load them into the application.)*

## Architecture & Contributions
Nexrad Spatial is built strictly for high-performance spatial analytics. All radar parsing modules are designed to remain independent of Unreal Engine so they can be abstracted to other APIs. 
For detailed technical documentation on the data structures, UE5.7 upgrade paths, and implementation notes, please see `Architecture.md`.

## Recent Changelog
* **Historical Archive Enhancements:** Added intelligent teleportation that instantly transports the camera to the correct radar site when a historical session is loaded. Overhauled the historical UI with auto-scrolling, clear download trackers, and dynamic disk loading feedback.
* **Map Tile Stability:** Implemented graceful failure handling for map tiles. The system now robustly caches and seamlessly ignores missing or failed tile downloads without crashing or halting the rendering thread.
* **Interactive NWS Warnings:** Added interactive National Weather Service warning popups. Users can click on warning polygons in VR to view detailed alert information in a larger, improved warning box.
* **Level 3 Storm Tracks & Markers:** Integrated Level 3 storm tracks data. Added new Hail and TVS (Tornado Vortex Signature) markers that spawn dynamically at the echo top of storm cells, complete with altitude readings (ft AGL) and refined map marker accuracy.
* **Spatial Weather Audio:** Added immersive 3D spatialized rain and hail audio to the environment, and resolved previous audio stuttering issues during heavy storm processing.
* **3D Drawing & Laser Tools:** Introduced a new 3D drawing tool to annotate the environment and an adjustable laser pointer for precise VR controller interactions.
* **Elevation & Customization:** Added elevation exaggeration settings that dynamically adjust the 3D map and radar site positioning. Introduced opacity settings and a performance throttler.
* **Legal & UI Updates:** Added a legal disclaimer screen with persistence. Included color table support, updated the UI typography, and fixed various map tool collision bugs.
* **Optimization & Fixes:** Optimized the build process by moving includes to CPP files, fixed road placements, and resolved velocity color table parsing issues.

## Open Source Credits
In accordance with the GPLv2 license, the core radar ingestion, binary decoding, and processing architecture utilized within this application are fully open-source.
* **OpenStorm Base Framework:** Copyright (c) Jordan Schlick & Contributors.
* **Nexrad Spatial Modifications:** Copyright (c) 2026 Nexrad Spatial.

*If you are interested in the original desktop application without the VR ecosystem modifications, check out the original OpenStorm repository on GitHub.*