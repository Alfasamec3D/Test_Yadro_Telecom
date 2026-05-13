#pragma once

#include <cstddef>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "Resources.hpp"

namespace Terracraft {

class Room final {
 private:
  const size_t id_;
  const std::vector<size_t> neighbors_;
  std::map<ResourceType, int> resources_;
  std::set<ResourceType> grabbed_resources_;

 public:
  Room(const size_t& id, std::vector<size_t>::const_iterator start_neigh,
       std::vector<size_t>::const_iterator end_neigh,
       const std::map<ResourceType, int>& resources)
      : id_(id), neighbors_(start_neigh, end_neigh), resources_(resources) {}

  const std::vector<size_t>& neighbors() const { return neighbors_; }

  const std::map<ResourceType, int>& resources() const { return resources_; }

  const std::set<ResourceType>& grabbed_resources() const {
    return grabbed_resources_;
  }

  int grab(ResourceType type) {
    grabbed_resources_.insert(type);
    int loot = resources_[type];
    resources_[type] = 0;
    return loot;
  }

  friend std::ostream& operator<<(std::ostream& os, const Room& room) {
    os << "state " << room.id_;

    for (const auto& [type, count] : room.resources_) {
      os << ' ';
      if (room.grabbed_resources().count(type))
        os << '_';
      else
        os << count;
    }
    return os;
  }
};

class Dungeon final {
 private:
  const ResourceType target_;
  std::vector<Room> rooms_;

 public:
  Dungeon(const ResourceType& target, std::vector<Room>::iterator start_rooms,
          std::vector<Room>::iterator end_rooms)
      : target_(target), rooms_(start_rooms, end_rooms) {}

  const ResourceType& target() const { return target_; }

  Room& room_id(const size_t& id) { return rooms_[id]; }

  const Room& room_id(const size_t& id) const { return rooms_[id]; }

  size_t N() const { return rooms_.size() - 1; }
};
}  // namespace Terracraft