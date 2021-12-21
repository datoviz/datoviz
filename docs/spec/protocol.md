# Rendering protocol specification


## Objects

* **Canvas**: a physical window
* **Board**: an offscreen canvas
* **Graphics**: a graphics pipeline
* **Compute**: a compute pipeline
* **Dat**: a portion of a GPU buffer, that can be allocated, resized, and deallocated
* **Tex**: a 1D, 2D, or 3D GPU image
* **Sampler**: a GPU object specifying how pixels are fetched from a **Tex**
* **Prop**: a graphics property (e.g.: position, color, marker type, etc)


## Actions

* **Create**
* **Update**
* **Get**
* **Set**
* **Resize**
* **Bind**
* **Upload**
* **Upfill**
* **Download**
* **Record**
* **Delete**


## Recording commands

* **Begin**
* **Viewport**
* **Barrier**
* **Draw**
* **Launch**
* **End**


## Data specification

Data can be uploaded in two ways.

### Raw upload

With a **Raw upload**, bytes are sent straight to a **Dat** or a **Tex*. This is dependent on the underlying renderer's implementation.

A **Raw upload** is defined by:

* A **Dat** or **Tex** to upload to
* A byte offset
* A size, in bytes
* A data buffer (C pointer, Python NumPy array, JSON base64 string...)

### Graphics upload

With a **Graphics upload**, data corresponding to a given graphics property is sent. It is up to the renderer to transform it into bytes to be uploaded to a **Dat** or **Tex**.

A **Graphics upload** is defined by:

* A **Graphics** to upload to
* A **Prop**, defining which graphics property the data should be sent to
* An item offset
* An item count, the number of items to upload
* A data buffer (C pointer, Python NumPy array, JSON base64 string...)


## Picking

TODO


## Message specification


### Create Board

* `Id id`
* `uint32 width`
* `uint32 height`


### Create Canvas

* `Id id`
* `uint32 width`
* `uint32 height`


### Create Dat

* `Id id`
* `DatType type`
    * `vertex`
    * `index`
    * `uniform`
    * `storage`
* `Size size`


### Create Tex

* `Id id`
* `TexDims dims`
    * `1D`
    * `2D`
    * `3D`
* `uvec3 shape`
* `Format format`
    * a subset of Vulkan formats


### Create Sampler

* `Id id`
* `Filter filter`
    * `nearest`
    * `linear`
    * `cubic`
* `AddressMode mode`
    * `repeat`
    * `mirrored_repeat`
    * `clamp_to_edge`
    * `clamp_to_border`
    * `mirror_clamp_to_edge`


### Create Graphics

* `Id id`
* `Id board`
* `GraphicsType type`
    * see **Graphics spec**


### Raw Upload

* `Id dat`
* `Size offset`
* `Size size`
* `Buffer data`


### Graphics Upload

* `Id graphics`
* `Prop prop`
    * see **Graphics spec**
* `uint32 first_item` (in number of items)
* `uint32 item_count` (in number of items)
* `Size size` (in bytes)
* `Buffer data`


### Set Vertex


* `Id graphics`
* `Id dat`

Only used with **Raw Uploads**.


### Bind Dat

* `Id graphics`
* `uint32 slot_idx`
* `Id dat`


### Bind Tex

* `Id graphics`
* `uint32 slot_idx`
* `Id tex`
* `Id sampler`


### Record Begin

* `Id board`


### Record Viewport

* `Id board`
* `vec2 offset` (in framebuffer pixels)
* `vec2 shape` (in framebuffer pixels)


### Record Raw Draw

* `Id board`
* `Id graphics`
* `uint32 first_vertex`
* `uint32 vertex_count`

Only used with **Raw Uploads**.


### Record Graphics Draw

* `Id board`
* `Id graphics`
* `uint32 first_item`
* `uint32 item_count`

Only used with **Graphics Uploads**.


### Record End
* `Id board`
