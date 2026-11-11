#include "psui/scrub_control.hpp"
#include "imgui_internal.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {
int checks = 0;
void require(bool ok, const char *message) {
  ++checks;
  if (!ok) throw std::runtime_error(message);
}
void near(double actual, double expected, const char *message) {
  if (std::abs(actual - expected) > .00001)
    std::cerr << message << ": " << actual << " != " << expected << '\n';
  require(std::abs(actual - expected) <= .00001, message);
}
struct Fixture {
  double value = 100, speed = .5, minimum = 0, maximum = 1000;
  float scale = 1, x = 100, y = 60;
  bool integer = false, disabled = false;
  ImGuiID id = 0;
  psui::ScrubResult last;
  Fixture(float dpi = 1) : scale(dpi) {
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1000, 500};
    io.DeltaTime = 1 / 60.f;
    io.ConfigInputTrickleEventQueue = false;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    unsigned char *pixels; int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    frame(); frame();
  }
  ~Fixture() { ImGui::DestroyContext(); }
  void frame(const std::function<void(ImGuiIO &)> &events = {}) {
    auto &io = ImGui::GetIO();
    if (events) events(io);
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({900, 400});
    ImGui::Begin("test", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::SetCursorScreenPos({20, 50});
    ImGui::SetNextItemWidth(300 * scale);
    id = ImGui::GetID("##value");
    ImGui::BeginDisabled(disabled);
    last = psui::scrub_number("##value", value, minimum, maximum, speed, scale, integer);
    ImGui::EndDisabled();
    ImGui::End();
    ImGui::Render();
  }
  void down(bool shift = false, bool ctrl = false) {
    frame([&](ImGuiIO &io) {
      io.AddKeyEvent(ImGuiMod_Shift, shift);
      io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
      io.AddMousePosEvent(x, y); io.AddMouseButtonEvent(0, true);
    });
  }
  void move(float dx, float dy = 0) {
    x += dx; y += dy;
    frame([&](ImGuiIO &io) {io.AddMousePosEvent(x, y);});
  }
  void release() {frame([](ImGuiIO &io) {io.AddMouseButtonEvent(0, false);});}
  void click() {down(); release();}
  void type(const char *text) {frame([&](ImGuiIO &io) {io.AddInputCharactersUTF8(text);});}
};
double luminance(float v) {return v <= .04045f ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);}
double luminance(ImVec4 c) {return .2126*luminance(c.x)+.7152*luminance(c.y)+.0722*luminance(c.z);}
}
int main() {
  try {
    for (float dpi : {1.f, 1.5f, 2.f}) {
      for (int mode = 0; mode < 4; ++mode) {
        Fixture f(dpi);
        f.down(mode & 1, mode & 2);
        require(!ImGui::TempInputIsActive(f.id), "Ctrl press starts a scrub, not text input");
        f.move(20*dpi);
        near(f.value, mode==1?200:mode==2?101:110, "modifier and DPI speed");
        f.release();
        require(!ImGui::TempInputIsActive(f.id), "drag release never enters editor");
      }
    }
    {
      Fixture f;
      f.down(); f.move(20); near(f.value,110,"normal drag");
      f.frame([](ImGuiIO &io){io.AddKeyEvent(ImGuiMod_Shift,true);});
      near(f.value,110,"changing modifier alone causes no jump");
      f.move(10); near(f.value,160,"new speed affects only subsequent distance");
      f.frame([](ImGuiIO &io){io.AddKeyEvent(ImGuiKey_Escape,true);});
      near(f.value,100,"Escape restores scrub activation value");
      require(f.last.consumed_escape,"scrub Escape consumed by widget");
    }
    {
      Fixture f; f.integer=true;f.speed=1;f.down(false,true);
      for(int n=0;n<20;++n)f.move(1);
      near(f.value,102,"fine integer scrubbing accumulates substeps");
    }
    {
      Fixture f;f.down();f.move(0,20);near(f.value,100,"vertical movement does not scrub");
      f.move(-20);near(f.value,90,"drag left reduces value");
    }
    {
      Fixture f;f.maximum=105;f.down();f.move(100);near(f.value,105,"upper clamp");
      f.move(-10);near(f.value,100,"reverse moves immediately away from upper bound");
      f.move(-1000);near(f.value,0,"lower clamp");
    }
    {
      Fixture f;f.disabled=true;f.down(true);f.move(50);near(f.value,100,"disabled field cannot change");
      require(!ImGui::TempInputIsActive(f.id),"disabled field never edits");
    }
    {
      Fixture f;f.click();require(ImGui::TempInputIsActive(f.id),"ordinary click enters text input");
      f.type("123.456789");near(f.value,123.456789,"manual entry retains precision");
      f.frame([](ImGuiIO &io){io.AddKeyEvent(ImGuiMod_Ctrl,true);io.AddKeyEvent(ImGuiKey_A,true);});
      f.frame([](ImGuiIO &io){io.AddKeyEvent(ImGuiMod_Ctrl,false);io.AddKeyEvent(ImGuiKey_A,false);});
      f.type("245.25");near(f.value,245.25,"Ctrl+A works normally inside text editor");
      f.frame([](ImGuiIO &io){io.AddKeyEvent(ImGuiKey_Enter,true);});
      require(!ImGui::TempInputIsActive(f.id),"Enter commits editor");
    }
    {
      Fixture f;f.click();f.type("271.125");
      f.frame([](ImGuiIO &io){io.AddKeyEvent(ImGuiKey_Escape,true);});
      near(f.value,100,"Escape restores typed activation value");
      require(f.last.consumed_escape,"text Escape consumed by widget");
    }
    {
      Fixture f;f.click();f.type("1e999");
      require(std::isfinite(f.value)&&f.value>=f.minimum&&f.value<=f.maximum,"nonfinite or excessive input cannot escape bounds");
    }
    const int surfaces[4][4]={{240,255,228,208},{184,210,195,165},{83,65,100,68},{50,38,70,42}};
    for(const auto &surface : surfaces) {
      const int bg=surface[0];
      const ImVec4 background(bg/255.f,bg/255.f,bg/255.f,1);
      const auto blue=psui::numeric_color(background);
      require(blue.z>blue.y&&blue.y>blue.x,"numeric colour is blue in every theme");
      for(int grey : surface) {
        const double a=luminance(ImVec4(grey/255.f,grey/255.f,grey/255.f,1)),b=luminance(blue);
        require((std::max(a,b)+.05)/(std::min(a,b)+.05)>=4.5,"blue text contrast meets 4.5:1 in all four themes, including hover and active fields");
      }
    }
    std::cout<<"PASS: "<<checks<<" numeric scrub checks; normal/Shift/Ctrl; DPI; accumulator; bounds; disabled; click/type; Ctrl+A; Enter/Escape; theme contrast.\n";
    return 0;
  } catch(const std::exception &e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
