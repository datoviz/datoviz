#!/usr/bin/env bash
set -euo pipefail

frames=50000
stat_runs=5
bench_runs=3
sample_freq=999
profile_timeout=120
out_dir=""
run_nsys=0
bin="./build-profile/testing/dvz_live_canvas"
scenario_bin="./build-profile/examples/c/start/scatter"
scenario_frames=1200

usage() {
    cat <<'EOF'
usage: scripts/profile_live_canvas.sh [options]

Profile dvz_live_canvas clear vs scene-drp2 and save a report directory.

Options:
  --frames N       Frames per benchmark/profile run (default: 50000)
  --stat-runs N    Repetitions for perf stat -r (default: 5)
  --bench-runs N   Plain benchmark repetitions before profiling (default: 3)
  --freq N         perf record sample frequency (default: 999)
  --timeout N      Timeout in seconds for perf record and nsys runs (default: 120)
  --out DIR        Output directory (default: build/profiles/live-canvas-<timestamp>)
  --bin PATH       dvz_live_canvas path (default: ./build-profile/testing/dvz_live_canvas)
  --scenario-bin PATH high-level scenario path (default: ./build-profile/examples/c/start/scatter)
  --scenario-frames N measured high-level scenario frames (default: 1200)
  --nsys           Capture Nsight Systems traces for scene-drp2 and scene-app
  -h, --help       Show this help

Example:
  just profile-canvas
  just profile-canvas --frames 30000 --nsys
EOF
}

while (($# > 0)); do
    case "$1" in
        --frames)
            frames="${2:?missing value for --frames}"
            shift 2
            ;;
        --stat-runs)
            stat_runs="${2:?missing value for --stat-runs}"
            shift 2
            ;;
        --bench-runs)
            bench_runs="${2:?missing value for --bench-runs}"
            shift 2
            ;;
        --freq)
            sample_freq="${2:?missing value for --freq}"
            shift 2
            ;;
        --timeout)
            profile_timeout="${2:?missing value for --timeout}"
            shift 2
            ;;
        --out)
            out_dir="${2:?missing value for --out}"
            shift 2
            ;;
        --bin)
            bin="${2:?missing value for --bin}"
            shift 2
            ;;
        --scenario-bin)
            scenario_bin="${2:?missing value for --scenario-bin}"
            shift 2
            ;;
        --scenario-frames)
            scenario_frames="${2:?missing value for --scenario-frames}"
            shift 2
            ;;
        --nsys)
            run_nsys=1
            shift
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

if [[ ! "$frames" =~ ^[0-9]+$ ]] || ((frames == 0)); then
    echo "error: --frames must be a positive integer" >&2
    exit 1
fi
if [[ ! "$stat_runs" =~ ^[0-9]+$ ]] || ((stat_runs == 0)); then
    echo "error: --stat-runs must be a positive integer" >&2
    exit 1
fi
if [[ ! "$bench_runs" =~ ^[0-9]+$ ]] || ((bench_runs == 0)); then
    echo "error: --bench-runs must be a positive integer" >&2
    exit 1
fi
if [[ ! "$sample_freq" =~ ^[0-9]+$ ]] || ((sample_freq == 0)); then
    echo "error: --freq must be a positive integer" >&2
    exit 1
fi
if [[ ! "$profile_timeout" =~ ^[0-9]+$ ]] || ((profile_timeout == 0)); then
    echo "error: --timeout must be a positive integer" >&2
    exit 1
fi
if [[ ! "$scenario_frames" =~ ^[0-9]+$ ]] || ((scenario_frames < 2)); then
    echo "error: --scenario-frames must be an integer of at least two" >&2
    exit 1
fi
if [[ ! -x "$bin" ]]; then
    echo "error: '$bin' is missing or not executable; run 'just build-profile' first" >&2
    exit 1
fi
if [[ ! -x "$scenario_bin" ]]; then
    echo "error: '$scenario_bin' is missing or not executable; run 'just build-profile' first" >&2
    exit 1
fi
perf_available=0
perf_unavailable_reason=""
if command -v perf >/dev/null 2>&1; then
    perf_probe_file="$(mktemp)"
    if perf stat -e task-clock -- true >/dev/null 2>"$perf_probe_file"; then
        perf_available=1
    else
        perf_unavailable_reason="$(cat "$perf_probe_file")"
    fi
    rm -f "$perf_probe_file"
else
    perf_unavailable_reason="perf was not found in PATH"
fi

if [[ -z "$out_dir" ]]; then
    out_dir="build/profiles/live-canvas-$(date +%Y%m%d-%H%M%S)"
fi
mkdir -p "$out_dir"

run_case() {
    local mode="$1"
    shift
    "$bin" --benchmark --frames "$frames" --draw "$mode" "$@"
}

run_with_timeout() {
    if command -v timeout >/dev/null 2>&1; then
        timeout --kill-after=5s "$profile_timeout" "$@"
    else
        "$@"
    fi
}

record_command() {
    printf '%q ' "$@"
    printf '\n'
}

{
    echo "date: $(date --iso-8601=seconds 2>/dev/null || date)"
    echo "host: $(hostname)"
    echo "kernel: $(uname -a)"
    echo "repo: $(pwd)"
    echo "git: $(git rev-parse --short HEAD 2>/dev/null || true)"
    echo "binary: $bin"
    echo "scenario_binary: $scenario_bin"
    echo "scenario_frames: $scenario_frames"
    echo "frames: $frames"
    echo "bench_runs: $bench_runs"
    echo "stat_runs: $stat_runs"
    echo "sample_freq: $sample_freq"
    echo "profile_timeout: $profile_timeout"
    echo
    echo "binary file:"
    file "$bin" || true
    echo
    echo "linked libraries:"
    ldd "$bin" 2>/dev/null || true
    echo
    echo "perf version:"
    perf --version || true
} >"$out_dir/metadata.txt"

echo "Writing live-canvas profiling report to $out_dir"

for mode in clear scene-drp2; do
    log="$out_dir/benchmark-$mode.log"
    : >"$log"
    for run in $(seq 1 "$bench_runs"); do
        {
            echo "===== $mode benchmark run $run/$bench_runs ====="
            record_command "$bin" --benchmark --frames "$frames" --draw "$mode"
            run_case "$mode"
            echo
        } 2>&1 | tee -a "$log"
    done
done

scenario_log="$out_dir/benchmark-scene-app.log"
: >"$scenario_log"
for run in $(seq 1 "$bench_runs"); do
    {
        echo "===== scene-app benchmark run $run/$bench_runs ====="
        record_command env DVZ_PRESENT_MODE=immediate "$scenario_bin" --benchmark "$scenario_frames"
        env DVZ_PRESENT_MODE=immediate "$scenario_bin" --benchmark "$scenario_frames"
        echo
    } 2>&1 | tee -a "$scenario_log"
done

for mode in clear scene-drp2; do
    log="$out_dir/perf-stat-$mode.txt"
    echo "===== $mode perf stat ===== (logging to $log)"
    {
        echo "===== $mode perf stat ====="
        record_command perf stat -r "$stat_runs" -d -- \
            "$bin" --benchmark --frames "$frames" --draw "$mode"
        if ((perf_available)); then
            perf stat -r "$stat_runs" -d -- \
                "$bin" --benchmark --frames "$frames" --draw "$mode" || true
        else
            echo "perf is unavailable in this environment."
            echo
            printf '%s\n' "$perf_unavailable_reason"
        fi
    } >"$log" 2>&1
done

scenario_stat_log="$out_dir/perf-stat-scene-app.txt"
echo "===== scene-app perf stat ===== (logging to $scenario_stat_log)"
{
    echo "===== scene-app perf stat ====="
    record_command perf stat -r "$stat_runs" -d -- env DVZ_PRESENT_MODE=immediate \
        "$scenario_bin" --benchmark "$scenario_frames"
    if ((perf_available)); then
        perf stat -r "$stat_runs" -d -- env DVZ_PRESENT_MODE=immediate \
            "$scenario_bin" --benchmark "$scenario_frames" || true
    else
        echo "perf is unavailable in this environment."
        echo
        printf '%s\n' "$perf_unavailable_reason"
    fi
} >"$scenario_stat_log" 2>&1

if ((perf_available)); then
    for mode in clear scene-drp2; do
        data="$out_dir/perf-$mode.data"
        log="$out_dir/perf-record-$mode.log"
        echo "===== $mode perf record ===== (logging to $log)"
        {
            echo "===== $mode perf record ====="
            record_command perf record -F "$sample_freq" -g --call-graph dwarf -o "$data" -- \
                "$bin" --benchmark --frames "$frames" --draw "$mode"
            run_with_timeout perf record -F "$sample_freq" -g --call-graph dwarf -o "$data" -- \
                "$bin" --benchmark --frames "$frames" --draw "$mode" || true
        } >"$log" 2>&1

        if [[ -f "$data" ]]; then
            run_with_timeout perf report --stdio --no-inline --call-graph none --no-children \
                -i "$data" \
                >"$out_dir/perf-report-$mode.no-children.txt" 2>&1 || true
            run_with_timeout perf report --stdio --no-inline --call-graph none --children \
                -i "$data" \
                >"$out_dir/perf-report-$mode.children.txt" 2>&1 || true
        fi
    done

    scenario_data="$out_dir/perf-scene-app.data"
    scenario_record_log="$out_dir/perf-record-scene-app.log"
    echo "===== scene-app perf record ===== (logging to $scenario_record_log)"
    {
        echo "===== scene-app perf record ====="
        record_command perf record -F "$sample_freq" -g --call-graph dwarf \
            -o "$scenario_data" -- env DVZ_PRESENT_MODE=immediate \
            "$scenario_bin" --benchmark "$scenario_frames"
        run_with_timeout perf record -F "$sample_freq" -g --call-graph dwarf \
            -o "$scenario_data" -- env DVZ_PRESENT_MODE=immediate \
            "$scenario_bin" --benchmark "$scenario_frames" || true
    } >"$scenario_record_log" 2>&1
    if [[ -f "$scenario_data" ]]; then
        run_with_timeout perf report --stdio --no-inline --call-graph none --no-children \
            -i "$scenario_data" >"$out_dir/perf-report-scene-app.no-children.txt" 2>&1 || true
        run_with_timeout perf report --stdio --no-inline --call-graph none --children \
            -i "$scenario_data" >"$out_dir/perf-report-scene-app.children.txt" 2>&1 || true
    fi

    if [[ -f "$out_dir/perf-clear.data" && -f "$out_dir/perf-scene-drp2.data" ]]; then
        run_with_timeout perf diff "$out_dir/perf-clear.data" "$out_dir/perf-scene-drp2.data" \
            >"$out_dir/perf-diff-clear-vs-scene-drp2.txt" 2>&1 || true
    fi
else
    {
        echo "perf is unavailable in this environment."
        echo
        printf '%s\n' "$perf_unavailable_reason"
        echo
        echo "On Linux, this is commonly caused by a restrictive perf_event_paranoid setting."
        if [[ -r /proc/sys/kernel/perf_event_paranoid ]]; then
            echo "Current perf_event_paranoid: $(cat /proc/sys/kernel/perf_event_paranoid)"
        fi
    } >"$out_dir/perf-unavailable.txt"
fi

if ((run_nsys)); then
    if command -v nsys >/dev/null 2>&1; then
        nsys_base="$out_dir/nsys-scene-drp2"
        echo "===== scene-drp2 nsys profile ===== (logging to $out_dir/nsys-scene-drp2.log)"
        run_with_timeout nsys profile --force-overwrite=true --trace=vulkan,osrt --sample=cpu \
            --output="$nsys_base" \
            "$bin" --benchmark --frames "$frames" --draw scene-drp2 \
            >"$out_dir/nsys-scene-drp2.log" 2>&1 || true
        if [[ -f "$nsys_base.nsys-rep" ]]; then
            nsys stats "$nsys_base.nsys-rep" >"$out_dir/nsys-scene-drp2-stats.txt" 2>&1 || true
        fi
        scenario_nsys_base="$out_dir/nsys-scene-app"
        echo "===== scene-app nsys profile ===== (logging to $out_dir/nsys-scene-app.log)"
        run_with_timeout nsys profile --force-overwrite=true --trace=vulkan,osrt --sample=cpu \
            --env-var=DVZ_PRESENT_MODE=immediate --output="$scenario_nsys_base" \
            "$scenario_bin" --benchmark "$scenario_frames" \
            >"$out_dir/nsys-scene-app.log" 2>&1 || true
        if [[ -f "$scenario_nsys_base.nsys-rep" ]]; then
            nsys stats "$scenario_nsys_base.nsys-rep" \
                >"$out_dir/nsys-scene-app-stats.txt" 2>&1 || true
        fi
    else
        echo "nsys requested but not found" >"$out_dir/nsys-scene-drp2.log"
    fi
fi

echo "Profiling report complete: $out_dir"

cat >"$out_dir/README.txt" <<EOF
Live canvas profiling report

Start here:
  metadata.txt
  benchmark-clear.log
  benchmark-scene-drp2.log
  benchmark-scene-app.log
  perf-stat-clear.txt
  perf-stat-scene-drp2.txt
  perf-stat-scene-app.txt
  perf-diff-clear-vs-scene-drp2.txt
  perf-report-scene-drp2.children.txt
  perf-report-scene-drp2.no-children.txt
  perf-report-scene-app.children.txt
  perf-report-scene-app.no-children.txt

Raw perf data:
  perf-clear.data
  perf-scene-drp2.data
  perf-scene-app.data

Useful local commands:
  perf report -i "$out_dir/perf-scene-drp2.data"
  perf diff "$out_dir/perf-clear.data" "$out_dir/perf-scene-drp2.data"
EOF

echo "Done. Report directory: $out_dir"
