# Native Byrne III.1

Package `org.isomorphismes.byrne.euclid.iii1` runs framework
`android.app.NativeActivity`, `hasCode=false`, and one C library
`lib/armeabi-v7a/libbyrne.so`. It uses NDK native_app_glue, AInputQueue,
ALooper and an RGBA ANativeWindow software buffer. No GPU, JNI, DEX,
Java/Kotlin application, C++, Gradle, Idris runtime or Lua runtime is needed.

## Build

Provision the exact inputs in `inputs.lock` on an x86_64 Linux build host.
The script downloads nothing. The pinned SDK is Android 34 extension 12,
revision 1; the build-tools installation must be 36.0.0; the NDK must be
29.0.14206865. A clean committed application checkout is mandatory.

Invoke the current Grease/Oils YSH implementation pinned by
`dilapidated-shed/grease@f19c94c6df18cddbdc1e81463e5bd689533e3c13` to
Oils `6d29702a10ea9eb72a43950554dbcd4174d07a89`:

`ysh /absolute/repository/android/build.grease /absolute/repository /absolute/ndk /absolute/build-tools /absolute/platform/android.jar /absolute/output /absolute/host-cc`

All six arguments are explicit absolute paths; the launch directory is arbitrary.
Do not interpret this script with Bash. The inherited Python-2 Grease
implementation and compiled Grease are both supported entrypoints for YSH;
the local receipt records which one actually ran. No mobile compilation.

The build runs the C tests, compiles the C app and separately compiles NDK's
own C glue, links, strips, packages with aapt2, adds the native library without
compression, aligns, signs, verifies, and inspects the final manifest and ABI.
Only the NDK glue receives its required unused-parameter warning exemption.
The app sources retain `-Werror`. The existing public stable test signer is
checked before compilation and after signing; see `signing/README.md`.

Output includes the APK, SHA256SUMS, numerical test output, host preview,
ELF inspection, manifest, package contents, signer verification and a build
receipt. Missing tools, a dirty tree, input-version or signer mismatch fails
closed. No alternate framework or build route exists.

## Construction and numerical domain

`geometry.h` exposes an opaque Circle. Only `geometry.c` can inspect its centre.
`proposition1.c` chooses a validated chord, bisects it, constructs the
perpendicular, obtains two cuts from the circle primitive and bisects that
diameter. The final candidate is checked against sixteen sampled boundary
points. It never reads the stored centre. A circle renderer or boundary
constraint may use kernel operations; those do not supply the proposition's
answer. The Idris sketch is preserved unchanged.

All geometry uses double precision. Residuals and drift are divided by radius;
acceptance tolerance is `1e-9` of radius. The finite kernel domain limits radius
to `[1e-6,1e9]` and centre coordinates to at most `1e6` radii. A chord shorter
than `1e-5` radii is rejected. The UI keeps endpoint angle separation at least
0.08 radians for legibility. A diameter is an admissible chord. Tangent, miss,
nonfinite and zero-direction inputs have separate statuses. Some translated,
very short chords amplify representational error beyond the budget and are
explicitly rejected by the final numerical checks. No stale valid centre is
shown after rejection.

Tests include literal midpoint and intersection fixtures, tangent/miss and
degenerate inputs, sixteen/31 independent equal-radius samples, translations,
tiny radii, near-degenerate cases and 1,440 generated chords. The same portable
drag state runs through 720 positions with pointer capture and no-jump tests.
Host geometry and raster previews are separate from Android launch evidence.

## Visual and reference provenance

`isomorphismes/byrne-euclid@10744dfafb1eceea3023df12e2b5d9744d324c42`,
`byrne-en-latex.tex`, Book III, Prop. I (lines 4753 onward), supplies the blue
circle, red solid/dashed chord halves and black diameter. `byrnebook.cls`
lines 341–344 supply RGB values. This rendition is by jemmybutton, CC-BY-SA
4.0 for the book; no source text, artwork or library was imported. Warm paper
and visible handles adapt the construction to touch. Typography is a small
fixed raster font for this initial execution slice.

Yellow disk: chord midpoint. Blue outlined rings: diameter endpoints.
Yellow outlined ring with black cross: constructed centre. Dashed black
extensions distinguish the infinite perpendicular from the solid diameter.
The blue right-angle mark makes perpendicularity visible. On a diametral
chord, the two midpoint markers coincide and remain distinct by shape.

`isomorphisms/android-NDK@d4a4719fb97e8031476bf822697dce8423bc9030`,
`ARCHITECTURE.md` and `apk/README.md`, define the reusable native-only package
route. This build follows that route without inheriting the historical DEX
harness's signer generation or mutable build-tools discovery.

`isomorphismes/geogebra@4873cd881d12dd11736826e4c5c5ede162a33deb`,
`AlgoIntersectLineConic.java` and `AlgoOrthoLinePointLine.java` in
`source/shared/common/src/main/java/org/geogebra/common/kernel/algos/`,
were inspected for perpendicular semantics and secant/tangent/miss handling.
No GeoGebra code or execution is claimed. The C kernel uses the circle-specific
projection solution; its tests are independent fixtures, not a GeoGebra oracle
run. Lua remains pinned at `6e22fedb74cf0c9b6656e9fce8b7331db847c605`
(5.4.8), unused and never fetched by this build.

## Physical MIRO A1 acceptance

Download the exact APK and verify its SHA-256 against the associated receipt.
Tap the APK to install; preserve package data with replacement installation.
Do not uninstall to resolve a signing conflict. Check the model is MIRO A1,
Android 14, with armeabi-v7a support. The app requests no permissions.

Launch, drag each red endpoint around the blue circle, and cross a diameter.
The yellow centre ring should stay still. Check the red halves remain equal,
the right-angle mark stays square, and both blue intersection rings stay on
the circle. Tap the outlined CHECKS control at the upper right to show numerical checks; residuals should
stay below `1e-9`, and UPDATES should increase. Switch away/back and check the
app redraws. Check selection does not jump when acquired near a handle.
Two-finger input cancels the drag until all fingers lift; no pan/zoom in this
fixed first-proposition view. Record failures with a screenshot and the APK
digest. Logcat alone cannot establish visual correctness.

For an existing verified Termux rish entrypoint, the one-command equivalent
launch is `rish -c 'am start -W -n org.isomorphismes.byrne.euclid.iii1/android.app.NativeActivity'`.
`android/phone-check.grease` provides the optional identity/launch probe when
an on-device Grease runtime already exists. It installs or builds nothing.
The user confirmed that version 0.1 renders and looks good on the MIRO A1,
but reported that its bottom numerical toggle could not be tapped. Version
0.1.1 moves the visible control and its full hit rectangle to the upper right.
Updated-package touch acceptance remains NOT_RUN until exercised on the phone.
