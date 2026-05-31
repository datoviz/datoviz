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

To emit one reviewed lane and a runner case-list file:

```sh
python3 tools/test_inventory.py \
  --lane scene-semantic \
  --output build/testing/test_inventory_scene-semantic.json \
  --case-list build/testing/test_lane_scene-semantic.txt
./build/testing/dvztest --case-list build/testing/test_lane_scene-semantic.txt
```

The case list contains runner `case_id` values. `dvztest --case-list` also accepts display ids and
function names for hand-written lists.


## Just Recipes

The initial lane recipes are exact case-list wrappers around the inventory:

```sh
just test-inventory
just test-inventory scene-semantic
just test-lane scene-semantic
just test-fast
just test-scene-cpu
just test-drp2-contract
just test-runtime-vklite
just test-render-smoke
just test-render-conformance
just test-slow
```

Extra runner arguments may be forwarded to lane recipes, for example:

```sh
just test-fast --fail-fast
just test-render-smoke --jobs 4
```


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
