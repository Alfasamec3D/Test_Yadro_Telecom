#pragma once
enum ResourceType {
  RES_IRON = 0,
  RES_GOLD = 1,
  RES_GEMS = 2,
  RES_EXP = 3,
  RES_COUNT = 4
};

static const int BASE_VALUES[RES_COUNT] = {7, 11, 23, 1};

inline const char* RES_NAMES[RES_COUNT] = {"iron", "gold", "gems", "exp"};