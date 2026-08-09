# Get started

Datoviz is a GPU visualization engine for scientific data. It turns NumPy or C arrays into interactive 2D and 3D scenes, native windows, and offscreen images through the same retained scene model.

Datoviz is useful when data is large, changes over time, or needs more rendering control than a plotting function provides. It is an explicit rendering engine rather than a high-level plotting library; for a small conventional chart, a plotting library may be the shorter path.

[![Ten thousand colored points rendered in a Datoviz window](../assets/gallery/v0.4/start/start_scatter.webp)](quickstart.md)

*The Quickstart renders 10,000 points in an interactive window without external data.*


## Choose your path

### Python

1. [Install the Python package](install.md) in an isolated environment.
2. Run the [Python Quickstart](quickstart.md) to render NumPy arrays in an interactive window.

### C or C++

1. [Install or build Datoviz](install.md) for native development.
2. Build and run the [First C program](first-c-program.md), then use the [C/C++ integration guide](../how-to/c-integration.md) in your own project.


## After your first window

Read [Core concepts](core-concepts.md) for the scene, figure, panel, visual, data, controller, and view model. Then adapt the closest working [Example](../examples/index.md), use a focused [How-To guide](../how-to/index.md), or consult the [Reference](../reference/index.md) for exact contracts and feature status.


## Other integration paths

Use [Choose your layer](choose-your-layer.md) for browser examples, Qt embedding, offscreen services, or lower runtime layers. The [AI-assisted workflow](ai-workflow.md) explains how to request and verify current Datoviz v0.4 code from a coding assistant.
