# Application Events

In addition to mouse and keyboard input, Datoviz supports two types of programmatic event callbacks:

- **Frame callbacks**: triggered at every rendered frame
- **Timers**: triggered at fixed time intervals

These are useful for animations, simulation steps, or triggering regular updates independent of user input.

---

## Frame events

Frame events are called once per render loop iteration. They allow you to update visuals or application state continuously, such as animating objects or polling data.

To register a frame event handler, decorate a function named `on_frame`:

```python
@app.connect(figure)
def on_frame(ev):
    # Update something every frame
    print("Rendering frame")
```

* Triggered once per frame (hundreds of thousands of frames per second by default, \~60 FPS or the monitor refresh rate when the environment variable `DVZ_VSYNC=1` activates vertical synchronization)
* Bound to a specific `Figure`
* Receives an event object (`ev`), though it is typically unused

Avoid any computationally intensive operations in this function to prevent frame rate drops.

---

## Timer events

Use timers to trigger a function at regular time intervals.

```python
@app.timer(delay=0.0, period=1.0, max_count=0)
def my_timer(ev):
    print("Timer tick")
```

### Parameters:

| Argument    | Description                                      |
| ----------- | ------------------------------------------------ |
| `delay`     | Initial delay in seconds before the timer starts |
| `period`    | Time between calls (in seconds)                  |
| `max_count` | Maximum number of timer calls (0 = unlimited)    |


---

## Summary

| Event type   | Trigger                     | Use case                         |
| ------------ | --------------------------- | -------------------------------- |
| `on_frame`   | Every render frame          | Animations, continuous updates   |
| `@app.timer` | Fixed interval (in seconds) | Simulation steps, timed triggers |

These callbacks provide core building blocks for interactive and time-based applications in Datoviz.

See also:

* [User Input](input.md): mouse and keyboard events
* [GUI](gui.md): interactive GUI panels and widgets
