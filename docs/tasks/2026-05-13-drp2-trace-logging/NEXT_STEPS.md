# Next Steps

The trace/status implementation is usable. Follow-up should be narrow:

1. run focused `just test app` after any app trace/status changes,
2. exercise one interactive GLFW example with normal trace mode and verify changed frames print a
   full stream while unchanged frames do not spam output,
3. check both default colors and `NO_COLOR=1` / `DVZ_DRP2_TRACE_COLOR=0` output,
4. exercise full trace mode on a small scene and confirm command fields remain readable,
5. keep trace output out of examples unless debugging a specific issue,
6. if normal output becomes noisy again, add a focused `src/app/tests/test_app.c` case before
   changing the formatting rules.

Do not turn app trace into a general profiler yet. Timing/profiling should be a separate design pass.
