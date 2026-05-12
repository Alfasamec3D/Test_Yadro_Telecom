#pragma once
#include <map>
#include <string>

enum ResourceType {
  RES_IRON = 0,
  RES_GOLD = 1,
  RES_GEMS = 2,
  RES_EXP = 3,
};


const std::map<ResourceType, int> BASE_VALUES = {
    {RES_IRON, 7}, {RES_GOLD, 11}, {RES_GEMS, 23}, {RES_EXP, 1}};

inline const std::map<ResourceType, std::string> RES_NAMES = {
    {RES_IRON, "iron"},
    {RES_GOLD, "gold"},
    {RES_GEMS, "gems"},
    {RES_EXP, "exp"}};