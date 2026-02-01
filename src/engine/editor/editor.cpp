#include "engine/editor/editor.hpp"

#include <imgui.h>

namespace editor {

void Editor::buildPanels() {
  ImGui::Begin("Quark");
  ImGui::Text("Hello World");
  static float f = 0.0F;
  ImGui::SliderFloat("float", &f, 0.0F, 1.0F);
  ImGui::End();
}

} // namespace editor
