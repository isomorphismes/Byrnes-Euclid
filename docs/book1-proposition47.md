# Book I, Proposition 47 — draft interaction

Reference: Oliver Byrne, *The First Six Books of the Elements of Euclid* (1847), Book I, Proposition XLVII, pp. 48–49. A useful public scan is maintained by UBC at:

https://personal.math.ubc.ca/~cass/euclid/book1/images/bookI-prop47.html

## Draft choice

This draft treats the proposition as an area identity that survives continuous deformation of a right triangle.

The right-angle vertex is fixed. The blue endpoint moves only along the blue leg and the yellow endpoint only along the yellow leg. Those constraints are deliberate: the interaction cannot accidentally turn the figure into a non-right triangle while still displaying the Pythagorean conclusion.

For each state the program reconstructs all three squares from the two leg lengths. The red square is built outward from the hypotenuse, the blue square from the blue leg, and the small dark square from the yellow leg. A perpendicular from the right-angle vertex through the hypotenuse divides the red square in the classical Euclid I.47 manner; two additional black joins echo Byrne's proof diagram.

The executable checks are not the proof. They are regressions for the implementation: right-angle residual, square residual, and normalized area residual. The default numerical panel is off in the Android build and can be toggled from the large upper-right control.

## Runtime boundary

The proposition module does not depend on Android. `code/proposition47.c` constructs the Euclidean figure and reports residuals. `code/i47_applet.c` owns touch constraints and screen mapping. `code/i47_render.c` owns Byrne-style presentation. `android/i47/native_main.c` is only the NativeActivity adapter.

This is a separate package from III.1 so both drafts can be installed together on a phone.
