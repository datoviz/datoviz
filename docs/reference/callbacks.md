# Callbacks

Callbacks are borrowed execution hooks. They let an app react to frame, input, timer, query, or host
events while Datoviz keeps ownership of scene and runtime state.

## Lifetime Rules

| Topic | Rule |
| --- | --- |
| Callback pointer | Must remain valid until it is unregistered or the owning object is destroyed. |
| User data | Application-owned. Datoviz stores the pointer value but does not copy or free the pointee. |
| Borrowed objects | Scene, app, view, panel, event, query, or frame objects passed to a callback are borrowed for the callback duration unless the API documents a longer lifetime. |
| Unregistration | Prefer explicit unregister/disconnect helpers when available. Destroying the owner invalidates registrations owned by that object. |
| Reentrancy | Do not recursively run the app/frame loop from inside callbacks unless the API explicitly allows it. |

Subscription APIs return `DVZ_CALLBACK_ID_NONE` on failure. A callback id is scoped to the router
or owner that returned it; unsubscribe while that owner and the callback's `user_data` are still
alive.

## Expected Callback Work

Callbacks should mutate retained scene state, enqueue application work, or record small pieces of
interaction state. Keep slow work outside callbacks so the next frame can follow the normal
validation, planning, DRP2 emission, and runtime execution path.

Good callback actions:

- update visual data, visibility, selection, annotations, or controller state;
- enqueue a query or consume a fresh query result;
- update application-owned mode/toggle state;
- request another frame through the host loop.

Avoid:

- destroying borrowed backend handles;
- blocking on long CPU or I/O work;
- storing borrowed event/result pointers after return;
- reimplementing standard controller navigation in raw input callbacks.

## Common Callback Classes

| Class | Typical purpose | Notes |
| --- | --- | --- |
| Frame/timer | Animation, streaming data, periodic updates. | Mutate retained state and let normal frame planning handle the render. |
| Input | Custom shortcuts, gestures, or host integration. | Use controllers first for standard pan/zoom/3D navigation. |
| Query/readback | Apply pick/probe/query results to hover, selection, readouts, or app state. | Check request identity/freshness before mutating current state. |
| Host integration | Qt, GLFW external surfaces, GUI controls. | The host owns host UI state; Datoviz owns retained scene state. |

## Threading

Unless a specific API says otherwise, call Datoviz scene and runtime APIs from the same thread or
host event context that owns the app/runtime. Treat callbacks as part of that owner thread.

If another thread produces data, hand off immutable data or application-owned messages to the owner
thread, then update Datoviz retained state there.

## See Also

- [Input events](../how-to/input-events.md)
- [Controllers](controllers.md)
- [Queries](queries.md)
- [Objects and lifetimes](objects-and-lifetimes.md)
