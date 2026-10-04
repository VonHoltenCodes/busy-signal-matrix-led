// Renders unscaled frames from the firmware's own drawing code, named the
// way sim/rawframes.js names them, so the two can be diffed pixel for pixel.
//
//   hostsim <key> <frames> <outdir>
//
// key is a scene key, "lab" (idle pass, come in) or "labmsg" (message pass,
// busy). Must stay in step with rawframes.js.
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "Gfx.h"
#include "Scenes.h"
#include "Lab.h"

static const char *LAB_MSG = "BACK IN 10 MIN";

int main(int argc, char **argv) {
  if (argc < 4) { fprintf(stderr, "usage: hostsim <key> <frames> <outdir>\n"); return 2; }
  std::string key = argv[1];
  int frames = atoi(argv[2]);
  std::string out = argv[3];
  float dur;
  int idx = -1;
  if (key == "lab") dur = labCycle("");
  else if (key == "labmsg") dur = labCycle(LAB_MSG);
  else {
    idx = sceneIndexByKey(key.c_str());
    if (idx < 0) { fprintf(stderr, "no such scene: %s\n", key.c_str()); return 1; }
    dur = SCENES[idx].dur;
  }
  for (int f = 0; f < frames; f++) {
    float t = ((float)f / frames) * dur;
    gResetClip();
    gClear(0);
    if (key == "lab") labFrame(t, false, "");
    else if (key == "labmsg") labFrame(t, true, LAB_MSG);
    else SCENES[idx].fn(t);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s_%02d.ppm", out.c_str(), key.c_str(), f);
    FILE *fp = fopen(path, "wb");
    if (!fp) { perror(path); return 1; }
    fprintf(fp, "P6\n64 32\n255\n");
    fwrite(gFb, 1, sizeof(gFb), fp);
    fclose(fp);
  }
  return 0;
}
