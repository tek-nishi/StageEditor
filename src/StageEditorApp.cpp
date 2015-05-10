﻿
#include "Defines.hpp"
#include <chrono>
#include <iomanip>
#include "cinder/app/AppNative.h"
#include "cinder/System.h"
#include "cinder/Matrix22.h"
#include "cinder/gl/gl.h"
#include "cinder/Params/Params.h"
#include "JsonUtil.hpp"
#include "Stage.hpp"
#include "StageDrawer.hpp"


using namespace ci;
using namespace ci::app;


namespace ngs {

class StageEditorApp : public AppNative {
  ci::JsonTree params_;

  std::vector<std::string> stage_path;
  std::string copy_path;

  
  int current_stage;
  Stage stage;

  Vec2f view_offset;
  float view_rotate;
  Vec2f view_scale;

  Vec2f prev_drag_pos;

  bool on_cursor;
  Vec2i cursor_pos;
  
  bool selected;
  Vec2i selected_pos;

  params::InterfaceGlRef settings_panel;
  params::InterfaceGlRef property_panel;
  

  
  void prepareSettings(Settings* settings) override {
    // アプリ起動時の設定はここで処理する
    params_ = Json::readFromFile("params.json");

    auto size = Json::getVec2<int>(params_["app.size"]);
    settings->setWindowSize(size);

    for (const auto& path : params_["app.stage"]) {
      stage_path.push_back(path.getValue<std::string>());      
    }
    copy_path = params_["app.copy_path"].getValue<std::string>();
    
    auto active_touch = ci::System::hasMultiTouch();
    if (active_touch) {
      settings->enableMultiTouch();
    }
  }

	void setup() override {
    view_offset = Vec2f(200, 500);
    view_rotate = 180.0f;
    view_scale  = Vec2f(20, 20);

    selected = false;

    current_stage = 0;
    loadStage(current_stage);

    settings_panel = params::InterfaceGl::create("settings", Json::getVec2<int>(params_["app.settings.size"]));
    settings_panel->setPosition(Json::getVec2<int>(params_["app.settings.position"]));
    setupSettingsPanel();

    property_panel = params::InterfaceGl::create("property", Json::getVec2<int>(params_["app.property.size"]));
    property_panel->setPosition(Json::getVec2<int>(params_["app.property.position"]));

    gl::enableAlphaBlending();
  }


  void mouseMove(MouseEvent event) override {
    Matrix22f matrix;

    matrix.rotate(toRadians(view_rotate));
    matrix.scale(view_scale);
    auto pos = matrix.inverted() * Vec2f(event.getPos() - view_offset);

    on_cursor = false;
    if ((pos.x >= 0.0f) && (pos.x < stage.size.x)) {
      if ((pos.y >= 0.0f) && (pos.y < stage.size.y)) {
        on_cursor = true;
        cursor_pos.x = pos.x;
        cursor_pos.y = pos.y;
      }
    }
  }

  void mouseDown(MouseEvent event) override {
    if (event.isLeft()) {
      prev_drag_pos = event.getPos();

      if (on_cursor) {
        selected = true;
        selected_pos = cursor_pos;

        if (stage.isItemCube(selected_pos)) {
          setupItemPropertyPanel();
        }
        else if (stage.isMovingCube(selected_pos)) {
          setupMovingPropertyPanel();
        }
        else if (stage.isSwitchCube(selected_pos)) {
          setupSwitchPropertyPanel();
        }
        else {
          clearPropertyPanel();
        }
      }
    }
  }

  void mouseDrag(MouseEvent event) override {
    if (!on_cursor && event.isLeftDown()) {
      auto pos = event.getPos();

      view_offset += pos - prev_drag_pos;
      prev_drag_pos = pos;
    }
  }

  void keyDown(KeyEvent event) override {
    auto chara  = event.getChar();

    switch (chara) {
    case 'W':
      writeStage(current_stage);
      break;

    case 'C':
      copyAllStagesToApp();
      break;

    case ',':
      current_stage -= 1;
      if (current_stage < 0) current_stage = stage_path.size() - 1;
      on_cursor = false;
      selected  = false;

      loadStage(current_stage);
      setupSettingsPanel();
      clearPropertyPanel();
      break;

    case '.':
      current_stage = (current_stage + 1) % stage_path.size();
      on_cursor = false;
      selected  = false;

      loadStage(current_stage);
      setupSettingsPanel();
      clearPropertyPanel();
      break;

      
    default:
      if (on_cursor) {
        switch (chara) {
        case 'i':
          stage.toggleItem(cursor_pos);
          break;

        case 'm':
          stage.toggleMoving(cursor_pos);
          break;

        case 's':
          stage.toggleSwitch(cursor_pos);
          break;

        case '-':
          stage.changeHeight(cursor_pos, -1);
          break;
        
        case '^':
          stage.changeHeight(cursor_pos, 1);
          break;

        case '0':
          stage.setHeight(cursor_pos, 0);
          break;
        }
      }
      if (selected && stage.isSwitchCube(selected_pos)) {
        switch (chara) {
        case '[':
          stage.reduceSwitchTarget(selected_pos);
          setupSwitchPropertyPanel();
          break;

        case ']':
          stage.addSwitchTarget(selected_pos);
          setupSwitchPropertyPanel();
          break;
        }
      }
    }
  }
  
  
	void update() override {
  }
  
	void draw() override {
    gl::clear( Color( 0, 0, 0 ) );

    gl::pushModelView();

    gl::translate(view_offset);
    gl::rotate(view_rotate);
    gl::scale(view_scale);
    
    StageDrawer::draw(stage);

    if (selected && stage.isSwitchCube(selected_pos)) {
      StageDrawer::drawSwitchTarget(stage.getTarget(selected_pos));
    }
    
    if (on_cursor) {
      gl::color(0, 0, 1);
      gl::lineWidth(2);
      Rectf rect(cursor_pos.x, cursor_pos.y, cursor_pos.x + 0.9, cursor_pos.y + 0.9);
      gl::drawStrokedRect(rect);
    }
    
    if (selected) {
      gl::color(1, 0, 0);
      gl::lineWidth(2);
      Rectf rect(selected_pos.x, selected_pos.y, selected_pos.x + 0.9, selected_pos.y + 0.9);
      gl::drawStrokedRect(rect);
    }
    
    gl::popModelView();

    settings_panel->draw();
    property_panel->draw();
  }


  std::string makeStagePath(const int stage_num) {
    return stage_path[stage_num];
  }

  
  void writeStage(const int stage_num) {
    backupStage(stage_num);
    
    ci::JsonTree stage_data = ci::JsonTree::makeObject("stage");
    
    auto body     = ci::JsonTree::makeArray("body");
    auto items    = ci::JsonTree::makeArray("items");
    auto moving   = ci::JsonTree::makeArray("moving");
    auto switches = ci::JsonTree::makeArray("switches");
    
    for (const auto& rows : stage.body) {
      body.pushBack(jsonArrayFromStageBody(rows));
        
      for (const auto& cube : rows) {
        if (cube.item) {
          items.pushBack(jsonArrayFromVec3(cube.pos));
        }
        
        if (cube.moving) {
          moving.pushBack(makeMoving(cube.pos, cube.pattern));
        }
        
        if (cube.sw) {
          switches.pushBack(makeSwitch(cube.pos, cube.target));
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
    stage_data.addChild(ci::JsonTree("light_tween", stage.light_tween));
    
    auto full_path = getDocumentPath(makeStagePath(stage_num));
    stage_data.write(full_path, ci::JsonTree::WriteOptions().createDocument(true));
  }

  void backupStage(const int stage_num) {
    auto origin_path = getDocumentPath(makeStagePath(stage_num));

    auto backup_path = getDocumentPath(std::string("backup/") + makeStagePath(stage_num))
      + createUniquePath();

    boost::filesystem::copy_file(origin_path, backup_path);
  }

  void loadStage(const int stage_num) {
    auto stage_params = Json::readFromFile(makeStagePath(stage_num));
    stage = Stage(stage_params["stage"]);
  }

  void copyAllStagesToApp() {
    for (const auto& path : stage_path) {
      auto path_from = getDocumentPath(path);
      auto path_to = getDocumentPath(copy_path + path);

      // TIPS:上書き許可
      boost::filesystem::copy_file(path_from, path_to,
                                   boost::filesystem::copy_option::overwrite_if_exists);
    }
  }

  
  static ci::JsonTree makeMoving(const ci::Vec3i& pos, const std::string& pattern) {
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
  
  static ci::JsonTree makeSwitch(const ci::Vec3i& pos, const std::vector<std::string>& target) {
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
  

  static ci::JsonTree jsonArrayFromStageBody(const std::vector<Stage::Cube>& cubes) {
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
  static ci::JsonTree jsonArrayFromVec3(const T& vector) {
    std::ostringstream text;
    text << "[ "
         << vector.x << ", "
         << vector.y << ", "
         << vector.z << " ]";

    return ci::JsonTree(text.str());
  }
  
  template <typename T>
  static ci::JsonTree jsonArrayFromColor(const std::string& key, const T& color) {
    std::ostringstream text;
    text << "{ \"" << key << "\":";
    text << "[ "
         << color.r << ", "
         << color.g << ", "
         << color.b << " ] }";

    return ci::JsonTree(text.str());
  }


  static std::string getDocumentPath(const std::string& path) {
    std::ostringstream full_path;
    full_path << PREPRO_TO_STR(SRCROOT) << "../assets/" << path;
    return full_path.str();
  }


  static std::string createUniquePath() {
    static int unique_num = 0;
    
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    const tm* lt = std::localtime(&t);
    
    std::ostringstream path;
    path << "." << std::put_time(lt, "%F-%T.")
         << unique_num;

    unique_num += 1;

    return path.str();
  }

  
  void setupSettingsPanel() {
    settings_panel->clear();

    {
      std::ostringstream text;
      text << "stage: " << stage_path[current_stage];
      settings_panel->addText(text.str());
    }

    settings_panel->addSeparator();

    settings_panel->addParam("color", &stage.color);
    settings_panel->addParam("bg_color", &stage.bg_color);

    settings_panel->addSeparator();
    
    settings_panel->addParam("x_offset", &stage.x_offset);
    settings_panel->addParam("pickable", &stage.pickable)
      .min(0);

    settings_panel->addSeparator();

    settings_panel->addParam("build_speed", &stage.build_speed)
      .min(0)
      .step(0.05);
    
    settings_panel->addParam("collapse_speed", &stage.collapse_speed)
      .min(0)
      .step(0.05);
    
    settings_panel->addParam("auto_collapse", &stage.auto_collapse)
      .min(0)
      .step(0.05);

    settings_panel->addSeparator();

    settings_panel->addParam("light_tween", &stage.light_tween);

    settings_panel->addSeparator();

    settings_panel->addParam("width", &stage.size.x)
      .min(1)
      .updateFn([this]() {
          on_cursor = false;
          selected  = false;
          clearPropertyPanel();
          stage.resize();
        });

    
    settings_panel->addParam("length", &stage.size.y)
      .min(1)
      .updateFn([this]() {
          on_cursor = false;
          selected  = false;
          clearPropertyPanel();
          stage.resize();
        });

    settings_panel->addSeparator();

    settings_panel->addText("save: W  stage change: , .");
    settings_panel->addText("item: i moving: m switch: s");
    settings_panel->addText("change height: - ^ 0");
    settings_panel->addText("copy to app: C");
  }


  void clearPropertyPanel() {
    property_panel->clear();
  }
  
  void setupItemPropertyPanel() {
    property_panel->clear();
  }
  
  void setupMovingPropertyPanel() {
    property_panel->clear();

    property_panel->addText("moving");
    
    property_panel->addSeparator();
    
    property_panel->addParam("pattern", &stage.getPattern(selected_pos));
  }

  void setupSwitchPropertyPanel() {
    property_panel->clear();

    property_panel->addText("switch");
    
    property_panel->addSeparator();

    auto& target = stage.getTarget(selected_pos);
    size_t index = 1;
    for (auto& t : target) {
      std::ostringstream text;
      text << "target:" << index;
      property_panel->addParam(text.str(), &t);

      index += 1;
    }
    
    property_panel->addSeparator();

    property_panel->addText("target number change: [ ]");
  }
  
};

}


CINDER_APP_NATIVE( ngs::StageEditorApp, RendererGl )
