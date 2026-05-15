# Scene Integration Specs

This directory contains boundaries with host applications, external UI systems, threading, custom
visuals, and display integration.

Use these files when scene behavior crosses into application-owned state, UI frameworks, background
threads, high-DPI windows, or custom user-provided visual families.


## Files

1. [EXTERNAL_UI.md](EXTERNAL_UI.md): boundary with UI frameworks such as ImGui.
2. [THREAD_SAFETY.md](THREAD_SAFETY.md): threading model and async data handoff.
3. [HIGH_DPI.md](HIGH_DPI.md): logical pixels, device pixel ratio, and DPI changes.
4. [CUSTOM_VISUALS.md](CUSTOM_VISUALS.md): registration and integration of user-defined visuals.
5. [HOSTED_BACKENDS.md](HOSTED_BACKENDS.md): Qt, Python console, IPython, Jupyter, SDL, Tk,
   and other host-owned event-loop integrations.


## Active Proposal Inputs

1. [../proposals/UI_BACKEND_INTEGRATION.md](../proposals/UI_BACKEND_INTEGRATION.md)
