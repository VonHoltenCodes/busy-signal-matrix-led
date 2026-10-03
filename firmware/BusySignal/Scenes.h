// Jurassic Park birthday show. Each scene is a pure function of elapsed
// seconds, drawn into the Gfx framebuffer. Ported line-for-line from
// sim/scenes.js - keep the two in step when either changes.
#pragma once
#include <Arduino.h>

struct SceneDef {
  const char *key;
  const char *name;
  float dur;              // seconds before the party playlist advances
  void (*fn)(float t);
};

extern const SceneDef SCENES[];
extern const int SCENE_COUNT;

int sceneIndexByKey(const char *key);
