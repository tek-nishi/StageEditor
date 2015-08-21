#pragma once

//
// Stageのserialize/desirialize
//

#include "Stage.hpp"
#include "JsonUtil.hpp"


namespace ngs {
namespace StageSerializer {

ci::JsonTree makeMoving(const ci::Vec3i& pos, const std::string& pattern) {
  std::ostringstream text;
  text << "{ \"entry\": ["
       << pos.x << ","
       << pos.y << ","
       << pos.z
       << "], \"pattern\": [ "
       << pattern
       << "] }";

  return ci::JsonTree(text.str());
}
  
ci::JsonTree makeSwitch(const ci::Vec3i& pos, const std::vector<std::string>& target) {
  std::ostringstream text;
  text << "{ \"position\": ["
       << pos.x << ","
       << pos.y << ","
       << pos.z
       << "], \"target\": [ ";

  size_t index = 0;
  for (const auto& t : target) {
    if (index > 0) text << ", ";
    text << "[ " << t << " ]";
    index += 1;
  }
  text << "] }";

  return ci::JsonTree(text.str());
}

ci::JsonTree makeFalling(const ci::Vec3i& pos,
                         const float interval, const float delay) {
  std::ostringstream text;
  text << "{ \"entry\": ["
       << pos.x << ","
       << pos.y << ","
       << pos.z
       << "], \"interval\": "
       << interval << ","
       << " \"delay\": "
       << delay
       << " }";

  return ci::JsonTree(text.str());
}

ci::JsonTree makeOneway(const ci::Vec3i& pos,
                        const std::string direction, const int power) {
  std::ostringstream text;
  text << "{ \"position\": ["
       << pos.x << ","
       << pos.y << ","
       << pos.z
       << "], \"direction\": \""
       << direction << "\","
       << " \"power\": "
       << power
       << " }";

  return ci::JsonTree(text.str());
}


ci::JsonTree jsonArrayFromStageBody(const std::vector<Stage::Cube>& cubes) {
  std::ostringstream text;
  text << "[";

  size_t i = 0;
  for (const auto& cube : cubes) {
    if (i > 0) text << ",";
    ++i;

    text << cube.pos.y;
  }

  text << "]";

  return ci::JsonTree(text.str());
}
  
template <typename T>
ci::JsonTree jsonArrayFromVec3(const T& vector) {
  std::ostringstream text;
  text << "[ "
       << vector.x << ", "
       << vector.y << ", "
       << vector.z << " ]";

  return ci::JsonTree(text.str());
}
  
template <typename T>
ci::JsonTree jsonArrayFromColor(const std::string& key, const T& color) {
  std::ostringstream text;
  text << "{ \"" << key << "\":";
  text << "[ "
       << color.r << ", "
       << color.g << ", "
       << color.b << " ] }";

  return ci::JsonTree(text.str());
}


Stage deserialize(const std::string& path) {
  auto params = Json::readFromFile(path);
  
  Stage stage;

  stage.size = ci::Vec2i::zero();
    
  int z = 0;
  for (const auto& rows : params["body"]) {
    std::vector<Stage::Cube> line;

    int x = 0;
    for (const auto& p : rows) {
      ci::Vec3i cube_pos(x, p.getValue<int>(), z);
      line.emplace_back(cube_pos);

      x += 1;
    }
    stage.body.push_back(std::move(line));

    stage.size.x = std::max(stage.size.x, x);
    z += 1;
  }
  stage.size.y = z;

  stage.color    = Json::getColor<float>(params["color"]);
  stage.bg_color = Json::getColor<float>(params["bg_color"]);

  stage.x_offset = Json::getValue<int>(params, "x_offset", 0);
  stage.pickable = Json::getValue<int>(params, "pickable", 0);

  stage.build_speed    = Json::getValue(params, "build_speed", 0.0f);
  stage.collapse_speed = Json::getValue(params, "collapse_speed", 0.0f);
  stage.auto_collapse  = Json::getValue(params, "auto_collapse", 0.0f);

  stage.camera = Json::getValue(params, "camera", std::string("normal"));
  stage.light_tween = Json::getValue(params, "light_tween", std::string("default"));
    
  if (params.hasChild("items")) {
    for (const auto& p : params["items"]) {
      const auto pos = Json::getVec3<int>(p);
      auto* cube = stage.getCube(pos);
      if (cube) {
        cube->type = Stage::Cube::ITEM;
      }
    }
  }
    
  if (params.hasChild("moving")) {
    for (const auto& p : params["moving"]) {
      const auto pos = Json::getVec3<int>(p["entry"]);
      auto* cube = stage.getCube(pos);
      if (cube) {
        cube->type = Stage::Cube::MOVING;

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
      auto* cube = stage.getCube(pos);
      if (cube) {
        cube->type = Stage::Cube::SWITCH;

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

  if (params.hasChild("falling")) {
    for (const auto& p : params["falling"]) {
      const auto pos = Json::getVec3<int>(p["entry"]);

      const float interval = p["interval"].getValue<float>();
      const float delay    = p["delay"].getValue<float>();

      auto* cube = stage.getCube(pos);
      if (cube) {
        cube->type = Stage::Cube::FALLING;
        cube->interval = interval;
        cube->delay    = delay;
      }      
    }
  }

  if (params.hasChild("oneways")) {
    for (const auto& p : params["oneways"]) {
      const auto pos = Json::getVec3<int>(p["position"]);

      const auto direction = p["direction"].getValue<std::string>();
      const auto power     = p["power"].getValue<int>();

      auto* cube = stage.getCube(pos);
      if (cube) {
        cube->type = Stage::Cube::ONEWAY;
        cube->direction = direction;
        cube->power     = power;
      }      
    }
  }

  return stage;
}


void serialize(const Stage& stage, const std::string& path) {
  ci::JsonTree stage_data = ci::JsonTree::makeObject("stage");
    
  auto body     = ci::JsonTree::makeArray("body");
  auto items    = ci::JsonTree::makeArray("items");
  auto moving   = ci::JsonTree::makeArray("moving");
  auto switches = ci::JsonTree::makeArray("switches");
  auto falling  = ci::JsonTree::makeArray("falling");
  auto oneways  = ci::JsonTree::makeArray("oneways");
    
  for (const auto& rows : stage.body) {
    body.pushBack(jsonArrayFromStageBody(rows));
        
    for (const auto& cube : rows) {
      switch (cube.type) {
      case Stage::Cube::ITEM:
        items.pushBack(jsonArrayFromVec3(cube.pos));
        break;

      case Stage::Cube::MOVING:
        moving.pushBack(makeMoving(cube.pos, cube.pattern));
        break;

      case Stage::Cube::SWITCH:
        switches.pushBack(makeSwitch(cube.pos, cube.target));
        break;

      case Stage::Cube::FALLING:
        falling.pushBack(makeFalling(cube.pos, cube.interval, cube.delay));
        break;

      case Stage::Cube::ONEWAY:
        oneways.pushBack(makeOneway(cube.pos, cube.direction, cube.power));
        break;
      }
    }
  }

  stage_data.addChild(body);

  if (items.hasChildren()) {
    stage_data.addChild(items);
  }
  if (moving.hasChildren()) {
    stage_data.addChild(moving);
  }
  if (switches.hasChildren()) {
    stage_data.addChild(switches);
  }
  if (falling.hasChildren()) {
    stage_data.addChild(falling);
  }
  if (oneways.hasChildren()) {
    stage_data.addChild(oneways);
  }

  stage_data.addChild(jsonArrayFromColor("color", stage.color)["color"]);
  stage_data.addChild(jsonArrayFromColor("bg_color", stage.bg_color)["bg_color"]);
    
  stage_data.addChild(ci::JsonTree("x_offset", stage.x_offset))
    .addChild(ci::JsonTree("pickable", stage.pickable));

  if (stage.build_speed > 0.0f) {
    stage_data.addChild(ci::JsonTree("build_speed", stage.build_speed));
  }
  if (stage.collapse_speed > 0.0f) {
    stage_data.addChild(ci::JsonTree("collapse_speed", stage.collapse_speed));
  }
  if (stage.auto_collapse > 0.0f) {
    stage_data.addChild(ci::JsonTree("auto_collapse", stage.auto_collapse));
  }
  stage_data.addChild(ci::JsonTree("camera", stage.camera));
  stage_data.addChild(ci::JsonTree("light_tween", stage.light_tween));
    
  stage_data.write(path);
}

}
}
