# Scene Dashboard Notes

Status: informative v0.5+ pressure notes.

This directory contains dense dashboard and multi-panel pressure notes. The canonical v0.4 panel and
grid layout contract lives in [`../core/PANEL_LAYOUT.md`](../core/PANEL_LAYOUT.md). Dashboard files
should not redefine current panel layout semantics.

Dashboard files are not app-runner instructions; implementation staging belongs under `agents/`.


## Files

1. [GRID_LAYOUT.md](GRID_LAYOUT.md): landed grid/subplot context plus dashboard follow-up
   boundaries; use `core/PANEL_LAYOUT.md` for current rules.
2. [DASHBOARD_RENDERING_ROADMAP.md](DASHBOARD_RENDERING_ROADMAP.md): broader rendering and
   integration roadmap for dashboard-style scenes.
