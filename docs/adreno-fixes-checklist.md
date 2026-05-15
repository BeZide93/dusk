# Adreno Fixes Checklist

Branch: `Adreno-fixes`

## Working Hypothesis

- Reported issue: vertex explosions on some Adreno GPUs.
- Relevant renderer path: Aurora GX over WebGPU/Dawn, used by Android Vulkan.
- The GX renderer manually pulls GameCube vertex data from storage buffers in WGSL instead of using standard vertex attributes.
- On Android RelWithDebInfo builds, `NDEBUG` is active, so Dawn previously enabled both `skip_validation` and `disable_robustness`.
- With robustness disabled, out-of-range storage-buffer reads or bad matrix indices can produce undefined values, which can become exploded vertex positions on Adreno drivers.

## Code Anchors

- Dawn toggles: `extern/aurora/lib/webgpu/gpu.cpp`
- Vertex/storage buffers and static bind group: `extern/aurora/lib/gfx/common.cpp`
- GX vertex shader generation and storage loads: `extern/aurora/lib/gx/shader.cpp`
- Indexed array upload path: `extern/aurora/lib/gx/command_processor.cpp`
- Uniform packing for vertex start and array starts: `extern/aurora/lib/gx/shader_info.cpp`

## First Test Fixes Applied

- Keep Dawn robustness enabled on Android release-style builds by not enabling the `disable_robustness` toggle on Android.
- Clamp GX position/normal matrix indices to the valid `MaxPnMtx` range before indexing `postex_mtx` and `nrm_mtx`.
- Pass indexed vertex array upload sizes to the shader and clamp indexed attribute fetches to the last safe entry.
- Add Android-only guard zeroes after vertex/storage uploads, keep guarded uploads aligned, and disable GX draw merging on Android to keep guarded ranges isolated.

## Follow-Up Fixes If Needed

- Add Adreno adapter detection using `g_adapterInfo.device`/`description` and gate the workaround only to Adreno.
- If driver issues persist, investigate a CPU-normalized vertex decode path or real WebGPU vertex attributes for Adreno.

## Test Expectations

- If explosions disappear, the root cause is likely robustness/OOB-sensitive shader vertex pulling.
- If explosions remain but are reduced, indexed array fetch bounds are the next suspect.
- If there is no change, investigate Adreno shader compiler issues around unaligned storage-buffer loads.
