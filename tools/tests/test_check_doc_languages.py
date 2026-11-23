"""Regression checks for page-wide Python/C alternatives and their rendered ancestry."""

import sys
import tempfile
from pathlib import Path
from unittest import TestCase, main

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import check_doc_languages as gate  # noqa: E402

PAIR = """=== "Python"

    ```python
    print(1)
    ```

=== "C"

    ```c
    puts("1");
    ```
"""


def _tabs(labels_and_code):
    labels = ''.join(f'<label>{name}</label>' for name, _ in labels_and_code)
    blocks = ''.join(f'<div class="tabbed-block">{code}</div>' for _, code in labels_and_code)
    return (
        '<div class="tabbed-set"><div class="tabbed-labels">'
        + labels
        + '</div><div class="tabbed-content">'
        + blocks
        + '</div></div>'
    )


def _code(language):
    return f'<div class="language-{language} highlight"><pre><code>example</code></pre></div>'


class LanguageGateTest(TestCase):
    """Cover source indentation, includes, auxiliary source panes and generated HTML."""

    def check_source(self, text, includes=None):
        """Check a temporary repository containing one page and optional included sources."""
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            page = root / 'page.md'
            page.write_text(text)
            for name, body in (includes or {}).items():
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(body)
            return gate.check_markdown(page, root)

    def check_rendered(self, text):
        """Run the HTML gate on a rendered article fixture."""
        with tempfile.TemporaryDirectory() as folder:
            page = Path(folder) / 'page.html'
            page.write_text('<article>' + text + '</article>')
            return gate.check_html(page)

    def test_valid_pairs_and_common_languages(self):
        """Shell, shader, JSON and literal output blocks stay common to both choices."""
        common = ''.join(
            f'\n```{language}\nshared\n```\n' for language in ['sh', 'glsl', 'json', 'text']
        )
        self.assertEqual(self.check_source(PAIR + common), [])

    def test_pure_c_pages_and_c_glsl_tabs_remain_valid(self):
        """A C-only course lesson does not acquire a Python requirement."""
        self.assertEqual(self.check_source(PAIR.split('=== "C"')[1]), [])
        self.assertEqual(
            self.check_source(
                '=== "C"\n\n    ```c\n    int x;\n    ```\n'
                '=== "GLSL"\n\n    ```glsl\n    main() {}\n    ```'
            ),
            [],
        )

    def test_empty_primary_panes_are_rejected(self):
        """Labels and empty fences do not count as a language counterpart."""
        for original in ['print(1)', 'puts("1");']:
            with self.subTest(original=original):
                self.assertTrue(self.check_source(PAIR.replace(original, '')))
        for empty in ['Python', 'C']:
            with self.subTest(empty=empty):
                html = _tabs(
                    [
                        ('Python', '' if empty == 'Python' else _code('python')),
                        ('C', '' if empty == 'C' else _code('c')),
                    ]
                )
                self.assertTrue(self.check_rendered(html))
        self.assertTrue(
            self.check_rendered(
                _tabs(
                    [
                        ('Python', '<pre class="language-python"><code>  </code></pre>'),
                        ('C', _code('c')),
                    ]
                )
            )
        )

    def test_shell_only_pairs_need_commands_in_both_panes(self):
        """Build/run alternatives can use shell commands, but neither pane may be empty."""
        source = PAIR.replace('```python', '```sh').replace('```c', '```sh')
        source = source.replace('print(1)', 'python example.py').replace(
            'puts("1");', 'cc example.c'
        )
        self.assertEqual(self.check_source(source), [])
        self.assertTrue(self.check_source(source.replace('python example.py', '')))
        self.assertTrue(self.check_source(source.replace('cc example.c', '')))
        self.assertEqual(
            self.check_rendered(
                _tabs(
                    [
                        ('Python', _code('sh')),
                        ('C', _code('sh')),
                    ]
                )
            ),
            [],
        )

    def test_empty_included_source_is_not_a_counterpart(self):
        """A source include must contribute actual content to its declared language fence."""
        source = PAIR.replace('print(1)', '--8<-- "example.py"')
        self.assertTrue(self.check_source(source, {'example.py': '  \n'}))
        self.assertEqual(self.check_source(source, {'example.py': 'print(1)'}), [])

    def test_matching_counterpart_can_be_nested_and_included(self):
        """Nested details preserve a primary pane's matching included source block."""
        source = PAIR.replace(
            '    ```python\n    print(1)\n    ```',
            '    ??? info "More"\n\n        ```python\n        --8<-- "example.py"\n        ```',
        )
        self.assertEqual(self.check_source(source, {'example.py': 'print(1)'}), [])
        hidden = '<details><summary>More</summary>' + _code('python') + '</details>'
        self.assertEqual(
            self.check_rendered(
                _tabs(
                    [
                        ('Python', hidden),
                        ('C', _code('c')),
                    ]
                )
            ),
            [],
        )

    def test_bare_c_or_python_blocks_fail_on_mixed_pages(self):
        """Every host-language snippet follows the page's selected language."""
        for language in ['c', 'python', 'cpp', 'pycon']:
            with self.subTest(language=language):
                problems = self.check_source(PAIR + f'\n## More\n\n```{language}\ncode\n```\n')
                self.assertTrue(any('bare' in p.message for p in problems))

    def test_opposite_language_inside_a_tab_fails(self):
        """Indentation alone must not hide a C snippet in the Python alternative."""
        self.assertTrue(self.check_source(PAIR.replace('```python', '```c')))

    def test_single_language_set_on_a_mixed_page_fails(self):
        """An isolated C tab is still visible when another set selects Python."""
        self.assertTrue(
            self.check_source(PAIR + '\n## More\n=== "C"\n\n    ```c\n    int x;\n    ```')
        )

    def test_separate_half_sets_on_one_page_are_rejected(self):
        """Splitting Python and C into independent tabsets must not bypass the page gate."""
        source = PAIR.replace('=== "C"', '## Separate set\n\n=== "C"')
        self.assertTrue(self.check_source(source))
        html = _tabs([('Python', _code('python'))]) + _tabs([('C', _code('c'))])
        self.assertTrue(self.check_rendered(html))

    def test_duplicate_host_labels_are_rejected(self):
        """Adjacent unseparated pairs merge into one tabset and would lose examples."""
        self.assertTrue(self.check_source(PAIR + '\n' + PAIR))
        html = _tabs([('Python', _code('python')), ('C', _code('c'))] * 2)
        self.assertTrue(self.check_rendered(html))

    def test_nested_details_and_os_tabs_keep_outer_language_context(self):
        """Closed details and nested operating-system tabs do not bypass the gate."""
        valid = """=== "Python"

    ??? info "More"

        === "Linux"

            ```python
            print(1)
            ```

        === "Windows"

            ```python
            print(1)
            ```

=== "C"

    ```c
    puts("1");
    ```
"""
        self.assertEqual(self.check_source(valid), [])
        self.assertTrue(
            self.check_source(valid.replace('            ```python', '            ```c', 1))
        )

    def test_markdown_and_code_includes_are_checked(self):
        """Included Markdown keeps its scope, and code-include languages must match."""
        includes = {
            'part.md': '```c\nint x;\n```',
            'example.c': 'int x;',
            'example.py': 'print(1)',
        }
        self.assertTrue(self.check_source(PAIR + '\n## More\n--8<-- "part.md"', includes))
        paired_includes = PAIR.replace('print(1)', '--8<-- "example.py"').replace(
            'puts("1");', '--8<-- "example.c"'
        )
        self.assertEqual(self.check_source(paired_includes, includes), [])
        self.assertTrue(
            self.check_source(paired_includes.replace('example.py', 'example.c'), includes)
        )

    def test_missing_or_cyclic_include_fails(self):
        """An unreadable include cannot silently remove a snippet from the check."""
        self.assertTrue(self.check_source('--8<-- "missing.md"'))
        self.assertTrue(self.check_source('--8<-- "part.md"', {'part.md': '--8<-- "page.md"'}))

    def test_auxiliary_c_python_and_glsl_panes_are_valid(self):
        """Gallery helper sources remain inspectable alongside the host-language alternatives."""
        source = (
            PAIR + '\n=== "Model"\n\n    ```c\n    int x;\n    ```\n'
            '\n=== "Preparation"\n\n    ```python\n    print(2)\n    ```\n'
            '\n=== "GLSL"\n\n    ```glsl\n    main() {}\n    ```\n'
        )
        self.assertEqual(self.check_source(source), [])
        html = _tabs(
            [
                ('Python', _code('python')),
                ('C', _code('c')),
                ('Model', _code('c')),
                ('GLSL', _code('glsl')),
            ]
        )
        self.assertEqual(self.check_rendered(html), [])

    def test_ambiguous_auxiliary_host_language_pane_fails(self):
        """A single helper tab must not expose C and Python together."""
        source = (
            PAIR + '\n=== "Helper"\n\n    ```c\n    int x;\n    ```\n'
            '\n    ```python\n    print(1)\n    ```'
        )
        self.assertTrue(self.check_source(source))

    def test_html_rejects_bare_and_mismatched_code(self):
        """Rendered ancestry is checked independently of Markdown indentation."""
        valid = _tabs([('Python', _code('python')), ('C', _code('c'))])
        self.assertEqual(self.check_rendered(valid), [])
        self.assertTrue(self.check_rendered(valid + _code('c')))
        self.assertTrue(self.check_rendered(_tabs([('Python', _code('c')), ('C', _code('c'))])))

    def test_report_counts_all_host_blocks_including_helper_panes(self):
        """The review inventory reports each mixed page and both language counts."""
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            page = root / 'page.md'
            page.write_text(PAIR)
            report = {}
            self.assertEqual(gate.check_markdown(page, root, report=report), [])
            self.assertTrue(report['mixed'])
            self.assertEqual(report['code_blocks'], {'python': 1, 'c': 1})
            self.assertEqual(report['language_tabsets'], 1)
            self.assertEqual(report['errors'], [])

    def test_source_tab_markers_inside_code_and_comments_are_ignored(self):
        """Examples of tab syntax in prose do not create real language groups."""
        self.assertEqual(
            self.check_source('```text\n=== "Python"\n=== "C"\n```\n```c\nint x;\n```'), []
        )
        self.assertEqual(
            self.check_source('<!--\n=== "Python"\n=== "C"\n-->\n```c\nint x;\n```'), []
        )


if __name__ == '__main__':
    main()
