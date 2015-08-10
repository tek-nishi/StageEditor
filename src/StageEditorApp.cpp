
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
#include "StageSerializer.hpp"
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
  

  float bg_duration;
  Color bg_color;

  
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
    settings_panel->setOptions("", "refresh=0.033");
    setupSettingsPanel();

    property_panel = params::InterfaceGl::create("property", Json::getVec2<int>(params_["app.property.size"]));
    property_panel->setPosition(Json::getVec2<int>(params_["app.property.position"]));

    gl::enableAlphaBlending();
    bg_color = Color::black();
    bg_duration = 0.0f;
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
        else if (stage.isFallingCube(selected_pos)) {
          setupFallingPropertyPanel();
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
      bg_color = Color(0.5, 0, 0);
      bg_duration = 0.5;
      break;

    case 'C':
      copyAllStagesToApp();
      bg_color = Color(0.5, 0.5, 0);
      bg_duration = 0.5;
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

    case 'K':
      stage.clear();
      on_cursor = false;
      selected  = false;

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

        case 'f':
          stage.toggleFalling(cursor_pos);
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
    if (bg_duration > 0.0f) {
      bg_duration -= 1 / 60.0;
      if (bg_duration <= 0.0f) {
        bg_color = Color::black();
      }
    }
  }
  
	void draw() override {
    gl::clear(bg_color);

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

  void loadStage(const int stage_num) {
    auto path = makeStagePath(stage_num);
    stage = StageSerializer::deserialize(path);
  }

  void writeStage(const int stage_num) {
    backupStage(stage_num);

    stage.validate();

    auto path = getDocumentPath(makeStagePath(stage_num));
    StageSerializer::serialize(stage, path);
  }

  void backupStage(const int stage_num) {
    auto origin_path = getDocumentPath(makeStagePath(stage_num));

    auto backup_path = getDocumentPath(std::string("backup/") + makeStagePath(stage_num))
      + createUniquePath();

    boost::filesystem::copy_file(origin_path, backup_path);
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


  static std::string getDocumentPath(const std::string& path) {
#if defined (CINDER_MAC)
    // OSXはプロジェクト位置基底(assertが実行ファイルと同梱されてしまうため)
    std::ostringstream full_path;
    full_path << PREPRO_TO_STR(SRCROOT) << "../assets/" << path;

    return full_path.str();
#else
    // Windowsはassetから
    return getAssetPath().str();
#endif
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

    settings_panel->addParam("x", &cursor_pos.x, false);
    settings_panel->addParam("z", &cursor_pos.y, false);

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
    settings_panel->addText("item: i moving: m switch: s falling: f");
    settings_panel->addText("change height: - ^ 0");
    settings_panel->addText("copy to app: C");
    settings_panel->addText("cleanup stage: K");
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
  
  void setupFallingPropertyPanel() {
    property_panel->clear();

    property_panel->addText("falling");

    auto& interval = stage.getInterval(selected_pos);
    auto& delay = stage.getDelay(selected_pos);
    
    property_panel->addParam("interval", &interval);
    property_panel->addParam("delay", &delay);
  }
  
};

}


CINDER_APP_NATIVE( ngs::StageEditorApp, RendererGl )
