# Byrne I.47 Android draft

This target packages Book I, Proposition 47 as a separate NativeActivity APK so it can coexist with the III.1 draft.

- package: `org.isomorphismes.byrne.euclid.i47`
- runtime: C geometry/raster substrate plus pinned Lua proposition logic
- ABI: `armeabi-v7a`, Android API 21 minimum / 34 target
- application DEX: none
- signer: the repository's existing pinned public development signer; the producer verifies both key bytes and certificate digest before packaging
- Lua: commit `6e22fedb74cf0c9b6656e9fce8b7331db847c605`, compiled into the native library; `lua/book1_prop47.lua` is packaged as an APK asset

The interaction keeps the right angle exact. Drag the blue endpoint along one leg or the yellow endpoint along the other; the three squares and Euclid/Byrne proof lines recompute continuously.

## C / Lua boundary

C owns the exact numerical geometry kernel, NativeActivity glue, pointer capture, and software raster primitives. Lua owns the proposition state, the call sequence into the C geometry kernel, leg constraints and clamping, layout coordinates, captions, and Byrne colour-role assignments.

The Lua file is not generated into a C string. Host tests load the same `lua/book1_prop47.lua` file that the APK packages under `assets/book1_prop47.lua`. The Android adapter reads that asset through `AAssetManager` and initializes the proposition runtime from those bytes.

The build fails closed if the Lua submodule is absent, dirty, or at a commit other than the pinned gitlink. It also fails on a dirty source tree, wrong NDK/build-tools/platform inputs, a changed signer, a non-ARM32 native library, DEX/C++ leakage, missing Lua asset, alignment failure, or a package-identity mismatch.

Run the same six-input producer shape as III.1, but with `android/i47/build.grease`.

The host test covers generated right triangles, square construction, orthogonality, the area identity, Lua state synchronization, Lua-owned presentation values, endpoint capture, and constrained leg dragging. The build also writes a 576×1152 PPM preview for visual inspection before installing on the MIRO A1.
