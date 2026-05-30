# Release Communication

Release communication should make the public release easy to understand and attractive to the right
audience without hiding limitations.


## Public Messages

Refine these messages for release notes, README, website, blog, and social posts:

1. Datoviz v0.4 is a modular C visualization engine for scientific GPU visualization.
2. The main user model is retained scene rendering with GPU-backed visuals and interaction.
3. Lower layers are available for advanced users who need DRP2, Vulkan runtime, canvas, stream,
   offscreen, video, or backend integration.
4. Python raw bindings expose the C API; high-level object-oriented plotting belongs to
   VisPy2/GSP.
5. v0.4 prioritizes architecture, performance, and release proof over long-term API lock-in; v0.5
   may still break API where needed.


## Blog And Announcement Assets

Generate launch assets from current examples, not mockups:

1. one short launch video;
2. several 10-20 second clips from gallery examples;
3. high-quality screenshots for README, website home page, blog post, LinkedIn, X, and release
   notes;
4. architecture diagram showing `scene -> frame plan -> drp2 -> vklite/canvas -> stream/video`;
5. comparison-style examples inspired by strong scientific visualization galleries while keeping
   Datoviz's own visual identity.

Public communication should link to exact examples, source files, docs pages, known limitations, and
release artifacts where possible.


## Channels

Use these channels when the release artifacts and docs are ready:

1. GitHub release;
2. project website;
3. release blog post or news page;
4. LinkedIn post;
5. X post or thread;
6. relevant scientific Python, visualization, and GPU communities;
7. direct notes to early adopters, collaborators, and selected dataset authors.


## Feedback Policy

Each release candidate and final announcement should include a concrete feedback request:

1. what the project wants tested;
2. which APIs or examples are considered supported, experimental, advanced/unstable, deferred, or
   external/GSP;
3. where users should report build, driver, docs, example, or correctness issues;
4. which known limitations are already tracked.

Known limitations must remain visible in the release notes, docs, and website.
