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
    
    std::string pattern;
    std::vector<std::string> target;
    

    Cube() :
      pos(ci::Vec3i::zero()),
      item(false),
      moving(false),
      sw(false)
    {}
    
    Cube(const ci::Vec3f& cube_pos) :
      pos(cube_pos),
      item(false),
      moving(false),
      sw(false)
    {}
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

  
  Stage() = default;
  
  Stage(const ci::JsonTree& params) {
    size = ci::Vec2i::zero();
    
    int z = 0;
    for (const auto& rows : params["body"]) {
      std::vector<Cube> line;

      int x = 0;
      for (const auto& p : rows) {
        ci::Vec3i cube_pos(x, p.getValue<int>(), z);
        line.emplace_back(cube_pos);

        x += 1;
      }
      body.push_back(std::move(line));

      size.x = std::max(size.x, x);
      z += 1;
    }
    size.y = z;

    color    = Json::getColor<float>(params["color"]);
    bg_color = Json::getColor<float>(params["bg_color"]);

    x_offset = Json::getValue<int>(params, "x_offset", 0);
    pickable = Json::getValue<int>(params, "pickable", 0);

    build_speed    = Json::getValue(params, "build_speed", 0.0f);
    collapse_speed = Json::getValue(params, "collapse_speed", 0.0f);
    auto_collapse  = Json::getValue(params, "auto_collapse", 0.0f);

    light_tween = Json::getValue(params, "light_tween", std::string("default"));
    
    if (params.hasChild("items")) {
      for (const auto& p : params["items"]) {
        const auto pos = Json::getVec3<int>(p);
        auto* cube = getCube(pos);
        if (cube) {
          cube->item = true;
        }
      }
    }
    
    if (params.hasChild("moving")) {
      for (const auto& p : params["moving"]) {
        const auto pos = Json::getVec3<int>(p["entry"]);
        auto* cube = getCube(pos);
        if (cube) {
          cube->moving = true;

          // 文字列の状態で編集するため
          // 移動パターンを取り出して変換している
          std::ostringstream pattern;
          size_t index = 0;
          for (const auto& value : p["pattern"]) {
            if (index > 0) pattern << ", ";
            pattern << value.getValue<int>();
            index += 1;
          }
          cube->pattern = pattern.str();
        }
      }
    }
    
    if (params.hasChild("switches")) {
      for (const auto& p : params["switches"]) {
        const auto pos = Json::getVec3<int>(p["position"]);
        auto* cube = getCube(pos);
        if (cube) {
          cube->sw = true;

          // 文字列の状態で編集するため
          // 移動パターンを取り出して変換している
          for (const auto& value : p["target"]) {
            std::ostringstream target;
            size_t index = 0;
            for (const auto& v : value) {
              if (index > 0) target << ", ";
              target << v.getValue<int>();
              index += 1;
            }
            cube->target.push_back(target.str());
          }
        }
      }
    }
  }


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


private:
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
