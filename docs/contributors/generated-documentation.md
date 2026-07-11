# Generated Documentation

The public example pages and machine-readable inventories under `docs/examples/` are generated from
`examples/c/MANIFEST.yaml`, `docs/examples/navigation.yaml`, and the canonical example source
comments. The navigation file is authored and owns section grouping and ordering; the Markdown and
JSON outputs are committed so reviews and release branches contain the exact documentation that
will be published.

Do not edit generated example pages directly. Update the manifest, source description, or generator,
then run:

```sh
just docs-generate
just docs-check-generated
```

MkDocs treats these committed files as read-only inputs. Documentation builds must not regenerate
or modify the source tree.
