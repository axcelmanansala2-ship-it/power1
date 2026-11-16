#pragma once

//CUSTOM SLIDER
float ImLerp(float a, float b, float t) { return a + (b - a) * t; }

float ImClamp(float v, float min, float max) {
    return (v < min) ? min : (v > max) ? max : v;
}

// Custom slider function
bool CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.0f") {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    float height = ImGui::GetFrameHeight();

    // Compute label and value sizes
    ImVec2 label_size = ImGui::CalcTextSize(label);
    char value_buf[32];
    snprintf(value_buf, sizeof(value_buf), format, *v);
    ImVec2 value_size = ImGui::CalcTextSize(value_buf);

    // Get draw list
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddText(pos, IM_COL32(168, 176, 186, 255), label);
    draw_list->AddText(ImVec2(pos.x + width - value_size.x, pos.y), IM_COL32(90, 210, 235, 255), value_buf);

    // Move cursor below label/value
    float text_height = (label_size.y > value_size.y) ? label_size.y : value_size.y;
    pos.y += text_height + style.ItemInnerSpacing.y;
    ImGui::SetCursorScreenPos(pos);

    // Slider logic
    ImGui::InvisibleButton(label, ImVec2(width, height));
    bool active = ImGui::IsItemActive();
    bool hovered = ImGui::IsItemHovered();
    bool changed = false;

    if (active && ImGui::IsMouseDragging(0)) {
        float t = (ImGui::GetMousePos().x - pos.x) / width;
        t = ImClamp(t, 0.0f, 1.0f);
        *v = v_min + (v_max - v_min) * t;
        changed = true;
    }

    float track_height = 6.0f;
    float t = (v_max > v_min) ? (*v - v_min) / (v_max - v_min) : 0.f;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    float grab_pos = pos.x + width * t;
    float mid = pos.y + height * 0.5f;

    draw_list->AddRectFilled(
        ImVec2(pos.x, mid - track_height * 0.5f),
        ImVec2(pos.x + width, mid + track_height * 0.5f),
        IM_COL32(255, 255, 255, 18),
        track_height
    );
    draw_list->AddRectFilled(
        ImVec2(pos.x, mid - track_height * 0.5f),
        ImVec2(grab_pos, mid + track_height * 0.5f),
        active ? IM_COL32(120, 220, 245, 255) : IM_COL32(70, 196, 230, 255),
        track_height
    );
    draw_list->AddCircleFilled(ImVec2(grab_pos, mid), active ? 9.0f : 7.0f, IM_COL32(245, 248, 250, 255), 16);
    ImGui::Dummy(ImVec2(0, 6));

    return changed;
}

#define SLIDER(label, v, v_min, v_max, ...) CustomSliderFloat(label, v, v_min, v_max, ##__VA_ARGS__)
//CUSTOM SLIDER END.

int DrawMacOSHeaderButtons()
{
    const float btn = 40.0f;
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.94f, 0.96f, 0.98f, 1.f), "Accel");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.45f, 0.50f, 0.56f, 1.f), "menu");
    float right = ImGui::GetWindowContentRegionMax().x - btn;
    if (right < 8.0f) right = 8.0f;
    ImGui::SameLine(right);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 1.f, 1.f, 0.06f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.28f, 0.32f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.28f, 0.32f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.96f, 0.98f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, btn * 0.5f);

    int action = ImGui::Button("x##close", ImVec2(btn, btn)) ? 1 : 0;

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    ImGui::Dummy(ImVec2(0, 4));
    ImVec2 line = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddLine(line, ImVec2(line.x + width, line.y), IM_COL32(255, 255, 255, 22), 1.f);
    ImGui::Dummy(ImVec2(0, 6));
    return action;
}

std::map<std::string, ImVec4> colorStates = {
    {"ENEMY",   ImVec4(1.0f, 0.0f, 0.0f, 1.0f)},
    {"BOT",     ImVec4(0.0f, 1.0f, 0.0f, 1.0f)},
    {"HIDDEN",  ImVec4(1.0f, 1.0f, 1.0f, 1.0f)},
    {"KNOCKED", ImVec4(1.0f, 0.65f, 0.0f, 1.0f)},
    {"DEFAULT", ImVec4(1.0f, 1.0f, 1.0f, 1.0f)}
};


