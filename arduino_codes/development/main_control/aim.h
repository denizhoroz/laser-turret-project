// aim.h — parallax compensation.
// Camera and laser are mounted on different rigs; their axes don't converge
// at zero. On the firing edge (false→true) shift pitch UP by
// PITCH_AIM_OFFSET_STEPS; on the firing-off edge shift back DOWN so
// vision-driven tracking resumes from the camera-aligned position.
#pragma once

void aim(bool firingOn);
