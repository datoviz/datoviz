"""Generate a compact deterministic image-embedding toy corpus bundle."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import zipfile
from pathlib import Path

import numpy as np


DEFAULT_OUTPUT = Path("data/examples/generated/image_embedding_toy")
SEED = 20260517
GENERATED_AT = "2026-05-17T00:00:00Z"


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _write_json(path: Path, payload: dict) -> None:
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf8")


def _write_npz(path: Path, **arrays: np.ndarray) -> None:
    with zipfile.ZipFile(path, "w") as archive:
        for name, array in arrays.items():
            buffer = io.BytesIO()
            np.save(buffer, np.asarray(array), allow_pickle=False)
            info = zipfile.ZipInfo(f"{name}.npy", (2026, 5, 17, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, buffer.getvalue(), compress_type=zipfile.ZIP_DEFLATED)


def _render_pattern(label: int, size: int, rng: np.random.Generator) -> np.ndarray:
    grid = np.linspace(-1.0, 1.0, size, dtype=np.float32)
    xx, yy = np.meshgrid(grid, grid, indexing="xy")
    angle = rng.uniform(-0.35, 0.35)
    ca = np.cos(angle)
    sa = np.sin(angle)
    xr = ca * xx - sa * yy
    yr = sa * xx + ca * yy
    if label == 0:
        image = np.exp(-8.0 * ((xr + 0.15) ** 2 + (yr - 0.1) ** 2))
    elif label == 1:
        image = np.exp(-65.0 * np.abs(xr - 0.1))
    elif label == 2:
        radius = np.sqrt((xr - 0.04) ** 2 + (yr + 0.04) ** 2)
        image = np.exp(-180.0 * (radius - 0.48) ** 2)
    else:
        image = ((np.abs(xr) < 0.14) | (np.abs(yr) < 0.14)).astype(np.float32)
    image += rng.normal(0.0, 0.045, image.shape).astype(np.float32)
    image = np.clip(image, 0.0, 1.0)
    return (255.0 * image).astype(np.uint8)


def generate_bundle(output: Path, count: int, size: int) -> None:
    rng = np.random.default_rng(SEED)
    output.mkdir(parents=True, exist_ok=True)

    labels = np.arange(count, dtype=np.uint16) % 4
    images = np.empty((count, size, size), dtype=np.uint8)
    embedding_2d = np.empty((count, 2), dtype=np.float32)
    embedding_8d = np.empty((count, 8), dtype=np.float32)
    centers = np.array([[-1.0, -0.6], [0.85, -0.7], [-0.75, 0.8], [0.9, 0.75]], dtype=np.float32)
    names = np.array(["blob", "bar", "ring", "cross"])

    for idx, label in enumerate(labels):
        label_int = int(label)
        images[idx] = _render_pattern(label_int, size, rng)
        local = rng.normal(0.0, 0.12, 2).astype(np.float32)
        embedding_2d[idx] = centers[label_int] + local
        moments = np.array(
            [
                images[idx].mean() / 255.0,
                images[idx].std() / 255.0,
                embedding_2d[idx, 0],
                embedding_2d[idx, 1],
                np.count_nonzero(images[idx] > 128) / float(size * size),
                label_int / 3.0,
                np.sin(idx * 0.17),
                np.cos(idx * 0.11),
            ],
            dtype=np.float32,
        )
        embedding_8d[idx] = moments

    data_path = output / "data.npz"
    _write_npz(
        data_path,
        images=images,
        labels=labels,
        label_names=names,
        embedding_2d=embedding_2d,
        embedding_8d=embedding_8d,
    )

    manifest = {
        "name": "image_embedding_toy",
        "kind": "synthetic_image_embedding",
        "version": 1,
        "files": {"data.npz": _sha256(data_path)},
        "arrays": {
            "images": list(images.shape),
            "labels": list(labels.shape),
            "label_names": list(names.shape),
            "embedding_2d": list(embedding_2d.shape),
            "embedding_8d": list(embedding_8d.shape),
        },
        "labels": {str(i): name for i, name in enumerate(names.tolist())},
    }
    _write_json(output / "manifest.json", manifest)
    _write_json(
        output / "provenance.json",
        {
            "generated_at": GENERATED_AT,
            "generator": Path(__file__).as_posix(),
            "seed": SEED,
            "method": (
                "Procedural grayscale glyph corpus with clustered low-dimensional embeddings"
            ),
            "parameters": {"count": count, "size": size},
        },
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--count", type=int, default=96)
    parser.add_argument("--size", type=int, default=28)
    args = parser.parse_args()
    generate_bundle(args.output, args.count, args.size)


if __name__ == "__main__":
    main()
