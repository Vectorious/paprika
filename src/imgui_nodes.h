#pragma once


#include "paprika.h"


internal ImU32
GetConnectorColor(Node_Value_Type type)
{
    ImU32 result = 0;

    switch (type)
    {
        case NodeValueType_Number: result = IM_COL32(200, 128, 128, 255); break;
        case NodeValueType_Player: result = IM_COL32(128, 128, 200, 255); break;

        DefaultPanic;
    }

    return result;
}

internal ImU32
HsvToRgb(Vector3 hsv)
{
    f32 h = hsv.x;
    f32 s = hsv.y;
    f32 v = hsv.z;

    i32 h_i = (i32)(h * 6.0f);
    f32 f = h*6.0f - (f32)h_i;
    f32 p = v * (1.0f - s);
    f32 q = v * (1.0f - f*s);
    f32 t = v * (1.0f - (1.0f - f) * s);

    Vector3 rgb;
    if      (h_i == 0)  rgb = V3(v, t, p);
    else if (h_i == 1)  rgb = V3(q, v, p);
    else if (h_i == 2)  rgb = V3(p, v, t);
    else if (h_i == 3)  rgb = V3(p, q, v);
    else if (h_i == 4)  rgb = V3(t, p, v);
    else                rgb = V3(v, p, q);
    
    ImU32 result = IM_COL32(rgb.r * 255, rgb.g * 255, rgb.b * 255, 255);
    return result;
}

// TODO: Probably rework this. Possibly hardcode the colors?
internal ImU32
GetNodeColor(Node_Type type)
{
    Assert(type);

    const f32 ratio = 1.0f / (f32)NodeTypeCount;
    Vector3 hsv = V3(ratio * (type - 1), 0.5f, 0.6f);
    ImU32 result = HsvToRgb(hsv);
    return result;
}

internal ImVec2
GetConnectorCoords(Node_System *nodes, Node_Connector_Reference conref, ImVec2 node_size, ImVec2 region_min)
{
    Assert(conref.node);

    Node *node = GetNode(nodes, conref.node);

    ImVec2 min = region_min + (ImVec2)node->position;

    ImVec2 result;
    if (conref.connector == 0)
    {
        result = ImVec2(min.x + (0.5f * node_size.x), min.y + node_size.y);
    }
    else
    {
        Node_Info node_info = GetNodeInfo(node->type);
        f32 alpha = (f32)conref.connector / (node_info.input_count + 1);
        result = Lerp(min, ImVec2(min.x + node_size.x, min.y), alpha);
    }

    return result;
}

internal void
ImGuiNode(Node_Editor_Window *editor, Node_System *nodes, Node_ID id, ImVec4 **draw_lines, ImVec2 region_min, Node_State state)
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    bool32 mouse_released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImU32 connector_center_color = IM_COL32(44, 44, 44, 255);

    ImVec2 node_size = ImVec2(140.0f, 64.0f);

    Node *node = GetNode(nodes, id);

    if (node->type)
    {
        Node_Info node_info = GetNodeInfo(node->type);
        ImGui::SetCursorPos(node->position);

        ImGuiID node_imgui_id = ImGui::GetID(node);
        ImGui::PushID(node_imgui_id);

        if (ImGui::InvisibleButton("", node_size) ||
            ImGui::IsItemActive())
        {
            editor->dragging_node = id;
            Vector2 new_pos = node->position + (Vector2)ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f);
            node->position = V2(Max(0.0f, new_pos.x), Max(0.0f, new_pos.y));
        }

        ImGui::SetItemAllowOverlap();

        if (id > 1 && ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Remove node"))
            {
                RemoveNode(nodes, id);
                ImGui::EndPopup();
                ImGui::PopID();
                return;
            }

            ImGui::EndPopup();
        }

        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        ImVec2 center = min + (ImVec2)(0.5f * node_size);

        draw_list->AddRectFilled(min, max, GetNodeColor(node->type));
        draw_list->AddRect(min, max, IM_COL32_BLACK);

        for (Node_Connector_Reference conref = CreateNodeConnectorReference(id, 0);
             conref.connector <= node_info.input_count;
             ++conref.connector)
        {  
            ImU32 con_color;
            if (conref.connector)
                con_color = GetConnectorColor(node_info.input_type[conref.connector - 1]);
            else if (node_info.output_type)
                con_color = GetConnectorColor(node_info.output_type);
            else
                continue;

            ImVec2 mid = GetConnectorCoords(nodes, conref, node_size, region_min);

            f32 con_radius = 8.0f;
            draw_list->AddCircleFilled(mid, con_radius, con_color);
            draw_list->AddCircle(mid, con_radius, connector_center_color);
            draw_list->AddCircleFilled(mid, 0.5f * con_radius, connector_center_color);

            ImVec2 con_min = mid - ImVec2(con_radius, con_radius);
            ImVec2 con_size = ImVec2(2.0f * con_radius, 2.0f * con_radius);
            ImVec2 con_max = con_min + con_size;

            ImGui::SetCursorScreenPos(con_min);
            ImGuiID con_imgui_id = ImGui::GetID(node->connections + conref.connector - 1);
            ImGui::PushID(con_imgui_id);

            if (ImGui::InvisibleButton("", con_size, ImGuiButtonFlags_PressedOnClick))
                editor->in_line = conref;

            if (ImGui::IsItemActive())
                *((*draw_lines)++) = ImVec4(mid.x, mid.y, mouse_pos.x, mouse_pos.y);

            bool32 con_hovered = (con_min.x <= mouse_pos.x && mouse_pos.x <= con_max.x &&
                                  con_min.y <= mouse_pos.y && mouse_pos.y <= con_max.y);
            
            if (editor->in_line.node && editor->in_line.node != id && con_hovered && mouse_released)
            {
                Node_Connector_Reference a = conref;
                Node_Connector_Reference b = editor->in_line;
                if (conref.connector)
                    ConnectNodes(nodes, a, b);
                else
                    ConnectNodes(nodes, b, a);
                editor->in_line = CreateNodeConnectorReference(0, 0);
            }

            if (conref.connector)
            {
                Node_ID conn_id = GetNodeConnector(nodes, conref);
                if (conn_id)
                {
                    ImVec2 conn_coord = GetConnectorCoords(nodes, CreateNodeConnectorReference(conn_id, 0), node_size, region_min);
                    *((*draw_lines)++) = ImVec4(mid.x, mid.y, conn_coord.x, conn_coord.y);
                }
            }

            ImGui::PopID();
        }

        ImVec2 text_size = ImGui::CalcTextSize(node_info.name);
        draw_list->AddText(ImVec2(center.x - 0.5f*text_size.x, min.y + text_size.y), IM_COL32_WHITE, node_info.name);

        if (node->type == NodeType_Constant)
        {
            ImGui::PushID(&node->value);
            ImGui::SetCursorScreenPos(ImVec2(center.x - 58.0f, center.y + 2.0f));
            ImGui::PushItemWidth(116.0f);
            ImGui::InputFloat("", &node->value);
            ImGui::PopID();
        }
        else
        {
            char buf[64];
            if (!state.resolved)
                sprintf(buf, "Invalid");
            else if (node_info.output_type == NodeValueType_Player)
                sprintf(buf, state.value == 0.0 ? "Red" : "Blue");
            else
                sprintf(buf, "%f", state.value);
            text_size = ImGui::CalcTextSize(buf);
            draw_list->AddText(ImVec2(center.x - 0.5f*text_size.x, center.y + 5.0f), IM_COL32_WHITE, buf);
        }

        ImGui::PopID();
    }
}

internal void
ImGuiNodes(Node_System *nodes, Node_Editor_Window *editor, Calc_Nodes_Result *calc)
{
    ImVec2 region_min = ImGui::GetCursorScreenPos() - ImVec2(10.0f, 10.0f);

    bool32 mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    ImDrawList *draw_list = ImGui::GetWindowDrawList(); 

    ImU32 connector_center_color = IM_COL32(44, 44, 44, 255);

    ImVec4 draw_lines[512];
    ImVec4 *draw_ptr = draw_lines;

    for (Node_ID id = 1; id <= ArrayCount(nodes->nodes); ++id)
    {
        ImGuiNode(editor, nodes, id, &draw_ptr, region_min, calc->states[id - 1]);
    }

    for (ImVec4 *draw_line = draw_lines; draw_line < draw_ptr; ++draw_line)
    {
        ImVec2 a = ImVec2(draw_line->x, draw_line->y);
        ImVec2 b = ImVec2(draw_line->z, draw_line->w);
        ImVec2 a_1 = ImVec2(a.x, Lerp(a.y, b.y, 0.4f));
        ImVec2 b_1 = ImVec2(b.x, Lerp(b.y, a.y, 0.4f));
        draw_list->AddBezierCubic(a, a_1, b_1, b, connector_center_color, 4.0f);
        draw_list->AddBezierCubic(a, a_1, b_1, b, IM_COL32_WHITE, 2.0f);
    }

    if (!mouse_down)
    {
        Node_Connector_Reference conref = CreateNodeConnectorReference(0, 0);
        if (editor->in_line.node && editor->in_line.connector)
        {
            ConnectNodes(nodes, editor->in_line, conref);
        }
        editor->in_line = conref;

        if (editor->dragging_node)
        {
            editor->dragging_node = 0;
        }
    }
}
