#!/usr/bin/env python3
"""Adapt a portable DVZR recording directory to the browser WebGPU DRP2 JSON shape."""

from __future__ import annotations

import argparse
import base64
import gzip
import json
from pathlib import Path
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[1]
WGSL_DIR = REPO_ROOT / "src" / "scene" / "wgsl"

BUFFER_USAGE = [
    (0x0001, "COPY_SRC"),
    (0x0002, "COPY_DST"),
    (0x0004, "MAP_READ"),
    (0x0008, "MAP_WRITE"),
    (0x0010, "VERTEX"),
    (0x0020, "INDEX"),
    (0x0040, "UNIFORM"),
    (0x0080, "STORAGE"),
]

TEXTURE_USAGE = [
    (0x0001, "COPY_SRC"),
    (0x0002, "COPY_DST"),
    (0x0004, "TEXTURE_BINDING"),
    (0x0008, "STORAGE_BINDING"),
    (0x0010, "RENDER_ATTACHMENT"),
]

TEXTURE_FORMATS = {
    37: "rgba8unorm",
    44: "bgra8unorm",
    98: "r32uint",
    126: "depth32float",
}

VERTEX_FORMATS = {
    37: "unorm8x4",
    100: "float32",
    103: "float32x2",
    106: "float32x3",
    109: "float32x4",
    98: "uint32",
}

TOPOLOGIES = {
    0: "point-list",
    1: "line-list",
    2: "line-strip",
    3: "triangle-list",
    4: "triangle-strip",
}

DEPTH_COMPARE = {
    0: "never",
    1: "less",
    2: "equal",
    3: "less-equal",
    4: "greater",
    5: "not-equal",
    6: "greater-equal",
    7: "always",
}

BINDING_TYPES = {
    1: "uniform_buffer",
    2: "storage_buffer",
    3: "sampled_texture",
    4: "storage_texture",
    5: "sampler",
}

RESOURCE_KINDS = {
    1: "buffer",
    2: "texture",
    3: "texture_view",
    4: "sampler",
}

SHADER_VARIANTS = {
    ("scene.primitive", "default", "VERTEX"): "primitive.vert.wgsl",
    ("scene.primitive", "default", "FRAGMENT"): "primitive.frag.wgsl",
    ("scene.primitive", "lit", "VERTEX"): "primitive_lit.vert.wgsl",
    ("scene.primitive", "lit", "FRAGMENT"): "primitive_lit.frag.wgsl",
}


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert a DVZR recording directory to a WebGPU-compatible DRP2 stream JSON."
    )
    parser.add_argument("recording", type=Path, help="Input .dvzr recording directory.")
    parser.add_argument("output", type=Path, help="Output WebGPU stream JSON path.")
    parser.add_argument(
        "--name",
        default=None,
        help="Output stream name. Defaults to the output filename stem.",
    )
    parser.add_argument(
        "--frame",
        type=int,
        default=None,
        help="Keep commands through this recorded frame index instead of all frames.",
    )
    parser.add_argument(
        "--gzip-threshold",
        type=int,
        default=512,
        help="Compress binary payloads at or above this byte size.",
    )
    return parser.parse_args()


def _read_records(recording: Path) -> list[dict[str, Any]]:
    stream_path = recording / "stream.jsonl"
    records: list[dict[str, Any]] = []
    with stream_path.open("r", encoding="utf8") as stream:
        for line in stream:
            line = line.strip()
            if line:
                records.append(json.loads(line))
    return records


def _records_through_frame(records: list[dict[str, Any]], frame: int | None) -> list[dict[str, Any]]:
    if frame is None:
        return records
    frames = [record for record in records if record.get("type") == "frame"]
    if frame < 0 or frame >= len(frames):
        raise ValueError(f"frame index {frame} is out of range, recording has {len(frames)} frames")
    end = frames[frame]["first_command"] + frames[frame]["command_count"]
    return [
        record
        for record in records
        if record.get("type") != "command" or int(record["index"]) < end
    ]


def _flag_names(value: int, table: list[tuple[int, str]]) -> list[str]:
    return [name for bit, name in table if value & bit]


def _shader_stage(stage: str) -> str:
    upper = stage.upper()
    if upper in {"VERTEX", "FRAGMENT", "COMPUTE"}:
        return upper
    raise ValueError(f"unsupported shader stage {stage!r}")


def _read_blob(recording: Path, command: dict[str, Any]) -> bytes:
    blob = command.get("payload_blob")
    if not blob:
        return b""
    return (recording / blob).read_bytes()


def _payload_field(data: bytes, threshold: int) -> dict[str, Any]:
    if len(data) >= threshold:
        compressed = gzip.compress(data)
        if len(compressed) < len(data):
            return {
                "data": base64.b64encode(compressed).decode("ascii"),
                "data_encoding": "base64+gzip",
                "uncompressed_size": len(data),
            }
    return {"data": base64.b64encode(data).decode("ascii"), "data_encoding": "base64"}


def _load_wgsl(filename: str) -> str:
    path = WGSL_DIR / filename
    text = path.read_text(encoding="utf8")
    lines: list[str] = []
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith('#include "') and stripped.endswith('"'):
            include = stripped[len('#include "'):-1]
            lines.append(_load_wgsl(include))
        else:
            lines.append(line)
    return "\n".join(lines) + "\n"


def _infer_shader_identities(commands: list[dict[str, Any]]) -> dict[int, tuple[str, str]]:
    identities: dict[int, tuple[str, str]] = {}
    for command in commands:
        if command.get("op") != "CreateShaderModule":
            continue
        family = command.get("builtin_family")
        variant = command.get("builtin_variant")
        if family and variant:
            identities[int(command["id"])] = (str(family), str(variant))

    for command in commands:
        if command.get("op") != "CreateRenderPipeline":
            continue
        vs_id = int(command["vertex_shader_module_id"])
        fs_id = int(command["fragment_shader_module_id"])
        if vs_id in identities and fs_id in identities:
            continue
        binding_count = int(command.get("binding_count", 0))
        attr_count = int(command.get("attr_count", 0))
        if binding_count == 3 and attr_count == 3:
            identities.setdefault(vs_id, ("scene.primitive", "lit"))
            identities.setdefault(fs_id, ("scene.primitive", "lit"))
        elif binding_count == 2 and attr_count == 2:
            identities.setdefault(vs_id, ("scene.primitive", "default"))
            identities.setdefault(fs_id, ("scene.primitive", "default"))
    return identities


def _recorded_canvas_extent(commands: list[dict[str, Any]]) -> dict[str, int] | None:
    for command in commands:
        if command.get("op") != "CreateTexture":
            continue
        if not (int(command.get("usage", 0)) & 0x10):
            continue
        return {
            "width": int(command["width"]),
            "height": int(command["height"]),
        }
    return None


class IdMap:
    def __init__(self) -> None:
        self._map: dict[int, int] = {}
        self._next = 1

    def map(self, value: int) -> int:
        if value == 0:
            return 0
        if value not in self._map:
            self._map[value] = self._next
            self._next += 1
        return self._map[value]

    def synthetic(self) -> int:
        value = self._next
        self._next += 1
        return value


def _entries(command: dict[str, Any]) -> list[dict[str, Any]]:
    entries = []
    for i in range(int(command.get("entry_count", 0))):
        binding_type = BINDING_TYPES.get(int(command.get(f"entry{i}_binding_type", 0)))
        if binding_type is None:
            raise ValueError(f"unsupported bind-group-layout binding type in {command}")
        visibility = []
        vis = int(command.get(f"entry{i}_visibility", 0))
        if vis & 0x1:
            visibility.append("VERTEX")
        if vis & 0x2:
            visibility.append("FRAGMENT")
        if vis & 0x4:
            visibility.append("COMPUTE")
        entry = {
            "binding": int(command[f"entry{i}_binding"]),
            "binding_type": binding_type,
            "visibility": visibility,
        }
        if binding_type == "storage_buffer":
            entry["access"] = "read_write" if int(command.get(f"entry{i}_access", 0)) else "read"
        if int(command.get(f"entry{i}_has_dynamic_offset", 0)):
            entry["has_dynamic_offset"] = True
        entries.append(entry)
    return entries


def _bind_group_entries(command: dict[str, Any], ids: IdMap) -> list[dict[str, Any]]:
    entries = []
    for i in range(int(command.get("entry_count", 0))):
        binding_type = BINDING_TYPES.get(int(command.get(f"entry{i}_binding_type", 0)))
        resource_kind = RESOURCE_KINDS.get(int(command.get(f"entry{i}_resource_kind", 0)))
        if binding_type is None or resource_kind is None:
            raise ValueError(f"unsupported bind-group entry in {command}")
        entries.append(
            {
                "binding": int(command[f"entry{i}_binding"]),
                "binding_type": binding_type,
                "resource_kind": resource_kind,
                "resource_id": ids.map(int(command[f"entry{i}_resource_id"])),
                "offset": int(command.get(f"entry{i}_offset", 0)),
                "size": int(command.get(f"entry{i}_size", 0)),
            }
        )
    return entries


def _vertex_buffers(command: dict[str, Any]) -> list[dict[str, Any]]:
    buffers = []
    for binding in range(int(command.get("binding_count", 0))):
        attrs = []
        for attr in range(int(command.get("attr_count", 0))):
            if int(command.get(f"attr{attr}_binding", 0)) != binding:
                continue
            fmt = VERTEX_FORMATS.get(int(command.get(f"attr{attr}_format", 0)))
            if fmt is None:
                raise ValueError(f"unsupported vertex format {command.get(f'attr{attr}_format')}")
            attrs.append(
                {
                    "shader_location": int(command[f"attr{attr}_location"]),
                    "offset": int(command[f"attr{attr}_offset"]),
                    "format": fmt,
                }
            )
        buffers.append(
            {
                "array_stride": int(command[f"binding{binding}_stride"]),
                "step_mode": "instance"
                if int(command.get(f"binding{binding}_step_mode", 0))
                else "vertex",
                "attributes": attrs,
            }
        )
    return buffers


def _color_targets(command: dict[str, Any]) -> list[dict[str, Any]]:
    count = int(command.get("color_target_count", 0)) or 1
    targets = []
    for i in range(count):
        fmt_value = int(command.get(f"ct{i}_format", 0))
        targets.append({"format": TEXTURE_FORMATS.get(fmt_value, "canvas")})
    return targets


def _convert_command(
    recording: Path,
    command: dict[str, Any],
    ids: IdMap,
    identities: dict[int, tuple[str, str]],
    canvas_texture_ids: set[int],
    gzip_threshold: int,
) -> list[dict[str, Any]]:
    op = command.get("op")
    if op == "HelloRenderer":
        return [{"cmd": "HelloRenderer", "version": {"major": 2, "minor": 0}, "client_name": command.get("name", "dvzr")}]
    if op == "RendererHelloReply":
        return [{"cmd": "RendererHelloReply", "version": {"major": 2, "minor": 0}, "status": "ok", "renderer_name": command.get("name", "datoviz")}]
    if op == "CreateTexture":
        texture_id = int(command["id"])
        if int(command.get("usage", 0)) & 0x10:
            canvas_texture_ids.add(texture_id)
            return []
        return [{
            "cmd": "CreateTexture",
            "id": ids.map(texture_id),
            "dimension": "3d" if int(command.get("depth", 1)) > 1 else "2d",
            "width": int(command["width"]),
            "height": int(command["height"]),
            "depth": int(command.get("depth", 1)),
            "format": TEXTURE_FORMATS[int(command.get("format", 37))],
            "usage": _flag_names(int(command["usage"]), TEXTURE_USAGE),
            "mip_level_count": 1,
            "sample_count": 1,
        }]
    if op == "CreateBuffer":
        return [{
            "cmd": "CreateBuffer",
            "id": ids.map(int(command["id"])),
            "size": int(command["size"]),
            "usage": _flag_names(int(command["usage"]), BUFFER_USAGE),
        }]
    if op == "WriteBuffer":
        data = _read_blob(recording, command)
        return [{
            "cmd": "WriteBuffer",
            "buffer_id": ids.map(int(command["buffer_id"])),
            "offset": int(command.get("offset", 0)),
            "size": int(command.get("size", len(data))),
            **_payload_field(data, gzip_threshold),
        }]
    if op == "CreateBindGroupLayout":
        return [{"cmd": "CreateBindGroupLayout", "id": ids.map(int(command["id"])), "entries": _entries(command)}]
    if op == "CreateBindGroup":
        return [{
            "cmd": "CreateBindGroup",
            "id": ids.map(int(command["id"])),
            "bind_group_layout_id": ids.map(int(command["bind_group_layout_id"])),
            "entries": _bind_group_entries(command, ids),
        }]
    if op == "CreateShaderModule":
        shader_id = int(command["id"])
        stage = _shader_stage(str(command["stage"]))
        fmt = str(command.get("format", "wgsl"))
        family, variant = identities.get(shader_id, ("", ""))
        if fmt == "wgsl":
            code = _read_blob(recording, command).decode("utf8").rstrip("\0")
        elif family and variant and (family, variant, stage) in SHADER_VARIANTS:
            code = _load_wgsl(SHADER_VARIANTS[(family, variant, stage)])
        else:
            raise ValueError(
                f"unsupported non-WGSL shader id {shader_id}: format={fmt} "
                f"identity={family}/{variant}/{stage}"
            )
        out: dict[str, Any] = {
            "cmd": "CreateShaderModule",
            "id": ids.map(shader_id),
            "stage": stage,
            "format": "wgsl",
            "entry_point": "main",
            "code": code,
        }
        if family:
            out["builtin_family"] = family
            out["builtin_variant"] = variant
            out["builtin_version"] = int(command.get("builtin_version", 1))
        return [out]
    if op == "CreateRenderPipeline":
        out = {
            "cmd": "CreateRenderPipeline",
            "id": ids.map(int(command["id"])),
            "vertex_buffer_slots": int(command.get("vertex_buffer_slots", 0)),
            "vertex_shader_module_id": ids.map(int(command["vertex_shader_module_id"])),
            "fragment_shader_module_id": ids.map(int(command["fragment_shader_module_id"])),
            "topology": TOPOLOGIES[int(command.get("topology", 3))],
            "vertex_buffers": _vertex_buffers(command),
            "color_targets": _color_targets(command),
        }
        bgl = [
            ids.map(int(command[f"bgl{i}_id"]))
            for i in range(int(command.get("bind_group_layout_count", 0)))
        ]
        if bgl:
            out["bind_group_layout_ids"] = bgl
        if int(command.get("has_depth_attachment", 0)):
            out["depth_stencil"] = {
                "format": "depth32float",
                "depth_write_enabled": bool(int(command.get("depth_write_enabled", 0))),
                "depth_compare": DEPTH_COMPARE[int(command.get("depth_compare_op", 7))],
            }
        if command.get("builtin_pipeline"):
            out["builtin_pipeline"] = command["builtin_pipeline"]
            out["builtin_version"] = int(command.get("builtin_version", 1))
        return [out]
    if op == "BeginCommandEncoder":
        return [{"cmd": "BeginCommandEncoder", "id": ids.map(int(command["id"]))}]
    if op == "BeginRenderPass":
        texture_id = int(command.get("ca0_texture_id", command.get("texture_id", 0)))
        color_texture_id = 0 if texture_id in canvas_texture_ids else ids.map(texture_id)
        out = {
            "cmd": "BeginRenderPass",
            "id": ids.map(int(command["id"])),
            "encoder_id": ids.map(int(command["encoder_id"])),
            "color_attachments": [{
                "texture_id": color_texture_id,
                "load_op": "clear" if int(command.get("ca0_clear", command.get("clear", 1))) else "load",
                "store_op": "store",
                "clear_value": {
                    "r": float(command.get("ca0_clear_color0", command.get("clear_color0", 0))),
                    "g": float(command.get("ca0_clear_color1", command.get("clear_color1", 0))),
                    "b": float(command.get("ca0_clear_color2", command.get("clear_color2", 0))),
                    "a": float(command.get("ca0_clear_color3", command.get("clear_color3", 1))),
                },
            }],
        }
        if int(command.get("has_depth_attachment", 0)):
            out["depth_stencil_attachment"] = {
                "texture_id": "__depth__",
                "depth_load_op": "clear",
                "depth_store_op": "store",
                "depth_clear_value": float(command.get("clear_depth", 1)),
            }
        return [out]
    if op in {"SetViewport", "SetScissor"}:
        if (
            float(command.get("x", 0)) == 0
            and float(command.get("y", 0)) == 0
            and float(command.get("width", 1)) == 1
            and float(command.get("height", 1)) == 1
        ):
            return []
        raise ValueError(f"{op} is only supported for full normalized extent in this adapter")
    if op == "SetPipeline":
        return [{"cmd": "SetPipeline", "pass_id": ids.map(int(command["pass_id"])), "pipeline_id": ids.map(int(command["pipeline_id"]))}]
    if op == "SetBindGroup":
        return [{
            "cmd": "SetBindGroup",
            "pass_id": ids.map(int(command["pass_id"])),
            "slot": int(command.get("slot", 0)),
            "bind_group_id": ids.map(int(command["bind_group_id"])),
        }]
    if op == "SetVertexBuffer":
        return [{"cmd": "SetVertexBuffer", "pass_id": ids.map(int(command["pass_id"])), "slot": int(command.get("slot", 0)), "buffer_id": ids.map(int(command["buffer_id"])), "offset": int(command.get("offset", 0))}]
    if op == "SetIndexBuffer":
        return [{"cmd": "SetIndexBuffer", "pass_id": ids.map(int(command["pass_id"])), "buffer_id": ids.map(int(command["buffer_id"])), "index_format": command.get("index_format", "uint32"), "offset": int(command.get("offset", 0))}]
    if op == "Draw":
        return [{"cmd": "Draw", "pass_id": ids.map(int(command["pass_id"])), "vertex_count": int(command["vertex_count"]), "instance_count": int(command.get("instance_count", 1)), "first_vertex": int(command.get("first_vertex", 0)), "first_instance": int(command.get("first_instance", 0))}]
    if op == "DrawIndexed":
        return [{"cmd": "DrawIndexed", "pass_id": ids.map(int(command["pass_id"])), "index_count": int(command["index_count"]), "instance_count": int(command.get("instance_count", 1)), "first_index": int(command.get("first_index", 0)), "base_vertex": int(command.get("base_vertex", 0)), "first_instance": int(command.get("first_instance", 0))}]
    if op == "EndRenderPass":
        return [{"cmd": "EndRenderPass", "pass_id": ids.map(int(command["pass_id"]))}]
    if op == "FinishCommandEncoder":
        return [{"cmd": "FinishCommandEncoder", "encoder_id": ids.map(int(command["encoder_id"])), "command_buffer_id": ids.map(int(command["command_buffer_id"]))}]
    if op == "QueueSubmit":
        return [{"cmd": "QueueSubmit", "command_buffer_ids": [ids.map(int(command["command_buffer_id"]))], "submission_id": ids.map(int(command["submission_id"]))}]
    raise ValueError(f"unsupported command op {op!r}")


def adapt(recording: Path, output: Path, name: str, frame: int | None, gzip_threshold: int) -> None:
    records = _records_through_frame(_read_records(recording), frame)
    source_commands = [record for record in records if record.get("type") == "command"]
    identities = _infer_shader_identities(source_commands)
    canvas_extent = _recorded_canvas_extent(source_commands)
    ids = IdMap()
    canvas_texture_ids: set[int] = set()
    commands: list[dict[str, Any]] = []
    needs_depth = False

    for command in source_commands:
        converted = _convert_command(
            recording, command, ids, identities, canvas_texture_ids, gzip_threshold)
        for item in converted:
            if item.get("depth_stencil_attachment", {}).get("texture_id") == "__depth__":
                needs_depth = True
            commands.append(item)

    if needs_depth:
        depth_id = ids.synthetic()
        depth_command = {
            "cmd": "CreateTexture",
            "id": depth_id,
            "dimension": "2d",
            "width": "canvas",
            "height": "canvas",
            "depth": 1,
            "format": "depth32float",
            "usage": ["RENDER_ATTACHMENT"],
            "mip_level_count": 1,
            "sample_count": 1,
        }
        insert_at = next(
            (i for i, command in enumerate(commands) if command["cmd"] == "BeginCommandEncoder"),
            len(commands),
        )
        commands.insert(insert_at, depth_command)
        for command in commands:
            attachment = command.get("depth_stencil_attachment")
            if attachment is not None and attachment.get("texture_id") == "__depth__":
                attachment["texture_id"] = depth_id

    output.parent.mkdir(parents=True, exist_ok=True)
    try:
        source = str(recording.relative_to(REPO_ROOT))
    except ValueError:
        source = str(recording)
    output.write_text(
        json.dumps(
            {
                "name": name,
                "version": {"major": 2, "minor": 0},
                "source": source,
                **({"canvas": canvas_extent} if canvas_extent is not None else {}),
                "commands": commands,
                "expected": {"outcome": "success"},
            },
            indent=2,
        )
        + "\n",
        encoding="utf8",
    )


def main() -> int:
    args = _parse_args()
    recording = args.recording.resolve()
    output = args.output.resolve()
    adapt(recording, output, args.name or output.stem, args.frame, args.gzip_threshold)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
