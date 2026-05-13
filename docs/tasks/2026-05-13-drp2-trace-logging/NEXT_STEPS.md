# Next Steps

The trace/status implementation is usable. Follow-up should be narrow:

1. run focused `just test app` after any app trace/status changes,
2. exercise one interactive GLFW example with normal trace mode and verify unchanged frames do not
   spam output,
3. exercise full trace mode on a small scene and confirm command fields remain readable,
4. keep trace output out of examples unless debugging a specific issue,
5. if normal output becomes noisy again, add a focused `src/app/tests/test_app.c` case before
   changing the formatting rules.

Do not turn app trace into a general profiler yet. Timing/profiling should be a separate design pass.
