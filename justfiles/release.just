version:
    #!/usr/bin/env sh
    set -e
    VERSION=$(awk '
    /#define DVZ_VERSION_MAJOR/ { major = $3 }
    /#define DVZ_VERSION_MINOR/ { minor = $3 }
    /#define DVZ_VERSION_PATCH/ { patch = $3 }
    /#define DVZ_VERSION_DEV/ { dev = $3 }
    END { print major "." minor "." patch dev }
    ' "include/datoviz_version.h")
    echo ${VERSION}
#

version-bump version:
    @just bump "{{version}}"
#

bump version:
    #!/usr/bin/env python
    from datetime import datetime
    import os
    import re

    # Define the version
    version = "{{version}}"
    major, minor, patch = version.split('.')
    dev = ""
    if "-" in patch:
        patch, dev = patch.split("-")
        dev = "-" + dev
    # dev variable contains either "" or "-dev"

    # Function to update file content using regex
    def update_file(file_path, patterns_replacements):
        with open(file_path, 'r') as file:
            content = file.read()
        for pattern, replacement in patterns_replacements:
            content = re.sub(pattern, replacement, content, flags=re.MULTILINE)
        with open(file_path, 'w') as file:
            file.write(content)

    # Update the include file with the new version numbers
    include_file = "include/datoviz_version.h"
    include_patterns_replacements = [
        (r'#define DVZ_VERSION_MAJOR \d+', f'#define DVZ_VERSION_MAJOR {major}'),
        (r'#define DVZ_VERSION_MINOR \d+', f'#define DVZ_VERSION_MINOR {minor}'),
        (r'#define DVZ_VERSION_PATCH \d+', f'#define DVZ_VERSION_PATCH {patch}'),
        (r'#define DVZ_VERSION_DEVEL[^\n]*', f'#define DVZ_VERSION_DEVEL {dev}'.strip())
    ]
    update_file(include_file, include_patterns_replacements)
    print(f"Updated {include_file}")

    # Get today's date in ISO format (YYYY-MM-DD)
    today_date = datetime.now().strftime("%Y-%m-%d")

    # Update the CITATION.cff file with the new version number
    citation_file = "CITATION.cff"
    citation_patterns_replacements = [
        (r'^version: .+', f'version: {version}'),
        (r'^date-released: .+', f'date-released: {today_date}'),
    ]
    update_file(citation_file, citation_patterns_replacements)
    print(f"Updated {citation_file}")

    # Update the pyproject.toml file with the new version number
    toml_file = "pyproject.toml"
    toml_patterns_replacements = [
        (r'^version = ".+"', f'version = "{version}"')
    ]
    update_file(toml_file, toml_patterns_replacements)
    print(f"Updated {toml_file}")

    # Call the `just ctypes` command
    os.system("just ctypes")
    print("Updated ctypes wrapper")
#

tag version:
    git tag -a v{{version}} -m "v{{version}}"
#

release-tag version:
    @just tag "{{version}}"
#

runid:
    @echo "Use the manual .github/workflows/wheels.yml workflow for v0.4 RC wheel builds." >&2
    @echo "Query the selected GitHub Actions run ID explicitly before downloading artifacts." >&2
    @exit 1
#

# Download the built wheel artifacts
download:
    #!/usr/bin/env sh
    tag=$(git describe --tags --abbrev=0)
    echo "Tag: $tag"

    run_id=$(just runid)
    echo "Workflow run: $run_id"

    if [ -z "$run_id" ]; then
        echo "No successful wheel workflow run found"
        exit 1
    fi

    artifacts_dir="release_artifacts/$tag"
    if ! ls $artifacts_dir/*.whl 1> /dev/null 2>&1; then
        gh run download "$run_id" --dir "$artifacts_dir"
        find "$artifacts_dir" -mindepth 2 -type f -exec mv -t "$artifacts_dir" {} +
        find "$artifacts_dir" -type d -empty -delete
    fi
#

draft:
    #!/usr/bin/env sh
    just download

    tag=$(git describe --tags --abbrev=0)
    artifacts_dir="release_artifacts/$tag"
    gh release create "$tag" --draft --title "$tag" --notes "" $artifacts_dir/*
    # gh release upload "$tag"
#

release-draft:
    @just draft
#

upload:
    #!/usr/bin/env sh

    # Put this in your ~/.pypirc:
    # [pypi]
    #     username = __token__
    #     password = pypi-YOUR_API_TOKEN_HERE

    tag=$(git describe --tags --abbrev=0)
    artifacts_dir="release_artifacts/$tag"
    twine upload $artifacts_dir/*.whl
#

pypi-upload:
    @just upload
#

wheels:
    #!/usr/bin/env sh
    echo "The v0.4 wheel workflow is manual: .github/workflows/wheels.yml" >&2
    echo "Use local validation first: just wheel-ci-local <platform-tag>" >&2
    echo "Then dispatch the GitHub Actions workflow from the Actions tab or gh workflow run." >&2
    exit 1
#

nightly arg='':
    #!/usr/bin/env sh
    set -e

    DATE=$(date +%Y%m%d)
    VERSION_TAG="dev${DATE}"
    OUTDIR="dist"

    echo "📦 Building nightly wheel with tag: $VERSION_TAG and arg: {{arg}}"

    # Optionally clean the dist directory
    rm -rf $OUTDIR/*
    mkdir -p $OUTDIR

    # Build the wheel
    just wheel {{arg}}

    # Find the built wheel
    WHEEL=$(ls $OUTDIR/datoviz-*.whl | head -n 1)

    if [ ! -f "$WHEEL" ]; then
        echo "❌ No wheel found in $OUTDIR/"
        exit 1
    fi

    # Only rename if not already tagged
    BASENAME=$(basename "$WHEEL")
    if echo "$BASENAME" | grep -q "$VERSION_TAG"; then
        echo "✅ Wheel already tagged with $VERSION_TAG: $BASENAME"
    else
        NEWNAME=$(echo "$BASENAME" | sed "s/dev0/$VERSION_TAG/")
        if [ "$BASENAME" = "$NEWNAME" ]; then
            echo "ℹ️  No rename needed: $BASENAME == $NEWNAME"
        else
            echo "Renaming $BASENAME → $NEWNAME"
            mv "$WHEEL" "$OUTDIR/$NEWNAME"
        fi
    fi

    echo "✅ Nightly wheel ready: $OUTDIR/$(ls $OUTDIR | grep $VERSION_TAG)"
#

# Display the list of commits since the last tag.
commits since='' until='':
    #!/usr/bin/env sh
    set -e
    tag=""
    if [ -z "{{since}}" ]; then
        tag=$(git describe --tags --abbrev=0)
        since=$(git log -1 --date=format:'%Y-%m-%d' --format=%ad "${tag}")
    else
        since="{{since}}"
    fi
    if [ -z "{{until}}" ]; then
        until=$(date +%Y-%m-%d)
    else
        until="{{until}}"
    fi
    echo "commits between ${since} (tag: ${tag}) and ${until}:\n"
    git log --since="$since" --until="$until" --pretty=format:"%s" | sort | uniq
#

release-commits since='' until='':
    @just commits "{{since}}" "{{until}}"
#

release-preflight profile='rc1' *args='':
    @python3 tasks/release_preflight.py "{{profile}}" {{args}}
#


# -------------------------------------------------------------------------------------------------
dockerpush name:
    docker build -t rossant/datoviz_{{name}}:latest -f docker/Dockerfile_{{name}} .
    docker login
    docker push rossant/datoviz_{{name}}:latest
    # docker run -it rossant/datoviz_{{name}}:latest
#

docker-push name:
    @just dockerpush "{{name}}"
#

# on macOS do
# export DOCKER_HOST=$(docker context inspect | jq -r '.[0].Endpoints.docker.Host')
[linux]
[macos]
act arg:
    act --bind --env USING_ACT=1 -j {{arg}}
#

ci-act arg:
    @just act "{{arg}}"
#

[windows]
act arg:
