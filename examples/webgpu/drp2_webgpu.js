const statusEl = document.querySelector("#status");
const canvas = document.querySelector("#viewport");



function setStatus(message, isError = false) {
  statusEl.textContent = message;
  statusEl.style.color = isError ? "#ff8f8f" : "#9be59b";
}



function required(value, message) {
  if (value === undefined || value === null) {
    throw new Error(message);
  }
  return value;
}



function mapLoadOp(loadOp) {
  if (loadOp === undefined || loadOp === "clear" || loadOp === "load") {
    return loadOp ?? "clear";
  }
  throw new Error(`unsupported load_op: ${loadOp}`);
}



function mapStoreOp(storeOp) {
  if (storeOp === undefined || storeOp === "store" || storeOp === "discard") {
    return storeOp ?? "store";
  }
  throw new Error(`unsupported store_op: ${storeOp}`);
}



function mapTopology(topology) {
  if (topology === undefined || topology === "triangle-list") {
    return "triangle-list";
  }
  throw new Error(`unsupported topology: ${topology}`);
}



function clearValue(value) {
  if (value === undefined) {
    return { r: 0, g: 0, b: 0, a: 1 };
  }
  return {
    r: value.r ?? 0,
    g: value.g ?? 0,
    b: value.b ?? 0,
    a: value.a ?? 1,
  };
}



function resizeCanvasToDisplaySize(device, context, format) {
  const scale = Math.max(1, window.devicePixelRatio || 1);
  const width = Math.max(1, Math.floor(canvas.clientWidth * scale));
  const height = Math.max(1, Math.floor(canvas.clientHeight * scale));
  if (canvas.width === width && canvas.height === height) {
    return false;
  }
  canvas.width = width;
  canvas.height = height;
  context.configure({
    device,
    format,
    alphaMode: "opaque",
  });
  return true;
}



async function initWebGPU() {
  if (!navigator.gpu) {
    throw new Error("WebGPU is not available in this browser");
  }

  const adapter = await navigator.gpu.requestAdapter();
  if (!adapter) {
    throw new Error("no WebGPU adapter is available");
  }

  const device = await adapter.requestDevice();
  const context = canvas.getContext("webgpu");
  if (!context) {
    throw new Error("failed to create a WebGPU canvas context");
  }

  const format = navigator.gpu.getPreferredCanvasFormat();
  resizeCanvasToDisplaySize(device, context, format);

  return { device, context, format };
}



function makePipeline(device, canvasFormat, shaders, command) {
  const vertexShader = required(
    shaders.get(command.vertex_shader_module_id),
    `unknown vertex shader module ${command.vertex_shader_module_id}`,
  );
  const fragmentShader = required(
    shaders.get(command.fragment_shader_module_id),
    `unknown fragment shader module ${command.fragment_shader_module_id}`,
  );

  const colorTargets = required(command.color_targets, "CreateRenderPipeline needs color_targets");
  if (colorTargets.length !== 1) {
    throw new Error("only one color target is supported by this PoC");
  }

  const streamFormat = colorTargets[0].format;
  const targetFormat = streamFormat === "canvas" ? canvasFormat : streamFormat;
  if (targetFormat !== canvasFormat) {
    throw new Error(
      `pipeline color target ${targetFormat} does not match canvas format ${canvasFormat}`,
    );
  }

  return device.createRenderPipeline({
    label: command.label,
    layout: "auto",
    vertex: {
      module: vertexShader.module,
      entryPoint: vertexShader.entryPoint,
    },
    fragment: {
      module: fragmentShader.module,
      entryPoint: fragmentShader.entryPoint,
      targets: [{ format: targetFormat }],
    },
    primitive: {
      topology: mapTopology(command.topology),
    },
  });
}



function beginRenderPass(context, encoders, command) {
  const encoder = required(
    encoders.get(command.encoder_id),
    `unknown command encoder ${command.encoder_id}`,
  );
  const attachments = required(command.color_attachments, "BeginRenderPass needs color_attachments");

  if (attachments.length !== 1) {
    throw new Error("only one color attachment is supported by this PoC");
  }

  const attachment = attachments[0];
  if (attachment.texture_id !== 0) {
    throw new Error("this PoC only supports texture_id 0 as the current canvas texture");
  }

  return encoder.beginRenderPass({
    label: command.label,
    colorAttachments: [
      {
        view: context.getCurrentTexture().createView(),
        loadOp: mapLoadOp(attachment.load_op),
        storeOp: mapStoreOp(attachment.store_op),
        clearValue: clearValue(attachment.clear_value),
      },
    ],
  });
}



async function executeDrp2Stream(device, context, canvasFormat, stream) {
  const shaders = new Map();
  const pipelines = new Map();
  const encoders = new Map();
  const passes = new Map();
  const commandBuffers = new Map();

  for (const command of stream.commands) {
    switch (command.cmd) {
      case "HelloRenderer":
      case "RendererHelloReply":
        break;

      case "CreateShaderModule": {
        if (command.format !== "wgsl") {
          throw new Error(`unsupported shader format: ${command.format}`);
        }
        const module = device.createShaderModule({
          label: command.label,
          code: required(command.code, "CreateShaderModule needs code"),
        });
        const info = await module.getCompilationInfo();
        const errors = info.messages.filter((message) => message.type === "error");
        if (errors.length > 0) {
          throw new Error(errors.map((message) => message.message).join("\n"));
        }
        shaders.set(command.id, {
          module,
          entryPoint: command.entry_point ?? "main",
          stage: command.stage,
        });
        break;
      }

      case "CreateRenderPipeline":
        pipelines.set(command.id, makePipeline(device, canvasFormat, shaders, command));
        break;

      case "BeginCommandEncoder":
        encoders.set(command.id, device.createCommandEncoder({ label: command.label }));
        break;

      case "BeginRenderPass":
        passes.set(command.id, beginRenderPass(context, encoders, command));
        break;

      case "SetPipeline": {
        const pass = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        const pipeline = required(
          pipelines.get(command.pipeline_id),
          `unknown pipeline ${command.pipeline_id}`,
        );
        pass.setPipeline(pipeline);
        break;
      }

      case "Draw": {
        const pass = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        pass.draw(
          command.vertex_count,
          command.instance_count ?? 1,
          command.first_vertex ?? 0,
          command.first_instance ?? 0,
        );
        break;
      }

      case "EndRenderPass": {
        const pass = required(passes.get(command.pass_id), `unknown pass ${command.pass_id}`);
        pass.end();
        passes.delete(command.pass_id);
        break;
      }

      case "FinishCommandEncoder": {
        const encoder = required(
          encoders.get(command.encoder_id),
          `unknown command encoder ${command.encoder_id}`,
        );
        commandBuffers.set(command.command_buffer_id, encoder.finish());
        encoders.delete(command.encoder_id);
        break;
      }

      case "QueueSubmit": {
        const ids = command.command_buffer_ids ?? [command.command_buffer_id];
        const buffers = ids.map((id) =>
          required(commandBuffers.get(id), `unknown command buffer ${id}`),
        );
        device.queue.submit(buffers);
        await device.queue.onSubmittedWorkDone();
        break;
      }

      default:
        throw new Error(`unsupported DRP2 command in WebGPU PoC: ${command.cmd}`);
    }
  }
}



async function main() {
  try {
    const { device, context, format } = await initWebGPU();
    const response = await fetch("./streams/hello_triangle_wgsl.json", { cache: "no-cache" });
    if (!response.ok) {
      throw new Error(`failed to load stream: ${response.status} ${response.statusText}`);
    }
    const stream = await response.json();

    const render = async () => {
      resizeCanvasToDisplaySize(device, context, format);
      await executeDrp2Stream(device, context, format, stream);
      setStatus(`Rendered ${stream.name}`);
    };

    await render();
    new ResizeObserver(() => {
      render().catch((error) => setStatus(error.message, true));
    }).observe(canvas);
  } catch (error) {
    setStatus(error.message, true);
    console.error(error);
  }
}



main();
