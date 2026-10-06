module Byrne.BookI.Proposition47

%default total

public export
data ByrneColour = Red | Blue | Yellow | Black | Paper

public export
record Point2 where
  constructor MkPoint2
  x : Double
  y : Double

public export
record PositiveLength where
  constructor MkPositiveLength
  value : Double

public export
record RightTriangle where
  constructor MkRightTriangle
  rightAngle : Point2
  blueLeg : PositiveLength
  yellowLeg : PositiveLength
  rotation : Double

public export
record SquareAreaPicture where
  constructor MkSquareAreaPicture
  hypotenuseSquare : Double
  blueSquare : Double
  yellowSquare : Double
  residual : Double

square : Double -> Double
square x = x * x

public export
pythagoreanPicture : RightTriangle -> SquareAreaPicture
pythagoreanPicture triangle =
  let blue = value triangle.blueLeg
      yellow = value triangle.yellowLeg
      redSquared = square blue + square yellow
  in MkSquareAreaPicture redSquared (square blue) (square yellow)
       (redSquared - square blue - square yellow)

-- Interaction intent, not an Android API: only the two positive leg lengths vary.
-- A renderer may rotate or scale the whole picture, but dragging cannot destroy
-- the right-angle invariant encoded by RightTriangle.
