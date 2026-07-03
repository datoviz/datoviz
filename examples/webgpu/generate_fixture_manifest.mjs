import { readdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";

const root = process.cwd();
const fixtureDir = join(root, "spec", "drp2", "fixtures", "positive");
const negativeFixtureDir = join(root, "spec", "drp2", "fixtures", "negative");
const paths = readdirSync(fixtureDir)
  .filter((name) => name.endsWith(".json"))
  .sort()
  .map((name) => `/spec/drp2/fixtures/positive/${name}`);
const negativePaths = readdirSync(negativeFixtureDir)
  .filter((name) => name.endsWith(".json"))
  .sort()
  .map((name) => `/spec/drp2/fixtures/negative/${name}`);

const manifest = {
  generated_from: {
    positive: "spec/drp2/fixtures/positive/*.json",
    webgpu_streams: "examples/webgpu/streams/*_wgsl.json",
    negative_parity: "spec/drp2/fixtures/negative/*.json",
  },
  positive: paths,
  webgpu_streams: [
    "/examples/webgpu/streams/attachment_multi_color_wgsl.json",
    "/examples/webgpu/streams/attachment_depth_wgsl.json",
  ],
  negative_parity: negativePaths,
};

writeFileSync(
  join(root, "examples", "webgpu", "fixture_manifest.json"),
  `${JSON.stringify(manifest, null, 2)}\n`,
);
