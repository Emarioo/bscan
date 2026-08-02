

**Restrictions**
- Background is static. We can subtract the background from the current image.
- The body is distinct from the background. No consideration for blue shirt in front of blue wall.
- One human body in the image.
- Lightning is normal, room lamps on, no special equipment. Should work at day and night.


**What we trying to do**
2D pixel array from camera into bone positions in 3D space.
We extract head,torso,legs,arms from image. We determine their position
and orientation relative to each other.


