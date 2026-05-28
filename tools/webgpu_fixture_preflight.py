#!/usr/bin/env python3
"""Preflight DRP2 fixtures and WebGPU streams against the fixture-runner contract."""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Sequence


ROOT_DIR = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST_PATH = ROOT_DIR / 'examples' / 'webgpu' / 'fixture_manifest.json'

VERTEX_FORMAT_BYTES = {
    'float32': 4,
    'float32x2': 8,
    'float32x3': 12,
    'float32x4': 16,
    'uint32': 4,
    'uint32x2': 8,
    'uint32x3': 12,
    'uint32x4': 16,
    'sint32': 4,
    'sint32x2': 8,
    'sint32x3': 12,
    'sint32x4': 16,
    'uint8': 1,
    'unorm8x4': 4,
    'snorm8x4': 4,
    'unorm16x2': 4,
    'unorm16x4': 8,
    'snorm16x2': 4,
    'snorm16x4': 8,
}

INDEX_FORMAT_BYTES = {
    'uint16': 2,
    'uint32': 4,
}

CANVAS_TEXTURE_FORMAT = 'rgba8unorm'

SUPPORTED_TEXTURE_FORMATS = {
    'bgra8unorm',
    'depth32float',
    'r16float',
    'r32uint',
    'rg32uint',
    'rgba8unorm',
    'rgba16float',
}

SUPPORTED_SHADER_FORMATS = {'wgsl'}

WGSL_BINDING_RE = re.compile(
    r'@group\(\s*(?P<group>\d+)\s*\)\s*'
    r'@binding\(\s*(?P<binding>\d+)\s*\)\s*'
    r'var(?:<(?P<address_space>[^>]*)>)?\s+'
    r'[A-Za-z_][A-Za-z0-9_]*\s*:\s*'
    r'(?P<resource_type>[^;]+);'
)


@dataclass(frozen=True)
class WGSLBindingRequirement:
    """Store one reflected WGSL resource binding requirement."""

    group: int
    binding: int
    binding_type: str
    stage: str
    access: Optional[str]


@dataclass
class WebGPUPreflightResult:
    """Store the preflight result for one fixture or stream."""

    fixture_path: str
    fixture_name: str
    passed: bool
    command_index: Optional[int]
    message: Optional[str]


class WebGPUPreflightFailure(Exception):
    """Represent a WebGPU fixture preflight failure."""

    def __init__(self, command_index: int, message: str) -> None:
        super().__init__(message)
        self.command_index = command_index
        self.message = message


class WebGPUFixturePreflight:
    """Check DRP2 fixtures and streams against strict WebGPU dashboard assumptions."""

    def __init__(self, root_dir: Path) -> None:
        self.root_dir = root_dir

    def discover(
        self, selected_paths: Sequence[str], manifest_path: Optional[Path] = None
    ) -> List[Path]:
        """Discover fixture paths from explicit selections or the committed WebGPU manifest."""

        if selected_paths:
            fixtures: List[Path] = []
            for selected in selected_paths:
                path = (self.root_dir / selected).resolve()
                if path.is_dir():
                    fixtures.extend(sorted(path.glob('*.json')))
                else:
                    fixtures.append(path)
            return sorted(path for path in fixtures if path.exists() and path.suffix == '.json')

        manifest = self._load_json(manifest_path or DEFAULT_MANIFEST_PATH)
        paths = []
        for entry in manifest.get('positive', []) + manifest.get('webgpu_streams', []):
            relative = entry[1:] if entry.startswith('/') else entry
            paths.append((self.root_dir / relative).resolve())
        return sorted(paths)

    def run_fixtures(self, fixture_paths: Iterable[Path]) -> List[WebGPUPreflightResult]:
        """Run preflight checks for every fixture path."""

        return [self.run_fixture(path) for path in fixture_paths]

    def run_fixture(self, fixture_path: Path) -> WebGPUPreflightResult:
        """Run WebGPU preflight checks for one fixture or stream."""

        fixture = self._load_json(fixture_path)
        try:
            self.validate_fixture(fixture)
        except WebGPUPreflightFailure as exc:
            return WebGPUPreflightResult(
                fixture_path=str(fixture_path.relative_to(self.root_dir)),
                fixture_name=fixture.get('name', fixture_path.stem),
                passed=False,
                command_index=exc.command_index,
                message=exc.message,
            )

        return WebGPUPreflightResult(
            fixture_path=str(fixture_path.relative_to(self.root_dir)),
            fixture_name=fixture.get('name', fixture_path.stem),
            passed=True,
            command_index=None,
            message=None,
        )

    def validate_fixture(self, fixture: Dict[str, Any]) -> None:
        """Validate a loaded fixture object against WebGPU preflight rules."""

        buffers: Dict[int, Dict[str, Any]] = {}
        bind_group_layouts: Dict[int, Dict[str, Any]] = {}
        pipelines: Dict[int, Dict[str, Any]] = {}
        shaders: Dict[int, Dict[str, Any]] = {}
        passes: Dict[int, Dict[str, Any]] = {}
        textures: Dict[int, Dict[str, Any]] = {}

        for index, command in enumerate(fixture['commands']):
            cmd = command['cmd']
            if cmd == 'CreateBuffer':
                buffers[command['id']] = command
            elif cmd == 'CreateTexture':
                self._check_texture_format(index, command.get('format'))
                textures[command['id']] = command
            elif cmd == 'CreateBindGroupLayout':
                self._check_bind_group_layout(index, command)
                bind_group_layouts[command['id']] = command
            elif cmd == 'CreateShaderModule':
                self._check_shader_format(index, command.get('format'))
                shaders[command['id']] = command
            elif cmd == 'CreateRenderPipeline':
                self._check_render_pipeline(index, command, shaders, bind_group_layouts)
                pipelines[command['id']] = command
            elif cmd == 'CreateComputePipeline':
                self._check_compute_pipeline(index, command, shaders, bind_group_layouts)
            elif cmd == 'BeginComputePass':
                passes[command['id']] = {'kind': 'compute'}
            elif cmd == 'BeginRenderPass':
                attachment_formats = self._render_pass_attachment_formats(index, command, textures)
                passes[command['id']] = {
                    'kind': 'render',
                    'id': command['id'],
                    'pipeline_id': None,
                    'vertex_buffers': {},
                    'index_buffer': None,
                    'color_attachment_formats': attachment_formats['color'],
                    'depth_stencil_format': attachment_formats['depth_stencil'],
                }
            elif cmd == 'SetPipeline':
                render_pass = self._require_pass(index, passes, command['pass_id'])
                if render_pass['kind'] != 'render':
                    continue
                if command['pipeline_id'] not in pipelines:
                    raise WebGPUPreflightFailure(
                        index, f'unknown render pipeline {command["pipeline_id"]}'
                    )
                render_pass['pipeline_id'] = command['pipeline_id']
                self._check_pipeline_pass_compatibility(
                    index, pipelines[command['pipeline_id']], render_pass
                )
            elif cmd == 'SetVertexBuffer':
                render_pass = self._require_render_pass(index, passes, command['pass_id'])
                if command['buffer_id'] not in buffers:
                    raise WebGPUPreflightFailure(index, f'unknown vertex buffer {command["buffer_id"]}')
                render_pass['vertex_buffers'][command['slot']] = {
                    'buffer_id': command['buffer_id'],
                    'offset': command['offset'],
                    'size': command.get('size'),
                }
            elif cmd == 'SetIndexBuffer':
                render_pass = self._require_render_pass(index, passes, command['pass_id'])
                if command['buffer_id'] not in buffers:
                    raise WebGPUPreflightFailure(index, f'unknown index buffer {command["buffer_id"]}')
                render_pass['index_buffer'] = {
                    'buffer_id': command['buffer_id'],
                    'offset': command['offset'],
                    'size': command.get('size'),
                    'index_format': command['index_format'],
                }
            elif cmd == 'Draw':
                self._check_draw(index, command, passes, pipelines, buffers)
            elif cmd == 'DrawIndexed':
                self._check_draw_indexed(index, command, passes, pipelines, buffers)

    def _check_bind_group_layout(self, index: int, command: Dict[str, Any]) -> None:
        for entry in command.get('entries', []):
            if 'visibility' not in entry:
                raise WebGPUPreflightFailure(
                    index,
                    f'bind-group layout {command["id"]} binding {entry["binding"]} needs visibility',
                )
            if entry['binding_type'] == 'storage_buffer' and 'access' not in entry:
                raise WebGPUPreflightFailure(
                    index,
                    f'bind-group layout {command["id"]} storage binding {entry["binding"]} needs access',
                )

    def _check_texture_format(self, index: int, fmt: Optional[str]) -> None:
        if self._texture_format(fmt) not in SUPPORTED_TEXTURE_FORMATS:
            raise WebGPUPreflightFailure(index, f'unsupported texture format {fmt}')

    def _check_shader_format(self, index: int, fmt: Optional[str]) -> None:
        if fmt not in SUPPORTED_SHADER_FORMATS:
            raise WebGPUPreflightFailure(index, f'unsupported shader format {fmt}')

    def _texture_format(self, fmt: Optional[str]) -> Optional[str]:
        if fmt == 'canvas':
            return CANVAS_TEXTURE_FORMAT
        return fmt

    def _check_render_pipeline(
        self,
        index: int,
        command: Dict[str, Any],
        shaders: Dict[int, Dict[str, Any]],
        bind_group_layouts: Dict[int, Dict[str, Any]],
    ) -> None:
        if 'vertex_buffers' not in command:
            raise WebGPUPreflightFailure(
                index, f'render pipeline {command["id"]} needs explicit vertex_buffers'
            )
        if 'color_targets' not in command:
            raise WebGPUPreflightFailure(
                index, f'render pipeline {command["id"]} needs explicit color_targets'
            )
        if command.get('vertex_buffer_slots') != len(command['vertex_buffers']):
            raise WebGPUPreflightFailure(
                index,
                f'render pipeline {command["id"]} vertex_buffer_slots does not match vertex_buffers',
            )
        for slot, vertex_buffer in enumerate(command['vertex_buffers']):
            stride = vertex_buffer.get('array_stride')
            if stride is None:
                raise WebGPUPreflightFailure(
                    index, f'render pipeline {command["id"]} vertex buffer {slot} needs array_stride'
                )
            for attribute in vertex_buffer.get('attributes', []):
                fmt = attribute.get('format')
                if fmt not in VERTEX_FORMAT_BYTES:
                    raise WebGPUPreflightFailure(index, f'unsupported vertex format {fmt}')
                if attribute.get('offset', 0) + VERTEX_FORMAT_BYTES[fmt] > stride:
                    raise WebGPUPreflightFailure(
                        index,
                        f'render pipeline {command["id"]} vertex attribute exceeds stride',
                    )
        for color_target in command['color_targets']:
            self._check_texture_format(index, color_target.get('format'))
        depth_stencil = command.get('depth_stencil')
        if depth_stencil is not None:
            self._check_texture_format(index, depth_stencil.get('format'))
        command['_webgpu_color_target_formats'] = [
            self._texture_format(target.get('format')) for target in command['color_targets']
        ]
        command['_webgpu_depth_stencil_format'] = (
            None if depth_stencil is None else self._texture_format(depth_stencil.get('format'))
        )
        for color_target in command['color_targets']:
            self._check_color_target_state(index, command['id'], color_target)
        self._check_pipeline_shader_bindings(
            index,
            f'render pipeline {command["id"]}',
            command,
            [
                ('VERTEX', command.get('vertex_shader_module_id')),
                ('FRAGMENT', command.get('fragment_shader_module_id')),
            ],
            shaders,
            bind_group_layouts,
        )

    def _check_color_target_state(
        self, index: int, pipeline_id: int, color_target: Dict[str, Any]
    ) -> None:
        fmt = self._texture_format(color_target.get('format'))
        if color_target.get('blend') is not None and fmt in {'depth32float', 'r32uint', 'rg32uint'}:
            raise WebGPUPreflightFailure(
                index, f'render pipeline {pipeline_id} color target format {fmt} does not support blending'
            )
        write_mask = color_target.get('write_mask')
        if write_mask is not None and 'all' in write_mask and len(write_mask) > 1:
            raise WebGPUPreflightFailure(
                index,
                f'render pipeline {pipeline_id} color target write_mask cannot combine all '
                'with individual channels',
            )

    def _check_compute_pipeline(
        self,
        index: int,
        command: Dict[str, Any],
        shaders: Dict[int, Dict[str, Any]],
        bind_group_layouts: Dict[int, Dict[str, Any]],
    ) -> None:
        self._check_pipeline_shader_bindings(
            index,
            f'compute pipeline {command["id"]}',
            command,
            [('COMPUTE', command.get('compute_shader_module_id'))],
            shaders,
            bind_group_layouts,
        )

    def _check_pipeline_shader_bindings(
        self,
        index: int,
        pipeline_label: str,
        command: Dict[str, Any],
        shader_refs: Sequence[tuple[str, Optional[int]]],
        shaders: Dict[int, Dict[str, Any]],
        bind_group_layouts: Dict[int, Dict[str, Any]],
    ) -> None:
        layout_ids = command.get('bind_group_layout_ids', [])
        for stage, shader_id in shader_refs:
            if shader_id is None:
                continue
            shader = shaders.get(shader_id)
            if shader is None or shader.get('format') != 'wgsl':
                continue
            for requirement in self._wgsl_binding_requirements(shader, stage):
                if requirement.group >= len(layout_ids):
                    raise WebGPUPreflightFailure(
                        index,
                        f'{pipeline_label} shader {shader_id} requires group '
                        f'{requirement.group} binding {requirement.binding}',
                    )
                layout_id = layout_ids[requirement.group]
                layout = bind_group_layouts.get(layout_id)
                if layout is None:
                    raise WebGPUPreflightFailure(
                        index,
                        f'{pipeline_label} references unknown bind-group layout {layout_id}',
                    )
                self._check_shader_binding_matches_layout(
                    index, pipeline_label, shader_id, requirement, layout
                )

    def _check_shader_binding_matches_layout(
        self,
        index: int,
        pipeline_label: str,
        shader_id: int,
        requirement: WGSLBindingRequirement,
        layout: Dict[str, Any],
    ) -> None:
        layout_entry = None
        for entry in layout.get('entries', []):
            if entry['binding'] == requirement.binding:
                layout_entry = entry
                break
        if layout_entry is None:
            raise WebGPUPreflightFailure(
                index,
                f'{pipeline_label} shader {shader_id} requires group {requirement.group} '
                f'binding {requirement.binding}, missing from layout {layout["id"]}',
            )
        if layout_entry['binding_type'] != requirement.binding_type:
            raise WebGPUPreflightFailure(
                index,
                f'{pipeline_label} shader {shader_id} group {requirement.group} binding '
                f'{requirement.binding} uses {requirement.binding_type}, layout {layout["id"]} '
                f'uses {layout_entry["binding_type"]}',
            )
        if requirement.stage not in layout_entry.get('visibility', []):
            raise WebGPUPreflightFailure(
                index,
                f'{pipeline_label} shader {shader_id} group {requirement.group} binding '
                f'{requirement.binding} needs {requirement.stage} visibility',
            )
        if requirement.access is not None and layout_entry.get('access') != requirement.access:
            raise WebGPUPreflightFailure(
                index,
                f'{pipeline_label} shader {shader_id} group {requirement.group} binding '
                f'{requirement.binding} uses {requirement.access} storage access, layout '
                f'{layout["id"]} uses {layout_entry.get("access")}',
            )

    def _wgsl_binding_requirements(
        self, shader: Dict[str, Any], stage: str
    ) -> List[WGSLBindingRequirement]:
        code = shader.get('code')
        if not isinstance(code, str):
            return []

        requirements: List[WGSLBindingRequirement] = []
        for match in WGSL_BINDING_RE.finditer(code):
            binding_type, access = self._wgsl_binding_type(
                match.group('address_space'), match.group('resource_type')
            )
            if binding_type is None:
                continue
            requirements.append(
                WGSLBindingRequirement(
                    group=int(match.group('group')),
                    binding=int(match.group('binding')),
                    binding_type=binding_type,
                    stage=stage,
                    access=access,
                )
            )
        return requirements

    def _wgsl_binding_type(
        self, address_space: Optional[str], resource_type: str
    ) -> tuple[Optional[str], Optional[str]]:
        resource_type = resource_type.strip()
        address_parts = []
        if address_space is not None:
            address_parts = [part.strip() for part in address_space.split(',')]
        if address_parts and address_parts[0] == 'uniform':
            return 'uniform_buffer', None
        if address_parts and address_parts[0] == 'storage':
            access = 'read_write'
            if len(address_parts) > 1 and address_parts[1] == 'read':
                access = 'read'
            return 'storage_buffer', access
        if resource_type.startswith('texture_storage_'):
            return 'storage_texture', None
        if resource_type.startswith('texture_') or resource_type.startswith('texture_depth'):
            return 'sampled_texture', None
        if resource_type.startswith('sampler'):
            return 'sampler', None
        return None, None

    def _check_draw(
        self,
        index: int,
        command: Dict[str, Any],
        passes: Dict[int, Dict[str, Any]],
        pipelines: Dict[int, Dict[str, Any]],
        buffers: Dict[int, Dict[str, Any]],
    ) -> None:
        render_pass = self._require_render_pass(index, passes, command['pass_id'])
        pipeline = self._require_pipeline(index, pipelines, render_pass)
        self._check_vertex_ranges(
            index,
            pipeline,
            render_pass,
            buffers,
            command['vertex_count'],
            command.get('instance_count', 1),
            command.get('first_vertex', 0),
            command.get('first_instance', 0),
        )

    def _check_draw_indexed(
        self,
        index: int,
        command: Dict[str, Any],
        passes: Dict[int, Dict[str, Any]],
        pipelines: Dict[int, Dict[str, Any]],
        buffers: Dict[int, Dict[str, Any]],
    ) -> None:
        render_pass = self._require_render_pass(index, passes, command['pass_id'])
        pipeline = self._require_pipeline(index, pipelines, render_pass)
        self._check_vertex_ranges(
            index,
            pipeline,
            render_pass,
            buffers,
            1,
            command.get('instance_count', 1),
            0,
            command.get('first_instance', 0),
        )
        self._check_index_range(index, command, render_pass, buffers)

    def _check_vertex_ranges(
        self,
        index: int,
        pipeline: Dict[str, Any],
        render_pass: Dict[str, Any],
        buffers: Dict[int, Dict[str, Any]],
        vertex_count: int,
        instance_count: int,
        first_vertex: int,
        first_instance: int,
    ) -> None:
        for slot, layout in enumerate(pipeline['vertex_buffers']):
            if slot not in render_pass['vertex_buffers']:
                raise WebGPUPreflightFailure(
                    index, f'render pass is missing vertex buffer slot {slot}'
                )
            binding = render_pass['vertex_buffers'][slot]
            buffer = buffers[binding['buffer_id']]
            step_mode = layout.get('step_mode', 'vertex')
            if step_mode == 'instance':
                first = first_instance
                count = instance_count
            else:
                first = first_vertex
                count = vertex_count
            required = self._required_vertex_bytes(layout, first, count)
            available = self._available_binding_bytes(buffer, binding)
            if required > available:
                raise WebGPUPreflightFailure(
                    index,
                    f'vertex buffer slot {slot} needs {required} bytes but binding has {available}',
                )

    def _check_index_range(
        self,
        index: int,
        command: Dict[str, Any],
        render_pass: Dict[str, Any],
        buffers: Dict[int, Dict[str, Any]],
    ) -> None:
        binding = render_pass.get('index_buffer')
        if binding is None:
            raise WebGPUPreflightFailure(index, 'render pass is missing index buffer')
        index_size = INDEX_FORMAT_BYTES[binding['index_format']]
        first = command.get('first_index', 0)
        count = command['index_count']
        required = (first + count) * index_size
        available = self._available_binding_bytes(buffers[binding['buffer_id']], binding)
        if required > available:
            raise WebGPUPreflightFailure(
                index, f'index buffer needs {required} bytes but binding has {available}'
            )

    def _render_pass_attachment_formats(
        self, index: int, command: Dict[str, Any], textures: Dict[int, Dict[str, Any]]
    ) -> Dict[str, Any]:
        color_formats = []
        for attachment in command.get('color_attachments', []):
            self._check_load_store_ops(index, attachment)
            color_formats.append(self._attachment_format(index, attachment, textures))

        depth_stencil_format = None
        depth_stencil_attachment = command.get('depth_stencil_attachment')
        if depth_stencil_attachment is not None:
            self._check_load_store_ops(index, depth_stencil_attachment, depth_stencil=True)
            depth_stencil_format = self._attachment_format(index, depth_stencil_attachment, textures)

        return {'color': color_formats, 'depth_stencil': depth_stencil_format}

    def _attachment_format(
        self, index: int, attachment: Dict[str, Any], textures: Dict[int, Dict[str, Any]]
    ) -> str:
        texture_id = attachment.get('texture_id')
        if texture_id == 0:
            return CANVAS_TEXTURE_FORMAT
        texture = textures.get(texture_id)
        if texture is None:
            raise WebGPUPreflightFailure(index, f'unknown attachment texture {texture_id}')
        fmt = self._texture_format(texture.get('format'))
        if fmt not in SUPPORTED_TEXTURE_FORMATS:
            raise WebGPUPreflightFailure(index, f'unsupported attachment texture format {fmt}')
        return fmt

    def _check_load_store_ops(
        self, index: int, attachment: Dict[str, Any], depth_stencil: bool = False
    ) -> None:
        load_keys = ['depth_load_op', 'stencil_load_op'] if depth_stencil else ['load_op']
        store_keys = ['depth_store_op', 'stencil_store_op'] if depth_stencil else ['store_op']
        for key in load_keys:
            if key in attachment and attachment[key] not in {'clear', 'load'}:
                raise WebGPUPreflightFailure(index, f'unsupported {key}: {attachment[key]}')
        for key in store_keys:
            if key in attachment and attachment[key] not in {'store', 'discard'}:
                raise WebGPUPreflightFailure(index, f'unsupported {key}: {attachment[key]}')

    def _check_pipeline_pass_compatibility(
        self, index: int, pipeline: Dict[str, Any], render_pass: Dict[str, Any]
    ) -> None:
        pipeline_color_formats = pipeline['_webgpu_color_target_formats']
        pass_color_formats = render_pass['color_attachment_formats']
        if len(pipeline_color_formats) != len(pass_color_formats):
            raise WebGPUPreflightFailure(
                index,
                f'render pipeline {pipeline["id"]} color target count '
                f'{len(pipeline_color_formats)} does not match render pass {render_pass["id"]} '
                f'color attachment count {len(pass_color_formats)}',
            )
        for target_index, (pipeline_format, pass_format) in enumerate(
            zip(pipeline_color_formats, pass_color_formats)
        ):
            if pipeline_format != pass_format:
                raise WebGPUPreflightFailure(
                    index,
                    f'render pipeline {pipeline["id"]} color target {target_index} format '
                    f'{pipeline_format} does not match render pass {render_pass["id"]} '
                    f'attachment format {pass_format}',
                )

        pipeline_depth_format = pipeline['_webgpu_depth_stencil_format']
        pass_depth_format = render_pass['depth_stencil_format']
        if pipeline_depth_format != pass_depth_format:
            raise WebGPUPreflightFailure(
                index,
                f'render pipeline {pipeline["id"]} depth_stencil format '
                f'{pipeline_depth_format or "none"} does not match render pass {render_pass["id"]} '
                f'attachment format {pass_depth_format or "none"}',
            )

    def _required_vertex_bytes(self, layout: Dict[str, Any], first: int, count: int) -> int:
        if count == 0:
            return 0
        stride = layout['array_stride']
        attribute_end = 0
        for attribute in layout.get('attributes', []):
            attribute_end = max(
                attribute_end,
                attribute.get('offset', 0) + VERTEX_FORMAT_BYTES[attribute['format']],
            )
        return first * stride + (count - 1) * stride + attribute_end

    def _available_binding_bytes(
        self, buffer: Dict[str, Any], binding: Dict[str, Optional[int]]
    ) -> int:
        size = binding.get('size')
        if size is not None:
            return size
        return max(0, buffer['size'] - binding['offset'])

    def _require_pass(
        self, index: int, passes: Dict[int, Dict[str, Any]], pass_id: int
    ) -> Dict[str, Any]:
        if pass_id not in passes:
            raise WebGPUPreflightFailure(index, f'unknown pass {pass_id}')
        return passes[pass_id]

    def _require_render_pass(
        self, index: int, passes: Dict[int, Dict[str, Any]], pass_id: int
    ) -> Dict[str, Any]:
        render_pass = self._require_pass(index, passes, pass_id)
        if render_pass['kind'] != 'render':
            raise WebGPUPreflightFailure(index, f'pass {pass_id} is not a render pass')
        return render_pass

    def _require_pipeline(
        self, index: int, pipelines: Dict[int, Dict[str, Any]], render_pass: Dict[str, Any]
    ) -> Dict[str, Any]:
        pipeline_id = render_pass.get('pipeline_id')
        if pipeline_id is None:
            raise WebGPUPreflightFailure(index, 'render pass has no bound pipeline')
        return pipelines[pipeline_id]

    def _load_json(self, path: Path) -> Dict[str, Any]:
        with path.open('r', encoding='utf-8') as stream:
            return json.load(stream)


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description='Preflight DRP2 fixtures for WebGPU execution.')
    parser.add_argument('paths', nargs='*', help='Optional fixture files or directories to check.')
    parser.add_argument(
        '--manifest',
        type=Path,
        default=DEFAULT_MANIFEST_PATH,
        help='Fixture manifest to use when no explicit path is provided.',
    )
    parser.add_argument('--json', action='store_true', help='Emit structured JSON results.')
    return parser


def _print_text(results: Sequence[WebGPUPreflightResult]) -> None:
    for result in results:
        status = 'PASS' if result.passed else 'FAIL'
        print(f'{status} {result.fixture_path} ({result.fixture_name})')
        if not result.passed:
            print(f'  command_index: {result.command_index}')
            print(f'  message: {result.message}')
    total = len(results)
    passed = sum(1 for result in results if result.passed)
    failed = total - passed
    print(f'SUMMARY total={total} passed={passed} failed={failed}')


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = _build_arg_parser()
    args = parser.parse_args(argv)

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    fixture_paths = preflight.discover(args.paths, args.manifest)
    results = preflight.run_fixtures(fixture_paths)

    if args.json:
        print(json.dumps([asdict(result) for result in results], indent=2))
    else:
        _print_text(results)

    return 0 if all(result.passed for result in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
