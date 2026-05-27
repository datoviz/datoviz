from pathlib import Path


QUERY_DIR = Path("src/scene/query")

FORBIDDEN_PATTERNS = (
    "DVZ_VISUAL_TYPE_",
    "_attr_index(",
    "visual->attrs",
    "visual->field",
    "visual->texture",
    "visual->volume",
    "visual->path",
    "visual->mesh",
    "visual->buffer",
)


def test_scene_query_core_has_no_visual_family_internals() -> None:
    if not QUERY_DIR.exists():
        return

    checked = False
    for path in sorted(QUERY_DIR.rglob("*")):
        if path.suffix not in {".c", ".cc", ".cpp", ".h", ".hpp"}:
            continue
        checked = True
        text = path.read_text(encoding="utf-8")
        for pattern in FORBIDDEN_PATTERNS:
            assert pattern not in text, f"{path} contains forbidden query-core pattern {pattern!r}"

    assert checked, "src/scene/query must contain at least one checked source/header file"
