# Physical Release Validation

This policy applies to every Datoviz release candidate and final release. It defines the physical,
maintainer-guided checks that complement CI, installed-artifact automation, and offscreen captures.
The runnable procedure lives in
[`docs/contributors/release-physical-validation.md`](../../docs/contributors/release-physical-validation.md).


## Required Proof

Each available required machine class must provide both:

1. installed-artifact evidence from the accepted candidate wheel; and
2. a maintainer-guided live interaction pass on the same machine.

The installed-artifact phase proves imports, ABI, CLI, native dependency inventory, a CMake
consumer, and the selected automated render profile. The live phase proves visible output,
interaction, resize, close, and reopen behavior that unattended checks cannot establish.

A launch without the requested human interaction is not a manual pass. An offscreen render does not
replace live interaction, and a checkout-built example must not be reported as installed-wheel
evidence.


## Artifact And Release Identity

Evidence must record the release version, artifact commit, release commit, successful wheel-workflow
run, artifact filename, and artifact checksum. The workflow `headSha` must equal the artifact commit.
The release commit may equal or descend from the artifact commit.

When the commits differ, audit and record the complete intervening diff. Existing artifacts remain
eligible only when every intervening change is demonstrably artifact-neutral and runtime-neutral,
such as release-process prose, authored documentation, or site navigation. Changes to native or
Python source, public headers, bindings or generators, examples used by the live pass, CMake/build
configuration, packaging, dependencies, version metadata, wheel workflows, vendored runtime
payloads, or any uncertain path require regenerated artifacts and repeated affected evidence.

A changed wheel checksum always identifies a new artifact and invalidates installed-artifact
evidence for the prior checksum. A runtime-affecting commit invalidates manual evidence on every
required machine unless the release policy explicitly narrows the affected matrix with recorded
technical justification. Do not rebuild merely because the release commit advanced through an
audited artifact-neutral diff.


## Manual Interaction Coverage

The canonical live set must remain short and stable while covering:

1. a 2D view with pan and zoom;
2. a 3D view with arcball or fly controls;
3. text and layout during resize;
4. image or color-scale interaction;
5. mesh or textured-mesh rendering;
6. picking, query, or readback;
7. normal close and at least one reopen.

The runnable procedure identifies the current examples and exact maintainer actions. Example
metadata should eventually generate this selection, but a generated list must preserve these
coverage requirements and remain reviewable as release policy.


## Agent Conduct And Evidence

The local agent launches one example at a time, states the requested action, waits for the
maintainer's observation, and records `pass`, `fail`, or `skip` with a reason. It must not infer a
visual or interaction pass from process exit status.

Evidence records machine and GPU/driver facts, candidate identity, validator output, each manual
result, captures when practical, and lifecycle anomalies. A required failure blocks that machine's
release profile until fixed or explicitly accepted through the release known-issue process.
