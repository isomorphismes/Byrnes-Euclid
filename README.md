# Byrne's Euclid

Touch-first, draggable applets and small Android packages for individual propositions from Oliver Byrne's edition of Euclid.

Current Android target: Book III, Proposition 1 — find the centre of a given circle.

## Implementation contract

- Android runtime/application code is C, not C++.
- Build/package through the NDK/NativeActivity route recorded in `isomorphisms/android-NDK`; do not introduce Gradle as the application build system.
- Keep Byrne's paper/black/red/blue/yellow visual language rather than replacing it with generic dynamic-geometry styling.
- GeoGebra is an algorithm/reference oracle for geometry, not an application dependency or a reason to import its Java/Gradle architecture.
- Idris type files are design/specification material only; they are not runtime APK dependencies.

## Current sketches

- [Euclid III.1 type sketch](types/Byrne/BookIII/Proposition1.idr) — construction-state model that deliberately gives proposition code no direct way to read a circle's stored centre.
- [Touch interaction](docs/touch-interaction.md) — engine-neutral interaction contract adapted from the working Wegert phone UI.
- [Touch contract](code/touch_contract.h) — small C vocabulary for capture, hit radius, and drag threshold.
- [Wegert mirror](mirrors/wegert/PROVENANCE.md) — pinned local copies of the NDK touch, JNI, and direct-DEX examples so a fork does not need to reconstruct the Android path from another repository.

The mirror is reference material, not a dependency. Proposition geometry remains separate from platform input and packaging.
