#pragma once

//
// Stageデータ
//

#include <vector>
#include <boost/algorithm/clamp.hpp>


namespace ngs {

struct Stage {
  
  struct Cube {
    enum {
      NONE    = 0,
      ITEM    = 1 << 0,
      MOVING  = 1 << 1,
      SWITCH  = 1 << 2,
      FALLING = 1 << 3,
      ONEWAY  = 1 << 4,
    };
  
    ci::Vec3i pos;
    int type;
    
    std::string pattern;                            // MovingCube
    std::vector<std::string> target;                // Switch
    float interval;                                 // FallingCube
    float delay;                                    // FallingCube
    std::string direction;                          // Oneway
    int power;                                      // Oneway

    Cube() :
      pos(ci::Vec3i::zero()),
      type(NONE),
      interval(0.0),
      delay(0),
      direction("up"),
      power(0)
    {}
    
    explicit Cube(const ci::Vec3f& cube_pos) :
      Cube()
    {
      pos = cube_pos;
    }

    void cleanup() {
      pattern.clear();
      target.clear();
      
      interval = 0.0f;
      delay    = 0.0f;

      direction = std::string("up");
      power = 0;
    }
  };
  
  std::vector<std::vector<Cube> > body;

  ci::Color color;
  ci::Color bg_color;

  int x_offset;
  int pickable;

  float build_speed;
  float collapse_speed;
  float auto_collapse;

  std::string camera;
  
  std::string light_tween;

  ci::Vec2i size;

  
  void toggleItem(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      if (cube->type & ~Cube::ITEM) return;
      cube->type ^= Cube::ITEM;
    }
  }
  
  void toggleMoving(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      if (cube->type & ~Cube::MOVING) return;
      cube->type ^= Cube::MOVING;
    }
  }

  void toggleSwitch(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      if (cube->type & ~Cube::SWITCH) return;
      cube->type ^= Cube::SWITCH;
    }
  }

  void toggleFalling(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      if (cube->type & ~Cube::FALLING) return;
      cube->type ^= Cube::FALLING;
    }
  }
  
  void toggleOneway(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      if (cube->type & ~Cube::ONEWAY) return;
      cube->type ^= Cube::ONEWAY;
    }
  }

  void changeHeight(const ci::Vec2i& pos, const int value) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      cube->pos.y += value;

      cube->pos.y = boost::algorithm::clamp(cube->pos.y, -1, 10);
    }
  }

  void setHeight(const ci::Vec2i& pos, const int value) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      cube->pos.y = value;
    }
  }

  std::string& getPattern(const ci::Vec2i& pos) {
    static auto null_data = std::string();
    
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    
    return cube ? cube->pattern : null_data;
  }

  std::vector<std::string>& getTarget(const ci::Vec2i& pos) {
    static auto null_data = std::vector<std::string>();
    
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    
    return cube ? cube->target : null_data;
  }

  float& getInterval(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));

    static float dummy_param = 0.0f;
    return cube ? cube->interval : dummy_param;
  }

  float& getDelay(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));

    static float dummy_param = 0.0f;
    return cube ? cube->delay : dummy_param;
  }

  std::string& getDirection(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));

    static std::string dummy_param = std::string();
    return cube ? cube->direction : dummy_param;
  }
  
  int& getPower(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));

    static int dummy_param = 0;
    return cube ? cube->power : dummy_param;
  }

  
  void reduceSwitchTarget(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube && (cube->target.size() > 1)) {
      cube->target.pop_back();
    }
  }

  void addSwitchTarget(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      cube->target.push_back("0, 0, 0");
    }
  }
  

  void resize() {
    body.resize(size.y);
    
    float z = 0.0f;
    for (auto& rows : body) {
      rows.resize(size.x);
      float x = 0.0f;
      for (auto& cube : rows) {
        cube.pos.x = x;
        cube.pos.z = z;

        x += 1.0f;
      }
      z += 1.0f;
    }
  }


  bool isItemCube(const ci::Vec2i& pos) const {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    return cube && (cube->type & Cube::ITEM);
  }

  bool isMovingCube(const ci::Vec2i& pos) const {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    return cube && (cube->type & Cube::MOVING);
  }
  
  bool isSwitchCube(const ci::Vec2i& pos) const {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    return cube && (cube->type & Cube::SWITCH);
  }

  bool isFallingCube(const ci::Vec2i& pos) const {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    return cube && (cube->type & Cube::FALLING);
  }

  bool isOnewayCube(const ci::Vec2i& pos) const {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    return cube && (cube->type & Cube::ONEWAY);
  }

  const Cube* const getCube(const ci::Vec3i& pos) const {
    for (auto& rows : body) {
      for (auto& cube : rows) {
        // 高さは無視
        if ((cube.pos.x == pos.x) && (cube.pos.z == pos.z)) return &cube;
      }
    }

    return nullptr;
  }

  // TODO:constの有無でメソッドを使い分ける作戦
  Cube* const getCube(const ci::Vec3i& pos) {
    return const_cast<Cube*>(static_cast<const Stage*>(this)->getCube(pos));
  }


  void clear() {
    for (auto& row : body) {
      for (auto& cube : row) {
        cube.pos.y = 0;
        cube.type  = Cube::NONE;
        cube.cleanup();
      }
    }
  }
  
  void validate() {
    for (auto& row : body) {
      for (auto& cube : row) {
        if (cube.pos.y < 0) {
          cube.type = Cube::NONE;
        }
      }
    }
  }

};

}
