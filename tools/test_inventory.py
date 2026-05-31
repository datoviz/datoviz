#!/usr/bin/env python3
"""Generate a lane-oriented inventory from the in-tree test runner."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import Counter
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Iterable, Sequence


ROOT_DIR = Path(__file__).resolve().parents[1]
DEFAULT_RUNNER = ROOT_DIR / "build" / "testing" / "dvztest"
DEFAULT_OUTPUT = ROOT_DIR / "build" / "testing" / "test_inventory.json"

LIST_LINE_RE = re.compile(
    r"^(?P<display_id>\S+)\s+"
    r"function=(?P<function>\S*)\s+"
    r"resources=(?P<resources>\S*)\s+"
    r"isolation=(?P<isolation>\S*)\s+"
    r"fixture=(?P<fixture>\S*)\s+"
    r"fixture_scope=(?P<fixture_scope>\S*)$"
)

CORE_MODULES = {"common", "ds", "fileio", "geom", "math", "thread"}
RUNTIME_MODULES = {"vk", "vklite"}
RENDER_MODULES = {"canvas", "window", "stream", "video", "gui"}
SLOW_CHURN_KEYWORDS = (
    "churn",
    "device_lost",
    "failure_injection",
    "failfast",
    "long",
    "rebuild",
    "recreate",
    "recovery",
    "repeat",
    "resize",
    "steady",
    "wait",
)


@dataclass(frozen=True)
class TestCaseInventory:
    """Inventory record for one runner case."""

    case_id: str
    display_id: str
    module: str
    group: str
    name: str
    function: str
    resources: list[str]
    isolation: str
    fixture: str | None
    fixture_scope: str
    lane: str
    lane_reason: str
    elapsed_ns: int | None = None
    status: str | None = None


@dataclass(frozen=True)
class TestInventory:
    """Full test inventory document."""

    schema_version: int
    source: dict[str, Any]
    summary: dict[str, Any]
    cases: list[TestCaseInventory] = field(default_factory=list)


def _split_tokens(value: str) -> list[str]:
    if value == "" or value == "none":
        return []
    return [token for token in value.split(",") if token]


def _run_text(command: Sequence[str]) -> str:
    completed = subprocess.run(
        command,
        cwd=ROOT_DIR,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return completed.stdout


def _parse_group_lines(text: str) -> set[tuple[str, str]]:
    groups: set[tuple[str, str]] = set()
    for raw in text.splitlines():
        line = raw.strip()
        if not line:
            continue
        parts = line.split("/", 1)
        if len(parts) != 2:
            continue
        groups.add((parts[0], parts[1]))
    return groups


def _split_display_id(display_id: str, groups: set[tuple[str, str]]) -> tuple[str, str, str]:
    parts = display_id.split("/")
    if len(parts) < 2:
        raise ValueError(f"invalid test display id: {display_id}")

    module = parts[0]
    if len(parts) >= 3 and (module, parts[1]) in groups:
        return module, parts[1], "/".join(parts[2:])
    return module, "default", "/".join(parts[1:])


def _classify_lane(
    module: str, group: str, name: str, function: str, resources: Iterable[str], fixture: str | None
) -> tuple[str, str]:
    resource_set = set(resources)
    text = f"{module}/{group}/{name}/{function}".lower()

    if any(keyword in text for keyword in SLOW_CHURN_KEYWORDS):
        return "slow-churn", "name contains churn/lifetime keyword"

    if module == "drp2":
        if group == "vklite-runtime" or fixture == "drp2-vklite-runtime":
            return "runtime-vklite", "DRP2 stream executes through vklite"
        return "drp2-contract", "DRP2 stream/schema/runtime contract"

    if module == "scene":
        if {"gpu", "vulkan"} & resource_set:
            if group in {"query", "sample-profile"}:
                return "render-conformance", "scene readback or sampled render assertion"
            return "render-smoke", "scene GPU/offscreen render coverage"
        return "scene-semantic", "scene behavior without GPU execution"

    if module in RUNTIME_MODULES:
        return "runtime-vklite", "Vulkan/vklite runtime coverage"

    if module in RENDER_MODULES:
        if {"gpu", "vulkan", "glfw", "video"} & resource_set:
            return "render-smoke", "presentation/offscreen render coverage"
        return "fast-cpu", "CPU-only support module coverage"

    if module in CORE_MODULES or resource_set <= {"cpu", "filesystem", "env", "log-capture"}:
        return "fast-cpu", "CPU-only module coverage"

    return "fast-cpu", "default CPU-oriented lane"


def parse_list_output(list_text: str, groups_text: str) -> list[TestCaseInventory]:
    """Parse `dvztest --list` and `dvztest --list-groups` output."""

    groups = _parse_group_lines(groups_text)
    cases: list[TestCaseInventory] = []
    for raw in list_text.splitlines():
        line = raw.strip()
        if not line:
            continue
        match = LIST_LINE_RE.match(line)
        if match is None:
            raise ValueError(f"could not parse runner list line: {line}")

        values = match.groupdict()
        module, group, name = _split_display_id(values["display_id"], groups)
        resources = _split_tokens(values["resources"])
        fixture = values["fixture"] or None
        lane, lane_reason = _classify_lane(
            module, group, name, values["function"], resources, fixture
        )
        cases.append(
            TestCaseInventory(
                case_id=f"{module}/{group}/{values['function']}",
                display_id=values["display_id"],
                module=module,
                group=group,
                name=name,
                function=values["function"],
                resources=resources,
                isolation=values["isolation"],
                fixture=fixture,
                fixture_scope=values["fixture_scope"],
                lane=lane,
                lane_reason=lane_reason,
            )
        )
    return cases


def _timing_by_function(path: Path) -> dict[str, dict[str, Any]]:
    with path.open("r", encoding="utf-8") as stream:
        data = json.load(stream)
    timing: dict[str, dict[str, Any]] = {}
    for case in data.get("cases", []):
        function = case.get("function")
        if not function or function in timing:
            continue
        timing[function] = {
            "elapsed_ns": case.get("elapsed_ns"),
            "status": case.get("status"),
        }
    return timing


def _apply_timing(
    cases: list[TestCaseInventory], timing: dict[str, dict[str, Any]]
) -> list[TestCaseInventory]:
    merged: list[TestCaseInventory] = []
    for case in cases:
        values = asdict(case)
        case_timing = timing.get(case.function)
        if case_timing is not None:
            values["elapsed_ns"] = case_timing.get("elapsed_ns")
            values["status"] = case_timing.get("status")
        merged.append(TestCaseInventory(**values))
    return merged


def _counter_dict(values: Iterable[str]) -> dict[str, int]:
    return dict(sorted(Counter(values).items()))


def _summary(cases: list[TestCaseInventory]) -> dict[str, Any]:
    return {
        "total": len(cases),
        "by_lane": _counter_dict(case.lane for case in cases),
        "by_module": _counter_dict(case.module for case in cases),
        "by_group": _counter_dict(f"{case.module}/{case.group}" for case in cases),
        "by_isolation": _counter_dict(case.isolation for case in cases),
        "by_resource_set": _counter_dict(
            ",".join(case.resources) if case.resources else "none" for case in cases
        ),
        "with_fixture": sum(1 for case in cases if case.fixture is not None),
        "with_timing": sum(1 for case in cases if case.elapsed_ns is not None),
    }


def build_inventory(
    runner: Path,
    timing_json: Path | None = None,
    extra_runner_args: Sequence[str] = (),
) -> TestInventory:
    """Build a test inventory by querying the runner."""

    list_text = _run_text([str(runner), *extra_runner_args, "--list"])
    groups_text = _run_text([str(runner), *extra_runner_args, "--list-groups"])
    cases = parse_list_output(list_text, groups_text)
    if timing_json is not None:
        cases = _apply_timing(cases, _timing_by_function(timing_json))

    return TestInventory(
        schema_version=1,
        source={
            "runner": str(runner.relative_to(ROOT_DIR) if runner.is_relative_to(ROOT_DIR) else runner),
            "extra_runner_args": list(extra_runner_args),
            "timing_json": str(timing_json) if timing_json is not None else None,
        },
        summary=_summary(cases),
        cases=cases,
    )


def _write_inventory(path: Path, inventory: TestInventory) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as stream:
        json.dump(asdict(inventory), stream, indent=2)
        stream.write("\n")


def _write_markdown(path: Path, inventory: TestInventory) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    summary = inventory.summary
    lines = [
        "# Test Inventory Summary",
        "",
        f"Total cases: {summary['total']}",
        "",
        "## Lanes",
        "",
    ]
    for lane, count in summary["by_lane"].items():
        lines.append(f"- `{lane}`: {count}")
    lines.extend(["", "## Modules", ""])
    for module, count in summary["by_module"].items():
        lines.append(f"- `{module}`: {count}")
    lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def _print_summary(inventory: TestInventory) -> None:
    summary = inventory.summary
    print(f"cases: {summary['total']}")
    print("lanes:")
    for lane, count in summary["by_lane"].items():
        print(f"  {lane}: {count}")
    print("modules:")
    for module, count in summary["by_module"].items():
        print(f"  {module}: {count}")


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path, default=DEFAULT_RUNNER)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--markdown", type=Path)
    parser.add_argument(
        "--timing-json",
        type=Path,
        help="Optional JSON results from a previous runner execution, used to add elapsed time.",
    )
    parser.add_argument(
        "runner_args",
        nargs=argparse.REMAINDER,
        help="Optional runner filters before --list, for example: -- --module scene.",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    runner = args.runner.resolve()
    if not runner.exists():
        print(f"missing test runner: {runner}; run `just build` first", file=sys.stderr)
        return 1

    extra_runner_args = list(args.runner_args)
    if extra_runner_args and extra_runner_args[0] == "--":
        extra_runner_args = extra_runner_args[1:]

    inventory = build_inventory(
        runner,
        timing_json=args.timing_json.resolve() if args.timing_json else None,
        extra_runner_args=extra_runner_args,
    )
    _write_inventory(args.output, inventory)
    if args.markdown is not None:
        _write_markdown(args.markdown, inventory)
    _print_summary(inventory)
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
