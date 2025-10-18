"""
# Matplotlib/Datoviz performance comparison

On a simple interactive 2D scatter plot with axes.

---
tags:
  - benchmark
  - matplotlib
  - scatter
in_gallery: false
make_screenshot: false
---

"""

import argparse
import json
import os
import platform
import shutil
import subprocess
import time
import uuid
from pathlib import Path

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import psutil

import datoviz as dvz

# -------------------------------------------------------------------------------------------------
# Benchmark config
# -------------------------------------------------------------------------------------------------

plt.rcParams['axes.linewidth'] = 0.75
plt.rcParams['xtick.major.width'] = 0.75
plt.rcParams['ytick.major.width'] = 0.75
plt.rcParams['xtick.labelsize'] = 6
plt.rcParams['ytick.labelsize'] = 6

COLORS = {
    'matplotlib': '#4698c6',
    'datoviz': '#554596',
}


def format_eng(n, _):
    if n >= 1e6:
        return f'{int(n / 1e6)}M'
    elif n >= 1e3:
        return f'{int(n / 1e3)}k'
    else:
        return str(int(n))


N_VALUES = [int(1e3), int(1e4), int(1e5), int(1e6), int(1e7), int(1e8)]

W, H = 800, 600
DPI = 200.0
XLIM = (0, 1)
YLIM = (0, 1)
FACTOR = 1.02
MAX_FRAMES = 100

CURDIR = Path(__file__).parent.resolve()
BENCHMARK_ID = str(uuid.uuid4())[:8]
BENCHMARK_ID = 'debug'  # DEBUG
RESULTS_PATH = CURDIR / f'benchmark_results_{BENCHMARK_ID}.json'


# -------------------------------------------------------------------------------------------------
# Update functions
# -------------------------------------------------------------------------------------------------


def zoom_datoviz(panel, total_zoom: float):
    """
    Set the total zoom level of a Datoviz panel to the given scalar value.

    Parameters
    ----------
    panel : dvz.Panel
        The Datoviz panel.
    total_zoom : float
        The desired total zoom level (e.g., 2.0 to zoom in, 0.5 to zoom out).
    """
    panzoom = panel.panzoom()
    panzoom.zoom(total_zoom, total_zoom)


def zoom_matplotlib(ax, total_zoom: float):
    """
    Set the total zoom level of a Matplotlib Axes to the given scalar value.

    Parameters
    ----------
    ax : matplotlib.axes.Axes
        The Matplotlib axes.
    total_zoom : float
        The desired total zoom level (e.g., 2.0 to zoom in, 0.5 to zoom out).
    """
    xlim = ax.get_xlim()
    ylim = ax.get_ylim()

    x_center = 0.5 * (xlim[0] + xlim[1])
    y_center = 0.5 * (ylim[0] + ylim[1])

    x_range = (xlim[1] - xlim[0]) / total_zoom
    y_range = (ylim[1] - ylim[0]) / total_zoom

    ax.set_xlim(x_center - x_range / 2, x_center + x_range / 2)
    ax.set_ylim(y_center - y_range / 2, y_center + y_range / 2)


# -------------------------------------------------------------------------------------------------
# Generate shared data
# -------------------------------------------------------------------------------------------------


def generate_data(n):
    rng = np.random.default_rng(3141)
    x, y = rng.random((2, n))
    color = rng.integers(100, 240, size=(n, 4), dtype=np.uint8)
    color[:, 3] = 255
    size = rng.uniform(5, 10, size=n)
    return x, y, color, color / 255.0, size


# Store generated data for all n
DATA = {n: generate_data(n) for n in N_VALUES}

# -------------------------------------------------------------------------------------------------
# Benchmark Datoviz
# -------------------------------------------------------------------------------------------------


def run_datoviz(n, benchmark=True, screenshot=False):
    if screenshot:
        os.environ['DVZ_CAPTURE_PNG'] = str(CURDIR / f'screenshot_dvz_{n}.png')

    x, y, color_uint8, _, size = DATA[n]
    app = dvz.App(background='white')
    figure = app.figure(W, H)
    panel = figure.panel()
    axes = panel.axes(XLIM, YLIM)
    visual = app.point(position=axes.normalize(x, y), color=color_uint8, size=size)
    panel.add(visual)

    t_start = time.perf_counter()
    frame_count = 0
    zoom_factor = 1.0
    t_first_frame = None

    @app.connect(figure)
    def on_frame(ev):
        nonlocal frame_count, t_first_frame, zoom_factor

        if frame_count % 10 == 0 and frame_count < MAX_FRAMES:
            zoom_factor *= 1.1 * FACTOR  # tweak to ~ match matplotlib (?)
            zoom_datoviz(panel, zoom_factor)
            panel.update()

            # DEBUG
            # time.sleep(0.1)

        if benchmark and frame_count >= MAX_FRAMES:
            app.stop()

        now = time.perf_counter()
        if t_first_frame is None:
            t_first_frame = now

        frame_count += 1

    if not benchmark:
        app.run()
        app.destroy()
        return

    app.run()
    t_end = time.perf_counter()
    app.destroy()

    actual_duration = t_end - (t_first_frame or 0)
    fps = frame_count / actual_duration

    return {
        'backend': 'datoviz',
        'n': n,
        'fps': fps,
        'first_frame_time': t_first_frame - t_start if t_first_frame else None,
    }


# -------------------------------------------------------------------------------------------------
# Benchmark Matplotlib
# -------------------------------------------------------------------------------------------------


def run_matplotlib(n, benchmark=True, screenshot=False):
    # Too slow
    if n > 1e6:
        return {
            'backend': 'matplotlib',
            'n': n,
            'fps': None,
            'first_frame_time': None,
        }

    x, y, _, color_float, size = DATA[n]
    fig, ax = plt.subplots(figsize=(W / DPI, H / DPI), dpi=DPI)
    ax.set_xlim(XLIM)
    ax.set_ylim(YLIM)
    ax.set_facecolor('white')
    ax.scatter(x, y, s=size, c=color_float, marker='o', linewidths=0)
    plt.tight_layout()
    plt.show(block=False)

    if screenshot:
        fig.savefig(CURDIR / f'screenshot_mpl_{n}.png', dpi=150)

    t_start = time.perf_counter()
    frame_count = 0
    zoom_factor = 1.0
    t_first_frame = None

    while True:
        if frame_count % 10 == 0:
            zoom_factor *= FACTOR
            zoom_matplotlib(ax, zoom_factor)

        fig.canvas.draw()
        plt.pause(0.001)

        if frame_count >= MAX_FRAMES:
            break

        now = time.perf_counter()
        if t_first_frame is None:
            t_first_frame = now

        frame_count += 1

    if not benchmark:
        plt.show()
        return

    t_end = time.perf_counter()
    plt.close(fig)

    actual_duration = t_end - (t_first_frame or 0)
    fps = frame_count / actual_duration

    return {
        'backend': 'matplotlib',
        'n': n,
        'fps': fps,
        'first_frame_time': t_first_frame - t_start if t_first_frame else None,
    }


# -------------------------------------------------------------------------------------------------
# System Info
# -------------------------------------------------------------------------------------------------


def get_system_info():
    info = {
        'platform': platform.system(),
        'platform_release': platform.release(),
        'platform_version': platform.version(),
        'architecture': platform.machine(),
        'cpu_count': psutil.cpu_count(logical=True),
        'cpu_model': platform.processor(),
        'ram_total_gb': round(psutil.virtual_memory().total / 1e9, 2),
    }
    try:
        import GPUtil

        gpus = GPUtil.getGPUs()
        if gpus:
            info['gpu'] = {
                'name': gpus[0].name,
                'memory_total': gpus[0].memoryTotal,
                'driver': gpus[0].driver,
            }
    except ImportError:
        info['gpu'] = None

    # Matplotlib version
    info['matplotlib'] = {
        'version': matplotlib.__version__,
    }

    # Datoviz version
    info['datoviz'] = dvz.get_version()

    try:
        # Git branch and commit (assuming it's a local repo)
        repo_dir = os.path.dirname(dvz.__file__)
        git_exe = shutil.which('git')
        branch = (
            subprocess.check_output(
                [git_exe, '-C', repo_dir, 'rev-parse', '--abbrev-ref', 'HEAD'],
                stderr=subprocess.DEVNULL,
            )
            .decode()
            .strip()
        )
        commit = (
            subprocess.check_output(
                [git_exe, '-C', repo_dir, 'rev-parse', 'HEAD'], stderr=subprocess.DEVNULL
            )
            .decode()
            .strip()
        )
        info['datoviz']['git_branch'] = branch
        info['datoviz']['git_commit'] = commit

    except Exception:
        pass

    return info


# -------------------------------------------------------------------------------------------------
# Plotting
# -------------------------------------------------------------------------------------------------


def plot_results():
    COLORS = {
        'matplotlib': '#4698c6',
        'datoviz': '#554596',
    }

    path = RESULTS_PATH
    if not path.exists():
        print(f'Benchmark results not found: {path}')
        return

    with open(path) as f:
        data = json.load(f)

    results = data['results']
    backends = ['matplotlib', 'datoviz']
    metrics = ['first_frame_time', 'fps']
    metric_labels = {
        'first_frame_time': 'Time to first frame (s) (lower is better)',
        'fps': 'Frames per second (FPS) (higher is better)',
    }
    ylabels = {
        'first_frame_time': 'seconds',
        'fps': 'FPS',
    }

    ns_all = sorted(set(r['n'] for r in results))
    x = np.arange(len(ns_all))  # positions on x-axis
    width = 0.35

    fig, axes = plt.subplots(2, 2, figsize=(8, 6), dpi=150, sharex=True)
    fig.subplots_adjust(hspace=0.4)
    axes[0][0].set_yscale('log')
    axes[0][1].set_yscale('log')

    for col, metric in enumerate(metrics):
        ax = axes[0, col]
        ax_ratio = axes[1, col]
        ax_ratio.axhline(1.0, linestyle='--', color='gray', linewidth=1.0, alpha=0.6)

        values = {backend: [] for backend in backends}
        ratios = []

        for n in ns_all:
            vals = {}
            for backend in backends:
                match = next((r for r in results if r['backend'] == backend and r['n'] == n), None)
                val = match.get(metric) if match else None
                vals[backend] = val

            for backend in backends:
                values[backend].append(vals[backend])

            # Speed ratio (only if both present and matplotlib > 0)
            a = vals['datoviz']
            b = vals['matplotlib']
            ratio = a / b if a is not None and b and b > 0 else None
            if col == 0 and ratio is not None:
                ratio = 1.0 / ratio
            ratios.append(ratio)

        # Bar plot (top row)
        for i, backend in enumerate(backends):
            offset = -width / 2 if backend == 'matplotlib' else width / 2
            y = [v if v is not None else np.nan for v in values[backend]]
            ax.bar(x + offset, y, width=width, label=backend, color=COLORS[backend])

        ax.set_title(metric_labels[metric], fontsize=10)

        ax.set_xticks(x)
        ax.set_xticklabels([format_eng(n, None) for n in ns_all], fontsize=8)

        ax.tick_params(axis='y', labelsize=8)
        ax.set_ylabel(ylabels[metric], fontsize=9)

        ax.grid(True, linestyle='--', alpha=0.3)

        # Speed ratio (bottom row)
        valid_ratios = [r if r is not None else 0 for r in ratios]
        mask = [r is not None for r in ratios]
        ratio_vals = np.array(valid_ratios)

        bar = ax_ratio.bar(x, ratio_vals, width=0.5, color='#CE533B')
        for i, r in enumerate(ratios):
            if r is not None:
                ax_ratio.text(x[i], r + 0.05, f'{r:.1f}×', ha='center', va='bottom', fontsize=7)

        ax_ratio.set_xticks(x)
        ax_ratio.set_xticklabels([format_eng(n, None) for n in ns_all], fontsize=8)

        ax_ratio.tick_params(axis='y', labelsize=8)
        ax_ratio.set_ylabel('Speedup', fontsize=9)
        ax_ratio.set_ylim(0, max(r for r in ratios if r) * 1.3)

        ax_ratio.grid(True, linestyle='--', alpha=0.3)

    axes[0, 0].legend(fontsize=8, loc='upper right', frameon=False)
    fig.suptitle('Datoviz vs Matplotlib Performance (scatter plot)', fontsize=12)
    plt.tight_layout()
    fig.savefig(CURDIR / 'benchmark.png', dpi=150)
    plt.show()


# -------------------------------------------------------------------------------------------------
# Main benchmark loop
# -------------------------------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(description='Datoviz vs Matplotlib benchmark.')
    parser.add_argument('--benchmark', action='store_true', help='Run the benchmark')
    parser.add_argument('--plot', action='store_true', help='Plot benchmark results')
    parser.add_argument(
        '--show', choices=['matplotlib', 'datoviz'], help='Show one plot interactively'
    )
    parser.add_argument('--screenshot', action='store_true', help='Save a PNG screenshot')
    args = parser.parse_args()

    if not any([args.benchmark, args.plot, args.show]):
        parser.print_help()
        return

    if args.plot:
        plot_results()
        return

    if args.show:
        n = N_VALUES[0]
        if args.show == 'matplotlib':
            run_matplotlib(n, benchmark=False, screenshot=args.screenshot)
        elif args.show == 'datoviz':
            run_datoviz(n, benchmark=False, screenshot=args.screenshot)
        return

    # Benchmark mode
    if args.benchmark:
        results = {
            'system': get_system_info(),
            'timestamp': time.strftime('%Y-%m-%d %H:%M:%S'),
            'frames': MAX_FRAMES,
            'window_size': [W, H],
            'results': [],
        }

        for n in N_VALUES:
            print(f'Running Datoviz benchmark for n={n}')
            result_dvz = run_datoviz(n, benchmark=True, screenshot=args.screenshot)
            results['results'].append(result_dvz)

            print(f'Running Matplotlib benchmark for n={n}')
            result_mpl = run_matplotlib(n, benchmark=True, screenshot=args.screenshot)
            results['results'].append(result_mpl)

        if RESULTS_PATH.exists():
            with open(RESULTS_PATH) as f:
                existing = json.load(f)

            # Index existing results by (backend, n)
            result_map = {(r['backend'], r['n']): r for r in existing['results']}

            # Overwrite or add new results
            for r in results['results']:
                result_map[(r['backend'], r['n'])] = r

            # Rebuild merged result list
            merged_results = list(result_map.values())

            # Update metadata
            existing['results'] = merged_results
            existing['timestamp'] = results['timestamp']
            existing['frames'] = results['frames']
            existing['window_size'] = results['window_size']
            existing['system'] = results['system']
            results = existing

        with open(RESULTS_PATH, 'w') as f:
            json.dump(results, f, indent=2)

        print(f'Benchmark saved to {RESULTS_PATH}')


if __name__ == '__main__':
    main()
