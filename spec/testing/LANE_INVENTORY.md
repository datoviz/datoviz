# Test Lane Inventory

Status: active Phase 1 inventory workflow.

Use the inventory to make lane ownership explicit before moving, merging, or deleting tests. The
generated files are working artifacts under `build/testing/`; do not commit them unless a later
release note explicitly asks for a captured snapshot.


## Command

From the repository root:

```sh
python3 tools/test_inventory.py \
  --output build/testing/test_inventory.json \
  --markdown build/testing/test_inventory.md
```

The default command is metadata-only. It queries:

```sh
./build/testing/dvztest --list
./build/testing/dvztest --list-groups
```

It does not execute test cases.


## Timing Data

To add elapsed time, first run a selected validation set with runner JSON enabled:

```sh
./build/testing/dvztest --module common --json /tmp/dvztest-common.json
python3 tools/test_inventory.py --timing-json /tmp/dvztest-common.json
```

Only matching cases receive `elapsed_ns` and `status`. Full-suite timing should be captured only
when the suite was intentionally run for validation.


## Review Rules

1. Treat the generated `lane` field as a first-pass routing hint.
2. Review `slow-churn`, `render-smoke`, `render-conformance`, and `runtime-vklite` cases before
   using the inventory to delete or merge coverage.
3. Keep `release-proof` as an explicit selected subset. Do not infer it from lane heuristics.
4. Use the inventory to choose file-split boundaries, CTest labels, and future `just` lane recipes.
5. Preserve runner case names while splitting files so old filters keep working.
