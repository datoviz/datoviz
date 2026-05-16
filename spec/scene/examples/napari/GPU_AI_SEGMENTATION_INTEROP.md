# GPU AI Segmentation Interop

> **Agent Pickup**
> - **Category:** `napari`
> - **Implementation target:** Napari-class scene pressure test, usually staged as a current v0.4 demo plus a richer follow-up.
> - **Data policy:** Use public sample data or deterministic synthetic fallback; cache prepared arrays/textures explicitly.
> - **Preprocessing:** Required when data is downloaded, resampled, tiled, labeled, or packed for GPU upload.
> - **Validation:** Stage-specific acceptance criteria covering current v0.4 behavior and the richer napari-class target.


## Purpose

This demo illustrates a future napari workflow where an AI or GPU image-processing step produces an output that is immediately visualized without a CPU readback/reupload cycle. It does not need to run a real neural network; the point is to demonstrate renderer architecture: compute writes a probability map or segmentation-like texture, and render consumes it directly.

This is the best demo for explaining why a modern backend is not only about FPS. It is about enabling new dataflow patterns for AI-assisted biological image analysis.

## Napari use case being mirrored

Napari is commonly used downstream of segmentation and machine-learning workflows. Plugins such as Cellpose, StarDist, empanada, trackastra, and cupy-based image processing produce masks, probability maps, labels, tracks, or detections that users inspect and correct interactively.

Relevant napari concepts:

- `Image` layer: raw microscopy image.
- `Image` or `Labels` layer: model probability / segmentation output.
- Layer controls: threshold, opacity, colormap, blending.
- AI-assisted workflow: model output changes repeatedly during parameter tuning or proofreading.

References:

- Napari/cupy integration tutorial emphasizing minimized CPU/GPU transfer: https://biapol.github.io/PoL-BioImage-Analysis-TS-GPU-Accelerated-Image-Analysis/25_cupy/50_napari-cupy-image-processing.html
- Napari-cupy-image-processing plugin: https://napari-hub.org/plugins/napari-cupy-image-processing.html
- Cellpose and napari course example: https://iah-public.pages.pasteur.fr/bioimage_analysis_with_python_course/notebooks/09-Advanced.html
- Empanada 3D napari tutorial: https://empanada.readthedocs.io/en/latest/tutorials/3d_tutorial.html

## Dataset

Primary dataset: reuse **BBBC038** images from `LARGE_LABELS_SEGMENTATION`.

Reason:

- It keeps the data dependency simple.
- It provides real nuclei images.
- It visually maps to probability maps and segmentation masks.

Alternative dataset:

- `skimage.data.cells3d()` for a small 3D microscopy stack.
- A small local OME-Zarr image if already available.

## Generated GPU-side output

The demo should generate one of these outputs on the GPU:

### Option A: fake probability map

A compute shader applies a simple image-processing pipeline:

1. Read grayscale image texture.
2. Apply local blur or smoothing.
3. Compute edge/contrast-enhanced response.
4. Write probability-like float texture.
5. Render probability as colormap overlay with threshold.

### Option B: evolving segmentation mask

A compute shader produces a dynamic binary mask:

1. Read grayscale image.
2. Compare against threshold.
3. Apply simple time-varying morphology/noise.
4. Write `r8uint` or `r32uint` mask texture.
5. Render mask as Labels-like overlay.

### Option C: toy interactive AI refinement

A brush or cursor influences the probability map:

1. User moves cursor or changes seed point.
2. Compute shader updates a local region.
3. Render immediately.

Option A is easiest and robust. Option C is most impressive if time allows.

## Download and cache strategy

Reuse the BBBC038 cache:

```text
~/.cache/datoviz-napari-demos/bbbc038_labels/<image_id>.npz
```

Only the raw image is required. If labels are available, they can be shown as ground truth for comparison.

## Datoviz adaptation

### Required passes

1. **Upload pass**
   - Upload source image to GPU texture once.
2. **Compute pass**
   - Input: source image texture.
   - Output: probability texture or segmentation texture.
3. **Render pass**
   - Draw raw image.
   - Draw probability/mask overlay from compute output.

### GPU resources

```text
source_image: sampled texture, r8unorm or r16float
probability: storage texture or storage buffer, r32float / r16float
mask: optional storage texture, r8uint / r32uint
params: uniform buffer with threshold, smoothing, time, brush position
```

### Required Datoviz/GSP features

- Compute pipeline.
- Storage texture or storage buffer.
- Compute-to-render synchronization.
- Same-frame resource reuse.
- Uniform update without reallocation.

## Demo UI

Controls:

- Threshold slider.
- Smoothing radius or kernel strength.
- Overlay opacity.
- Show probability / thresholded mask / boundaries.
- Animate toggle.
- Optional brush position follows mouse.
- Optional compare with BBBC038 ground-truth labels.

## What to present in the demo

Show the raw nuclei image. Enable a probability overlay that updates live when the threshold/smoothing slider changes. Show that the output texture is produced by a compute pass and then rendered directly. If ground-truth labels are available, toggle them as a comparison.

Suggested message:

> This is not a real AI model; it is a dataflow demo. It shows the kind of architecture we need for AI-assisted napari workflows: GPU-produced outputs should be displayable immediately, without unnecessary CPU round-trips.

## Why this pressures the architecture

- Requires compute/render interop.
- Requires explicit resource usage transitions or backend-managed synchronization.
- Requires storage textures/buffers.
- Demonstrates where WebGPU/Vulkan-like semantics are useful.
- Connects directly to AI segmentation, proofreading, and model-parameter tuning.

## Minimal implementation plan

1. Reuse BBBC038 image cache.
2. Implement source image upload.
3. Implement compute shader producing probability texture.
4. Implement render shader overlay with threshold and colormap.
5. Add GUI controls.
6. Add on-screen text explaining: CPU image upload once, then compute + render each frame.

## Acceptance criteria

- Raw image uploaded once.
- Compute shader writes an output each frame or on parameter change.
- Render shader displays the compute output directly.
- Threshold/opacity updates are interactive.
- Demo can run without CUDA or PyTorch; it only needs the Datoviz GPU backend.
