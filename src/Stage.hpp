#pragma once

//
// Stageデータ
//

#include <vector>
#include <boost/algorithm/clamp.hpp>


namespace ngs {

struct Stage {
  struct Cube {
    ci::Vec3i pos;
    bool item;
    bool moving;
    bool sw;
    bool falling;
    
    std::string pattern;                            // MovingCube
    std::vector<std::string> target;                // Switch
    float interval;                                 // FallingCube
    float delay;                                    // FallingCube

    Cube() :
      pos(ci::Vec3i::zero()),
      item(false),
      moving(false),
      sw(false),
      falling(false),
      delay(0)
    {}
    
    explicit Cube(const ci::Vec3f& cube_pos) :
      Cube()
    {
      pos = cube_pos;
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

  std::string light_tween;

  ci::Vec2i size;

  
  void toggleItem(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      cube->item = !cube->item;
    }
  }
  
  void toggleMoving(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      cube->moving = !cube->moving;
    }
  }

  void toggleSwitch(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      cube->sw = !cube->sw;
    }
  }

  void toggleFalling(const ci::Vec2i& pos) {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    if (cube) {
      cube->falling = !cube->falling;
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
    return cube && cube->item;
  }

  bool isMovingCube(const ci::Vec2i& pos) const {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    return cube && cube->moving;
  }
  
  bool isSwitchCube(const ci::Vec2i& pos) const {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    return cube && cube->sw;
  }

  bool isFallingCube(const ci::Vec2i& pos) const {
    auto* cube = getCube(ci::Vec3i(pos.x, 0, pos.y));
    return cube && cube->falling;
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

};

}
