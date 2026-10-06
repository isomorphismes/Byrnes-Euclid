# First ARMv7 execution artifact

Built from clean source `962f0562244908c1d29ff4eabaad0b8295c0083b` using
`android/build.grease`. The adjacent receipt names all inputs and the signer.
Two independent work/output directories produced byte-identical signed APKs.
SHA-256: `c847ab7e81df8abb892bc1b7ad6e7773b547b8731a6d54cb615b266ade5b231e`.
Size: 57,763 bytes. No Lua or application DEX.

PASS: C geometry and portable pointer tests (GCC and Clang), software-renderer
host preview, ARMv7 build, ELF32/ARM, manifest/package, alignment and stable
signer. AddressSanitizer/UndefinedBehaviorSanitizer pass for geometry and
rendering with `detect_leaks=0`; this environment cannot run LeakSanitizer
because its process inspection is unavailable. Leak detection is NOT_RUN.
Missing stable signer was tested in an isolated committed fixture: exit 2,
`FAIL missing input`, and no APK. Physical device and Android emulator launch
are NOT_RUN; no connected Android runtime, adb or emulator was available.

This commit only publishes the built artifact and its evidence. It is not a
new geometry revision. Use a replacement install; do not uninstall on a
signing conflict. Drag both red handles and enable checks from the bottom
caption; see `../../android/README.md` for physical acceptance.
