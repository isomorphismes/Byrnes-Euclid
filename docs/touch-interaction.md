# Touch interaction sketch

The implemented first target is Euclid III.1: find the centre of a given circle.
The earlier I.11 examples below remain reusable interaction design material.
For III.1, the red chord endpoints are constrained inputs; all midpoint,
perpendicular, diameter, and centre objects are dependent. The fixed initial
view implements capture, density-aware hit testing, movement threshold,
angle-offset dragging without acquisition jumps, and cancel handling. Empty
space does not pan, and adding a second finger cancels capture until release.
Pan/pinch remain later view features rather than changing dependent geometry.

The touch layer should feel like moving a diagram, not operating a CAD program. The geometry engine owns exact constraints. Touch owns approximate selection and human movement intent.

## Copy from Wegert

Wegert already has the right physical interaction model:

- density-aware screen-space hit targets around objects that may be visually much smaller than a fingertip;
- capture the object on finger-down and keep that capture until release;
- a small movement threshold separates a tap from a drag;
- dragging starts from the object's original position, so acquiring a nearby hit target does not make the object jump to the finger center;
- one-finger drag on empty space pans the view;
- two fingers pan and pinch-zoom together;
- UI controls can block geometry gestures rather than accidentally becoming a pan or pinch;
- cancellation clears transient gesture state.

The important distinction is: **finger selection is approximate; the geometry is not.**

A 24 dp hit radius does not mean a point has become a 24 dp mathematical disk. It only means the finger may acquire that exact point from nearby screen coordinates.

## Euclid object roles

Each proposition should mark geometric objects by interaction role rather than by platform widget type.

- `free`: directly draggable input to the construction.
- `constrained`: draggable only along its defining locus, for example the chosen point of I.11 sliding along the given line.
- `dependent`: never directly dragged; recomputed from the construction whenever an input moves.
- `view`: empty-space pan / pinch state, separate from proposition geometry.

For I.11, moving the given point or the admissible free construction input should immediately recompute the copied distance, equilateral triangle, apex, and final perpendicular.

## Common gesture contract

1. Finger down performs screen-space hit testing.
2. If an admissible object is hit, capture its stable object identity.
3. Otherwise capture the view.
4. Do nothing geometric until movement exceeds the drag threshold.
5. On every accepted move, update only the captured free/constrained input.
6. Recompute all dependent geometry.
7. Redraw immediately using Byrne's color relationships.
8. Finger up or cancel releases the capture.

Dependent objects should not silently become free just because the finger landed on them.

## Android NDK adapter

The current Wegert implementation uses the small native surface we need:

- `AInputEvent_getType`
- `AMotionEvent_getAction`
- `AMotionEvent_getPointerCount`
- `AMotionEvent_getX`
- `AMotionEvent_getY`
- `AMOTION_EVENT_ACTION_DOWN`
- `AMOTION_EVENT_ACTION_POINTER_DOWN`
- `AMOTION_EVENT_ACTION_MOVE`
- `AMOTION_EVENT_ACTION_POINTER_UP`
- `AMOTION_EVENT_ACTION_UP`
- `AMOTION_EVENT_ACTION_CANCEL`
- `AConfiguration_getDensity`

Those Android details should terminate at an adapter. The proposition and geometry code should receive ordinary touch positions and gesture state, not `AInputEvent *`.

## Browser adapter

A browser version can map Pointer Events onto the same contract. The proposition should not know whether a drag originated in Android NDK input, browser Pointer Events, or another frontend.

## Local mirrors

`mirrors/wegert/` is intentionally redundant. It pins the known-good touch, JNI, and direct-DEX examples in this repository instead of making a future fork reconstruct the path from references to other repositories.

The mirror is reference material, not a dependency. Provenance and license stay beside it.
