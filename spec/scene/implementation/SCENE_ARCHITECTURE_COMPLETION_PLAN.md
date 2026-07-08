# Scene Architecture Completion Plan

This file is retired as an active execution plan.

The broad scene architecture/source split has been completed far enough that this document should
not be used as a queue for new work. Durable visual-family boundary rules now live in
[`../visuals/BOUNDARY_CONTRACT.md`](../visuals/BOUNDARY_CONTRACT.md).

Use that file before:

1. removing remaining concrete visual-family branches from generic code;
2. moving visual-specific attributes, descriptors, lifecycle state, uploads, bounds, and query
   behavior into family folders or explicit shared visual subsystems;
3. adding guardrail checks that prevent generic code from including family-private headers or
   switching on concrete visual types;
4. changing scene visual-boundary validation.

Historical context for the completed broad source split is kept in git history.

Do not add new active work to this file. Update `../visuals/BOUNDARY_CONTRACT.md` when the durable
visual-boundary contract changes.
