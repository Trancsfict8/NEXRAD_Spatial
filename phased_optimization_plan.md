# Nexrad Spatial - Phased Optimization Plan

## Phase 1: Memory Pool Pre-allocation (Highest Impact / Least Disruptive)
- Move all `new` operator allocations for `buffer`, `rayInfo`, and `sweepInfo` into the `RadarData` class via a new `AllocateBuffers()` initialization routine, leveraging the fixed bounds defined in `RadarDataSettings`.
- Modify the Parsers (`Nexrad.cpp`, `MiniRad.cpp`, `OdimH5.cpp`) to only initialize the active `usedBufferSize` of the pre-allocated buffers instead of continually destroying and rebuilding them.

## Phase 2: Cache-Line Alignment & Struct Packing (Low Impact / Low Disruptive)
- Decorate `RadarData`, `SweepInfo`, and `RayInfo` structs with `alignas(64)`.
- Reorder fields inside `SweepInfo` and `RayInfo` from largest to smallest to minimize struct padding. Convert `bool interpolated` into a bitfield if future flags are anticipated.

## Phase 3: Direct-to-Sparse Ingestion (Moderate Impact / Moderate Disruptive)
- Eliminate the dense intermediate `sweepBuffer`.
- Rewrite `SparseCompression.h/.cpp` to expose an append-only API. Modify the parsers to decode raw bytes and append them directly to the sparse RLE structure, instantly pruning `noDataValue` airspace.
- Update `InterpolateSweep()` to step through the sparse spans instead of dense coordinates.

## Phase 4: GPU-side Quantization (High Impact / Most Disruptive)
- Convert the core memory buffer to `uint8_t` or `uint16_t`.
- Offload the `value = value * multiplier + offset` scaling math to the Unreal Engine shader to keep the CPU footprint tiny.

---
### Phase 1 Implementation Notes
Changes implemented:
1. `RadarData.h` & `RadarData.cpp`: Added `AllocateBuffers()` to handle one-time pre-allocation of `buffer`, `sweepInfo`, and `rayInfo` pools to eliminate heap fragmentation in hot paths. Added a `float* compressionWorkspace` to safely isolate memory when running sparse compression routines without allocating 80MB dense buffers.
2. `RadarDataHolder.cpp`: Updated `RadarLoader::Task()` to invoke `AllocateBuffers()` before starting the threaded parsers.
3. `Nexrad.cpp`, `MiniRad.cpp`, `OdimH5.cpp`: Stripped out dynamic `delete[]` and `new` allocations for `sweepInfo`, `rayInfo`, and `buffer`. Re-routed compression routines to utilize the small `compressionWorkspace` memory pool, and fixed initialization bugs to guarantee full-cone `noDataValue` clearing when memory is first spun up.
