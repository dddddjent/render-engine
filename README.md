# Engine

# Requirements

- C++ 20
- Cuda >= 12.6
- Vulkan

# Usage

The engine works as a library.

## Project

### main.cpp

- Derive classes
- Load config
- Load custom scripts

```cpp
Engine engine;
engine.init(config, &render_engine, &ui_engine, &physics_engine, std::move(scripts));
engine.run();
engine.cleanup();
```

### Custom Engines

- RenderEngine is not customizable
- Override `init()`, `cleanup()`
- UIEngine
  - Builtin `ImGuiEngine`
  - `handle_input()`
  - `getDrawUIFunction()`
- PhysicsEngine
  - Builtin `CudaEngine`
  - `step()`
  - `sync()`

### Custom Scripts

- Base class `Script` in `function/script/script.h`
  - Override `init()`, `step()`, `destroy()`
  - They happen before the internal engine starts, after the internal systems are stepped, and before the internal systems are destroyed.
- Use a `std::vector<std::unique_ptr<Script>>` to submit them to the engine.
- The engine will stop when `g_ctx.continue_to_run` is false.

### Config

Requires a config.json

- It doesn't require to supply all the fields

- Render graph:

  - default: objects only
  - fire_field: requires two fields, fire and smoke. Need `fire_colors.npy`
  - smoke_field: at most 2 fields
  - vorticity_field: at most 2 fields
  - shader_directory: engine's xmake.lua compiles shaders to `${buildir}/shaders`. This should be the same as the xmake.lua.
  - extra_args: extra arguments to the graph

- Objects:

  - mesh and material are all references

- Fields:

  - Support loading npy/vti
    - It uses the fields name to identify the field in the vti
  - data_type: temperature, concentration

- Mesh: several different types

  - Sphere: pos, radius, tessellation
  - Cube: pos, scale
  - Plane: pos, normal, size
  - obj: path (only one mesh per object)

- Material: use reference to find textures

  - default\_{color|metaillic|roughness|normal|ao} are builtin textures

- Textures: only support jpg/png/tiff

- Lights: only support point lights

- Recorder:
  - record_from_start: whether start to record at launch

## Internal

### Config

- The top level `Configuration` is a `json`
- To convert json to a struct, use `JSON_GET(type, name, json, key)`
- Use `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` if the struct has a json field

### Global Context

- Include `vulkan_context`, `descriptor_manager`, `resource_manager`
- Find everything here

### Descriptor Manager

- Register gpu resources, reference them by handle
  - Shaders also use handles to find resources
- Register pipeline parameters
  - pipeline parameters submits all the handles

### Add New Things (New Resources)

- Define new types in `function/type/`
- Modify `function/resource_manager/resource_manager.h`

### Render Node

- `Constructor()`: specify the name and all the attachments this node needs

  - maybe some other things can be added in the arguments
  - initialize `attachment_descriptions` to let the graph know what attachments this node needs
    ```cpp
     {
         "previous_color",
         {
             previous_color,
             RenderAttachmentType::Color | RenderAttachmentType::Sampler,
             RenderAttachmentRW::Read,
             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
             VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
             VK_FORMAT_R32G32B32A32_SFLOAT,
         },
     },
    ```

- `init()`: init the render node
  - RenderAttachments: contains all the attachments in the render graph
- `record()`: similar to the `step()` function. Executed once per frame
- `onResize()`: things like framebuffer should be resized here
- `destroy()`
- common shaders are in `function/render/render_graph/shader/`

#### To add a new node

- derive from `RenderGraphNode`
- add the header to the `function/render/render_graph/node/node.h`

### Render Graph

- `init()`
  - specify all the nodes
  - `initAttachments()`
  - init all the nodes
  - specify the dependency graph of nodes
    - node to dependent nodes
  - `initGraph()`
- `getUIRenderpass()`
  - if the graph has a UI node, return the renderpass of that node
- `registerUIRenderfunction()`: this function will get all the render commands from the ui engine
- `record()`: record the commands in the render graph
- `onResize()`: resize nodes and attachments

#### To add a new graph

- derive from `RenderGraph`
- add the header to the `function/render/render_graph/graph/graph.h`
- add a new entry in `function/render/render_engine.cpp`'s `initRenderGraph()`

### Cuda Engine

- In `initExternalMem()`
- Import an external field

```cpp
for (int i = 0; i < g_ctx->rm->fields.fields.size(); i++) {
    auto& field = g_ctx->rm->fields.fields[i];
#ifdef _WIN64
    HANDLE handle = g_ctx->rm->fields.getVkFieldMemHandle(i);
#else
    int fd = g_ctx->rm->fields.getVkFieldMemHandle(i);
#endif
    CudaEngine::ExtImageDesc image_desc = {
#ifdef _WIN64
        handle,
#else
        fd,
#endif
        128 * 256 * 128 * sizeof(float),
        sizeof(float),
        128,
        256,
        128,
        field.name
    };
    this->importExtImage(image_desc); // add to extBuffers internally
}
```

- Import an external object (buffer)

```cpp
for (auto& object : g_ctx->rm->objects) {
#ifdef _WIN64
    HANDLE handle = object.getVkVertexMemHandle();
#else
    int fd = object.getVkVertexMemHandle();
#endif

    const auto& mesh = g_ctx->rm->meshes[object.mesh];
    size_t size = mesh.data.vertices.size() * sizeof(Vertex);
    CudaEngine::ExtBufferDesc buffer_desc = {
#ifdef _WIN64
        handle,
#else
        fd,
#endif
        size,
        object.name
    };
    this->importExtBuffer(buffer_desc); // add to extBuffers internally
}
```

- `step()`

```cpp
waitOnSemaphore(vkUpdateSemaphore);
// TODO
signalSemaphore(cuUpdateSemaphore);
```

- semaphores are managed in `CudaEngine`
