# Explanation

These pages explain the Datoviz v0.4 mental model: what the engine owns, how retained scene state
turns into GPU work, where runtime boundaries sit, and which parts are deliberately outside the
v0.4 release surface.

Use explanation pages when you need design context. Use how-to pages for task recipes and reference
pages for exact API names, status labels, and constraints.

## Reading Order

1. [Why Datoviz?](why-datoviz.md)
2. [Architecture](architecture.md)
3. [Scene Model](scene-model.md)
4. [Coordinate Systems](coordinate-systems.md)
5. [Frame Lifecycle](frame-lifecycle.md)
6. [Performance Model](performance-model.md)

The remaining pages go deeper into ownership, invalidation, interaction, queries, portability, and
the GSP/VisPy2 boundary.
