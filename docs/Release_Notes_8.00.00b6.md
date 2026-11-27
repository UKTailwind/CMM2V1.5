# Colour Maximite 2: MMBasic 8.00.00b6 (beta) release notes

*Draft, begun 27 November 2026 with a known problem found since 8.00.00b5. The rest is written at the release (docs/Release_Checklist.md, section 8).*

## Known problems

- **STEPPER arcs (G2, G3) run faster than their feed rate** wherever they are not moving along an axis. The firmware holds the speed of the faster-moving axis to the feed rate, not the speed along the arc. Along an axis an arc goes at the feed rate; elsewhere it goes faster, up to 1.41 times the feed at 45° and about 1.11 times on average round a circle. No axis goes over its STEPPER AXIS maximum speed, but a cutter, laser or pen moves along an arc faster than programmed. Straight moves (G0, G1) are not affected. On a Generation 3, a semicircle of radius 50 mm at F3000 (50 mm/s) reached 70.3 mm/s and took 3.10 s, where a G1 line of the same length went at 49.7 mm/s and took 3.41 s. The STEPPER code is the PicoMite's, which has the same fault (PicoMite issue #161); it will be corrected there first and then here. The arc figures in Stepper_Reference.pdf (a circle of 36 G2 arcs at 73-98% of the feed) include this overspeed.
