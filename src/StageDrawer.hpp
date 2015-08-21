#pragma once

//
// Stage描画
//

#include "cinder/gl/gl.h"
#include "Stage.hpp"


namespace ngs {
namespace StageDrawer {


void draw(const Stage& stage) {
  for (const auto& rows : stage.body) {
    for (const auto& cube : rows) {
      if (cube.pos.y < 0) continue;

      switch (cube.type) {
      case Stage::Cube::ITEM:
        ci::gl::color(1, 1, 0);
        break;

      case Stage::Cube::MOVING:
        ci::gl::color(0, 1, 0);
        break;

      case Stage::Cube::SWITCH:
        ci::gl::color(1, 0, 1);
        break;

      case Stage::Cube::FALLING:
        ci::gl::color(1, 0.5, 0);
        break;

      case Stage::Cube::ONEWAY:
        ci::gl::color(0, 0.5, 1);
        break;

      default:
        ci::gl::color(stage.color);
        break;
      }
      
      float size = 0.9;      
      ci::Rectf rect(cube.pos.x, cube.pos.z, cube.pos.x + size, cube.pos.z + size);
      ci::gl::drawSolidRect(rect);

      ci::gl::color(1, 0, 0);
      for (int i = 0; i < cube.pos.y; ++i) {
        float size = 0.1;
        float ofs_x = 0.1 + 0.2 * (i % 4);
        float ofs_y = 0.1 + 0.2 * (i / 4);
        ci::Rectf rect(cube.pos.x + ofs_x, cube.pos.z + ofs_y,
                       cube.pos.x + ofs_x + size, cube.pos.z + ofs_y + size);
        
        ci::gl::drawSolidRect(rect);
      }
    }
  }
}

void drawSwitchTarget(const std::vector<std::string>& targets) {
  ci::gl::color(0, 1, 1, 0.5);

  for (const auto& target : targets) {
    std::ostringstream text;
    text << "[" << target << "]";

    auto pos = Json::getVec3<int>(ci::JsonTree(text.str()));
    
    float size = 0.7;
    float ofs = 0.1;
    ci::Rectf rect(pos.x + ofs, pos.z + ofs,
                   pos.x + ofs + size, pos.z + ofs + size);
    ci::gl::drawSolidRect(rect);
  }
}

}
}
