#include "GUIControls.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

bool GUIControls::DragVec3(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
{
	bool bUpdateParameter = false;

	ImGui::PushID(label.c_str());
	ImGui::Columns(2);
	//Column 0
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text(label.c_str());
	ImGui::NextColumn();
	//Column 1
	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0,0 });

	float LineHeight = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 ButtonSize = { LineHeight + 3.0f, LineHeight };
	//Button X
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });

	if (ImGui::Button("X", ButtonSize))
	{
		values.x = resetValue;
		bUpdateParameter = true;
	}

	ImGui::PopStyleColor(3);
	//Value X
	ImGui::SameLine();
	bUpdateParameter |= ImGui::DragFloat("##X", &values.x, 0.1f, 0.f, 0.f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	//Button Y
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });

	if (ImGui::Button("Y", ButtonSize))
	{
		values.y = resetValue;
		bUpdateParameter = true;
	}

	ImGui::PopStyleColor(3);
	//Value Y
	ImGui::SameLine();
	bUpdateParameter |= ImGui::DragFloat("##Y", &values.y, 0.1f, 0.f, 0.f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	//Button Z
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });

	if (ImGui::Button("Z", ButtonSize))
	{
		values.z = resetValue;
		bUpdateParameter = true;
	}

	ImGui::PopStyleColor(3);
	//Value Z
	ImGui::SameLine();
	bUpdateParameter |= ImGui::DragFloat("##Z", &values.z, 0.1f, 0.f, 0.f, "%.2f");
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();
	ImGui::Columns(1);
	ImGui::PopID();

	return bUpdateParameter;
}
