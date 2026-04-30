#!/usr/bin/env bash
set -euo pipefail

frames=100000
draw_mode="scene-drp2"
interval="0.2"
out_dir=""
bin="./build-profile/testing/dvz_live_canvas"

usage() {
    cat <<'EOF'
usage: scripts/memory_live_canvas.sh [options]

Run dvz_live_canvas while sampling Linux /proc RSS/HWM memory counters.

Options:
  --frames N       Frames to run (default: 100000)
  --draw MODE      Draw mode: clear or scene-drp2 (default: scene-drp2)
  --interval S     Sampling interval in seconds (default: 0.2)
  --out DIR        Output directory (default: build/profiles/live-canvas-memory-<timestamp>)
  --bin PATH       dvz_live_canvas path (default: ./build-profile/testing/dvz_live_canvas)
  -h, --help       Show this help

Example:
  just memory-canvas-release --frames 100000 --draw scene-drp2
EOF
}

while (($# > 0)); do
    case "$1" in
        --frames)
            frames="${2:?missing value for --frames}"
            shift 2
            ;;
        --draw)
            draw_mode="${2:?missing value for --draw}"
            shift 2
            ;;
        --interval)
            interval="${2:?missing value for --interval}"
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
if [[ "$draw_mode" != "clear" && "$draw_mode" != "scene-drp2" ]]; then
    echo "error: --draw must be 'clear' or 'scene-drp2'" >&2
    exit 1
fi
if [[ ! "$interval" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
    echo "error: --interval must be a positive number" >&2
    exit 1
fi
if ! awk "BEGIN { exit !($interval > 0) }"; then
    echo "error: --interval must be greater than zero" >&2
    exit 1
fi
if [[ ! -x "$bin" ]]; then
    echo "error: '$bin' is missing or not executable; run 'just build-profile' first" >&2
    exit 1
fi

if [[ -z "$out_dir" ]]; then
    out_dir="build/profiles/live-canvas-memory-$(date +%Y%m%d-%H%M%S)"
fi
mkdir -p "$out_dir"

benchmark_log="$out_dir/benchmark.log"
rss_tsv="$out_dir/rss.tsv"
summary="$out_dir/summary.txt"
metadata="$out_dir/metadata.txt"

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
    echo "draw: $draw_mode"
    echo "interval: $interval"
    echo
    echo "binary file:"
    file "$bin" || true
    echo
    echo "linked libraries:"
    ldd "$bin" 2>/dev/null || true
} >"$metadata"

echo "Writing live-canvas memory report to $out_dir"

start_epoch="$(date +%s.%N)"
"$bin" --benchmark --frames "$frames" --draw "$draw_mode" >"$benchmark_log" 2>&1 &
pid=$!

printf "elapsed_s\tvmrss_kb\tvmhwm_kb\n" >"$rss_tsv"

peak_rss=0
last_rss=0
first_rss=""
last_hwm=0
sample_count=0

while kill -0 "$pid" 2>/dev/null; do
    status="/proc/$pid/status"
    if [[ -r "$status" ]]; then
        now="$(date +%s.%N)"
        elapsed="$(awk -v now="$now" -v start="$start_epoch" 'BEGIN { printf "%.6f", now - start }')"
        rss="$(awk '/^VmRSS:/ { print $2 }' "$status")"
        hwm="$(awk '/^VmHWM:/ { print $2 }' "$status")"
        rss="${rss:-0}"
        hwm="${hwm:-0}"
        printf "%s\t%s\t%s\n" "$elapsed" "$rss" "$hwm" >>"$rss_tsv"
        if [[ -z "$first_rss" && "$rss" != "0" ]]; then
            first_rss="$rss"
        fi
        last_rss="$rss"
        last_hwm="$hwm"
        if ((rss > peak_rss)); then
            peak_rss="$rss"
        fi
        if ((hwm > peak_rss)); then
            peak_rss="$hwm"
        fi
        sample_count=$((sample_count + 1))
    fi
    sleep "$interval"
done

set +e
wait "$pid"
exit_code=$?
set -e

if [[ -z "$first_rss" ]]; then
    first_rss=0
fi

rss_delta=$((last_rss - first_rss))

{
    echo "command: $(record_command "$bin" --benchmark --frames "$frames" --draw "$draw_mode")"
    echo "exit_code: $exit_code"
    echo "samples: $sample_count"
    echo "first_vmrss_kb: $first_rss"
    echo "last_vmrss_kb: $last_rss"
    echo "delta_vmrss_kb: $rss_delta"
    echo "peak_vmrss_or_vmhwm_kb: $peak_rss"
    echo "last_vmhwm_kb: $last_hwm"
    echo
    echo "benchmark_tail:"
    tail -40 "$benchmark_log" || true
} >"$summary"

cat >"$out_dir/README.txt" <<EOF
Live canvas memory report

Start here:
  summary.txt
  benchmark.log
  rss.tsv
  metadata.txt

Columns in rss.tsv:
  elapsed_s: seconds since launch
  vmrss_kb: current resident set size from /proc/<pid>/status
  vmhwm_kb: high-water resident set size from /proc/<pid>/status
EOF

cat "$benchmark_log"
echo "Done. Report directory: $out_dir"
exit "$exit_code"
