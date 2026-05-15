#!/usr/bin/env python3
"""Preflight DRP2 positive fixtures against the WebGPU fixture-runner contract."""

from __future__ import annotations

import argparse
import json
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


@dataclass
class WebGPUPreflightResult:
    """Store the preflight result for one positive fixture."""

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
    """Check positive DRP2 fixtures against strict WebGPU dashboard assumptions."""

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
        for entry in manifest.get('positive', []):
            relative = entry[1:] if entry.startswith('/') else entry
            paths.append((self.root_dir / relative).resolve())
        return sorted(paths)

    def run_fixtures(self, fixture_paths: Iterable[Path]) -> List[WebGPUPreflightResult]:
        """Run preflight checks for every fixture path."""

        return [self.run_fixture(path) for path in fixture_paths]

    def run_fixture(self, fixture_path: Path) -> WebGPUPreflightResult:
        """Run WebGPU preflight checks for one positive fixture."""

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
        pipelines: Dict[int, Dict[str, Any]] = {}
        passes: Dict[int, Dict[str, Any]] = {}

        for index, command in enumerate(fixture['commands']):
            cmd = command['cmd']
            if cmd == 'CreateBuffer':
                buffers[command['id']] = command
            elif cmd == 'CreateBindGroupLayout':
                self._check_bind_group_layout(index, command)
            elif cmd == 'CreateRenderPipeline':
                self._check_render_pipeline(index, command)
                pipelines[command['id']] = command
            elif cmd == 'BeginComputePass':
                passes[command['id']] = {'kind': 'compute'}
            elif cmd == 'BeginRenderPass':
                passes[command['id']] = {
                    'kind': 'render',
                    'pipeline_id': None,
                    'vertex_buffers': {},
                    'index_buffer': None,
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

    def _check_render_pipeline(self, index: int, command: Dict[str, Any]) -> None:
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
