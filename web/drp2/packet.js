import { COMMAND_NAMES } from "./command_metadata.js";

const PACKET_MAGIC = [0x44, 0x56, 0x50, 0x32, 0x50, 0x4b, 0x54, 0x00];
const HEADER_SIZE = 56;
const RECORD_SIZE = 32;
const NO_PAYLOAD = 0xffffffffffffffffn;
const LABEL_SIZE = 512;

const PACKET_KIND_NAMES = new Map([[1, "setup"], [2, "update"], [3, "frame"]]);

function align8(value) {
  return (value + 7) & ~7;
}

function asBytes(value) {
  if (value instanceof Uint8Array) return value;
  if (ArrayBuffer.isView(value)) return new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
  if (value instanceof ArrayBuffer) return new Uint8Array(value);
  throw new Error("DRP2 packet decoder needs Uint8Array or ArrayBuffer");
}

function readU64(view, offset) {
  const value = view.getBigUint64(offset, true);
  const number = Number(value);
  if (!Number.isSafeInteger(number)) throw new Error(`uint64 value ${value} exceeds JS safe integer range`);
  return number;
}

function readCString(bytes, offset, maxLength = LABEL_SIZE) {
  let end = offset;
  const limit = Math.min(bytes.byteLength, offset + maxLength);
  while (end < limit && bytes[end] !== 0) end++;
  return new TextDecoder().decode(bytes.subarray(offset, end));
}

function payloadView(arena, payloadOffset, payloadSize) {
  if (payloadOffset === NO_PAYLOAD) return null;
  const offset = Number(payloadOffset);
  const size = Number(payloadSize);
  if ((offset & 7) !== 0 || offset + size > arena.byteLength) {
    throw new Error("invalid DRP2 packet payload span");
  }
  return arena.subarray(offset, offset + size);
}

function usageMask(mask, entries) {
  return entries.filter(([bit]) => (mask & bit) !== 0).map(([, name]) => name);
}

function bufferUsage(mask) {
  return usageMask(mask, [
    [0x0001, "COPY_SRC"], [0x0002, "COPY_DST"], [0x0004, "MAP_READ"],
    [0x0008, "MAP_WRITE"], [0x0010, "VERTEX"], [0x0020, "INDEX"],
    [0x0040, "UNIFORM"], [0x0080, "STORAGE"],
  ]);
}

function textureUsage(mask) {
  return usageMask(mask, [
    [0x0001, "COPY_SRC"], [0x0002, "COPY_DST"], [0x0004, "TEXTURE_BINDING"],
    [0x0008, "STORAGE_BINDING"], [0x0010, "RENDER_ATTACHMENT"],
  ]);
}

function textureFormat(format) {
  return new Map([
    [0, "rgba8unorm"], [9, "r8unorm"], [37, "rgba8unorm"], [44, "bgra8unorm"], [76, "r16float"],
    [97, "rgba16float"], [98, "r32uint"], [99, "r32sint"], [100, "r32float"],
    [101, "rg32uint"], [126, "depth32float"],
  ]).get(format) ?? "rgba8unorm";
}

function vertexFormat(format) {
  return new Map([
    [100, "float32"], [103, "float32x2"], [106, "float32x3"],
    [109, "float32x4"], [37, "unorm8x4"], [13, "uint8"], [98, "uint32"],
  ]).get(format) ?? null;
}

function topology(value) {
  return ["point-list", "line-list", "line-strip", "triangle-list", "triangle-strip"][value] ?? "triangle-list";
}

function cullMode(value) {
  return new Map([[1, "front"], [2, "back"], [3, "front-and-back"]]).get(value) ?? "none";
}

function frontFace(value) {
  return value === 1 ? "clockwise" : "counter-clockwise";
}

function depthCompare(value) {
  return ["never", "less", "equal", "less-equal", "greater", "not-equal", "greater-or-equal", "always"][value] ?? "always";
}

function bindingType(value) {
  return ["none", "uniform_buffer", "storage_buffer", "sampled_texture", "storage_texture", "sampler"][value] ?? "none";
}

function resourceKind(value) {
  return ["none", "buffer", "texture", "texture_view", "sampler"][value] ?? "none";
}

function bindingAccess(value) {
  return value === 0 ? "read" : "read_write";
}

function visibility(mask) {
  const out = [];
  if ((mask & 0x01) !== 0) out.push("VERTEX");
  if ((mask & 0x02) !== 0) out.push("FRAGMENT");
  if ((mask & 0x04) !== 0) out.push("COMPUTE");
  return out;
}

function filterMode(value) {
  return value === 1 ? "nearest" : "linear";
}

function attachmentLoad(value) {
  return ["clear", "load", "dont_care"][value] ?? "unknown";
}

function attachmentStore(value) {
  return ["store", "dont_care"][value] ?? "unknown";
}

function attachmentAccess(value) {
  return ["write", "read", "read_write"][value] ?? "unknown";
}

function colorWriteMask(mask) {
  const value = mask === 0 ? 0x0f : mask;
  return usageMask(value, [[1, "red"], [2, "green"], [4, "blue"], [8, "alpha"]]);
}

function colorTarget(view, offset) {
  const target = {
    format: textureFormat(view.getUint32(offset + 0, true)),
    write_mask: colorWriteMask(view.getUint32(offset + 32, true)),
  };
  if (view.getUint8(offset + 4) !== 0) {
    target.blend = {
      color: {
        src_factor: view.getUint32(offset + 8, true),
        dst_factor: view.getUint32(offset + 12, true),
        operation: view.getUint32(offset + 16, true),
      },
      alpha: {
        src_factor: view.getUint32(offset + 20, true),
        dst_factor: view.getUint32(offset + 24, true),
        operation: view.getUint32(offset + 28, true),
      },
    };
  }
  return target;
}

function bindGroupLayoutEntry(view, offset) {
  const entry = {
    binding: view.getUint32(offset + 0, true),
    binding_type: bindingType(view.getUint32(offset + 4, true)),
  };
  const vis = visibility(view.getUint32(offset + 8, true));
  if (vis.length > 0) entry.visibility = vis;
  if (entry.binding_type === "storage_buffer" || entry.binding_type === "storage_texture") {
    entry.access = bindingAccess(view.getUint32(offset + 12, true));
  }
  if (view.getUint8(offset + 16) !== 0) entry.has_dynamic_offset = true;
  return entry;
}

function bindGroupEntry(view, offset) {
  const entry = {
    binding: view.getUint32(offset + 0, true),
    binding_type: bindingType(view.getUint32(offset + 4, true)),
    resource_kind: resourceKind(view.getUint32(offset + 8, true)),
    resource_id: readU64(view, offset + 16),
  };
  if (entry.resource_kind === "buffer") {
    entry.offset = readU64(view, offset + 24);
    entry.size = readU64(view, offset + 32);
  }
  return entry;
}

function colorAttachment(view, offset) {
  const attachment = {
    texture_id: readU64(view, offset + 0),
    load_op: attachmentLoad(view.getUint32(offset + 24, true)),
    store_op: attachmentStore(view.getUint32(offset + 28, true)),
    clear_value: {
      r: view.getFloat32(offset + 36, true),
      g: view.getFloat32(offset + 40, true),
      b: view.getFloat32(offset + 44, true),
      a: view.getFloat32(offset + 48, true),
    },
  };
  const resolveTextureId = readU64(view, offset + 8);
  const resolveMode = view.getUint32(offset + 16, true);
  if (resolveTextureId !== 0 || resolveMode !== 0) {
    attachment.resolve_target = {
      texture_id: resolveTextureId,
      mode: resolveMode || 1,
    };
  }
  const access = attachmentAccess(view.getUint32(offset + 32, true));
  if (access !== "write") attachment.access = access;
  return attachment;
}

function decodeCommand(type, view, bodyOffset, bodySize, arena, payloadOffset, payloadSize) {
  const bytes = new Uint8Array(view.buffer, view.byteOffset, view.byteLength);
  const cmd = COMMAND_NAMES[type] ?? "None";
  switch (type) {
  case 1:
    return { cmd, version: { major: 2, minor: 0 }, client_name: readCString(bytes, bodyOffset) };
  case 2:
    return { cmd, version: { major: 2, minor: 0 }, status: "ok", renderer_name: readCString(bytes, bodyOffset) };
  case 3:
    return { cmd, id: readU64(view, bodyOffset), size: readU64(view, bodyOffset + 8), usage: bufferUsage(view.getUint32(bodyOffset + 16, true)) };
  case 4:
    return { cmd, buffer_id: readU64(view, bodyOffset) };
  case 5:
    return {
      cmd,
      id: readU64(view, bodyOffset),
      dimension: view.getUint32(bodyOffset + 16, true) > 1 ? "3d" : "2d",
      width: view.getUint32(bodyOffset + 8, true),
      height: view.getUint32(bodyOffset + 12, true),
      depth: view.getUint32(bodyOffset + 16, true) || 1,
      format: textureFormat(view.getUint32(bodyOffset + 20, true)),
      usage: textureUsage(view.getUint32(bodyOffset + 24, true)),
      mip_level_count: 1,
      sample_count: view.getUint32(bodyOffset + 28, true) || 1,
    };
  case 6:
    return { cmd, texture_id: readU64(view, bodyOffset) };
  case 7: {
    const payload = payloadView(arena, payloadOffset, payloadSize);
    if (payload === null) throw new Error("CreateShaderModule needs payload");
    if (payload.byteLength === 0) throw new Error("CreateShaderModule needs non-empty payload");
    if (payload[payload.byteLength - 1] !== 0) {
      throw new Error("CreateShaderModule text payload needs terminator");
    }
    const codeBytes = payload[payload.byteLength - 1] === 0 ? payload.subarray(0, -1) : payload;
    if (codeBytes.byteLength === 0) {
      throw new Error("CreateShaderModule needs non-empty source");
    }
    const stage = readCString(bytes, bodyOffset + 8);
    if (stage === "") throw new Error("CreateShaderModule needs stage");
    const format = readCString(bytes, bodyOffset + 520);
    if (format === "") throw new Error("CreateShaderModule needs format");
    const command = {
      cmd,
      id: readU64(view, bodyOffset),
      stage,
      format,
      entry_point: "main",
      code: new TextDecoder().decode(codeBytes),
    };
    const builtinFamily = readCString(bytes, bodyOffset + 1032);
    if (builtinFamily !== "") {
      command.builtin_family = builtinFamily;
      command.builtin_variant = readCString(bytes, bodyOffset + 1544);
      command.builtin_version = view.getUint32(bodyOffset + 2056, true);
    }
    return command;
  }
  case 8:
    return { cmd, shader_module_id: readU64(view, bodyOffset) };
  case 9: {
    const command = {
      cmd,
      id: readU64(view, bodyOffset),
      vertex_buffer_slots: view.getUint32(bodyOffset + 24, true),
      vertex_shader_module_id: readU64(view, bodyOffset + 8),
      fragment_shader_module_id: readU64(view, bodyOffset + 16),
    };
    const bglCount = view.getUint32(bodyOffset + 28, true);
    if (bglCount > 0) {
      command.bind_group_layout_ids = Array.from({ length: bglCount }, (_, i) => readU64(view, bodyOffset + 32 + i * 8));
    }
    const builtin = readCString(bytes, bodyOffset + 636);
    if (builtin !== "") {
      command.builtin_pipeline = builtin;
      command.builtin_version = view.getUint32(bodyOffset + 1148, true);
    }
    const bindingCount = view.getUint32(bodyOffset + 244, true);
    if (bindingCount > 0) {
      command.topology = topology(view.getUint32(bodyOffset + 240, true));
      command.vertex_buffers = [];
      const attrCount = view.getUint32(bodyOffset + 376, true);
      for (let b = 0; b < bindingCount; b++) {
        const attributes = [];
        for (let a = 0; a < attrCount; a++) {
          if (view.getUint32(bodyOffset + 380 + a * 4, true) !== b) continue;
          const format = vertexFormat(view.getUint32(bodyOffset + 508 + a * 4, true));
          if (format === null) continue;
          attributes.push({
            shader_location: view.getUint32(bodyOffset + 444 + a * 4, true),
            offset: view.getUint32(bodyOffset + 572 + a * 4, true),
            format,
          });
        }
        command.vertex_buffers.push({
          array_stride: view.getUint32(bodyOffset + 248 + b * 4, true),
          step_mode: view.getUint32(bodyOffset + 312 + b * 4, true) === 1 ? "instance" : "vertex",
          attributes,
        });
      }
    } else if (command.vertex_buffer_slots === 0) {
      command.vertex_buffers = [];
    }
    if (view.getUint8(bodyOffset + 72) !== 0) {
      command.raster = {
        cull_mode: cullMode(view.getUint32(bodyOffset + 76, true)),
        front_face: frontFace(view.getUint32(bodyOffset + 80, true)),
      };
    }
    command.multisample = {
      sample_count: view.getUint32(bodyOffset + 84, true) || 1,
      alpha_to_coverage_enabled: view.getUint8(bodyOffset + 88) !== 0,
    };
    const colorTargetCount = view.getUint32(bodyOffset + 92, true) || 1;
    command.color_targets = Array.from({ length: colorTargetCount }, (_, i) => colorTarget(view, bodyOffset + 96 + i * 36));
    if (view.getUint8(bodyOffset + 64) !== 0) {
      command.depth_stencil = {
        format: "depth32float",
        depth_write_enabled: view.getUint8(bodyOffset + 65) !== 0,
        depth_compare: depthCompare(view.getUint32(bodyOffset + 68, true)),
      };
    }
    return command;
  }
  case 10:
    return { cmd, render_pipeline_id: readU64(view, bodyOffset) };
  case 11: {
    const count = view.getUint32(bodyOffset + 16, true);
    const command = { cmd, id: readU64(view, bodyOffset), compute_shader_module_id: readU64(view, bodyOffset + 8) };
    if (count > 0) command.bind_group_layout_ids = Array.from({ length: count }, (_, i) => readU64(view, bodyOffset + 24 + i * 8));
    return command;
  }
  case 12:
    return { cmd, compute_pipeline_id: readU64(view, bodyOffset) };
  case 13:
    return {
      cmd,
      id: readU64(view, bodyOffset),
      mag_filter: filterMode(view.getUint32(bodyOffset + 8, true)),
      min_filter: filterMode(view.getUint32(bodyOffset + 12, true)),
      mipmap_filter: "nearest",
      address_mode_u: "clamp-to-edge",
      address_mode_v: "clamp-to-edge",
    };
  case 14: {
    const count = view.getUint32(bodyOffset + 8, true);
    return { cmd, id: readU64(view, bodyOffset), entries: Array.from({ length: count }, (_, i) => bindGroupLayoutEntry(view, bodyOffset + 12 + i * 20)) };
  }
  case 15: {
    const count = view.getUint32(bodyOffset + 16, true);
    return {
      cmd,
      id: readU64(view, bodyOffset),
      bind_group_layout_id: readU64(view, bodyOffset + 8),
      entries: Array.from({ length: count }, (_, i) => bindGroupEntry(view, bodyOffset + 24 + i * 40)),
    };
  }
  case 16:
    return { cmd, bind_group_layout_id: readU64(view, bodyOffset) };
  case 17:
    return { cmd, bind_group_id: readU64(view, bodyOffset) };
  case 18: {
    const payload = payloadView(arena, payloadOffset, payloadSize);
    return { cmd, buffer_id: readU64(view, bodyOffset), offset: readU64(view, bodyOffset + 8), size: readU64(view, bodyOffset + 16), data: payload ?? new Uint8Array() };
  }
  case 19: {
    const payload = payloadView(arena, payloadOffset, payloadSize);
    return {
      cmd,
      texture_id: readU64(view, bodyOffset),
      mip_level: view.getUint32(bodyOffset + 8, true),
      origin: { x: view.getUint32(bodyOffset + 12, true), y: view.getUint32(bodyOffset + 16, true), z: view.getUint32(bodyOffset + 20, true) },
      size: { width: view.getUint32(bodyOffset + 24, true), height: view.getUint32(bodyOffset + 28, true), depth: view.getUint32(bodyOffset + 32, true) },
      bytes_per_row: view.getUint32(bodyOffset + 36, true),
      rows_per_image: view.getUint32(bodyOffset + 40, true),
      data: payload ?? new Uint8Array(),
    };
  }
  case 20:
    return { cmd, id: readU64(view, bodyOffset) };
  case 21: {
    const colorAttachmentCount = view.getUint32(bodyOffset + 24, true);
    const attachments = colorAttachmentCount > 0
      ? Array.from({ length: colorAttachmentCount }, (_, i) => colorAttachment(view, bodyOffset + 32 + i * 56))
      : [{
          texture_id: readU64(view, bodyOffset + 16),
          load_op: view.getUint8(bodyOffset + 324) !== 0 ? "clear" : "load",
          store_op: "store",
          clear_value: {
            r: view.getFloat32(bodyOffset + 292, true),
            g: view.getFloat32(bodyOffset + 296, true),
            b: view.getFloat32(bodyOffset + 300, true),
            a: view.getFloat32(bodyOffset + 304, true),
          },
        }];
    const command = { cmd, id: readU64(view, bodyOffset), encoder_id: readU64(view, bodyOffset + 8), color_attachments: attachments };
    if (view.getUint8(bodyOffset + 256) !== 0) {
      command.depth_stencil_attachment = {
        texture_id: readU64(view, bodyOffset + 264),
        depth_load_op: attachmentLoad(view.getUint32(bodyOffset + 272, true)),
        depth_store_op: attachmentStore(view.getUint32(bodyOffset + 276, true)),
        depth_clear_value: view.getFloat32(bodyOffset + 288, true),
      };
      const access = attachmentAccess(view.getUint32(bodyOffset + 280, true));
      if (access !== "read_write") command.depth_stencil_attachment.access = access;
    }
    return command;
  }
  case 22:
    return { cmd, id: readU64(view, bodyOffset), encoder_id: readU64(view, bodyOffset + 8) };
  case 23:
    return { cmd, pass_id: readU64(view, bodyOffset), x: view.getFloat32(bodyOffset + 8, true), y: view.getFloat32(bodyOffset + 12, true), width: view.getFloat32(bodyOffset + 16, true), height: view.getFloat32(bodyOffset + 20, true), min_depth: 0, max_depth: 1 };
  case 24:
    return { cmd, pass_id: readU64(view, bodyOffset), x: view.getFloat32(bodyOffset + 8, true), y: view.getFloat32(bodyOffset + 12, true), width: view.getFloat32(bodyOffset + 16, true), height: view.getFloat32(bodyOffset + 20, true) };
  case 25:
    return { cmd, pass_id: readU64(view, bodyOffset), pipeline_id: readU64(view, bodyOffset + 8) };
  case 26: {
    const count = view.getUint32(bodyOffset + 24, true);
    const command = { cmd, pass_id: readU64(view, bodyOffset), slot: view.getUint32(bodyOffset + 8, true), bind_group_id: readU64(view, bodyOffset + 16) };
    if (count > 0) command.dynamic_offsets = Array.from({ length: count }, (_, i) => readU64(view, bodyOffset + 32 + i * 8));
    return command;
  }
  case 27:
    return { cmd, pass_id: readU64(view, bodyOffset), slot: view.getUint32(bodyOffset + 8, true), buffer_id: readU64(view, bodyOffset + 16), offset: readU64(view, bodyOffset + 24) };
  case 28:
    return { cmd, pass_id: readU64(view, bodyOffset), buffer_id: readU64(view, bodyOffset + 8), index_format: readCString(bytes, bodyOffset + 16), offset: readU64(view, bodyOffset + 528) };
  case 29:
    return { cmd, pass_id: readU64(view, bodyOffset), vertex_count: view.getUint32(bodyOffset + 8, true), instance_count: view.getUint32(bodyOffset + 12, true), first_vertex: view.getUint32(bodyOffset + 16, true), first_instance: view.getUint32(bodyOffset + 20, true) };
  case 30:
    return { cmd, pass_id: readU64(view, bodyOffset), index_count: view.getUint32(bodyOffset + 8, true), instance_count: view.getUint32(bodyOffset + 12, true), first_index: view.getUint32(bodyOffset + 16, true), base_vertex: view.getInt32(bodyOffset + 20, true), first_instance: view.getUint32(bodyOffset + 24, true) };
  case 31:
    return { cmd, pass_id: readU64(view, bodyOffset) };
  case 32:
    return { cmd, pass_id: readU64(view, bodyOffset), x: view.getUint32(bodyOffset + 8, true), y: view.getUint32(bodyOffset + 12, true), z: view.getUint32(bodyOffset + 16, true) };
  case 33:
    return { cmd, pass_id: readU64(view, bodyOffset) };
  case 34:
    return {
      cmd,
      encoder_id: readU64(view, bodyOffset),
      buffer_id: readU64(view, bodyOffset + 8),
      offset: readU64(view, bodyOffset + 16),
      size: readU64(view, bodyOffset + 24),
      src_stage: readCString(bytes, bodyOffset + 32),
      src_access: readCString(bytes, bodyOffset + 544),
      dst_stage: readCString(bytes, bodyOffset + 1056),
      dst_access: readCString(bytes, bodyOffset + 1568),
    };
  case 35:
    return { cmd, encoder_id: readU64(view, bodyOffset), src_buffer_id: readU64(view, bodyOffset + 8), src_offset: readU64(view, bodyOffset + 16), dst_buffer_id: readU64(view, bodyOffset + 24), dst_offset: readU64(view, bodyOffset + 32), size: readU64(view, bodyOffset + 40) };
  case 36:
    return { cmd, encoder_id: readU64(view, bodyOffset), src_buffer_id: readU64(view, bodyOffset + 8), src_offset: readU64(view, bodyOffset + 16), bytes_per_row: view.getUint32(bodyOffset + 24, true), rows_per_image: view.getUint32(bodyOffset + 28, true), dst_texture_id: readU64(view, bodyOffset + 32), dst_mip_level: view.getUint32(bodyOffset + 40, true), dst_origin: { x: view.getUint32(bodyOffset + 44, true), y: view.getUint32(bodyOffset + 48, true), z: view.getUint32(bodyOffset + 52, true) }, size: { width: view.getUint32(bodyOffset + 56, true), height: view.getUint32(bodyOffset + 60, true), depth: view.getUint32(bodyOffset + 64, true) } };
  case 37:
    return { cmd, encoder_id: readU64(view, bodyOffset), src_texture_id: readU64(view, bodyOffset + 8), src_mip_level: 0, src_origin: { x: 0, y: 0, z: 0 }, size: { width: view.getUint32(bodyOffset + 32, true), height: view.getUint32(bodyOffset + 36, true), depth: 1 }, dst_buffer_id: readU64(view, bodyOffset + 16), dst_offset: readU64(view, bodyOffset + 24), bytes_per_row: view.getUint32(bodyOffset + 40, true), rows_per_image: view.getUint32(bodyOffset + 44, true) };
  case 38:
    return { cmd, encoder_id: readU64(view, bodyOffset), src_texture_id: readU64(view, bodyOffset + 8), src_mip_level: view.getUint32(bodyOffset + 16, true), src_origin: { x: view.getUint32(bodyOffset + 20, true), y: view.getUint32(bodyOffset + 24, true), z: view.getUint32(bodyOffset + 28, true) }, dst_texture_id: readU64(view, bodyOffset + 32), dst_mip_level: view.getUint32(bodyOffset + 40, true), dst_origin: { x: view.getUint32(bodyOffset + 44, true), y: view.getUint32(bodyOffset + 48, true), z: view.getUint32(bodyOffset + 52, true) }, size: { width: view.getUint32(bodyOffset + 56, true), height: view.getUint32(bodyOffset + 60, true), depth: view.getUint32(bodyOffset + 64, true) } };
  case 39:
    return { cmd, encoder_id: readU64(view, bodyOffset), command_buffer_id: readU64(view, bodyOffset + 8) };
  case 40: {
    const command = { cmd, command_buffer_ids: [readU64(view, bodyOffset)], submission_id: readU64(view, bodyOffset + 8) };
    if (view.getUint8(bodyOffset + 16) !== 0) {
      command.readbacks = [{ buffer_id: readU64(view, bodyOffset + 24), offset: readU64(view, bodyOffset + 32), size: readU64(view, bodyOffset + 40) }];
    }
    return command;
  }
  default:
    throw new Error(`unsupported DRP2 packet command type ${type} (${cmd}) body_size=${bodySize}`);
  }
}

export function decodeDrp2Packet(packetInput, arenaInput = new Uint8Array()) {
  const packet = asBytes(packetInput);
  const arena = asBytes(arenaInput);
  if (packet.byteLength < HEADER_SIZE) throw new Error("truncated DRP2 packet header");
  for (let i = 0; i < PACKET_MAGIC.length; i++) {
    if (packet[i] !== PACKET_MAGIC[i]) throw new Error("bad DRP2 packet magic");
  }
  const view = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
  const headerSize = view.getUint16(8, true);
  const major = view.getUint16(10, true);
  const minor = view.getUint16(12, true);
  const kind = view.getUint16(14, true);
  const flags = view.getUint32(16, true);
  const commandCount = view.getUint32(20, true);
  const commandBytes = readU64(view, 24);
  const arenaSize = readU64(view, 32);
  if (headerSize !== HEADER_SIZE || major !== 2 || flags !== 0 || !PACKET_KIND_NAMES.has(kind)) {
    throw new Error("unsupported DRP2 packet header");
  }
  if (HEADER_SIZE + commandBytes !== packet.byteLength) throw new Error("DRP2 packet size mismatch");
  if (arenaSize > arena.byteLength) throw new Error("DRP2 packet arena is truncated");

  const commands = [];
  let recordOffset = HEADER_SIZE;
  for (let i = 0; i < commandCount; i++) {
    if (recordOffset + RECORD_SIZE > packet.byteLength) throw new Error("truncated DRP2 packet record");
    const type = view.getUint32(recordOffset + 0, true);
    const recordFlags = view.getUint32(recordOffset + 4, true);
    const bodySize = view.getUint32(recordOffset + 8, true);
    const payloadOffset = view.getBigUint64(recordOffset + 16, true);
    const payloadSize = view.getBigUint64(recordOffset + 24, true);
    const bodyPadded = align8(bodySize);
    const bodyOffset = recordOffset + RECORD_SIZE;
    if (recordFlags !== 0 || bodySize === 0 || bodyOffset + bodyPadded > packet.byteLength) {
      throw new Error("invalid DRP2 packet record");
    }
    commands.push(decodeCommand(type, view, bodyOffset, bodySize, arena.subarray(0, arenaSize), payloadOffset, payloadSize));
    recordOffset = bodyOffset + bodyPadded;
  }
  if (recordOffset !== packet.byteLength) throw new Error("DRP2 packet trailing bytes");

  return {
    kind: PACKET_KIND_NAMES.get(kind),
    kind_id: kind,
    version: { major, minor },
    command_count: commandCount,
    command_bytes: commandBytes,
    arena_size: arenaSize,
    resource_version: readU64(view, 40),
    frame_index: readU64(view, 48),
    commands,
  };
}

export function decodeDrp2PacketSet(packetSet) {
  const phases = {};
  for (const key of ["setup", "update", "frame"]) {
    const span = packetSet[key];
    if (span?.packet !== undefined && span.packet.byteLength > 0) {
      const packet = decodeDrp2Packet(span.packet, span.arena ?? new Uint8Array());
      if (packet.kind !== key) {
        throw new Error(`DRP2 packet phase ${key} contains ${packet.kind} packet`);
      }
      phases[key] = packet;
    }
  }
  return phases;
}
