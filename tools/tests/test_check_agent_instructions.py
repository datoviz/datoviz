"""Structural instruction checks against valid and deliberately broken fixtures."""

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from tools.check_agent_instructions import check_markdown, check_paths, instruction_paths

SCRIPT = Path(__file__).resolve().parents[1] / 'check_agent_instructions.py'


class AgentInstructionsTests(unittest.TestCase):
    """Exercise checks in disposable directories without external services."""

    def setUp(self):
        """Create an isolated fixture directory for each case."""
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def write(self, name, text=''):
        """Create a fixture file under the test directory."""
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding='utf-8')
        return path

    def test_valid_markdown_structures(self):
        """Allow headings, nested lists, quotes, tables, and indented code."""
        path = self.write(
            'rules.md',
            """# Rules

One complete paragraph.

- First item.
  - Nested item.
1. Numbered item.

> First quote line.
> Second quote line.

| Task | Rule |
| --- | --- |
| A | B |
| C | D |

Heading
===

Another heading
---------------

***

    [code](not-a-link.md)
    continued code
""",
        )
        self.assertEqual(check_markdown(path), [])

    def test_table_without_outer_pipes(self):
        """Accept table rows without optional leading or trailing pipes."""
        path = self.write('rules.md', 'Task | Rule\n--- | ---\nA | B\nC | D\n')
        self.assertEqual(check_markdown(path), [])

    def test_wrapped_paragraph_and_list_report_lines(self):
        """Locate wrapped paragraphs and both bullet and numbered items."""
        path = self.write(
            'rules.md',
            'First line\ncontinued.\n\n- Item\n  continued.\n\n1. Item\n    continued.\n',
        )
        errors = check_markdown(path)
        self.assertEqual(len(errors), 3)
        for number, error in zip((2, 5, 8), errors, strict=True):
            self.assertIn(f':{number}: hard-wrapped', error)

    def test_fences_and_inline_code_do_not_create_links(self):
        """Ignore literal examples inside fenced blocks and code spans."""
        path = self.write(
            'rules.md',
            """````markdown
[example](absent.md)
```
still fenced
````

~~~text
[example](absent.md)
more code
~~~

Use `[example](absent.md)` or `` `[example](absent.md)` `` as literal examples.
""",
        )
        self.assertEqual(check_markdown(path), [])

    def test_local_links_with_fragments_titles_spaces_and_parentheses(self):
        """Resolve common Markdown destinations without fetching external URLs."""
        self.write('target (one).md', '# Target\n')
        self.write('target.md', '# Target\n')
        path = self.write(
            'rules.md',
            """[Relative](target.md#target) and [titled](target.md "A title").

[Angle](<target (one).md>) and [encoded](target%20%28one%29.md).

[Parentheses](target%20(one).md) and ![Image](target.md).

[Reference][target]

[target]: target.md 'A title'

[External](https://example.invalid/a) and [mail](mailto:e@example.invalid) and [fragment](#local).
""",
        )
        self.assertEqual(check_markdown(path), [])

    def test_missing_inline_image_and_reference_targets(self):
        """Reject missing targets in inline links, images, and reference definitions."""
        path = self.write(
            'rules.md', '[Missing](missing.md)\n\n![Image](missing.png)\n\n[id]: absent.md\n'
        )
        errors = check_markdown(path)
        self.assertEqual(len(errors), 3)
        self.assertTrue(all('missing local link target' in error for error in errors))

    def test_relative_targets_resolve_from_document_directory(self):
        """Resolve relative paths from the document rather than the working directory."""
        self.write('target.md')
        path = self.write('nested/rules.md', '[Valid](../target.md) and [Invalid](target.md).\n')
        errors = check_markdown(path)
        self.assertEqual(len(errors), 1)
        self.assertTrue(errors[0].endswith('target: target.md'))

    def test_shared_root_and_scoped_imports(self):
        """Accept the shared import convention at root and nested scopes."""
        paths = []
        for prefix in ('', 'spec/scene/'):
            paths.append(self.write(prefix + 'AGENTS.md', '# Instructions\n'))
            paths.append(self.write(prefix + 'CLAUDE.md', '@AGENTS.md\n'))
        self.assertEqual(check_paths(paths), [])

    def test_missing_or_divergent_claude_import(self):
        """Reject missing wrappers and separate Claude policy text."""
        shared = self.write('AGENTS.md', '# Instructions\n')
        self.assertIn('expected only @AGENTS.md', check_paths([shared])[0])
        self.write('CLAUDE.md', '@elsewhere.md\n')
        self.assertIn('expected only @AGENTS.md', check_paths([shared])[0])
        self.write('CLAUDE.md', '@AGENTS.md\nExtra instructions.\n')
        self.assertTrue(check_paths([shared]))

    def test_orphan_wrapper_and_missing_file(self):
        """Report missing shared instructions and explicitly requested files."""
        wrapper = self.write('CLAUDE.md', '@AGENTS.md\n')
        self.assertIn('missing sibling AGENTS.md', check_paths([wrapper])[0])
        self.assertIn('missing instruction file', check_paths([self.root / 'absent.md'])[0])

    def test_discovery_includes_new_scopes_and_excludes_handoffs_and_ignored_files(self):
        """Discover maintained instructions, including deleted tracked files."""
        # Fixed Git commands act only on this disposable fixture repository.
        subprocess.run(['git', 'init', '-q', str(self.root)], check=True)  # noqa: S603, S607
        self.write('.gitignore', 'scratch/\n')
        shared = self.write('AGENTS.md', '# Instructions\n')
        subprocess.run(['git', 'add', 'AGENTS.md'], cwd=self.root, check=True)  # noqa: S603, S607
        shared.unlink()
        self.write('spec/scene/AGENTS.md')
        self.write('agents/rules/NEW.md')
        self.write('agents/now/HANDOFF.md')
        self.write('scratch/AGENTS.md')
        found = {path.relative_to(self.root).as_posix() for path in instruction_paths(self.root)}
        self.assertEqual(
            found,
            {
                'AGENTS.md',
                'CLAUDE.md',
                'agents/README.md',
                'spec/scene/AGENTS.md',
                'agents/rules/NEW.md',
            },
        )

    def test_cli_exit_status(self):
        """Expose successful and failing checks to shell and CI callers."""
        path = self.write('rules.md', '# Valid\n')
        command = [sys.executable, str(SCRIPT), str(path)]
        # Run the repository checker with this test's fixture path, without a shell.
        result = subprocess.run(command, capture_output=True, text=True)  # noqa: S603
        self.assertEqual(result.returncode, 0, result.stderr)
        self.write('rules.md', '[Broken](absent.md)\n')
        result = subprocess.run(command, capture_output=True, text=True)  # noqa: S603
        self.assertEqual(result.returncode, 1)
        self.assertIn('missing local link target', result.stderr)


if __name__ == '__main__':
    unittest.main()
