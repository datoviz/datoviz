# Generated Documentation

The public example pages and machine-readable inventories under `docs/examples/` are generated from
`examples/c/MANIFEST.yaml`, `docs/examples/navigation.yaml`, and the canonical example source
comments. The navigation file is authored and owns section grouping and ordering; the Markdown and
JSON outputs are committed so reviews and release branches contain the exact documentation that
will be published.

`mkdocs.yml` declares only the Examples overview. The MkDocs hook expands the authored navigation
model into subsection overviews, grouped detail pages, and flat sections where requested.

Do not edit generated example pages directly. Update the manifest, source description, or generator,
then run:

```sh
just docs-generate
just docs-check-generated
```

MkDocs treats these committed files as read-only inputs. Documentation builds must not regenerate
or modify the source tree.

## Python example validation

Run every required manifest-declared Python example in an isolated subprocess with a bounded
offscreen render:

```sh
just python-examples-check
```

The runner continues after failures and writes logs, `results.json`, and `summary.md` under
`build/python-example-check/`. It can follow the authored review batches, resume at an example, or
write build-local nonblank PNG captures without changing committed gallery media:

```sh
just python-examples-check --batch 7
just python-examples-check --review-order --start-at features_controller_fly
just python-examples-check --capture-temp
```
