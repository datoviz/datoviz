from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import check_vulkan_course as course


VERTEX = """#version 450
layout(location = 0) out vec3 color;
void main() {
    gl_Position = vec4(0.0);
    color = vec3(1.0);
}
"""
FRAGMENT = """#version 450
layout(location = 0) in vec3 color;
layout(location = 0) out vec4 out_color;
void main() { out_color = vec4(color, 1.0); }
"""


def c_string(name: str, text: str) -> str:
    return f"static const char* {name} =\n" + "\n".join(
        "    " + json.dumps(line) for line in text.splitlines(keepends=True)
    ) + ";\n"


def annotated(name: str, text: str) -> str:
    return f"<!-- embedded-shader: {name} -->\n\n```glsl\n{text}```\n"


class EmbeddedShaderChecks(unittest.TestCase):
    def setUp(self) -> None:
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.page = self.root / "docs/gpu-graphics/04-triangle.md"
        self.source = self.root / "examples/c/vulkan/step04.c"
        self.page.parent.mkdir(parents=True)
        self.source.parent.mkdir(parents=True)
        self.source.write_text(
            c_string("VERTEX_GLSL", VERTEX) + c_string("FRAGMENT_GLSL", FRAGMENT)
            + "int main(void) { return 0; }\n"
        )

    def check(self, markdown: str) -> list[str]:
        self.page.write_text(markdown)
        errors: list[str] = []
        with patch.object(course, "ROOT", self.root):
            course._check_chapter(self.page, self.source, errors)
        return errors

    def page_text(self, vertex: str = VERTEX, fragment: str = FRAGMENT) -> str:
        return annotated("VERTEX_GLSL", vertex) + annotated("FRAGMENT_GLSL", fragment)

    def test_comments_and_formatting_match_decoded_source(self) -> None:
        readable = VERTEX.replace("#version 450", "#version 450 // GLSL version")
        readable = readable.replace("void main() {", "// One invocation.\nvoid main()\n{")
        readable = readable.replace("color =", "color /* output */ =")
        self.assertEqual(self.check(self.page_text(readable)), [])
        self.assertEqual(course._embedded_shader(self.source.read_text(), "VERTEX_GLSL"), VERTEX)

    def test_semantic_changes_are_rejected(self) -> None:
        for old, new in (
            ("1.0", "0.0"),
            ("gl_Position", "position"),
            ("out vec3", "in vec3"),
            ("location = 0", "location = 1"),
            ("color =", "color +="),
            ("#version 450\n", "#version 450 "),
        ):
            with self.subTest(change=(old, new)):
                self.assertTrue(any("differs from embedded" in e for e in self.check(
                    self.page_text(VERTEX.replace(old, new))
                )))

    def test_changed_c_initializer_is_detected(self) -> None:
        self.source.write_text(self.source.read_text().replace("vec3(1.0)", "vec3(0.0)"))
        self.assertTrue(any("differs from embedded" in e for e in self.check(self.page_text())))

    def test_c_line_splicing_precedes_comment_removal(self) -> None:
        decoy = '// Continued comment ' + "\\" + '\nstatic const char* VERTEX_GLSL = "wrong";\n'
        self.source.write_text(decoy + self.source.read_text())
        self.assertEqual(self.check(self.page_text()), [])

    def test_c_string_escapes_preserve_quoted_shader_text(self) -> None:
        text = '#version 450\n// Quoted "text" and a backslash: \\ value\n'
        source = c_string("VERTEX_GLSL", text)
        self.assertEqual(course._embedded_shader(source, "VERTEX_GLSL"), text)

    def test_missing_duplicate_or_swapped_stages_are_rejected(self) -> None:
        for markdown in (
            annotated("VERTEX_GLSL", VERTEX),
            self.page_text() + annotated("VERTEX_GLSL", VERTEX),
            self.page_text(FRAGMENT, VERTEX),
        ):
            with self.subTest(markdown=markdown):
                self.assertTrue(self.check(markdown))

    def test_annotation_does_not_allow_unknown_names_or_other_chapters(self) -> None:
        self.assertTrue(self.check(self.page_text().replace("VERTEX_GLSL -->", "OTHER -->")))
        self.page = self.page.with_name("05-shader-files.md")
        self.assertTrue(any("unexpected embedded" in e for e in self.check(self.page_text())))

    def test_annotation_requires_glsl(self) -> None:
        self.assertTrue(any("unexpected embedded" in e for e in self.check(
            self.page_text().replace("```glsl", "```c", 1)
        )))

    def test_c_excerpts_keep_the_verbatim_rule(self) -> None:
        self.assertEqual(self.check(self.page_text() + "```c\nint main(void) { return 0; }\n```"), [])
        self.assertEqual(self.check(self.page_text() + "```c\nint main(void) { return 0; } \t\n```"), [])
        errors = self.check(self.page_text() + "```c\nint main(void) { return 1; }\n```")
        self.assertTrue(any("verbatim" in error for error in errors))
        errors = self.check(self.page_text() + "```c\nint main(void) { return 0; }\u00a0\n```")
        self.assertTrue(any("verbatim" in error for error in errors))

    def test_external_shader_excerpts_keep_the_verbatim_rule(self) -> None:
        self.page = self.page.with_name("05-shader-files.md")
        self.source = self.source.with_name("step05.c")
        self.source.write_text("int main(void) { return 0; }")
        shader_dir = self.source.with_suffix("")
        shader_dir.mkdir()
        (shader_dir / "shader.vert").write_text(VERTEX)
        (shader_dir / "shader.frag").write_text(FRAGMENT)
        includes = "\n".join(
            f'```glsl\n--8<-- "examples/c/vulkan/step05/shader.{stage}"\n```'
            for stage in ("vert", "frag")
        )
        self.assertEqual(self.check(includes + "\n```glsl\n" + VERTEX + "```"), [])
        changed = VERTEX.replace("void main()", "// Readable comment.\nvoid main()")
        self.assertTrue(any("verbatim" in e for e in self.check(
            includes + "\n```glsl\n" + changed + "```"
        )))

    def test_operators_and_directive_boundaries_are_significant(self) -> None:
        self.assertNotEqual(course._shader_tokens("i++;"), course._shader_tokens("i + +;"))
        self.assertNotEqual(course._shader_tokens("a >>= b;"), course._shader_tokens("a > >= b;"))
        self.assertNotEqual(course._shader_tokens("#version 450\nx;"),
                            course._shader_tokens("#version 450 x;"))
        self.assertNotEqual(course._shader_tokens("vec3(1.0)"),
                            course._shader_tokens("vec3(1 .0)"))

    def test_comment_stripping_preserves_literals_and_token_separation(self) -> None:
        self.assertEqual(course._shader_tokens('"// string" /* removed */ x'), ['"// string"', "x"])
        self.assertNotEqual(course._shader_tokens("vec/* separator */3"),
                            course._shader_tokens("vec3"))

    def test_line_continuations_cannot_hide_code_in_comments(self) -> None:
        for newline in ("\n", "\r\n", "\r"):
            for padding in ("", " ", "\t"):
                readable = VERTEX.replace(
                    "    gl_Position",
                    "    // Continued comment " + "\\" + padding + newline + "    gl_Position",
                )
                with self.subTest(newline=repr(newline), padding=repr(padding)):
                    self.assertTrue(any("line continuations" in e for e in self.check(
                        self.page_text(readable)
                    )))

    def test_only_glsl_whitespace_is_ignored(self) -> None:
        for whitespace in ("\u00a0", "\u2003", "\u2028"):
            with self.subTest(whitespace=repr(whitespace)):
                errors = self.check(self.page_text(VERTEX.replace("out vec3", "out" + whitespace + "vec3")))
                self.assertTrue(any("non-ASCII whitespace" in error for error in errors))
        readable = "// Comment with Unicode π and a non-breaking space: \u00a0\n" + VERTEX
        self.assertEqual(self.check(self.page_text(readable)), [])
        self.assertEqual(self.check(self.page_text(VERTEX.replace("\n", "\r\n"))), [])

    def test_vertical_whitespace_does_not_end_directives(self) -> None:
        for whitespace in ("\v", "\f"):
            with self.subTest(whitespace=repr(whitespace)):
                self.assertTrue(any("differs from embedded" in e for e in self.check(
                    self.page_text(VERTEX.replace("#version 450\n", "#version 450" + whitespace))
                )))

    def test_unsupported_initializer_or_escape_fails_closed(self) -> None:
        for source in (
            self.source.read_text().replace("VERTEX_GLSL =", "VERTEX_GLSL = get_shader(); //"),
            self.source.read_text().replace(r"\n", r"\x0a", 1),
        ):
            with self.subTest(source=source):
                self.source.write_text(source)
                self.assertTrue(self.check(self.page_text()))


if __name__ == "__main__":
    unittest.main()
