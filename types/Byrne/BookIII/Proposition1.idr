module Byrne.BookIII.Proposition1

%default total
%unbound_implicits off

-- Design-only type sketch for Euclid III.1 in the Byrne app.
--
-- Runtime implementation remains C through the Android NDK/NativeActivity
-- path.  This file is not an APK dependency and does not choose a numeric
-- representation or renderer.
--
-- The central design rule is that proposition code receives only a Circle
-- handle.  There is deliberately no field or function that reveals a stored
-- centre.  The centre must be obtained through the Euclidean construction.

public export
data Point = PointRef Nat

public export
data Line = LineRef Nat

public export
data Circle = CircleRef Nat

public export
data ByrneColour
  = ByrnePaper
  | ByrneBlack
  | ByrneRed
  | ByrneBlue
  | ByrneYellow

-- A proper chord is created by the geometry kernel only after both selected
-- endpoints have been accepted as distinct points on the same circle.
public export
record Chord (circle : Circle) where
  constructor MkChord
  chord_start : Point
  chord_finish : Point

-- Euclid III.1 first bisects an arbitrary chord.
public export
record BisectedChord (circle : Circle) where
  constructor MkBisectedChord
  source_chord : Chord circle
  chord_midpoint : Point

-- The line through the chord midpoint perpendicular to the chord.
public export
record PerpendicularAxis (circle : Circle) where
  constructor MkPerpendicularAxis
  source_bisection : BisectedChord circle
  axis_line : Line

-- Intersect that perpendicular with the given circle in two points.
-- The type keeps those intersections attached to the same original circle
-- and to the construction that produced their line.
public export
record CircleCut (circle : Circle) where
  constructor MkCircleCut
  source_axis : PerpendicularAxis circle
  cut_first : Point
  cut_second : Point

-- Bisect the segment joining the two circle intersections.  This is the
-- candidate centre produced by the construction, before the proposition's
-- correctness invariant is discharged.
public export
record CenterCandidate (circle : Circle) where
  constructor MkCenterCandidate
  source_cut : CircleCut circle
  candidate_center : Point

-- A LocatedCenter is stronger than a bare Point: it can only be returned by
-- the proposition verifier/kernel after the III.1 invariant has been
-- established for the candidate and the original circle.
public export
record LocatedCenter (circle : Circle) where
  constructor MkLocatedCenter
  source_candidate : CenterCandidate circle
  located_center : Point

-- Engine-facing operation shapes.  These are semantic contracts, not an
-- instruction to copy GeoGebra's Java/Gradle architecture.  GeoGebra can be
-- used as an oracle/reference for the geometry while the Android executable
-- remains C + NDK.
public export
ChooseChord : Type
ChooseChord =
  (circle : Circle) ->
  Point ->
  Point ->
  Maybe (Chord circle)

public export
BisectChord : Type
BisectChord =
  (circle : Circle) ->
  Chord circle ->
  BisectedChord circle

public export
ConstructPerpendicularAxis : Type
ConstructPerpendicularAxis =
  (circle : Circle) ->
  BisectedChord circle ->
  PerpendicularAxis circle

public export
CutCircleWithAxis : Type
CutCircleWithAxis =
  (circle : Circle) ->
  PerpendicularAxis circle ->
  Maybe (CircleCut circle)

public export
BisectCircleCut : Type
BisectCircleCut =
  (circle : Circle) ->
  CircleCut circle ->
  CenterCandidate circle

public export
VerifyCenter : Type
VerifyCenter =
  (circle : Circle) ->
  CenterCandidate circle ->
  Maybe (LocatedCenter circle)

-- The complete III.1 path.  Embedding each earlier record in the next makes
-- the dependency chain explicit:
--
-- circle
--   -> chord
--   -> chord midpoint
--   -> perpendicular axis
--   -> two circle intersections
--   -> midpoint candidate
--   -> verified centre
--
-- A future proof-oriented refinement can replace Maybe with explicit evidence
-- for: distinct chord endpoints, point-on-circle, midpoint, perpendicularity,
-- line/circle intersection, equal radii, and the final CenterOf relation.
