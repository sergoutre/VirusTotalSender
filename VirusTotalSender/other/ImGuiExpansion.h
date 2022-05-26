#ifndef IMGUI_EXPANSION_H
#define IMGUI_EXPANSION_H

#include <imgui.h>
#include "imgui_notify.h"
#include "../classes/Requests.h"

#include <string>

namespace ImGui
{
    inline void CenterText(std::string text)
    {
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text.c_str()).x) / 2);
        ImGui::Text(text.c_str());
    }

    inline void setToClipboard(std::string text, std::string copy, std::string notify)
    {
        ImGui::Text(text.c_str()); ImGui::SameLine(0.0f, 0.0f);
        ImGui::Text(copy.c_str());

        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsItemClicked())
            {
                ImGui::SetClipboardText(copy.c_str());
                ImGui::InsertNotification({ ImGuiToastType_Success, 3000, notify.c_str() });
            }
        }
    }
    
    inline void Link(std::string link, std::string text = "")
    {
        if (text.empty()) text = link;

        ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 colors[] = { 0xFFFF7700, 0xFFFF9900 };

        if (ImGui::InvisibleButton(("##" + link).c_str(), textSize))
            VTSender::Requests::openLink(link);

        ImU32 color = ImGui::IsItemHovered() ? colors[0] : colors[1];
        drawList->AddText(cursorPos, color, text.c_str());
        drawList->AddLine(ImVec2(cursorPos.x, cursorPos.y + textSize.y), ImVec2(cursorPos.x + textSize.x, cursorPos.y + textSize.y), color);
    }
}

#endif