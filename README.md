# Byrne's Euclid

Touch-first, draggable applets and small Android packages for individual propositions from Oliver Byrne's edition of Euclid.

Current Android target: Book III, Proposition 1 — find the centre of a given circle.

The C implementation, live constrained chord interaction, paper/Byrne renderer,
numerical acceptance tests and DEX-free ARMv7 package build are in place.
See [native build and acceptance](android/README.md). Drag the red endpoints;
the yellow centre ring is obtained by bisecting the black constructed diameter.
Tap the large CHECKS control at the upper right to toggle numerical checks.

## Book I.47 draft

A second native draft lives alongside III.1: [Book I, Proposition 47](docs/book1-proposition47.md), the Pythagorean theorem in Byrne's visual language. It has its own package identity and producer under [`android/i47`](android/i47/README.md), so both APKs can coexist on a phone.

I.47 now uses the intended C + Lua split. C owns exact geometry, Android plumbing, pointer capture, and raster primitives. The pinned Lua runtime executes [`lua/book1_prop47.lua`](lua/book1_prop47.lua), which owns proposition state, recomputation sequencing, constrained leg dragging, layout, captions, and Byrne colour roles. The same Lua source is used by host tests and packaged as an APK asset.

Drag the blue or yellow endpoint to change the two legs while preserving the right angle. The red hypotenuse square, two side squares, proof lines, and numerical area checks recompute from the constrained geometry.

## Implementation contract

- Android low-level runtime/application substrate is C, not C++.
- Proposition-level behavior may use the pinned Lua submodule where it makes the code simpler and more reusable.
- Build/package through the NDK/NativeActivity route recorded in `isomorphisms/android-NDK`; do not introduce Gradle as the application build system.
- Keep Byrne's paper/black/red/blue/yellow visual language rather than replacing it with generic dynamic-geometry styling.
- GeoGebra is an algorithm/reference oracle for geometry, not an application dependency or a reason to import its Java/Gradle architecture.
- Idris type files are design/specification material only; they are not runtime APK dependencies.
- Lua comes from the pinned `third_party/lua` git submodule. Build scripts must not fetch or select a floating Lua version.

## Current sketches

- [Euclid III.1 type sketch](types/Byrne/BookIII/Proposition1.idr) — construction-state model that deliberately gives proposition code no direct way to read a circle's stored centre.
- [Euclid I.47 type sketch](types/Byrne/BookI/Proposition47.idr) — right-triangle/area-state model for the Pythagorean draft.
- [Touch interaction](docs/touch-interaction.md) — engine-neutral interaction contract adapted from the working Wegert phone UI.
- [Touch contract](code/touch_contract.h) — small C vocabulary for capture, hit radius, and drag threshold.
- [Wegert mirror](mirrors/wegert/PROVENANCE.md) — pinned local copies of the NDK touch, JNI, and direct-DEX examples so a fork does not need to reconstruct the Android path from another repository.

The mirror is reference material, not a dependency. Proposition geometry remains separate from platform input and packaging.
