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

    // Draw label (top-left)
    draw_list->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text), label);

    // Draw value (top-right)
    draw_list->AddText(ImVec2(pos.x + width - value_size.x, pos.y), ImGui::GetColorU32(ImGuiCol_Text), value_buf);

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

    // Increased track thickness
    float track_height = height * 0.45f; // Previously 0.15f
    float t = (*v - v_min) / (v_max - v_min);
    float grab_pos = pos.x + width * t;

    // Draw slider background (track)
    draw_list->AddRectFilled(
        ImVec2(pos.x, pos.y + height / 2 - track_height / 2),
        ImVec2(pos.x + width, pos.y + height / 2 + track_height / 2),
        ImGui::GetColorU32(ImGuiCol_FrameBg),
        track_height * 0.5f
    );

    // Draw filled portion of the slider
    draw_list->AddRectFilled(
        ImVec2(pos.x, pos.y + height / 2 - track_height / 2),
        ImVec2(grab_pos, pos.y + height / 2 + track_height / 2),
        ImGui::GetColorU32(ImGuiCol_SliderGrabActive),
        track_height * 0.5f
    );

    return changed;
}

#define SLIDER(label, v, v_min, v_max, ...) CustomSliderFloat(label, v, v_min, v_max, ##__VA_ARGS__)
//CUSTOM SLIDER END.

int DrawMacOSHeaderButtons()
{
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();
    const float btn = 46.0f;
    const float gap = 8.0f;
    const float top = winPos.y + 10.0f;
    const float xClose = winPos.x + winSize.x - btn - 12.0f;
    const float xMin = xClose - btn - gap;

    auto paint = [&](const char* id, ImVec2 pos, ImU32 fill, const char* glyph) -> bool {
        ImGui::SetCursorScreenPos(pos);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.08f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.16f));
        bool hit = ImGui::Button(id, ImVec2(btn, btn));
        ImGui::PopStyleColor(3);
        draw->AddRectFilled(pos, ImVec2(pos.x + btn, pos.y + btn), fill, 12.0f);
        ImVec2 ts = ImGui::CalcTextSize(glyph);
        draw->AddText(ImVec2(pos.x + (btn - ts.x) * 0.5f, pos.y + (btn - ts.y) * 0.5f), IM_COL32(20, 20, 20, 255), glyph);
        return hit;
    };

    int action = 0;
    if (paint("##min_menu", ImVec2(xMin, top), IM_COL32(255, 196, 64, 255), "-")) action = 1;
    if (paint("##close_menu", ImVec2(xClose, top), IM_COL32(255, 92, 85, 255), "X")) action = 2;
    return action;
}

std::map<std::string, ImVec4> colorStates = {
    {"ENEMY",   ImVec4(1.0f, 0.0f, 0.0f, 1.0f)},
    {"BOT",     ImVec4(0.0f, 1.0f, 0.0f, 1.0f)},
    {"HIDDEN",  ImVec4(1.0f, 1.0f, 1.0f, 1.0f)},
    {"KNOCKED", ImVec4(1.0f, 0.65f, 0.0f, 1.0f)},
    {"DEFAULT", ImVec4(1.0f, 1.0f, 1.0f, 1.0f)}
};


