import { readdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";

const root = process.cwd();
const fixtureDir = join(root, "spec", "drp2", "fixtures", "positive");
const paths = readdirSync(fixtureDir)
  .filter((name) => name.endsWith(".json"))
  .sort()
  .map((name) => `/spec/drp2/fixtures/positive/${name}`);

const manifest = {
  generated_from: "spec/drp2/fixtures/positive/*.json",
  positive: paths,
  webgpu_streams: [
    "/examples/webgpu/streams/attachment_multi_color_wgsl.json",
    "/examples/webgpu/streams/attachment_depth_wgsl.json",
  ],
};

writeFileSync(
  join(root, "examples", "webgpu", "fixture_manifest.json"),
  `${JSON.stringify(manifest, null, 2)}\n`,
);
