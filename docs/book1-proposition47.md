# Book I, Proposition 47 — draft interaction

Reference: Oliver Byrne, *The First Six Books of the Elements of Euclid* (1847), Book I, Proposition XLVII, pp. 48–49.

Public scan:
https://personal.math.ubc.ca/~cass/euclid/book1/images/bookI-prop47.html

## Draft choice

This draft treats the proposition as an area identity that survives continuous deformation of a right triangle.

The right-angle vertex is fixed. The blue endpoint moves only along the blue leg and the yellow endpoint only along the yellow leg. Those constraints are deliberate: the interaction cannot accidentally turn the figure into a non-right triangle while still displaying the Pythagorean conclusion.

For each state the program reconstructs all three squares from the two leg lengths. The red square is built outward from the hypotenuse, the blue square from the blue leg, and the small dark square from the yellow leg. A perpendicular from the right-angle vertex through the hypotenuse divides the red square in the classical Euclid I.47 manner; two additional black joins echo Byrne's proof diagram.

The executable checks are not the proof. They are regressions for the implementation: right-angle residual, square residual, and normalized area residual. The default numerical panel is off and can be toggled from the large upper-right control.

## Runtime boundary

The refactor makes the proposition boundary explicit.

- `code/proposition47.c`: exact numerical geometry and residuals.
- `code/i47_lua_bridge.c`: narrow C ↔ Lua bridge; exposes `geometry.construct` and validates the returned state/presentation tables.
- `lua/book1_prop47.lua`: proposition state, recomputation sequencing, drag constraints, layout coordinates, labels, and colour roles.
- `code/i47_applet.c`: low-level pointer capture plus synchronization between platform input and the Lua state.
- `code/i47_render.c`: software raster primitives consuming geometry and Lua-selected presentation roles.
- `android/i47/native_main.c`: NativeActivity adapter and APK-asset loading only.

The Lua source is a real APK asset and the host tests consume exactly the same source file. The pinned Lua submodule is compiled into `libbyrne.so`; there is no Java/Kotlin layer or DEX.

This remains a separate package from III.1 so both drafts can be installed together on a phone.
