# Re-implementing VIL and Hydrometeor Classification in OpenStorm VR

Please help me re-implement the **Vertically Integrated Liquid (VIL)** and **Hydrometeor Classification (HC)** radar products in my Unreal Engine 5.7 project (NexradSpatial). 

We attempted this in a previous session but had to roll back due to a few critical bugs. Please follow these guidelines to ensure we don't repeat the same mistakes:

### 1. The "Solid Grey Cylinder" Bug (Critical)
In our previous attempt, the new VIL and HC volumes rendered as giant, solid 360-degree grey cylinders instead of proper radar sweeps. This happened for two reasons:
*   **Missing Sweep Metadata:** When building the new `RadarData` objects in `deriveVolume`, you **must** call `radarData->CopyFrom(refData, true);`. The `true` parameter ensures that `sweepInfo` and `sliceAngles` metadata are properly cloned from the base reflectivity product. Without this, the volumetric renderer assumes start and end angles are uninitialized and draws a full 360-degree block.
*   **Missing Stats:** You **must** explicitly set `radarData->stats.minValue` and `radarData->stats.maxValue`. If these are missing, the color interpolator fails and colors the entire volume grey.

### 2. Custom Color Indexes
The `RadarColorIndex` system needs explicit support for these products. 
*   Create `RadarColorIndexVerticallyIntegratedLiquid` and `RadarColorIndexHydrometeorClassification` classes.
*   Make sure to register them in `RadarColorIndex::GetDefaultColorIndexForData` inside `RadarColorIndex.cpp`. If you forget to add them to the `switch` statement, the application falls back to `RadarColorIndexRelativeHue`, which breaks the visual presentation.

### 3. The 403 Forbidden / Auto-Detect Bug
During our last attempt, the application suddenly stopped auto-detecting the closest radar station on launch. This was caused by the National Weather Service (NOAA/Unidata) blocking our HTTP requests. 
*   **User-Agent Header:** In `RadarDataDownloader.cpp`, you must ensure that **all** `IHttpRequest` instances have a User-Agent header set: `httpRequest->SetHeader("User-Agent", "NexradSpatial/1.0");`.
*   **S3 XML Parsing:** The NWS Unidata archive uses an Amazon S3 bucket. A standard `dir.list` text parser will fail. If you touch the directory listing logic in `RadarDataDownloader::MakeRequest`, ensure it checks if the URL contains `s3.amazonaws.com` and parses the `<Contents><Key>...</Key><Size>...</Size></Contents>` XML nodes accordingly.

### 4. UI and Control Side-Effects
When adding the buttons for VIL and HC to the `SVRMenuWidget` UI or registering the inputs in `RadarViewPawn`, be extremely careful not to accidentally delete or overwrite existing button mappings (like the 'A' button controls). Make additive changes only.

### Task List
1. Create `VerticallyIntegratedLiquidProduct.cpp/.h` and `HydrometeorClassificationProduct.cpp/.h`.
2. Update `ProductList.cpp` to register them.
3. Update `RadarColorIndex.cpp/.h` with their specific colormaps and the `switch` statement registration.
4. Verify the `User-Agent` and S3 bucket parsing fixes are applied in `RadarDataDownloader.cpp`.
5. Add UI toggles to `SVRMenuWidget.cpp` safely.

Let me know when you're ready to start, and we can tackle these file-by-file!
