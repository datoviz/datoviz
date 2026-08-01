#!/usr/bin/env bash
set -euo pipefail

frames=60
live_bin="./build/testing/dvz_live_canvas"
scenario_bin="./build/examples/c/start/scatter"

usage() {
    echo "usage: scripts/check_present_paths.sh [--frames N] [--bin PATH] [--scenario-bin PATH]"
}

while (($# > 0)); do
    case "$1" in
        --frames)
            frames="${2:?missing value for --frames}"
            shift 2
            ;;
        --bin)
            live_bin="${2:?missing value for --bin}"
            shift 2
            ;;
        --scenario-bin)
            scenario_bin="${2:?missing value for --scenario-bin}"
            shift 2
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            echo "error: unknown option '$1'" >&2
            usage >&2
            exit 1
            ;;
    esac
done

if [[ ! "$frames" =~ ^[0-9]+$ ]] || ((frames < 2)); then
    echo "error: --frames must be an integer of at least two" >&2
    exit 1
fi
if [[ ! -x "$live_bin" ]]; then
    echo "error: '$live_bin' is missing or not executable" >&2
    exit 1
fi
if [[ ! -x "$scenario_bin" ]]; then
    echo "error: '$scenario_bin' is missing or not executable" >&2
    exit 1
fi

report_dir=$(mktemp -d "${TMPDIR:-/tmp}/dvz-present-check.XXXXXX")
trap 'rm -rf -- "$report_dir"' EXIT

validation_pattern='Validation Error|VUID-|scene-drp2: failed|canvas (frame|submit) error'

run_case() {
    local name="$1"
    shift
    local log="$report_dir/$name.log"
    echo "===== $name ====="
    if ! "$@" >"$log" 2>&1; then
        sed -n '1,240p' "$log" >&2
        echo "error: $name command failed" >&2
        return 1
    fi
    sed -n '1,240p' "$log"
    if grep -En "$validation_pattern" "$log"; then
        echo "error: $name reported a rendering or validation failure" >&2
        return 1
    fi
}

run_case blank env DVZ_PRESENT_MODE=immediate DVZ_APP_SCHEDULE=continuous \
    "$live_bin" --benchmark --frames "$frames" --draw clear --present immediate
grep -Eq 'benchmark: swapchain recreates total=[0-9]+ steady=0' "$report_dir/blank.log"

run_case scene-drp2 env DVZ_PRESENT_MODE=immediate DVZ_APP_SCHEDULE=continuous \
    "$live_bin" --benchmark --frames "$frames" --draw scene-drp2 --present immediate
grep -Eq 'scene-drp2: rendering into canvas target' "$report_dir/scene-drp2.log"
grep -Eq 'benchmark: scene_points=1' "$report_dir/scene-drp2.log"
grep -Eq 'benchmark: swapchain recreates total=[0-9]+ steady=0' "$report_dir/scene-drp2.log"

for scene_path in cached-plan cached-stream; do
    run_case "scene-drp2-$scene_path" env DVZ_PRESENT_MODE=immediate \
        DVZ_APP_SCHEDULE=continuous "$live_bin" --benchmark --frames "$frames" \
        --draw scene-drp2 --scene-path "$scene_path" --present immediate
    grep -Eq "benchmark: scene_path=$scene_path " \
        "$report_dir/scene-drp2-$scene_path.log"
    grep -Eq 'benchmark: swapchain recreates total=[0-9]+ steady=0' \
        "$report_dir/scene-drp2-$scene_path.log"
done

run_case scene-drp2-10k env DVZ_PRESENT_MODE=immediate DVZ_APP_SCHEDULE=continuous \
    "$live_bin" --benchmark --frames "$frames" --draw scene-drp2 --scene-points 10000 \
    --present immediate
grep -Eq 'benchmark: scene_points=10000' "$report_dir/scene-drp2-10k.log"
grep -Eq 'benchmark: swapchain recreates total=[0-9]+ steady=0' \
    "$report_dir/scene-drp2-10k.log"

run_case scatter-1 env DVZ_PRESENT_MODE=immediate DVZ_APP_SCHEDULE=continuous \
    DVZ_APP_FRAME_TIMING=1 DVZ_SCATTER_POINT_COUNT=1 "$scenario_bin" --benchmark "$frames"
grep -Eq 'scenario_benchmark_points: 1' "$report_dir/scatter-1.log"
grep -Eq "app_frame_timing: view=0 frames=$frames " "$report_dir/scatter-1.log"
grep -Eq "scenario_benchmark: scenario=start_scatter frames=$frames " \
    "$report_dir/scatter-1.log"

run_case scatter env DVZ_PRESENT_MODE=immediate DVZ_APP_SCHEDULE=continuous \
    DVZ_APP_FRAME_TIMING=1 "$scenario_bin" --benchmark "$frames"
grep -Eq "scenario_benchmark: scenario=start_scatter frames=$frames " "$report_dir/scatter.log"
grep -Eq 'scenario_benchmark_points: 10000' "$report_dir/scatter.log"
grep -Eq "app_frame_timing: view=0 frames=$frames " "$report_dir/scatter.log"

run_case scatter-panzoom env DVZ_PRESENT_MODE=immediate DVZ_APP_SCHEDULE=continuous \
    DVZ_APP_FRAME_TIMING=1 DVZ_SCATTER_BENCHMARK=panzoom-v1 \
    "$scenario_bin" --benchmark "$frames"
grep -Eq 'scenario_benchmark_workload: panzoom-v1' "$report_dir/scatter-panzoom.log"
grep -Eq "scenario_benchmark: scenario=start_scatter frames=$frames " \
    "$report_dir/scatter-panzoom.log"

echo "present_check: blank=pass scene-drp2=pass scene-controls=pass scene-10k=pass scatter-1=pass scatter=pass scatter-panzoom=pass frames=$frames"
