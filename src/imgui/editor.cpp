#include"editor.h"
#include <imgui.h>

namespace CRATER::UI {
	Editor::Editor(Scene::Scene& scene) :m_scene(scene){

	 }
 
	void Editor::renderUI() {
		ImGui::Begin("Debug");

		ImGuiIO& io = ImGui::GetIO();

		ImGui::Text("%.1f FPS  (%.3f ms/frame)",
			io.Framerate, 1000.0f / io.Framerate);

		auto& registry = m_scene.getRegistry();
		ImGui::Text("Entities: %zu",
			static_cast<size_t>(registry.storage<entt::entity>().size()));

		static int clicks = 0;
		if (ImGui::Button("Click me")) clicks++;
		ImGui::SameLine();
		ImGui::Text("clicked %d times", clicks);

		ImGui::End();
	}
	 
}