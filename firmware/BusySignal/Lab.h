// NEON PULSE LAB - the everyday face of the sign: fixed logo, corner lamps
// for busy / come in, and a jet that barrel-rolls through when idle or tows
// the message in its wake. Ported line-for-line from sim/lab.js - keep the
// two in step when either changes.
#pragma once
#include <Arduino.h>

// Seconds per pass: the idle fly-by with its quiet gap, or one message
// crossing plus its pause. msg may be empty.
float labCycle(const char *msg);

// Draws one frame into the Gfx framebuffer. t is seconds since the face
// last restarted; an empty msg flies the idle pass.
void labFrame(float t, bool busy, const char *msg);
