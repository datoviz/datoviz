# Physical Release Validation

This policy applies to every Datoviz release candidate and final release. It defines the physical,
maintainer-guided checks that complement CI, installed-artifact automation, and offscreen captures.
The runnable procedure lives in
[`docs/contributors/release-physical-validation.md`](../../docs/contributors/release-physical-validation.md).


## Required Proof

Each available required machine class must provide both:

1. installed-artifact evidence from the exact candidate wheel; and
2. a maintainer-guided live interaction pass on the same machine.

The installed-artifact phase proves imports, ABI, CLI, native dependency inventory, a CMake
consumer, and the selected automated render profile. The live phase proves visible output,
interaction, resize, close, and reopen behavior that unattended checks cannot establish.

A launch without the requested human interaction is not a manual pass. An offscreen render does not
replace live interaction, and a checkout-built example must not be reported as installed-wheel
evidence.


## Candidate Identity And Invalidation

Evidence must record the release version, candidate commit, successful wheel-workflow run, artifact
filename, and artifact checksum. The workflow `headSha` must equal the candidate commit.

Any change to the candidate commit or artifact checksum invalidates all earlier installed-artifact
and manual evidence. Regenerate the artifacts and repeat physical validation on every required
machine; do not carry a pass forward because the changed code appears platform-specific.


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
