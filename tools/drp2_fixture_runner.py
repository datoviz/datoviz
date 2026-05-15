#!/usr/bin/env python3
"""Run the DRP2 fixture corpus against the current spec contract."""

from __future__ import annotations

import argparse
import base64
import json
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Sequence
from urllib.parse import unquote, urlparse

import jsonschema

try:
    from referencing import Registry, Resource
except ImportError:  # pragma: no cover - exercised only outside prepared environments.
    Registry = None
    Resource = None


ROOT_DIR = Path(__file__).resolve().parents[1]
FIXTURES_DIR = ROOT_DIR / 'spec' / 'drp2' / 'fixtures'
FIXTURE_SCHEMA_PATH = FIXTURES_DIR / 'schema' / 'drp_fixture.schema.json'
COMMAND_SCHEMA_PATH = ROOT_DIR / 'spec' / 'drp2' / 'schema' / 'drp_command.json'
FIXTURE_DIRS = ('positive', 'negative', 'negative_schema')
SUPPORTED_DRP2_MAJOR_VERSION = 2
BYTES_PER_TEXEL = {
    'rgba8unorm': 4,
    'bgra8unorm': 4,
    'rgba8unorm-srgb': 4,
    'r16float': 2,
    'rgba16float': 8,
    'rgba32float': 16,
    'depth24plus': 4,
    'depth24plus-stencil8': 4,
    'depth32float': 4,
    'r32uint': 4,
    'rg32uint': 8,
    'r32sint': 4,
    'r32float': 4,
}


@dataclass
class FixtureResult:
    """Store the normalized result for one fixture."""

    fixture_path: str
    fixture_name: str
    actual_outcome: str
    actual_phase: Optional[str]
    actual_code: Optional[str]
    actual_command_index: Optional[int]
    passed: bool
    message: Optional[str]
    expected_outcome: Optional[str] = None
    expected_phase: Optional[str] = None
    expected_code: Optional[str] = None
    expected_command_index: Optional[int] = None


@dataclass
class ObjectState:
    """Track protocol-visible object state."""

    kind: str
    live: bool
    data: Dict[str, Any]


class SemanticFailure(Exception):
    """Represent a semantic validation failure."""

    def __init__(self, code: str, command_index: int, message: str) -> None:
        super().__init__(message)
        self.code = code
        self.command_index = command_index
        self.message = message


class CapabilityFailure(Exception):
    """Represent a capability validation failure."""

    def __init__(self, code: str, command_index: int, message: str) -> None:
        super().__init__(message)
        self.code = code
        self.command_index = command_index
        self.message = message


class DRP2CapabilityValidator:
    """Validate fixture-level capability gates for the active DRP2 corpus."""

    def __init__(self, capabilities: Dict[str, Any]) -> None:
        self.capabilities = capabilities

    def validate(self, commands: Sequence[Dict[str, Any]]) -> None:
        for index, command in enumerate(commands):
            handler = getattr(self, f'_handle_{command["cmd"]}', None)
            if handler is not None:
                handler(index, command)

    def _handle_CreateBuffer(self, index: int, command: Dict[str, Any]) -> None:
        max_buffer_size = self.capabilities.get('max_buffer_size')
        if max_buffer_size is not None and command['size'] > max_buffer_size:
            raise CapabilityFailure(
                'DRP2_ERR_UNSUPPORTED_CAPABILITY',
                index,
                f'buffer size {command["size"]} exceeds capability max_buffer_size {max_buffer_size}',
            )

    def _handle_CreateTexture(self, index: int, command: Dict[str, Any]) -> None:
        supported_formats = self.capabilities.get('supported_texture_formats')
        if supported_formats is not None and command['format'] not in supported_formats:
            raise CapabilityFailure(
                'DRP2_ERR_UNSUPPORTED_CAPABILITY',
                index,
                f'texture format {command["format"]} is not supported by the fixture capability set',
            )

        supported_sample_counts = self.capabilities.get('supported_sample_counts')
        if supported_sample_counts is not None and command['sample_count'] not in supported_sample_counts:
            raise CapabilityFailure(
                'DRP2_ERR_UNSUPPORTED_CAPABILITY',
                index,
                f'sample count {command["sample_count"]} is not supported by the fixture capability set',
            )

        dimension = command['dimension']
        if dimension == '1d':
            max_dimension = self.capabilities.get('max_texture_dimension_1d')
            extent = command['width']
        elif dimension == '2d':
            max_dimension = self.capabilities.get('max_texture_dimension_2d')
            extent = max(command['width'], command['height'])
        else:
            max_dimension = self.capabilities.get('max_texture_dimension_3d')
            extent = max(command['width'], command['height'], command['depth'])
        if max_dimension is not None and extent > max_dimension:
            raise CapabilityFailure(
                'DRP2_ERR_UNSUPPORTED_CAPABILITY',
                index,
                f'texture extent {extent} exceeds capability limit {max_dimension} for dimension {dimension}',
            )

    def _handle_CreateShaderModule(self, index: int, command: Dict[str, Any]) -> None:
        supported_formats = self.capabilities.get('supported_shader_formats')
        if supported_formats is not None and command['format'] not in supported_formats:
            raise CapabilityFailure(
                'DRP2_ERR_UNSUPPORTED_CAPABILITY',
                index,
                f'shader format {command["format"]} is not supported by the fixture capability set',
            )
        if 'fp64' in command.get('required_features', []) and not self.capabilities.get('supports_fp64', False):
            raise CapabilityFailure(
                'DRP2_ERR_FEATURE_REQUIRED',
                index,
                'shader module requires fp64 but the fixture capability set does not support it',
            )

class DRP2SemanticValidator:
    """Validate the current active DRP2 fixture corpus semantically."""

    def __init__(self) -> None:
        self.objects: Dict[int, ObjectState] = {}
        self.encoders: Dict[int, Dict[str, Any]] = {}
        self.passes: Dict[int, Dict[str, Any]] = {}
        self.command_buffers: Dict[int, Dict[str, Any]] = {}
        self.pending_readbacks: Dict[int, Dict[str, Any]] = {}
        self.handshake_started = False
        self.handshake_pending = False
        self.handshake_failed = False

    def validate(self, commands: Sequence[Dict[str, Any]]) -> None:
        """Validate a full command stream."""

        for index, command in enumerate(commands):
            self._validate_command(index, command)

    def _validate_command(self, index: int, command: Dict[str, Any]) -> None:
        cmd = command['cmd']
        if cmd not in ('HelloRenderer', 'RendererHelloReply', 'Error'):
            if not self.handshake_started or self.handshake_pending or self.handshake_failed:
                raise SemanticFailure(
                    'DRP2_ERR_INVALID_STATE',
                    index,
                    f'command {cmd} is invalid before handshake completion',
                )
        handler = getattr(self, f'_handle_{cmd}', None)
        if handler is None:
            return
        handler(index, command)

    def _validate_version(self, index: int, version: Dict[str, Any]) -> None:
        if version['major'] != SUPPORTED_DRP2_MAJOR_VERSION:
            raise SemanticFailure(
                'DRP2_ERR_UNSUPPORTED_VERSION',
                index,
                f'unsupported DRP2 major version {version["major"]}',
            )

    def _handle_HelloRenderer(self, index: int, command: Dict[str, Any]) -> None:
        if index != 0 or self.handshake_started or self.handshake_failed:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE',
                index,
                'HelloRenderer must be the first command of a fresh stream',
            )
        self._validate_version(index, command['version'])
        self.handshake_started = True
        self.handshake_pending = True

    def _handle_RendererHelloReply(self, index: int, command: Dict[str, Any]) -> None:
        if not self.handshake_started or not self.handshake_pending:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE',
                index,
                'RendererHelloReply requires a pending HelloRenderer handshake',
            )
        self._validate_version(index, command['version'])
        self.handshake_pending = False
        if command['status'] == 'ok':
            return
        self.handshake_failed = True

    def _handle_Error(self, index: int, command: Dict[str, Any]) -> None:
        if not self.handshake_started:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE',
                index,
                'Error requires an established stream after HelloRenderer',
            )

    def _reserve_id(self, index: int, obj_id: int, kind: str, data: Dict[str, Any]) -> None:
        state = self.objects.get(obj_id)
        if state is not None:
            raise SemanticFailure(
                'DRP2_ERR_DUPLICATE_ID', index, f'id {obj_id} is already reserved as {state.kind}'
            )
        self.objects[obj_id] = ObjectState(kind=kind, live=True, data=data)

    def _resolve_live(self, index: int, obj_id: int, kind: str) -> ObjectState:
        state = self.objects.get(obj_id)
        if state is None:
            raise SemanticFailure('DRP2_ERR_INVALID_ID', index, f'unknown id {obj_id}')
        if state.kind != kind:
            raise SemanticFailure(
                'DRP2_ERR_WRONG_OBJECT_TYPE',
                index,
                f'id {obj_id} is {state.kind}, expected {kind}',
            )
        if not state.live:
            raise SemanticFailure('DRP2_ERR_DESTROYED_OBJECT', index, f'{kind} {obj_id} is destroyed')
        return state

    def _resolve_encoder(self, index: int, encoder_id: int) -> Dict[str, Any]:
        self._resolve_live(index, encoder_id, 'encoder')
        encoder = self.encoders[encoder_id]
        if encoder['state'] != 'open':
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE', index, f'encoder {encoder_id} is not open'
            )
        return encoder

    def _resolve_pass(self, index: int, pass_id: int, expected_kind: Optional[str] = None) -> Dict[str, Any]:
        state = self._resolve_live(index, pass_id, 'pass')
        pass_info = self.passes[pass_id]
        if pass_info['state'] != 'open':
            raise SemanticFailure('DRP2_ERR_INVALID_STATE', index, f'pass {pass_id} is not open')
        if expected_kind is not None and pass_info['kind'] != expected_kind:
            raise SemanticFailure(
                'DRP2_ERR_PASS_MISMATCH',
                index,
                f'pass {pass_id} is {pass_info["kind"]}, expected {expected_kind}',
            )
        return pass_info

    def _require_active_pass_for_command(
        self, index: int, pass_id: int, expected_kind: str
    ) -> Dict[str, Any]:
        state = self.objects.get(pass_id)
        if state is None:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE',
                index,
                f'no open {expected_kind} pass is available for command target {pass_id}',
            )
        if state.kind != 'pass':
            raise SemanticFailure(
                'DRP2_ERR_WRONG_OBJECT_TYPE',
                index,
                f'id {pass_id} is {state.kind}, expected pass',
            )
        pass_info = self.passes[pass_id]
        if pass_info['kind'] != expected_kind:
            raise SemanticFailure(
                'DRP2_ERR_PASS_MISMATCH',
                index,
                f'pass {pass_id} is {pass_info["kind"]}, expected {expected_kind}',
            )
        if not state.live or pass_info['state'] != 'open':
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE', index, f'pass {pass_id} is not an open {expected_kind} pass'
            )
        return pass_info

    def _require_bound_pipeline(self, index: int, pass_info: Dict[str, Any]) -> None:
        if pass_info.get('bound_pipeline_id') is None:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE',
                index,
                f'{pass_info["kind"]} pass {pass_info["id"]} has no bound pipeline',
            )

    def _buffer_usage(self, index: int, buffer_id: int, required: str) -> ObjectState:
        state = self._resolve_live(index, buffer_id, 'buffer')
        usage = state.data['usage']
        if required not in usage:
            raise SemanticFailure(
                'DRP2_ERR_USAGE',
                index,
                f'buffer {buffer_id} does not allow {required}',
            )
        return state

    def _texture_usage(self, index: int, texture_id: int, required: str) -> ObjectState:
        state = self._resolve_live(index, texture_id, 'texture')
        usage = state.data['usage']
        if required not in usage:
            raise SemanticFailure(
                'DRP2_ERR_USAGE',
                index,
                f'texture {texture_id} does not allow {required}',
            )
        return state

    def _check_buffer_range(self, index: int, state: ObjectState, offset: int, size: int) -> None:
        if offset + size > state.data['size']:
            raise SemanticFailure(
                'DRP2_ERR_OUT_OF_RANGE',
                index,
                f'buffer range {offset}+{size} exceeds {state.data["size"]}',
            )

    def _check_texture_box(
        self,
        index: int,
        state: ObjectState,
        mip_level: int,
        origin: Dict[str, int],
        size: Dict[str, int],
    ) -> None:
        mip_level_count = state.data['mip_level_count']
        if mip_level >= mip_level_count:
            raise SemanticFailure(
                'DRP2_ERR_OUT_OF_RANGE',
                index,
                f'texture mip level {mip_level} exceeds mip_level_count {mip_level_count}',
            )
        width = max(1, state.data['width'] >> mip_level)
        height = max(1, state.data['height'] >> mip_level)
        depth = max(1, state.data['depth'] >> mip_level) if state.data['dimension'] == '3d' else state.data['depth']
        if origin['x'] + size['width'] > width:
            raise SemanticFailure('DRP2_ERR_OUT_OF_RANGE', index, 'texture write exceeds width')
        if origin['y'] + size['height'] > height:
            raise SemanticFailure('DRP2_ERR_OUT_OF_RANGE', index, 'texture write exceeds height')
        if origin['z'] + size['depth'] > depth:
            raise SemanticFailure('DRP2_ERR_OUT_OF_RANGE', index, 'texture write exceeds depth')

    def _required_transfer_bytes(
        self,
        index: int,
        texture_state: ObjectState,
        width: int,
        height: int,
        depth: int,
        bytes_per_row: int,
        rows_per_image: int,
    ) -> int:
        bytes_per_texel = BYTES_PER_TEXEL[texture_state.data['format']]
        min_bytes_per_row = width * bytes_per_texel
        if bytes_per_row < min_bytes_per_row:
            raise SemanticFailure(
                'DRP2_ERR_LAYOUT',
                index,
                f'bytes_per_row {bytes_per_row} is smaller than the required row footprint {min_bytes_per_row}',
            )
        if rows_per_image == 0:
            raise SemanticFailure(
                'DRP2_ERR_LAYOUT',
                index,
                'rows_per_image must be greater than zero for texture transfers',
            )
        if depth > 1 and rows_per_image < height:
            raise SemanticFailure(
                'DRP2_ERR_LAYOUT',
                index,
                f'rows_per_image {rows_per_image} is smaller than transfer height {height}',
            )
        return bytes_per_row * (height - 1) + min_bytes_per_row + bytes_per_row * rows_per_image * (depth - 1)

    def _resource_in_use(self, kind: str, obj_id: int) -> bool:
        token = (kind, obj_id)
        for encoder in self.encoders.values():
            if encoder['state'] == 'open' and token in encoder['resources']:
                return True
        for command_buffer in self.command_buffers.values():
            if token in command_buffer['resources']:
                return True
        return False

    def _handle_CreateBuffer(self, index: int, command: Dict[str, Any]) -> None:
        self._reserve_id(
            index,
            command['id'],
            'buffer',
            {'size': command['size'], 'usage': set(command['usage'])},
        )

    def _handle_DestroyBuffer(self, index: int, command: Dict[str, Any]) -> None:
        buffer_id = command['buffer_id']
        state = self._resolve_live(index, buffer_id, 'buffer')
        if self._resource_in_use('buffer', buffer_id):
            raise SemanticFailure(
                'DRP2_ERR_USAGE', index, f'buffer {buffer_id} is still referenced by recorded work'
            )
        state.live = False

    def _handle_WriteBuffer(self, index: int, command: Dict[str, Any]) -> None:
        state = self._resolve_live(index, command['buffer_id'], 'buffer')
        self._check_buffer_range(index, state, command['offset'], command['size'])

    def _handle_CreateTexture(self, index: int, command: Dict[str, Any]) -> None:
        self._reserve_id(
            index,
            command['id'],
            'texture',
                {
                    'width': command['width'],
                    'height': command['height'],
                    'depth': command['depth'],
                    'dimension': command['dimension'],
                    'format': command['format'],
                    'mip_level_count': command['mip_level_count'],
                    'usage': set(command['usage']),
                },
            )

    def _handle_CreateBindGroup(self, index: int, command: Dict[str, Any]) -> None:
        layout_state = self._resolve_live(index, command['bind_group_layout_id'], 'bind_group_layout')
        layout_entries_by_binding = {entry['binding']: entry for entry in layout_state.data['entries']}
        expected_entries = {
            binding: entry['binding_type'] for binding, entry in layout_entries_by_binding.items()
        }
        actual_entries = {entry['binding']: entry['binding_type'] for entry in command['entries']}
        if actual_entries != expected_entries:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_ARGUMENT',
                index,
                f'bind group {command["id"]} entries do not match layout {command["bind_group_layout_id"]}',
            )
        resources = set()
        for entry in command['entries']:
            binding_type = entry['binding_type']
            layout_entry = layout_entries_by_binding[entry['binding']]
            if binding_type in ('uniform_buffer', 'storage_buffer'):
                if layout_entry.get('has_dynamic_offset', False) and (
                    'offset' not in entry or 'size' not in entry
                ):
                    raise SemanticFailure(
                        'DRP2_ERR_INVALID_ARGUMENT',
                        index,
                        f'dynamic bind-group entry {entry["binding"]} requires explicit offset and size',
                    )
                if entry['resource_kind'] != 'buffer':
                    raise SemanticFailure(
                        'DRP2_ERR_WRONG_OBJECT_TYPE',
                        index,
                        f'bind-group entry {entry["binding"]} uses {binding_type} with {entry["resource_kind"]}',
                    )
                buffer_state = self._resolve_live(index, entry['resource_id'], 'buffer')
                required_usage = 'UNIFORM' if binding_type == 'uniform_buffer' else 'STORAGE'
                if required_usage not in buffer_state.data['usage']:
                    raise SemanticFailure(
                        'DRP2_ERR_USAGE',
                        index,
                        f'buffer {entry["resource_id"]} does not allow {required_usage}',
                    )
                if 'offset' in entry and 'size' in entry:
                    self._check_buffer_range(index, buffer_state, entry['offset'], entry['size'])
                resources.add(('buffer', entry['resource_id']))
            elif binding_type in ('sampled_texture', 'storage_texture'):
                if entry['resource_kind'] not in ('texture', 'texture_view'):
                    raise SemanticFailure(
                        'DRP2_ERR_WRONG_OBJECT_TYPE',
                        index,
                        f'bind-group entry {entry["binding"]} uses {binding_type} with {entry["resource_kind"]}',
                    )
                if entry['resource_kind'] == 'texture_view':
                    view_state = self._resolve_live(index, entry['resource_id'], 'texture_view')
                    texture_id = view_state.data['texture_id']
                    texture_state = self._resolve_live(index, texture_id, 'texture')
                    resources.add(('texture_view', entry['resource_id']))
                    resources.add(('texture', texture_id))
                else:
                    texture_state = self._resolve_live(index, entry['resource_id'], 'texture')
                    resources.add(('texture', entry['resource_id']))
                required_usage = (
                    'TEXTURE_BINDING' if binding_type == 'sampled_texture' else 'STORAGE_BINDING'
                )
                if required_usage not in texture_state.data['usage']:
                    raise SemanticFailure(
                        'DRP2_ERR_USAGE',
                        index,
                        f'texture {texture_state.data.get("id", entry["resource_id"])} does not allow {required_usage}',
                    )
            elif binding_type == 'sampler':
                if entry['resource_kind'] != 'sampler':
                    raise SemanticFailure(
                        'DRP2_ERR_WRONG_OBJECT_TYPE',
                        index,
                        f'bind-group entry {entry["binding"]} uses {binding_type} with {entry["resource_kind"]}',
                    )
                self._resolve_live(index, entry['resource_id'], 'sampler')
                resources.add(('sampler', entry['resource_id']))
        self._reserve_id(
            index,
            command['id'],
            'bind_group',
            {
                'bind_group_layout_id': command['bind_group_layout_id'],
                'entries': command['entries'],
                'resources': resources,
            },
        )

    def _handle_CreateBindGroupLayout(self, index: int, command: Dict[str, Any]) -> None:
        bindings = [entry['binding'] for entry in command['entries']]
        if len(bindings) != len(set(bindings)):
            raise SemanticFailure(
                'DRP2_ERR_INVALID_ARGUMENT',
                index,
                f'bind-group layout {command["id"]} has duplicate binding indices',
            )
        for entry in command['entries']:
            if entry.get('has_dynamic_offset', False) and entry['binding_type'] not in (
                'uniform_buffer',
                'storage_buffer',
            ):
                raise SemanticFailure(
                    'DRP2_ERR_INVALID_ARGUMENT',
                    index,
                    f'bind-group layout {command["id"]} marks non-buffer binding {entry["binding"]} as dynamic',
                )
        self._reserve_id(
            index,
            command['id'],
            'bind_group_layout',
            {
                'entries': command['entries'],
            },
        )

    def _handle_CreateShaderModule(self, index: int, command: Dict[str, Any]) -> None:
        self._reserve_id(
            index,
            command['id'],
            'shader_module',
            {
                'stage': command['stage'],
                'format': command['format'],
                'entry_point': command['entry_point'],
                'required_features': list(command.get('required_features', [])),
            },
        )

    def _handle_DestroyShaderModule(self, index: int, command: Dict[str, Any]) -> None:
        shader_module_id = command['shader_module_id']
        state = self._resolve_live(index, shader_module_id, 'shader_module')
        if self._resource_in_use('shader_module', shader_module_id):
            raise SemanticFailure(
                'DRP2_ERR_USAGE',
                index,
                f'shader module {shader_module_id} is still referenced by recorded work',
            )
        for object_state in self.objects.values():
            if not object_state.live or object_state.kind != 'pipeline':
                continue
            if shader_module_id in object_state.data.get('shader_module_ids', []):
                raise SemanticFailure(
                    'DRP2_ERR_USAGE',
                    index,
                    f'shader module {shader_module_id} is still referenced by live pipelines',
                )
        state.live = False

    def _handle_DestroyBindGroupLayout(self, index: int, command: Dict[str, Any]) -> None:
        layout_id = command['bind_group_layout_id']
        state = self._resolve_live(index, layout_id, 'bind_group_layout')
        if self._resource_in_use('bind_group_layout', layout_id):
            raise SemanticFailure(
                'DRP2_ERR_USAGE',
                index,
                f'bind-group layout {layout_id} is still referenced by recorded work',
            )
        for object_state in self.objects.values():
            if not object_state.live:
                continue
            if object_state.kind == 'bind_group' and object_state.data.get('bind_group_layout_id') == layout_id:
                raise SemanticFailure(
                    'DRP2_ERR_USAGE',
                    index,
                    f'bind-group layout {layout_id} is still referenced by live bind groups',
                )
            if object_state.kind == 'pipeline' and layout_id in object_state.data.get('bind_group_layout_ids', []):
                raise SemanticFailure(
                    'DRP2_ERR_USAGE',
                    index,
                    f'bind-group layout {layout_id} is still referenced by live pipelines',
                )
        state.live = False

    def _handle_DestroyBindGroup(self, index: int, command: Dict[str, Any]) -> None:
        bind_group_id = command['bind_group_id']
        state = self._resolve_live(index, bind_group_id, 'bind_group')
        if self._resource_in_use('bind_group', bind_group_id):
            raise SemanticFailure(
                'DRP2_ERR_USAGE',
                index,
                f'bind group {bind_group_id} is still referenced by recorded work',
            )
        state.live = False

    def _handle_CreateSampler(self, index: int, command: Dict[str, Any]) -> None:
        self._reserve_id(index, command['id'], 'sampler', {})

    def _handle_DestroySampler(self, index: int, command: Dict[str, Any]) -> None:
        sampler_id = command['id']
        state = self._resolve_live(index, sampler_id, 'sampler')
        if self._resource_in_use('sampler', sampler_id):
            raise SemanticFailure(
                'DRP2_ERR_USAGE',
                index,
                f'sampler {sampler_id} is still referenced by recorded work',
            )
        state.live = False

    def _handle_CreateTextureView(self, index: int, command: Dict[str, Any]) -> None:
        texture_state = self._resolve_live(index, command['texture_id'], 'texture')
        mip_range = command.get('mip_range')
        if mip_range is not None:
            mip_count = texture_state.data['mip_level_count']
            if mip_range['base'] + mip_range['count'] > mip_count:
                raise SemanticFailure(
                    'DRP2_ERR_OUT_OF_RANGE',
                    index,
                    f'mip_range base={mip_range["base"]} count={mip_range["count"]} exceeds '
                    f'texture mip_level_count={mip_count}',
                )
        self._reserve_id(
            index,
            command['id'],
            'texture_view',
            {'texture_id': command['texture_id']},
        )

    def _handle_DestroyTextureView(self, index: int, command: Dict[str, Any]) -> None:
        view_id = command['id']
        state = self._resolve_live(index, view_id, 'texture_view')
        if self._resource_in_use('texture_view', view_id):
            raise SemanticFailure(
                'DRP2_ERR_USAGE',
                index,
                f'texture view {view_id} is still referenced by recorded work',
            )
        state.live = False

    def _handle_CreateRenderPipeline(self, index: int, command: Dict[str, Any]) -> None:
        vertex_shader = self._resolve_live(index, command['vertex_shader_module_id'], 'shader_module')
        if vertex_shader.data['stage'] != 'VERTEX':
            raise SemanticFailure(
                'DRP2_ERR_INVALID_ARGUMENT',
                index,
                f'shader module {command["vertex_shader_module_id"]} is {vertex_shader.data["stage"]}, expected VERTEX',
            )
        fragment_shader = self._resolve_live(index, command['fragment_shader_module_id'], 'shader_module')
        if fragment_shader.data['stage'] != 'FRAGMENT':
            raise SemanticFailure(
                'DRP2_ERR_INVALID_ARGUMENT',
                index,
                f'shader module {command["fragment_shader_module_id"]} is {fragment_shader.data["stage"]}, expected FRAGMENT',
            )
        for layout_id in command.get('bind_group_layout_ids', []):
            self._resolve_live(index, layout_id, 'bind_group_layout')
        vertex_buffers = command.get('vertex_buffers')
        if vertex_buffers is not None and len(vertex_buffers) != command['vertex_buffer_slots']:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_ARGUMENT',
                index,
                f'vertex_buffers length {len(vertex_buffers)} does not match '
                f'vertex_buffer_slots {command["vertex_buffer_slots"]}',
            )
        self._reserve_id(
            index,
            command['id'],
            'pipeline',
            {
                'kind': 'render',
                'vertex_buffer_slots': command['vertex_buffer_slots'],
                'bind_group_layout_ids': list(command.get('bind_group_layout_ids', [])),
                'shader_module_ids': [
                    command['vertex_shader_module_id'],
                    command['fragment_shader_module_id'],
                ],
                'resources': {
                    ('shader_module', command['vertex_shader_module_id']),
                    ('shader_module', command['fragment_shader_module_id']),
                    *{('bind_group_layout', layout_id) for layout_id in command.get('bind_group_layout_ids', [])},
                },
            },
        )

    def _handle_DestroyRenderPipeline(self, index: int, command: Dict[str, Any]) -> None:
        pipeline_id = command['render_pipeline_id']
        state = self._resolve_live(index, pipeline_id, 'pipeline')
        if state.data['kind'] != 'render':
            raise SemanticFailure(
                'DRP2_ERR_WRONG_OBJECT_TYPE',
                index,
                f'pipeline {pipeline_id} is {state.data["kind"]}, expected render',
            )
        if self._resource_in_use('pipeline', pipeline_id):
            raise SemanticFailure(
                'DRP2_ERR_USAGE',
                index,
                f'render pipeline {pipeline_id} is still referenced by recorded work',
            )
        state.live = False

    def _handle_CreateComputePipeline(self, index: int, command: Dict[str, Any]) -> None:
        compute_shader = self._resolve_live(index, command['compute_shader_module_id'], 'shader_module')
        if compute_shader.data['stage'] != 'COMPUTE':
            raise SemanticFailure(
                'DRP2_ERR_INVALID_ARGUMENT',
                index,
                f'shader module {command["compute_shader_module_id"]} is {compute_shader.data["stage"]}, expected COMPUTE',
            )
        for layout_id in command.get('bind_group_layout_ids', []):
            self._resolve_live(index, layout_id, 'bind_group_layout')
        self._reserve_id(
            index,
            command['id'],
            'pipeline',
            {
                'kind': 'compute',
                'vertex_buffer_slots': 0,
                'bind_group_layout_ids': list(command.get('bind_group_layout_ids', [])),
                'shader_module_ids': [command['compute_shader_module_id']],
                'resources': {
                    ('shader_module', command['compute_shader_module_id']),
                    *{('bind_group_layout', layout_id) for layout_id in command.get('bind_group_layout_ids', [])},
                },
            },
        )

    def _handle_DestroyComputePipeline(self, index: int, command: Dict[str, Any]) -> None:
        pipeline_id = command['compute_pipeline_id']
        state = self._resolve_live(index, pipeline_id, 'pipeline')
        if state.data['kind'] != 'compute':
            raise SemanticFailure(
                'DRP2_ERR_WRONG_OBJECT_TYPE',
                index,
                f'pipeline {pipeline_id} is {state.data["kind"]}, expected compute',
            )
        if self._resource_in_use('pipeline', pipeline_id):
            raise SemanticFailure(
                'DRP2_ERR_USAGE',
                index,
                f'compute pipeline {pipeline_id} is still referenced by recorded work',
            )
        state.live = False

    def _handle_DestroyTexture(self, index: int, command: Dict[str, Any]) -> None:
        texture_id = command['texture_id']
        state = self._resolve_live(index, texture_id, 'texture')
        if self._resource_in_use('texture', texture_id):
            raise SemanticFailure(
                'DRP2_ERR_USAGE',
                index,
                f'texture {texture_id} is still referenced by recorded work',
            )
        state.live = False

    def _handle_WriteTexture(self, index: int, command: Dict[str, Any]) -> None:
        state = self._resolve_live(index, command['texture_id'], 'texture')
        self._check_texture_box(index, state, command['mip_level'], command['origin'], command['size'])
        required_bytes = self._required_transfer_bytes(
            index,
            state,
            command['size']['width'],
            command['size']['height'],
            command['size']['depth'],
            command['bytes_per_row'],
            command['rows_per_image'],
        )
        payload_size = len(base64.b64decode(command['data'], validate=True))
        if payload_size < required_bytes:
            raise SemanticFailure(
                'DRP2_ERR_LAYOUT',
                index,
                f'data payload size {payload_size} is smaller than required transfer footprint {required_bytes}',
            )

    def _handle_BeginCommandEncoder(self, index: int, command: Dict[str, Any]) -> None:
        encoder_id = command['id']
        self._reserve_id(index, encoder_id, 'encoder', {})
        self.encoders[encoder_id] = {'state': 'open', 'open_pass': None, 'resources': set()}

    def _handle_FinishCommandEncoder(self, index: int, command: Dict[str, Any]) -> None:
        encoder_id = command['encoder_id']
        encoder = self._resolve_encoder(index, encoder_id)
        if encoder['open_pass'] is not None:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE', index, f'encoder {encoder_id} still has an open pass'
            )
        command_buffer_id = command['command_buffer_id']
        self._reserve_id(index, command_buffer_id, 'command_buffer', {})
        encoder['state'] = 'finished'
        self.command_buffers[command_buffer_id] = {
            'resources': set(encoder['resources']),
            'state': 'finished',
            'submitted': False,
        }

    def _handle_BeginComputePass(self, index: int, command: Dict[str, Any]) -> None:
        encoder = self._resolve_encoder(index, command['encoder_id'])
        if encoder['open_pass'] is not None:
            raise SemanticFailure('DRP2_ERR_INVALID_STATE', index, 'encoder already has an open pass')
        pass_id = command['id']
        self._reserve_id(index, pass_id, 'pass', {})
        encoder['open_pass'] = pass_id
        self.passes[pass_id] = {
            'id': pass_id,
            'kind': 'compute',
            'encoder_id': command['encoder_id'],
            'state': 'open',
            'bound_pipeline_id': None,
            'bound_bind_groups': {},
            'vertex_buffers': {},
            'index_buffer': None,
        }

    def _handle_EndComputePass(self, index: int, command: Dict[str, Any]) -> None:
        pass_info = self._resolve_pass(index, command['pass_id'], expected_kind='compute')
        encoder = self.encoders[pass_info['encoder_id']]
        if encoder['open_pass'] != command['pass_id']:
            raise SemanticFailure('DRP2_ERR_INVALID_STATE', index, 'compute pass is not current')
        pass_info['state'] = 'ended'
        self.objects[command['pass_id']].live = False
        encoder['open_pass'] = None

    def _handle_BeginRenderPass(self, index: int, command: Dict[str, Any]) -> None:
        encoder = self._resolve_encoder(index, command['encoder_id'])
        if encoder['open_pass'] is not None:
            raise SemanticFailure('DRP2_ERR_INVALID_STATE', index, 'encoder already has an open pass')
        for attachment in command['color_attachments']:
            texture_id = attachment['texture_id']
            self._texture_usage(index, texture_id, 'RENDER_ATTACHMENT')
            encoder['resources'].add(('texture', texture_id))
        depth_attachment = command.get('depth_stencil_attachment')
        if depth_attachment is not None:
            texture_id = depth_attachment['texture_id']
            self._texture_usage(index, texture_id, 'RENDER_ATTACHMENT')
            encoder['resources'].add(('texture', texture_id))
        pass_id = command['id']
        self._reserve_id(index, pass_id, 'pass', {})
        encoder['open_pass'] = pass_id
        self.passes[pass_id] = {
            'id': pass_id,
            'kind': 'render',
            'encoder_id': command['encoder_id'],
            'state': 'open',
            'bound_pipeline_id': None,
            'bound_bind_groups': {},
            'vertex_buffers': {},
            'index_buffer': None,
        }

    def _handle_SetPipeline(self, index: int, command: Dict[str, Any]) -> None:
        pass_info = self._resolve_pass(index, command['pass_id'])
        pipeline = self._resolve_live(index, command['pipeline_id'], 'pipeline')
        if pipeline.data['kind'] != pass_info['kind']:
            raise SemanticFailure(
                'DRP2_ERR_PASS_MISMATCH',
                index,
                f'pipeline {command["pipeline_id"]} is {pipeline.data["kind"]}, expected {pass_info["kind"]}',
            )
        pass_info['bound_pipeline_id'] = command['pipeline_id']
        # A pipeline rebind resets compatibility assumptions for pass-local bind-group state.
        pass_info['bound_bind_groups'] = {}
        encoder = self.encoders[pass_info['encoder_id']]
        encoder['resources'].add(('pipeline', command['pipeline_id']))
        encoder['resources'].update(pipeline.data.get('resources', set()))

    def _handle_SetVertexBuffer(self, index: int, command: Dict[str, Any]) -> None:
        pass_info = self._require_active_pass_for_command(index, command['pass_id'], expected_kind='render')
        buffer_state = self._buffer_usage(index, command['buffer_id'], 'VERTEX')
        if 'size' in command:
            self._check_buffer_range(index, buffer_state, command['offset'], command['size'])
        pass_info['vertex_buffers'][command['slot']] = command['buffer_id']
        encoder = self.encoders[pass_info['encoder_id']]
        encoder['resources'].add(('buffer', command['buffer_id']))

    def _handle_SetIndexBuffer(self, index: int, command: Dict[str, Any]) -> None:
        pass_info = self._require_active_pass_for_command(index, command['pass_id'], expected_kind='render')
        buffer_state = self._buffer_usage(index, command['buffer_id'], 'INDEX')
        if 'size' in command:
            self._check_buffer_range(index, buffer_state, command['offset'], command['size'])
        pass_info['index_buffer'] = {
            'buffer_id': command['buffer_id'],
            'index_format': command['index_format'],
        }
        encoder = self.encoders[pass_info['encoder_id']]
        encoder['resources'].add(('buffer', command['buffer_id']))

    def _handle_SetBindGroup(self, index: int, command: Dict[str, Any]) -> None:
        pass_info = self._resolve_pass(index, command['pass_id'])
        self._require_bound_pipeline(index, pass_info)
        bind_group = self._resolve_live(index, command['bind_group_id'], 'bind_group')
        pipeline = self._resolve_live(index, pass_info['bound_pipeline_id'], 'pipeline')
        layout_ids = pipeline.data.get('bind_group_layout_ids', [])
        if command['slot'] >= len(layout_ids):
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE',
                index,
                f'pipeline {pass_info["bound_pipeline_id"]} has no bind-group layout for slot {command["slot"]}',
            )
        expected_layout_id = layout_ids[command['slot']]
        if bind_group.data['bind_group_layout_id'] != expected_layout_id:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE',
                index,
                f'bind group {command["bind_group_id"]} uses layout {bind_group.data["bind_group_layout_id"]}, expected {expected_layout_id}',
            )
        layout = self._resolve_live(index, expected_layout_id, 'bind_group_layout')
        dynamic_layout_entries = [
            entry for entry in layout.data['entries'] if entry.get('has_dynamic_offset', False)
        ]
        dynamic_offsets = command.get('dynamic_offsets', [])
        if len(dynamic_offsets) != len(dynamic_layout_entries):
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE',
                index,
                f'bind group {command["bind_group_id"]} expected {len(dynamic_layout_entries)} dynamic offsets, got {len(dynamic_offsets)}',
            )
        entries_by_binding = {entry['binding']: entry for entry in bind_group.data['entries']}
        for dynamic_index, layout_entry in enumerate(dynamic_layout_entries):
            bind_group_entry = entries_by_binding.get(layout_entry['binding'])
            if bind_group_entry is None:
                raise SemanticFailure(
                    'DRP2_ERR_INVALID_STATE',
                    index,
                    f'bind group {command["bind_group_id"]} is missing dynamic binding {layout_entry["binding"]}',
                )
            if bind_group_entry['resource_kind'] != 'buffer':
                raise SemanticFailure(
                    'DRP2_ERR_WRONG_OBJECT_TYPE',
                    index,
                    f'bind-group entry {layout_entry["binding"]} uses {bind_group_entry["resource_kind"]}, expected buffer',
                )
            buffer_state = self._resolve_live(index, bind_group_entry['resource_id'], 'buffer')
            base_offset = bind_group_entry.get('offset', 0)
            size = bind_group_entry.get('size')
            if size is not None:
                self._check_buffer_range(
                    index, buffer_state, base_offset + dynamic_offsets[dynamic_index], size
                )
        pass_info['bound_bind_groups'][command['slot']] = command['bind_group_id']
        encoder = self.encoders[pass_info['encoder_id']]
        encoder['resources'].add(('bind_group', command['bind_group_id']))
        encoder['resources'].add(('bind_group_layout', bind_group.data['bind_group_layout_id']))
        for resource in bind_group.data['resources']:
            encoder['resources'].add(resource)

    def _handle_SetViewport(self, index: int, command: Dict[str, Any]) -> None:
        self._require_active_pass_for_command(index, command['pass_id'], expected_kind='render')

    def _handle_SetScissor(self, index: int, command: Dict[str, Any]) -> None:
        self._require_active_pass_for_command(index, command['pass_id'], expected_kind='render')

    def _handle_SetBlendConstant(self, index: int, command: Dict[str, Any]) -> None:
        self._require_active_pass_for_command(index, command['pass_id'], expected_kind='render')

    def _handle_SetStencilReference(self, index: int, command: Dict[str, Any]) -> None:
        self._require_active_pass_for_command(index, command['pass_id'], expected_kind='render')

    def _handle_EndRenderPass(self, index: int, command: Dict[str, Any]) -> None:
        pass_info = self._resolve_pass(index, command['pass_id'], expected_kind='render')
        encoder = self.encoders[pass_info['encoder_id']]
        if encoder['open_pass'] != command['pass_id']:
            raise SemanticFailure('DRP2_ERR_INVALID_STATE', index, 'render pass is not current')
        pass_info['state'] = 'ended'
        self.objects[command['pass_id']].live = False
        encoder['open_pass'] = None

    def _handle_Draw(self, index: int, command: Dict[str, Any]) -> None:
        pass_info = self._require_active_pass_for_command(
            index, command['pass_id'], expected_kind='render'
        )
        self._require_bound_pipeline(index, pass_info)
        pipeline = self._resolve_live(index, pass_info['bound_pipeline_id'], 'pipeline')
        for slot in range(pipeline.data['vertex_buffer_slots']):
            if slot not in pass_info['vertex_buffers']:
                raise SemanticFailure(
                    'DRP2_ERR_INVALID_STATE',
                    index,
                    f'render pass {pass_info["id"]} is missing vertex buffer slot {slot}',
                )

    def _handle_DrawIndexed(self, index: int, command: Dict[str, Any]) -> None:
        pass_info = self._require_active_pass_for_command(
            index, command['pass_id'], expected_kind='render'
        )
        self._require_bound_pipeline(index, pass_info)
        pipeline = self._resolve_live(index, pass_info['bound_pipeline_id'], 'pipeline')
        for slot in range(pipeline.data['vertex_buffer_slots']):
            if slot not in pass_info['vertex_buffers']:
                raise SemanticFailure(
                    'DRP2_ERR_INVALID_STATE',
                    index,
                    f'render pass {pass_info["id"]} is missing vertex buffer slot {slot}',
                )
        if pass_info['index_buffer'] is None:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE',
                index,
                f'render pass {pass_info["id"]} has no bound index buffer',
            )

    def _handle_DispatchWorkgroups(self, index: int, command: Dict[str, Any]) -> None:
        pass_info = self._require_active_pass_for_command(
            index, command['pass_id'], expected_kind='compute'
        )
        self._require_bound_pipeline(index, pass_info)

    def _handle_CopyBufferToBuffer(self, index: int, command: Dict[str, Any]) -> None:
        encoder = self._resolve_encoder(index, command['encoder_id'])
        if encoder['open_pass'] is not None:
            raise SemanticFailure('DRP2_ERR_INVALID_STATE', index, 'copy command is inside a pass')
        src = self._buffer_usage(index, command['src_buffer_id'], 'COPY_SRC')
        dst = self._buffer_usage(index, command['dst_buffer_id'], 'COPY_DST')
        self._check_buffer_range(index, src, command['src_offset'], command['size'])
        self._check_buffer_range(index, dst, command['dst_offset'], command['size'])
        encoder['resources'].add(('buffer', command['src_buffer_id']))
        encoder['resources'].add(('buffer', command['dst_buffer_id']))

    def _handle_CopyBufferToTexture(self, index: int, command: Dict[str, Any]) -> None:
        encoder = self._resolve_encoder(index, command['encoder_id'])
        if encoder['open_pass'] is not None:
            raise SemanticFailure('DRP2_ERR_INVALID_STATE', index, 'copy command is inside a pass')
        src = self._buffer_usage(index, command['src_buffer_id'], 'COPY_SRC')
        dst = self._texture_usage(index, command['dst_texture_id'], 'COPY_DST')
        self._check_texture_box(
            index, dst, command['dst_mip_level'], command['dst_origin'], command['size']
        )
        required_bytes = self._required_transfer_bytes(
            index,
            dst,
            command['size']['width'],
            command['size']['height'],
            command['size']['depth'],
            command['bytes_per_row'],
            command['rows_per_image'],
        )
        self._check_buffer_range(index, src, command['src_offset'], required_bytes)
        encoder['resources'].add(('buffer', command['src_buffer_id']))
        encoder['resources'].add(('texture', command['dst_texture_id']))

    def _handle_CopyTextureToBuffer(self, index: int, command: Dict[str, Any]) -> None:
        encoder = self._resolve_encoder(index, command['encoder_id'])
        if encoder['open_pass'] is not None:
            raise SemanticFailure('DRP2_ERR_INVALID_STATE', index, 'copy command is inside a pass')
        src = self._texture_usage(index, command['src_texture_id'], 'COPY_SRC')
        self._check_texture_box(
            index, src, command['src_mip_level'], command['src_origin'], command['size']
        )
        dst = self._buffer_usage(index, command['dst_buffer_id'], 'COPY_DST')
        required_bytes = self._required_transfer_bytes(
            index,
            src,
            command['size']['width'],
            command['size']['height'],
            command['size']['depth'],
            command['bytes_per_row'],
            command['rows_per_image'],
        )
        self._check_buffer_range(index, dst, command['dst_offset'], required_bytes)
        encoder['resources'].add(('texture', command['src_texture_id']))
        encoder['resources'].add(('buffer', command['dst_buffer_id']))

    def _handle_CopyTextureToTexture(self, index: int, command: Dict[str, Any]) -> None:
        encoder = self._resolve_encoder(index, command['encoder_id'])
        if encoder['open_pass'] is not None:
            raise SemanticFailure('DRP2_ERR_INVALID_STATE', index, 'copy command is inside a pass')
        src = self._texture_usage(index, command['src_texture_id'], 'COPY_SRC')
        self._check_texture_box(
            index, src, command['src_mip_level'], command['src_origin'], command['size']
        )
        dst = self._texture_usage(index, command['dst_texture_id'], 'COPY_DST')
        self._check_texture_box(
            index, dst, command['dst_mip_level'], command['dst_origin'], command['size']
        )
        encoder['resources'].add(('texture', command['src_texture_id']))
        encoder['resources'].add(('texture', command['dst_texture_id']))

    def _handle_QueueSubmit(self, index: int, command: Dict[str, Any]) -> None:
        seen_command_buffer_ids = set()
        for command_buffer_id in command['command_buffer_ids']:
            if command_buffer_id in seen_command_buffer_ids:
                raise SemanticFailure(
                    'DRP2_ERR_INVALID_ARGUMENT',
                    index,
                    f'command buffer {command_buffer_id} appears more than once in the same submission',
                )
            seen_command_buffer_ids.add(command_buffer_id)
            self._resolve_live(index, command_buffer_id, 'command_buffer')
            command_buffer = self.command_buffers[command_buffer_id]
            if command_buffer['state'] != 'finished':
                raise SemanticFailure(
                    'DRP2_ERR_INVALID_STATE',
                    index,
                    f'command buffer {command_buffer_id} is not finished',
                )
            if command_buffer['submitted']:
                raise SemanticFailure(
                    'DRP2_ERR_INVALID_STATE',
                    index,
                    f'command buffer {command_buffer_id} was already submitted',
                )
            command_buffer['submitted'] = True

        readbacks = command.get('readbacks', [])
        if readbacks:
            submission_id = command.get('submission_id')
            if submission_id is None:
                raise SemanticFailure(
                    'DRP2_ERR_INVALID_ARGUMENT',
                    index,
                    'submission_id is required when readbacks are requested',
                )
            for rb in readbacks:
                buffer_state = self._buffer_usage(index, rb['buffer_id'], 'MAP_READ')
                self._check_buffer_range(index, buffer_state, rb['offset'], rb['size'])
            self.pending_readbacks[submission_id] = readbacks

    def _handle_QueueSubmitReply(self, index: int, command: Dict[str, Any]) -> None:
        submission_id = command['submission_id']
        if submission_id not in self.pending_readbacks:
            raise SemanticFailure(
                'DRP2_ERR_INVALID_STATE',
                index,
                f'QueueSubmitReply references submission {submission_id} with no pending readback request',
            )
        expected = self.pending_readbacks.pop(submission_id)
        actual = command.get('readbacks', [])
        if len(actual) != len(expected):
            raise SemanticFailure(
                'DRP2_ERR_INVALID_ARGUMENT',
                index,
                f'QueueSubmitReply readback count {len(actual)} does not match request count {len(expected)}',
            )
        for req, rep in zip(expected, actual):
            if (rep['buffer_id'] != req['buffer_id']
                    or rep['offset'] != req['offset']
                    or rep['size'] != req['size']):
                raise SemanticFailure(
                    'DRP2_ERR_INVALID_ARGUMENT',
                    index,
                    'QueueSubmitReply readback entry does not match the original request',
                )


class DRP2FixtureRunner:
    """Load, validate, and run DRP2 fixtures."""

    def __init__(self, root_dir: Path) -> None:
        self.root_dir = root_dir
        self.fixtures_dir = root_dir / 'spec' / 'drp2' / 'fixtures'
        self.fixture_schema = self._load_json(FIXTURE_SCHEMA_PATH)
        self.fixture_validator = self._build_validator(FIXTURE_SCHEMA_PATH, self.fixture_schema)
        self.command_schema = self._load_json(COMMAND_SCHEMA_PATH)
        self.command_validator = self._build_validator(COMMAND_SCHEMA_PATH, self.command_schema)

    def discover(
        self,
        selected_paths: Sequence[str],
        name_filter: Optional[str],
        tags: Sequence[str],
    ) -> List[Path]:
        """Discover fixtures in lexical order."""

        candidates: List[Path] = []
        if selected_paths:
            for selected in selected_paths:
                path = (self.root_dir / selected).resolve()
                if path.is_dir():
                    candidates.extend(sorted(path.glob('*.json')))
                else:
                    candidates.append(path)
        else:
            for fixture_dir in FIXTURE_DIRS:
                candidates.extend(sorted((self.fixtures_dir / fixture_dir).glob('*.json')))

        filtered: List[Path] = []
        for path in candidates:
            if not path.exists() or path.suffix != '.json':
                continue
            data = self._load_json(path)
            if name_filter and name_filter not in data.get('name', ''):
                continue
            if tags and not set(tags).issubset(set(data.get('tags', []))):
                continue
            filtered.append(path)
        return sorted(filtered)

    def run_fixtures(self, fixture_paths: Iterable[Path]) -> List[FixtureResult]:
        """Run all requested fixtures."""

        return [self.run_fixture(path) for path in fixture_paths]

    def run_fixture(self, fixture_path: Path) -> FixtureResult:
        """Run one fixture end to end."""

        fixture = self._load_json(fixture_path)
        try:
            self.fixture_validator.validate(fixture)
        except jsonschema.ValidationError as exc:
            return self._mismatch_result(
                fixture_path,
                fixture,
                actual_outcome='error',
                actual_phase=None,
                actual_code=None,
                actual_command_index=None,
                message=f'fixture envelope validation failed: {exc.message}',
            )

        actual = self._evaluate_fixture(fixture)
        return self._to_result(fixture_path, fixture, actual)

    def _evaluate_fixture(self, fixture: Dict[str, Any]) -> Dict[str, Any]:
        commands = fixture['commands']
        expected = fixture['expected']
        if expected['outcome'] == 'error' and expected.get('phase') == 'schema_validation':
            failure = self._validate_command_stream(commands, stop_on_first=True)
            if failure is None:
                return {
                    'outcome': 'success',
                    'phase': None,
                    'code': None,
                    'command_index': None,
                    'message': 'schema-negative fixture unexpectedly passed command schema validation',
                }
            return failure

        failure = self._validate_command_stream(commands, stop_on_first=True)
        if failure is not None:
            return failure

        try:
            DRP2SemanticValidator().validate(commands)
        except SemanticFailure as exc:
            return {
                'outcome': 'error',
                'phase': 'semantic_validation',
                'code': exc.code,
                'command_index': exc.command_index,
                'message': exc.message,
            }

        capabilities = fixture.get('capabilities')
        if capabilities is not None:
            try:
                DRP2CapabilityValidator(capabilities).validate(commands)
            except CapabilityFailure as exc:
                return {
                    'outcome': 'error',
                    'phase': 'capability_validation',
                    'code': exc.code,
                    'command_index': exc.command_index,
                    'message': exc.message,
                }

        return {
            'outcome': 'success',
            'phase': None,
            'code': None,
            'command_index': None,
            'message': None,
        }

    def _validate_command_stream(
        self, commands: Sequence[Dict[str, Any]], stop_on_first: bool
    ) -> Optional[Dict[str, Any]]:
        for index, command in enumerate(commands):
            errors = sorted(self.command_validator.iter_errors(command), key=lambda err: list(err.path))
            if errors:
                error = errors[0]
                return {
                    'outcome': 'error',
                    'phase': 'schema_validation',
                    'code': 'DRP2_ERR_INVALID_ARGUMENT',
                    'command_index': index,
                    'message': error.message,
                }
            if not stop_on_first:
                continue
        return None

    def _to_result(
        self, fixture_path: Path, fixture: Dict[str, Any], actual: Dict[str, Any]
    ) -> FixtureResult:
        expected = fixture['expected']
        passed = actual['outcome'] == expected['outcome']
        if passed and expected['outcome'] == 'error':
            passed = actual['phase'] == expected.get('phase') and actual['code'] == expected.get('code')
            if passed and 'command_index' in expected:
                passed = actual['command_index'] == expected['command_index']

        return FixtureResult(
            fixture_path=str(fixture_path.relative_to(self.root_dir)),
            fixture_name=fixture['name'],
            actual_outcome=actual['outcome'],
            actual_phase=actual['phase'],
            actual_code=actual['code'],
            actual_command_index=actual['command_index'],
            passed=passed,
            message=actual.get('message'),
            expected_outcome=expected.get('outcome'),
            expected_phase=expected.get('phase'),
            expected_code=expected.get('code'),
            expected_command_index=expected.get('command_index'),
        )

    def _mismatch_result(
        self,
        fixture_path: Path,
        fixture: Dict[str, Any],
        actual_outcome: str,
        actual_phase: Optional[str],
        actual_code: Optional[str],
        actual_command_index: Optional[int],
        message: str,
    ) -> FixtureResult:
        expected = fixture.get('expected', {})
        return FixtureResult(
            fixture_path=str(fixture_path.relative_to(self.root_dir)),
            fixture_name=fixture.get('name', fixture_path.stem),
            actual_outcome=actual_outcome,
            actual_phase=actual_phase,
            actual_code=actual_code,
            actual_command_index=actual_command_index,
            passed=False,
            message=message,
            expected_outcome=expected.get('outcome'),
            expected_phase=expected.get('phase'),
            expected_code=expected.get('code'),
            expected_command_index=expected.get('command_index'),
        )

    @staticmethod
    def _load_json(path: Path) -> Dict[str, Any]:
        with path.open('r', encoding='utf-8') as stream:
            return json.load(stream)

    @staticmethod
    def _build_validator(path: Path, schema: Dict[str, Any]) -> jsonschema.protocols.Validator:
        schema_copy = dict(schema)
        schema_copy['$id'] = path.resolve().as_uri()
        validator_cls = jsonschema.validators.validator_for(schema)
        validator_cls.check_schema(schema_copy)
        if Registry is not None:
            return validator_cls(schema_copy, registry=Registry(retrieve=_retrieve_schema_resource))

        base_uri = path.resolve().parent.as_uri() + '/'
        resolver = jsonschema.validators.RefResolver(base_uri=base_uri, referrer=schema_copy)
        return validator_cls(schema_copy, resolver=resolver)


def _retrieve_schema_resource(uri: str) -> Resource:
    """Load JSON schema resources for registry-based reference resolution."""

    parsed = urlparse(uri)
    if parsed.scheme != 'file':
        raise ValueError(f'unsupported schema uri: {uri}')
    schema_path = Path(unquote(parsed.path))
    with schema_path.open('r', encoding='utf-8') as stream:
        return Resource.from_contents(json.load(stream))


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description='Run the DRP2 fixture corpus.')
    parser.add_argument('paths', nargs='*', help='Optional fixture files or directories to run.')
    parser.add_argument('--name', help='Filter fixtures by substring match on fixture name.')
    parser.add_argument('--tag', action='append', default=[], help='Require a fixture tag.')
    parser.add_argument('--json', action='store_true', help='Emit structured JSON results.')
    return parser


def _print_text(results: Sequence[FixtureResult]) -> None:
    for result in results:
        status = 'PASS' if result.passed else 'FAIL'
        print(f'{status} {result.fixture_path} ({result.fixture_name})')
        if not result.passed:
            print(f'  actual: {result.actual_outcome} {result.actual_phase} {result.actual_code}')
            if result.actual_command_index is not None:
                print(f'  actual_command_index: {result.actual_command_index}')
            print(
                f'  expected: {result.expected_outcome} {result.expected_phase} {result.expected_code}'
            )
            if result.expected_command_index is not None:
                print(f'  expected_command_index: {result.expected_command_index}')
            if result.message:
                print(f'  message: {result.message}')

    total = len(results)
    passed = sum(1 for result in results if result.passed)
    failed = total - passed
    print(f'SUMMARY total={total} passed={passed} failed={failed}')


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = _build_arg_parser()
    args = parser.parse_args(argv)

    runner = DRP2FixtureRunner(ROOT_DIR)
    fixture_paths = runner.discover(args.paths, args.name, args.tag)
    results = runner.run_fixtures(fixture_paths)

    if args.json:
        print(json.dumps([asdict(result) for result in results], indent=2))
    else:
        _print_text(results)

    return 0 if all(result.passed for result in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
