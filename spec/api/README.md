# API Specs

This directory owns durable public API and API-positioning policy for v0.4.


## Index

1. [PUBLIC_API_CONVENTIONS.md](PUBLIC_API_CONVENTIONS.md): cross-module public C API naming,
   ownership, struct, callback, and binding-readiness rules.
2. [COLOR_TYPES.md](COLOR_TYPES.md): target public color types and conversion helpers.
3. [PYTHON_GSP_SCOPE.md](PYTHON_GSP_SCOPE.md): v0.4 ownership split between Datoviz, raw generated
   Python bindings, GSP, and VisPy2.
4. [GSP_BACKEND_STRATEGY.md](GSP_BACKEND_STRATEGY.md): strategic Datoviz/GSP/VisPy2 backend
   boundary and capability-extension model.


## Boundary

Use this directory for cross-module public API conventions and language/API positioning. Put raw
binding-generation mechanics in `../bindings/`, scene-specific API behavior in `../scene/api/`, and
release-review process in `../release/`.
