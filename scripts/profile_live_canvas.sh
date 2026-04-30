#!/usr/bin/env bash
set -euo pipefail

frames=50000
stat_runs=5
bench_runs=3
sample_freq=999
profile_timeout=120
out_dir=""
run_nsys=0
bin="./build/testing/dvz_live_canvas"

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
  --bin PATH       dvz_live_canvas path (default: ./build/testing/dvz_live_canvas)
  --nsys           Also capture an Nsight Systems Vulkan/OS runtime trace for scene-drp2
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
if [[ ! -x "$bin" ]]; then
    echo "error: '$bin' is missing or not executable; run 'just build RelWithDebInfo' first" >&2
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

for mode in clear scene-drp2; do
    log="$out_dir/perf-stat-$mode.txt"
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

if ((perf_available)); then
    for mode in clear scene-drp2; do
        data="$out_dir/perf-$mode.data"
        log="$out_dir/perf-record-$mode.log"
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
        run_with_timeout nsys profile --force-overwrite=true --trace=vulkan,osrt --sample=cpu \
            --output="$nsys_base" \
            "$bin" --benchmark --frames "$frames" --draw scene-drp2 \
            >"$out_dir/nsys-scene-drp2.log" 2>&1 || true
        if [[ -f "$nsys_base.nsys-rep" ]]; then
            nsys stats "$nsys_base.nsys-rep" >"$out_dir/nsys-scene-drp2-stats.txt" 2>&1 || true
        fi
    else
        echo "nsys requested but not found" >"$out_dir/nsys-scene-drp2.log"
    fi
fi

cat >"$out_dir/README.txt" <<EOF
Live canvas profiling report

Start here:
  metadata.txt
  benchmark-clear.log
  benchmark-scene-drp2.log
  perf-stat-clear.txt
  perf-stat-scene-drp2.txt
  perf-diff-clear-vs-scene-drp2.txt
  perf-report-scene-drp2.children.txt
  perf-report-scene-drp2.no-children.txt

Raw perf data:
  perf-clear.data
  perf-scene-drp2.data

Useful local commands:
  perf report -i "$out_dir/perf-scene-drp2.data"
  perf diff "$out_dir/perf-clear.data" "$out_dir/perf-scene-drp2.data"
EOF

echo "Done. Report directory: $out_dir"
