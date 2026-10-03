// Just enough of Arduino.h to build the firmware's drawing code on a PC,
// so parity.sh can diff it against the simulator.
#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <algorithm>
using std::max;
using std::min;
