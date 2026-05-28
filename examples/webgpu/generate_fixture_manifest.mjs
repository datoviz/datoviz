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
  negative_parity: [
    "/spec/drp2/fixtures/negative/invalid_bind_group_sampler_unknown_id.json",
    "/spec/drp2/fixtures/negative/invalid_capability_shader_module_format.json",
    "/spec/drp2/fixtures/negative/invalid_create_sampler_duplicate.json",
    "/spec/drp2/fixtures/negative/invalid_duplicate_buffer_id.json",
    "/spec/drp2/fixtures/negative/invalid_end_wrong_pass_kind.json",
    "/spec/drp2/fixtures/negative/invalid_pipeline_vertex_buffers_slot_mismatch.json",
    "/spec/drp2/fixtures/negative/invalid_queue_submit_readback_unknown_buffer.json",
    "/spec/drp2/fixtures/negative/invalid_queue_submit_reused_command_buffer.json",
    "/spec/drp2/fixtures/negative/invalid_queue_submit_unknown_command_buffer.json",
    "/spec/drp2/fixtures/negative/invalid_set_blend_constant_in_compute_pass.json",
    "/spec/drp2/fixtures/negative/invalid_set_scissor_in_compute_pass.json",
    "/spec/drp2/fixtures/negative/invalid_set_stencil_reference_in_compute_pass.json",
    "/spec/drp2/fixtures/negative/invalid_set_viewport_in_compute_pass.json",
    "/spec/drp2/fixtures/negative/invalid_texture_view_unknown_texture.json",
    "/spec/drp2/fixtures/negative/invalid_unknown_buffer_id_write.json",
    "/spec/drp2/fixtures/negative/invalid_wrong_object_type_destroy.json",
  ],
};

writeFileSync(
  join(root, "examples", "webgpu", "fixture_manifest.json"),
  `${JSON.stringify(manifest, null, 2)}\n`,
);
