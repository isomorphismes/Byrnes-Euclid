# Byrne I.47 Android draft

This target packages Book I, Proposition 47 as a separate NativeActivity APK so it can coexist with the III.1 draft.

- package: `org.isomorphismes.byrne.euclid.i47`
- runtime: C, software-rendered RGBA NativeActivity
- ABI: `armeabi-v7a`, Android API 21 minimum / 34 target
- application DEX: none
- signer: the repository's existing pinned public development signer; the producer verifies both key bytes and certificate digest before packaging
- Lua: pinned in the repository but not needed by this proposition

The interaction keeps the right angle exact. Drag the blue endpoint along one leg or the yellow endpoint along the other; the three squares and Euclid/Byrne proof lines recompute continuously. The red square is the square on the hypotenuse. The blue and black blocks are the two side squares, following the visual arrangement in Byrne's page 48.

Run the same six-input producer shape as III.1, but with `android/i47/build.grease`. It fails on a dirty source tree, wrong NDK/build-tools/platform inputs, a changed signer, a non-ARM32 native library, DEX/C++ leakage, alignment failure, or a package-identity mismatch.

The host test covers generated right triangles, square construction, orthogonality, the area identity, endpoint capture, and constrained leg dragging. The build also writes a 576×1152 PPM preview for visual inspection before installing on the MIRO A1.
