# Touch Support Plan

Status on 2026-05-16: Datoviz has a mouse-like pointer event path and hosted adapters can inject
pointer, wheel, resize, and keyboard events. Full multi-touch semantics are not active yet.


## Goal

Support touch-first navigation and selection without changing scene rendering semantics.

Touch support should cover:

1. single-finger pan/rotate gestures;
2. two-finger pinch zoom;
3. two-finger pan;
4. optional two-finger rotation when a controller supports it;
5. tap, double-tap, long-press, and drag selection policies;
6. hosted UI adapters on mobile, desktop touchscreens, and embedded toolkit views.


## Event Model

Keep the existing pointer event path for mouse-compatible input, but add a touch-aware layer above
backend events.

The recommended model is:

1. backend adapters emit raw contacts with stable contact ids;
2. the input layer normalizes contacts into panel-local touch state;
3. a gesture recognizer emits semantic gestures;
4. controllers consume gestures and mutate scene state;
5. the scene requests redraw through the ordinary invalidation path.

Do not make touch gestures emit DRP2 commands directly.


## Contact Data

Each raw contact should carry:

1. stable contact id;
2. phase: begin, move, end, cancel;
3. position in host-window coordinates;
4. content scale;
5. timestamp;
6. optional pressure, radius, and tilt when the platform provides them.

The first implementation may ignore pressure, radius, and tilt, but the event shape should leave
room for stylus and tablet input.


## Gesture Mapping

Default gesture mapping should be controller-specific:

| Controller | One finger | Two fingers | Double tap |
|---|---|---|---|
| panzoom | pan | pinch zoom and pan | reset |
| arcball | rotate | pan and pinch zoom | reset |
| turntable | orbit | pan and dolly | reset |
| fly | look | move or dolly, depending on mode | reset |

Mouse emulation remains acceptable for single-touch fallback, but it is not enough for full
support because pinch and multi-contact pan need contact geometry.


## Picking And Selection

Tap interactions should route through the same pick/probe request system as mouse clicks and hover.

Recommended defaults:

1. tap schedules a pick request;
2. long-press can enter selection or context-readout mode;
3. drag selection should be explicit and controller-owned;
4. stale pick/probe results must follow the existing latest-request-wins freshness rules.


## Platform Adapters

Android, iOS, Qt, GLFW, and future hosted adapters should translate platform-specific touch events
at the boundary. Controllers should not know whether the gesture came from UIKit, Android, Qt, or a
desktop touchscreen.

Hosted adapters may initially map:

1. one-finger touch to pointer drag;
2. pinch to wheel-like zoom;
3. two-finger pan to right-drag pan for controllers that already support it.

That compatibility layer is useful, but the long-term API should expose real multi-contact
gestures.


## Validation

Touch validation should include:

1. deterministic unit tests for contact-to-gesture recognition;
2. controller tests for panzoom, arcball, and turntable gesture application;
3. hosted adapter smoke tests on at least one mobile platform;
4. stale/canceled contact tests;
5. resize/content-scale tests while a gesture is active.
