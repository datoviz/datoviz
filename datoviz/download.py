"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Data downloader

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from pathlib import Path

# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------

ORG = 'datoviz'
REPO = 'data'
BRANCH = 'main'
RAW_URL_BASE = f'https://raw.githubusercontent.com/{ORG}/{REPO}/{BRANCH}'
MEDIA_URL_BASE = f'https://media.githubusercontent.com/media/{ORG}/{REPO}/{BRANCH}'


# -------------------------------------------------------------------------------------------------
# Functions
# -------------------------------------------------------------------------------------------------


def is_lfs_pointer(content_bytes: bytes) -> bool:
    """
    Check whether the file content is a Git LFS pointer file.
    """
    text = content_bytes.decode('utf-8', errors='ignore')
    return text.startswith('version https://git-lfs.github.com') and 'oid sha256:' in text


def download_data(rel_path: str, force_download: bool = False) -> Path:
    """
    Download a file from the datoviz/data GitHub repo into a local cache directory, handling Git
    LFS if needed.

    Parameters
    ----------
    rel_path : str
        Relative path within the repository (e.g., "misc/lidar.npz").
    force_download : bool
        If True, redownload even if cached.

    Returns
    -------
    Path
        Local path to the cached or downloaded file.
    """
    import requests
    from platformdirs import user_cache_dir

    # Resolve local cache path
    cache_dir = Path(user_cache_dir('datoviz')) / 'data'
    file_path = cache_dir / rel_path
    file_path.parent.mkdir(parents=True, exist_ok=True)

    if file_path.exists() and not force_download:
        return file_path

    raw_url = f'{RAW_URL_BASE}/{rel_path}'
    print(f'Checking: {raw_url}')

    try:
        r = requests.get(raw_url, stream=True, timeout=30)
        r.raise_for_status()
        content = r.content

        if is_lfs_pointer(content):
            # It's a Git LFS pointer
            media_url = f'{MEDIA_URL_BASE}/{rel_path}'
            print(f'Detected Git LFS file. Downloading from: {media_url}')
            r = requests.get(media_url, stream=True, timeout=60)
            r.raise_for_status()
            with open(file_path, 'wb') as f:
                for chunk in r.iter_content(chunk_size=8192):
                    f.write(chunk)
        else:
            # Normal file
            print(f'Downloading file from: {raw_url}')
            with open(file_path, 'wb') as f:
                f.write(content)

    except Exception as e:
        raise RuntimeError(f'Failed to download {rel_path}: {e}') from e

    print(f'Saved to: {file_path}')
    return file_path
