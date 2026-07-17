#include"editor.h"
#include <imgui.h>

namespace CRATER::UI {
	Editor::Editor(Scene::Scene& scene) :m_scene(scene){

	 }

	static bool inputText(const char* label, std::string& str) {
		char buf[256];
		std::snprintf(buf, sizeof(buf), "%s", str.c_str());
		if (ImGui::InputText(label, buf, sizeof(buf))) {
			str = buf;
			return true;
		}
		return false;
	}

	// ── Shared component field-drawing ─────────────────────────────────────────
	// These take a plain component reference, so the inspector (live ECS component)
	// and the create dialog (draft component) draw the exact same fields. One code
	// path, two data sources — no drift between "edit" and "create".

	static void drawTransformFields(Scene::TransformComponent& t) {
		// Read via Get / write via Set so the component's transformDirty flag stays honest.
		glm::vec3 pos = t.GetPosition();
		glm::vec3 rot = t.GetRotation();   // radians
		glm::vec3 scl = t.GetScale();
		if (ImGui::DragFloat3("Position", &pos.x, 0.1f))  t.SetPosition(pos);
		if (ImGui::DragFloat3("Rotation", &rot.x, 0.01f)) t.SetRotation(rot);
		if (ImGui::DragFloat3("Scale",    &scl.x, 0.05f)) t.SetScale(scl);
	}

	static void drawMeshFields(Scene::MeshComponent& m) {
		 
		inputText("Model Path", m.modelPath);
	}

	static void drawMaterialFields(Scene::MaterialComponent& mat) {
		inputText("Material ID", mat.materialID);
		inputText("Shader Path", mat.shaderID);
		const char* types[] = { "OpaqueMesh", "Skybox" };
		int cur = static_cast<int>(mat.type);
		if (ImGui::Combo("Pipeline", &cur, types, IM_ARRAYSIZE(types)))
			mat.type = static_cast<PipelineType>(cur);
	}

	void Editor::renderUI() {
		renderHierarchy();
		renderInspector();
		renderTopBar();
		renderConsole();
		 
	}

	void Editor::renderConsole() {
		ImGui::Begin("Console");
		if (ImGui::Button("Clear")) Log::clear();
		ImGui::Separator();

		ImGui::BeginChild("scroll", ImVec2(0, 0), 0, ImGuiWindowFlags_HorizontalScrollbar);
		for (const auto& line : Log::snapshot()) {       // copy-under-lock accessor
			ImVec4 col = line.level == Log::Level::Error ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
				: line.level == Log::Level::Warn ? ImVec4(1.0f, 0.8f, 0.4f, 1.0f)
				: ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
			ImGui::TextColored(col, "%s", line.text.c_str());
		}
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);                  // auto-scroll only when already at bottom
		ImGui::EndChild();
		ImGui::End();

	}

	void Editor::renderTopBar() {
		ImGui::Begin("Debug");

		ImGuiIO& io = ImGui::GetIO();

		ImGui::Text("%.1f FPS  (%.3f ms/frame)",
			io.Framerate, 1000.0f / io.Framerate);

		ImGui::End();
	}

	void Editor::renderHierarchy() {
		auto& registry = m_scene.getRegistry();

		ImGui::Begin("Hierarchy");

		if (ImGui::Button("New Entity"))
			ImGui::OpenPopup("Create Entity");
		drawCreateEntityPopup();

		ImGui::Separator();

		for (auto entity : registry.storage<entt::entity>()) {   // every entity in the scene
			ImGui::PushID(static_cast<int>(entity));             // <-- do NOT skip this, see below

			const char* name = "Entity";
			if (auto* tag = registry.try_get<Scene::TagComponent>(entity))
				name = tag->tag.c_str();

			if (ImGui::Selectable(name, m_selected == entity))
				m_selected = entity;                             // clicking a row selects it

			ImGui::PopID();
		}

		ImGui::End();
	}

	void Editor::renderInspector() {
		auto& registry = m_scene.getRegistry();

		ImGui::Begin("Inspector");

		if (!registry.valid(m_selected)) {
			ImGui::TextDisabled("No entity selected");
			ImGui::End();
			return;
		}

		if (auto* tag = registry.try_get<Scene::TagComponent>(m_selected)) {
			inputText("Name", tag->tag);
		}

		if (auto* t = registry.try_get<Scene::TransformComponent>(m_selected)) {
			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
				drawTransformFields(*t);
		}

		if (auto* m = registry.try_get<Scene::MeshComponent>(m_selected)) {
			if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
				drawMeshFields(*m);
		}

		if (auto* mat = registry.try_get<Scene::MaterialComponent>(m_selected)) {
			if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
				drawMaterialFields(*mat);
		}

		ImGui::Separator();

		// Add Component — a popup that only offers what the entity doesn't already have.
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("add_component");
		if (ImGui::BeginPopup("add_component")) {
			if (!registry.all_of<Scene::TagComponent>(m_selected) && ImGui::MenuItem("Tag"))
				registry.emplace<Scene::TagComponent>(m_selected).tag = "Entity";
			if (!registry.all_of<Scene::TransformComponent>(m_selected) && ImGui::MenuItem("Transform"))
				registry.emplace<Scene::TransformComponent>(m_selected);
			if (!registry.all_of<Scene::MeshComponent>(m_selected) && ImGui::MenuItem("Mesh"))
				registry.emplace<Scene::MeshComponent>(m_selected);
			if (!registry.all_of<Scene::MaterialComponent>(m_selected) && ImGui::MenuItem("Material"))
				registry.emplace<Scene::MaterialComponent>(m_selected);
			ImGui::EndPopup();
		}

		if (ImGui::Button("Delete Entity")) {
			m_scene.DestroyEntity(m_selected);   // lifecycle → Scene (see note 1)
			m_selected = entt::null;           // clear selection so the guard above catches it next frame
		}

		ImGui::End();
	}

	void Editor::drawCreateEntityPopup() {
		// BeginPopupModal must be called every frame; it only renders while the popup
		// is open (triggered by OpenPopup("Create Entity") from the New Entity button).
		if (!ImGui::BeginPopupModal("Create Entity", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		inputText("Name", m_draft.name);

		ImGui::SeparatorText("Transform");
		drawTransformFields(m_draft.transform);

		ImGui::SeparatorText("Mesh");
		drawMeshFields(m_draft.mesh);

		ImGui::SeparatorText("Material");
		drawMaterialFields(m_draft.material);

		ImGui::Separator();

		// Only allow commit once the entity is a complete renderable. Until then the
		// draft lives here in the UI, never in the scene.
		const bool valid =
			 !m_draft.mesh.modelPath.empty() &&
			!m_draft.material.materialID.empty() && !m_draft.material.shaderID.empty();

		ImGui::BeginDisabled(!valid);
		if (ImGui::Button("Create")) {
			// Entity enters the scene fully-formed — sync()'s poll will pick it up and load it.
			auto e = m_scene.CreateEntity();
			e.AddComponent<Scene::TagComponent>().tag = m_draft.name;
			e.AddComponent<Scene::TransformComponent>(m_draft.transform);
			e.AddComponent<Scene::MeshComponent>(m_draft.mesh);
			e.AddComponent<Scene::MaterialComponent>(m_draft.material);
			m_selected = e.GetHandle();
			m_draft    = Draft{};                // reset for the next creation
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			m_draft = Draft{};
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

}