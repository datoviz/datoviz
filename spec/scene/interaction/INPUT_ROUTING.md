# Input Router Dispatch

Status: normative for synchronous native input routing. Scene controllers and GUI forwarding consume this contract.

## Recognition and ownership

A router forwards raw originals without synthesizing gestures until a gesture activation is acquired. Each `dvz_pointer_gesture_handler()` call returns a distinct owned handle to one router-owned interpreter. The first activation initializes history; additional activations preserve it. Releasing the last handle disables recognition and resets history. Every handle borrows the router and must be destroyed before it.

Each window retains one activation, including offscreen windows. A GUI viewport borrows its source window's router and interpreter. Controllers subscribe to input; they do not create another recognition path.

## Native host pointer observations

GLFW button and wheel originals use the last cursor position observed through the host cursor callback, initialized from the host snapshot at creation or release of replay ownership. They do not query a newer OS cursor position during ingress: a query may run ahead of queued cursor callbacks and change the grab point relative to live GUI delivery. Cursor, button and wheel observations preserve callback order before GUI arbitration and scene routing; replay consumes those same coordinates without adding unobserved motion.

## Delivery

For a typed pointer emission, the router captures both typed and union audiences, updates canonical gesture history once, then delivers any newly derived gesture to union subscribers, the original to typed subscribers, and the original to union subscribers. An idle move, wheel, or long press release does not create a duplicate original. Already-semantic pointer events pass through once without recognition. `dvz_input_emit_event()` is union-only and does not interpret gestures.

Keyboard, text, resize, and scale emissions use the same audience rules. Resize state becomes observable before callbacks. If either audience snapshot cannot be allocated, the entire emission is dropped before callbacks, recognition, or resize-cache changes; the text API returns `DVZ_ERROR`, and void APIs log the failure.

## Mutation and nesting

Subscriber membership is captured at emission entry. New subscriptions cannot receive any remaining stage of that emission. Removing a subscription suppresses its remaining calls immediately; surviving callbacks retain registration order. Pointer derived and original delivery reuse the same union audience.

Explicit nested emissions capture their own current audiences and activation state, complete synchronously, then return to the outer emission. The outer event and any derived event are independent value copies. Releasing or acquiring activations inside callbacks affects nested and later recognition without invalidating an already-derived outer event.

All router operations are serialized on one thread. Same-thread nesting is supported. Routers, borrowed event payloads, and callback user data must remain valid during their uses; destroying a router during dispatch is forbidden. Unsubscribe before releasing callback user data.

Panel routing and controller transform ownership remain defined by [CONTROLLERS.md](CONTROLLERS.md); this contract governs delivery at one router, rather than scene-wide event consumption.
