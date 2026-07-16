#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["pillow>=11,<12"]
# ///
"""Prepare a cache-only Gaia/2MASS celestial sky for the textured-planet showcase."""

from __future__ import annotations

import argparse
import csv
import io
import json
import math
import struct
import sys
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

from common import CACHE_ROOT, REPO_ROOT, artifact, write_manifest, write_provenance

try:
    from PIL import Image, ImageFilter
except ImportError as exc:  # pragma: no cover
    raise SystemExit('missing Pillow; run this script with `uv run`') from exc


EXAMPLE_ID = 'planet_sky'
DEFAULT_OUTPUT = CACHE_ROOT / EXAMPLE_ID
DEFAULT_ORBIT_METADATA = CACHE_ROOT / 'orbital_debris' / 'prepared' / 'metadata.json'
GAIA_TAP_URL = 'https://gea.esac.esa.int/tap-server/tap/sync'
MASS_URL = (
    'https://assets.science.nasa.gov/content/dam/science/psd/photojournal/'
    'pia/pia04/pia04250/PIA04250.jpg'
)
MASS_HAMMER_INSET = 0.98
MASS_EDGE_WHITE_MIN = 240
MASS_EDGE_ALPHA_MIN = 64
MAGIC = b'DVZSKY2\0'
VERSION = 2
SNAPSHOT_TEXT_SIZE = 32
STAR_LIMIT = 8000
GALAXY_TEXTURE_WIDTH = 1024
GALAXY_TEXTURE_HEIGHT = 512
SKY_RADIUS = 46.0


# ICRS-to-galactic rotation from the IAU J2000 definition. Its transpose maps galactic vectors
# back to ICRS before the snapshot sidereal rotation is applied.
ICRS_TO_GALACTIC = (
    (-0.0548755604, -0.8734370902, -0.4838350155),
    (+0.4941094279, -0.4448296300, +0.7469822445),
    (-0.8676661490, -0.1980763734, +0.4559837762),
)


def _download(url: str, path: Path, *, offline: bool, force: bool) -> None:
    if path.exists() and not force:
        return
    if offline:
        raise RuntimeError(f'offline source is missing: {path}')
    if urllib.parse.urlparse(url).scheme != 'https':
        raise RuntimeError(f'only HTTPS sources are accepted: {url}')
    request = urllib.request.Request(  # noqa: S310 - scheme validated above
        url, headers={'User-Agent': 'Datoviz/0.4 example preparer'}
    )
    with urllib.request.urlopen(request, timeout=120) as response:  # noqa: S310
        payload = response.read()
    if not payload:
        raise RuntimeError(f'empty source response: {url}')
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)


def _gaia_query_url(limit: int) -> str:
    query = (
        f'SELECT TOP {limit} source_id,ra,dec,phot_g_mean_mag,bp_rp '  # noqa: S608
        'FROM gaiadr3.gaia_source '
        'WHERE phot_g_mean_mag < 9 AND bp_rp IS NOT NULL '
        'ORDER BY phot_g_mean_mag ASC'
    )
    params = urllib.parse.urlencode(
        {'REQUEST': 'doQuery', 'LANG': 'ADQL', 'FORMAT': 'csv', 'QUERY': query}
    )
    return f'{GAIA_TAP_URL}?{params}'


def _snapshot(path: Path) -> datetime:
    metadata = json.loads(path.read_text(encoding='utf8'))
    value = str(metadata['start_utc']).replace('Z', '+00:00')
    return datetime.fromisoformat(value).astimezone(timezone.utc)


def _julian_date(timestamp: datetime) -> float:
    year = timestamp.year
    month = timestamp.month
    day = (
        timestamp.day
        + (timestamp.hour + (timestamp.minute + timestamp.second / 60.0) / 60.0) / 24.0
    )
    if month <= 2:
        year -= 1
        month += 12
    century = year // 100
    correction = 2 - century + century // 4
    return (
        math.floor(365.25 * (year + 4716))
        + math.floor(30.6001 * (month + 1))
        + day
        + correction
        - 1524.5
    )


def _gmst_radians(timestamp: datetime) -> float:
    julian_date = _julian_date(timestamp)
    centuries = (julian_date - 2451545.0) / 36525.0
    seconds = (
        67310.54841
        + (876600.0 * 3600.0 + 8640184.812866) * centuries
        + 0.093104 * centuries * centuries
        - 6.2e-6 * centuries * centuries * centuries
    )
    return (seconds % 86400.0) * (2.0 * math.pi / 86400.0)


def _icrs_to_visual(vector: tuple[float, float, float], gmst: float) -> tuple[float, float, float]:
    cosine = math.cos(gmst)
    sine = math.sin(gmst)
    x_icrs, y_icrs, z_icrs = vector
    x_fixed = cosine * x_icrs + sine * y_icrs
    y_fixed = -sine * x_icrs + cosine * y_icrs
    return -SKY_RADIUS * y_fixed, SKY_RADIUS * z_icrs, -SKY_RADIUS * x_fixed


def _galactic_to_icrs_vector(
    galactic: tuple[float, float, float],
) -> tuple[float, float, float]:
    return tuple(
        sum(ICRS_TO_GALACTIC[row][column] * galactic[row] for row in range(3))
        for column in range(3)
    )


def _galactic_to_icrs(longitude: float, latitude: float) -> tuple[float, float, float]:
    cosine_latitude = math.cos(latitude)
    return _galactic_to_icrs_vector(
        (
            cosine_latitude * math.cos(longitude),
            cosine_latitude * math.sin(longitude),
            math.sin(latitude),
        )
    )


def _star_color(bp_rp: float) -> tuple[int, int, int, int]:
    # Compact display mapping of Gaia BP-RP color index: hot blue stars through solar white to
    # cool orange stars. The catalog value, not a random palette, determines each color.
    value = max(-0.5, min(3.5, bp_rp))
    if value < 0.5:
        t = (value + 0.5) / 1.0
        rgb0, rgb1 = (150, 190, 255), (238, 244, 255)
    elif value < 1.5:
        t = value - 0.5
        rgb0, rgb1 = (238, 244, 255), (255, 228, 180)
    else:
        t = (value - 1.5) / 2.0
        rgb0, rgb1 = (255, 228, 180), (255, 142, 72)
    rgb = tuple(round(a + t * (b - a)) for a, b in zip(rgb0, rgb1, strict=True))
    return rgb[0], rgb[1], rgb[2], 255


def _load_gaia(
    path: Path, gmst: float
) -> list[tuple[tuple[float, float, float], tuple[int, ...], float]]:
    rows = csv.DictReader(io.StringIO(path.read_text(encoding='utf8')))
    stars = []
    for row in rows:
        right_ascension = math.radians(float(row['ra']))
        declination = math.radians(float(row['dec']))
        cosine_declination = math.cos(declination)
        icrs = (
            cosine_declination * math.cos(right_ascension),
            cosine_declination * math.sin(right_ascension),
            math.sin(declination),
        )
        magnitude = float(row['phot_g_mean_mag'])
        prominence = math.exp(-0.32 * max(0.0, magnitude + 1.0))
        size = 1.05 + 5.2 * prominence
        red, green, blue, _alpha = _star_color(float(row['bp_rp']))
        alpha = round(150 + 105 * math.exp(-0.24 * max(0.0, magnitude + 1.0)))
        stars.append((_icrs_to_visual(icrs, gmst), (red, green, blue, alpha), size))
    if not stars:
        raise RuntimeError('Gaia query returned no usable stars')
    return stars


def _hammer_forward(longitude: float, latitude: float) -> tuple[float, float]:
    """Project galactic longitude/latitude into the normalized source Hammer ellipse."""
    cosine_latitude = math.cos(latitude)
    denominator = math.sqrt(max(1e-12, 1.0 + cosine_latitude * math.cos(0.5 * longitude)))
    return (
        cosine_latitude * math.sin(0.5 * longitude) / denominator,
        math.sin(latitude) / denominator,
    )


def _sample_rgb(pixels, width: int, height: int, x: float, y: float) -> tuple[int, int, int]:
    """Bilinearly sample one RGB image at floating-point pixel coordinates."""
    x = max(0.0, min(width - 1.0, x))
    y = max(0.0, min(height - 1.0, y))
    x0 = int(math.floor(x))
    y0 = int(math.floor(y))
    x1 = min(x0 + 1, width - 1)
    y1 = min(y0 + 1, height - 1)
    tx = x - x0
    ty = y - y0
    samples = (pixels[x0, y0], pixels[x1, y0], pixels[x0, y1], pixels[x1, y1])
    weights = ((1.0 - tx) * (1.0 - ty), tx * (1.0 - ty), (1.0 - tx) * ty, tx * ty)
    return tuple(
        round(
            sum(
                weight * sample[channel]
                for weight, sample in zip(weights, samples, strict=True)
            )
        )
        for channel in range(3)
    )


def _load_2mass(path: Path) -> bytes:
    with Image.open(path) as image:
        image = image.convert('RGB')
        width, height = image.size
        # The stable PIA04250 JPEG includes title/credit margins around its Hammer all-sky map.
        crop = image.crop(
            (
                round(0.0205 * width),
                round(0.037 * height),
                round(0.954 * width),
                round(0.934 * height),
            )
        )
        pixels = crop.load()
        crop_width, crop_height = crop.size

        reprojected_pixels = []
        for row in range(GALAXY_TEXTURE_HEIGHT):
            latitude = math.pi * (0.5 - (row + 0.5) / GALAXY_TEXTURE_HEIGHT)
            for column in range(GALAXY_TEXTURE_WIDTH):
                # Put the equirectangular seam at the galactic anticenter, not through the bright
                # galactic center. The stored sky transform includes the matching half turn.
                longitude = 2.0 * math.pi * (column + 0.5) / GALAXY_TEXTURE_WIDTH - math.pi
                hammer_x, hammer_y = _hammer_forward(longitude, latitude)
                # PIA04250 is presentation artwork: the sky ellipse is surrounded by a white
                # background and annotations. Sampling the mathematical Hammer boundary therefore
                # picks up white border pixels, which become a large wedge at the sphere's UV seam.
                # Stay just inside the photographed sky without masking real bright sky pixels.
                hammer_x *= MASS_HAMMER_INSET
                hammer_y *= MASS_HAMMER_INSET
                source_x = 0.5 * (hammer_x + 1.0) * (crop_width - 1)
                source_y = 0.5 * (1.0 - hammer_y) * (crop_height - 1)
                reprojected_pixels.append(
                    _sample_rgb(pixels, crop_width, crop_height, source_x, source_y)
                )

        # Gaia carries the resolved stars. Blur the point-source survey just enough to retain the
        # real large-scale Milky Way structure without turning every source pixel into background
        # grain at gallery resolution.
        reprojected = Image.new('RGB', (GALAXY_TEXTURE_WIDTH, GALAXY_TEXTURE_HEIGHT))
        reprojected.putdata(reprojected_pixels)
        diffuse = reprojected.filter(ImageFilter.GaussianBlur(radius=3.0))
        texture = bytearray(GALAXY_TEXTURE_WIDTH * GALAXY_TEXTURE_HEIGHT * 4)
        for index, (red, green, blue) in enumerate(diffuse.getdata()):
            luminance = (0.2126 * red + 0.7152 * green + 0.0722 * blue) / 255.0
            signal = min(1.0, max(0.0, (luminance - 0.035) / 0.45))
            alpha = round(110.0 * signal**0.82)
            gain = 0.82 + 0.30 * math.sqrt(signal)
            offset = 4 * index
            texture[offset + 0] = min(255, round(red * gain))
            texture[offset + 1] = min(255, round(green * gain))
            texture[offset + 2] = min(255, round(blue * gain))
            texture[offset + 3] = alpha
    return bytes(texture)


def _rgba_edge_metrics(texture: bytes, width: int, height: int) -> tuple[int, int]:
    """Return maximum alpha and bright-artifact count on an RGBA8 texture boundary."""
    if len(texture) != 4 * width * height:
        raise RuntimeError('invalid RGBA texture size')
    edge_indices = [
        *(column for column in range(width)),
        *((height - 1) * width + column for column in range(width)),
        *(row * width for row in range(height)),
        *(row * width + width - 1 for row in range(height)),
    ]
    pixels = [texture[4 * index : 4 * index + 4] for index in edge_indices]
    max_alpha = max(pixel[3] for pixel in pixels)
    bright_artifact_count = sum(
        min(pixel[:3]) >= MASS_EDGE_WHITE_MIN and pixel[3] >= MASS_EDGE_ALPHA_MIN
        for pixel in pixels
    )
    return max_alpha, bright_artifact_count


def _galactic_visual_transform(gmst: float) -> tuple[float, ...]:
    """Return a row-major rotation from sky-sphere local coordinates to visual coordinates."""
    # The local sphere longitude is shifted by pi so the texture seam lies at the galactic
    # anticenter. Apply that half turn before the physical galactic-to-ICRS/snapshot rotation.
    galactic_basis = ((-1.0, 0.0, 0.0), (0.0, -1.0, 0.0), (0.0, 0.0, 1.0))
    columns = []
    for basis in galactic_basis:
        visual = _icrs_to_visual(_galactic_to_icrs_vector(basis), gmst)
        columns.append(tuple(component / SKY_RADIUS for component in visual))
    return tuple(columns[column][row] for row in range(3) for column in range(3))


def _write_layer(
    file, layer: list[tuple[tuple[float, float, float], tuple[int, ...], float]]
) -> None:
    for position, _color, _size in layer:
        file.write(struct.pack('<3f', *position))
    for _position, color, _size in layer:
        file.write(struct.pack('<4B', *color))
    for _position, _color, size in layer:
        file.write(struct.pack('<f', size))


def prepare(args: argparse.Namespace) -> Path:
    """Download, orient, and serialize the real celestial layers."""
    bundle = args.output.resolve()
    source = bundle / 'source'
    prepared = bundle / 'prepared'
    gaia_path = source / 'gaia_dr3_bright.csv'
    mass_path = source / 'PIA04250.jpg'
    _download(
        _gaia_query_url(args.star_limit),
        gaia_path,
        offline=args.offline,
        force=args.refresh_source,
    )
    _download(MASS_URL, mass_path, offline=args.offline, force=args.refresh_source)

    snapshot = _snapshot(args.orbit_metadata.resolve())
    snapshot_text = snapshot.isoformat().replace('+00:00', 'Z')
    gmst = _gmst_radians(snapshot)
    stars = _load_gaia(gaia_path, gmst)
    galaxy_texture = _load_2mass(mass_path)
    galaxy_edge_alpha_max, galaxy_bright_edge_artifact_count = _rgba_edge_metrics(
        galaxy_texture, GALAXY_TEXTURE_WIDTH, GALAXY_TEXTURE_HEIGHT
    )
    if galaxy_bright_edge_artifact_count != 0:
        raise RuntimeError(
            '2MASS reprojection retained bright source-artwork pixels on the texture boundary'
        )
    galaxy_transform = _galactic_visual_transform(gmst)

    prepared.mkdir(parents=True, exist_ok=True)
    binary_path = prepared / 'planet_sky.bin'
    encoded_snapshot = snapshot_text.encode('ascii')
    if len(encoded_snapshot) >= SNAPSHOT_TEXT_SIZE:
        raise RuntimeError('snapshot timestamp exceeds binary field')
    with binary_path.open('wb') as file:
        file.write(
            struct.pack(
                '<8sIIII32s',
                MAGIC,
                VERSION,
                len(stars),
                GALAXY_TEXTURE_WIDTH,
                GALAXY_TEXTURE_HEIGHT,
                encoded_snapshot.ljust(SNAPSHOT_TEXT_SIZE, b'\0'),
            )
        )
        file.write(struct.pack('<9f', *galaxy_transform))
        _write_layer(file, stars)
        file.write(galaxy_texture)

    write_manifest(
        bundle,
        example_id=EXAMPLE_ID,
        title='Gaia and 2MASS Planet Sky',
        status='prepared-cache',
        script='tools/data/prepare_planet_sky.py',
        command=sys.argv,
        source={
            'kind': 'real-data',
            'gaia': 'Gaia DR3 bright-star astrometry and BP/RP photometry',
            'mass': '2MASS Point Source Catalog/Stars All Sky View, PIA04250',
            'snapshot_utc': snapshot_text,
        },
        artifacts=[artifact(binary_path, bundle, 'render_ready_sky', 'DVZSKY2')],
        validation={
            'star_count': len(stars),
            'galaxy_texture_width': GALAXY_TEXTURE_WIDTH,
            'galaxy_texture_height': GALAXY_TEXTURE_HEIGHT,
            'galaxy_texture_edge_alpha_max': galaxy_edge_alpha_max,
            'galaxy_texture_bright_edge_artifact_count': galaxy_bright_edge_artifact_count,
        },
        extra={'notes': ['Prepared under .cache; the data submodule is not modified.']},
    )
    write_provenance(
        bundle,
        title='Gaia and 2MASS Planet Sky',
        source_lines=[
            'Gaia DR3 gaia_source through the official ESA TAP service.',
            'NASA Photojournal PIA04250, Two Micron All Sky Survey.',
        ],
        processing_lines=[
            f'Selected {len(stars)} bright Gaia stars by G magnitude with BP-RP colors.',
            'Reprojected the 2MASS Hammer all-sky map to a continuous equirectangular texture '
            'with an inset that excludes the source artwork border.',
            f'Oriented both celestial layers to Earth at {snapshot_text} using GMST.',
        ],
        license_lines=[
            'Gaia data: ESA/Gaia/DPAC acknowledgement and citation requirements apply.',
            '2MASS image credit: 2MASS/IPAC-Caltech/University of Massachusetts.',
        ],
        notes=[
            'The 2MASS layer is an infrared false-color survey view, not naked-eye visible light.',
            'Celestial distance is display-only; angular positions and orientation carry meaning.',
        ],
    )
    return bundle


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument('--orbit-metadata', type=Path, default=DEFAULT_ORBIT_METADATA)
    parser.add_argument('--star-limit', type=int, default=STAR_LIMIT)
    parser.add_argument('--offline', action='store_true')
    parser.add_argument('--force', action='store_true')
    parser.add_argument('--refresh-source', action='store_true')
    return parser


def main() -> int:
    """Run the cache-only planet-sky preparation workflow."""
    args = _parser().parse_args()
    if args.star_limit <= 0:
        raise SystemExit('star limit must be positive')
    try:
        bundle = prepare(args)
    except (OSError, RuntimeError, ValueError, KeyError, csv.Error) as exc:
        raise SystemExit(f'planet-sky preparation failed: {exc}') from exc
    print(f'prepared real Gaia/2MASS sky bundle: {bundle.relative_to(REPO_ROOT)}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
