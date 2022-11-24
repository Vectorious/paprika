#include "paprika.h"
#include "imgui_nodes.h"

#if !INTERNAL_BUILD & !RELEASE_BUILD

#define IM_VEC2_CLASS_EXTRA                                                     \
        constexpr ImVec2(const Vector2& v) : x(v.x), y(v.y) {}                  \
        operator Vector2() const { return {x,y}; }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"

#include "implot/implot.h"
#include "implot/implot_internal.h"
#include "implot/implot.cpp"
#include "implot/implot_items.cpp"
#pragma clang diagnostic pop
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#endif


internal void
PlayerFrame(ImGuiID id, f32 width, char *name, i16 rank, u32 confidence, u32 prediction, u32 vs_wins, ImVec4 color)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, color);

    if (ImGui::BeginChild(id, ImVec2(width, 102.0f), true, ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::Text("%s", name);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", name);
        ImGui::Separator();
        ImGui::Text("%d", rank);
        ImGui::Text("%u%%", confidence);
        ImGui::Text("%u%%", prediction);
        ImGui::Text("%d", vs_wins);
    }
    ImGui::EndChild();

    ImGui::PopStyleColor();
}

internal void
VersusSeparator(ImGuiID id, ImVec4 color)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, color);
    ImGui::PushStyleColor(ImGuiCol_Border, color);

    if (ImGui::BeginChild(id, ImVec2(86.0f, 102.0f), true, ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::Text("%s", "    VS    ");
        ImGui::Separator();
        ImGui::Text("%s", "   RANK   ");
        ImGui::Text("%s", "CONFIDENCE");
        ImGui::Text("%s", "PREDICTION");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
        ImGui::Text("%s", " VS WINS");
    }
    ImGui::EndChild();

    ImGui::PopStyleColor(2);
}

internal void
PlayerTotals(Match match, bool32 tooltip = true)
{
    i64 match_total = match.player1total + match.player2total;
    f64 red_ratio = match_total ? (f64)match.player1total / match_total : 0.5;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 content_size = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
    if (content_size.x + content_size.y <= EPSILON)
        content_size = ImVec2(128, 20);

    ImGui::PushID((ImGuiID)match.timestamp);
    ImGui::InvisibleButton("", ImVec2(content_size.x, 20));

    if (tooltip && ImGui::IsItemHovered())
    {
        f32 red_odds = match.player1total > match.player2total ? (f32)match.player1total / match.player2total : 1.0f;
        f32 blue_odds = match.player2total > match.player1total ? (f32)match.player2total / match.player1total : 1.0f;
        ImGui::SetTooltip("%.1f:%.1f", red_odds, blue_odds);
    }

    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    ImU32 red_color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.8f, 0.3f, 0.3f, 0.4f));
    ImU32 blue_color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.8f, 0.4f));

    ImVec2 padding = ImVec2(3.0f, 3.0f);

    f32 mid_x = Lerp(min.x, max.x, (f32)red_ratio);
    draw_list->AddRectFilled(min, ImVec2(mid_x, max.y), red_color);
    draw_list->AddRectFilled(ImVec2(mid_x, min.y), max, blue_color);

    char amt_buf[32];
    sprintf(amt_buf, "$%llu", match.player1total);
    draw_list->AddText(min + padding, IM_COL32_WHITE, amt_buf);
    sprintf(amt_buf, "$%llu", match.player2total);
    draw_list->AddText(ImVec2(max.x - ImGui::CalcTextSize(amt_buf).x - padding.x, min.y + padding.y), IM_COL32_WHITE, amt_buf);

    ImGui::PopID();
}

internal void
MatchWindow(Paprika_State *paprika, Current_Match_Window *match_window)
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(380, 238), ImVec2(1920, 238));
    if (ImGui::Begin("Current Match", &match_window->active) && paprika->mm.characters.count)
    {
        Match match = paprika->current_match;
        Character *red = GetCharacter(&paprika->mm.characters, match.player1id);
        Character *blue = GetCharacter(&paprika->mm.characters, match.player2id);

        Matchup_Comparison comparison = paprika->current_match_comparison;

        u32 prediction_pct = (u32)(comparison.red_prediction * 100.0f);

        ImVec2 content_size = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        ImVec2 padding = window->WindowPadding;

        f32 avail_width = content_size.x - 86.0f - (2.0f * padding.x);
        f32 frame_width = Max(0.5f * avail_width, 128.0f);

        PlayerFrame((ImGuiID)&match, frame_width,
                    GetCharacterName(&paprika->mm.characters, red->name_offset),
                    red->rank.rank,
                    comparison.red_confidence * 100.0f,
                    prediction_pct,
                    comparison.vs_wins,
                    ImVec4(0.8f, 0.2f, 0.2f, 0.4f));

        ImGui::SameLine();
        VersusSeparator((ImGuiID)&match + 1, ImVec4(0.3f, 0.3f, 0.3f, 0.0f));
        ImGui::SameLine();

        PlayerFrame((ImGuiID)&match + 2, 0.0f,
                    GetCharacterName(&paprika->mm.characters, blue->name_offset),
                    blue->rank.rank,
                    comparison.blue_confidence * 100.0f,
                    100 - prediction_pct,
                    comparison.vs_count - comparison.vs_wins,
                    ImVec4(0.2f, 0.2f, 0.8f, 0.4f));

        ImGui::Text("Gamemode: %s", GetSaltyBetModeName(comparison.gamemode));

        ImGui::BeginChild((ImGuiID)&match + 3, ImVec2(0, 62), true);
        if (paprika->mode == PaprikaMode_Open || paprika->mode == PaprikaMode_Place_Wager)
        {
            Node_System *active_nodes = GetNodeSystem(&paprika->node_systems, match_window->selected_strategy);

            if (!active_nodes)
                ImGui::BeginDisabled();

            if (ImGui::Button("Strategy Wager"))
            {
                Calc_Nodes_Result calc = CalcNodes(active_nodes, &paprika->current_match_comparison);
                if (calc.all_nodes_resolved)
                {
                    paprika->pending_input.place_bet = true;
                    paprika->pending_input.player = calc.out.player + 1;
                    paprika->pending_input.wager = calc.out.wager;
                    ChangeMode(paprika, PaprikaMode_Place_Wager);
                }
            }

            if (!active_nodes)
                ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::Combo("##Selected Strategy", &match_window->selected_strategy, (char *)paprika->windows.strategies_list.base);

            ImGui::Separator();

            if (ImGui::Button("Red"))
            {
                paprika->pending_input.place_bet = true;
                paprika->pending_input.player = 1;
                paprika->pending_input.wager = match_window->wager;
                ChangeMode(paprika, PaprikaMode_Place_Wager);
            }
            ImGui::SameLine();
            if (ImGui::Button("Blue"))
            {
                paprika->pending_input.place_bet = true;
                paprika->pending_input.player = 2;
                paprika->pending_input.wager = match_window->wager;
                ChangeMode(paprika, PaprikaMode_Place_Wager);
            }

            ImGui::SameLine();

            const i64 zero = 0;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderScalar("##Wager", ImGuiDataType_S64, &match_window->wager, &zero, &comparison.balance, "$%lld");
        }
        else
        {
            PlayerTotals(match);

            if (paprika->mode == PaprikaMode_Await_Open && paprika->current_match.gamemode)
            {
                Arena_Offset winner_offset = match.winner ? blue->name_offset : red->name_offset;
                ImGui::Text("'%s' wins!", GetCharacterName(&paprika->mm.characters, winner_offset));
                if (paprika->zdata.wager)
                {
                    ImGui::SameLine();
                    i64 change = paprika->zdata.balance - paprika->current_match_comparison.balance;
                    char change_sign;
                    if (change > 0)
                    {
                        change_sign = '+';
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
                    }
                    else
                    {
                        change_sign = '-';
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.4f, 0.4f, 1.0f));
                    }
                    ImGui::Text("(%c$%lld)", change_sign, Abs(change));
                    ImGui::PopStyleColor();
                }
            }
            else if (paprika->zdata.wager)
            {
                i64 projected_winnings;
                char *wager_name;
                if (paprika->zdata.player == 1)
                {
                    projected_winnings = ceil(((f64)paprika->zdata.wager / match.player1total) * match.player2total);
                    wager_name = GetCharacterName(&paprika->mm.characters, red->name_offset);
                }
                else
                {
                    projected_winnings = ceil(((f64)paprika->zdata.wager / match.player2total) * match.player1total);
                    wager_name = GetCharacterName(&paprika->mm.characters, blue->name_offset);
                }
            
                ImGui::Text("Wager: '%s' ($%lld -> $%lld)", wager_name, paprika->zdata.wager, projected_winnings);
            }
        }
        ImGui::EndChild();

        ImGui::TextUnformatted(paprika->state.remaining);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", paprika->state.remaining);
    }
    ImGui::End();
}

internal void
StrategiesWindow(Paprika_State *paprika, Strategies_Window *strategies_window)
{
    Node_Systems *node_systems = &paprika->node_systems;

    ImGui::SetNextWindowSize(ImVec2(426, 214), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(349, 175), ImVec2(1920, 1080));
    if (ImGui::Begin("Strategies", &strategies_window->active))
    {
        i64 simulation_results[32];
        SimulateNodeSystems(paprika->player.balance,
                            GetBailout(paprika->player.rank),
                            (Salty_Bet_Mode)paprika->current_match.gamemode,
                            &paprika->node_systems,
                            &paprika->mm.matches,
                            &paprika->history,
                            simulation_results);

        if (ImGui::Button("New"))
            AddNodeSystem(node_systems);
        
        ImGui::SameLine();
        if (ImGui::Button("Open Editor"))
        {
            strategies_window->editor.active = true;
            ImGui::SetWindowFocus("###Strategy Editor");
        }

        ImGui::BeginChild("left pane", ImVec2(0, -70.0f), true);

        ImGui::Text("Strategy");
        ImGui::SameLine(240.0f);
        ImGui::Text("Simulation");

        ImGui::Separator();

        Node_System_ID ns_id = 1; 
        while (ns_id <= node_systems->count)
        {
            Node_System *node_system = GetNodeSystem(node_systems, ns_id);
            
            bool32 ns_deleted = false;

            ImGui::PushID(node_system);
            bool32 is_selected = strategies_window->selected_system == ns_id;
            bool32 is_renaming = strategies_window->renaming_system == ns_id;

            if (is_renaming)
            {
                ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("", node_system->name, sizeof(node_system->name),
                                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    strategies_window->renaming_system = 0;
                }
            }
            else
            {
                bool32 open_delete_popup = false;

                if (ImGui::Selectable(node_system->name, is_selected))
                    strategies_window->selected_system = ns_id;

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Rename"))
                        strategies_window->renaming_system = ns_id;
                    
                    if (ImGui::MenuItem("Delete"))
                        open_delete_popup = true;

                    ImGui::EndPopup();
                }

                if (open_delete_popup)
                    ImGui::OpenPopup("Delete strategy?");

                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                bool popup_true = true;
                if (ImGui::BeginPopupModal("Delete strategy?", &popup_true, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Are you sure you want to delete the strategy '%s'?", node_system->name);

                    if (ImGui::Button("Yes", ImVec2(120, 0)))
                    {
                        RemoveNodeSystem(node_systems, ns_id);
                        ns_deleted = true;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SetItemDefaultFocus();
                    ImGui::SameLine();

                    if (ImGui::Button("No", ImVec2(120, 0)))
                        ImGui::CloseCurrentPopup();

                    ImGui::EndPopup();
                }

                ImGui::SameLine(240.0f);

                i64 sim_result = simulation_results[ns_id - 1];
                char change_sign = sim_result > paprika->player.balance ? '+' : '-';
                ImGui::Text("%c$%lld", change_sign, Abs(sim_result - paprika->player.balance));
            }

            ImGui::PopID();

            if (!ns_deleted)
                ++ns_id;
        }
        ImGui::EndChild();

        char *strategy_list = (char *)paprika->windows.strategies_list.base;
        ImGui::Combo("Matchmaking", (int *)&node_systems->matchmaking_id, strategy_list);
        ImGui::Combo("Tournament", (int *)&node_systems->tournament_id, strategy_list);
        ImGui::Combo("Exhibition", (int *)&node_systems->exhibition_id, strategy_list);

        if (strategies_window->editor.active)
        {
            Node_System *node_system = GetNodeSystem(node_systems, strategies_window->selected_system);

            char title_buf[96];
            sprintf(title_buf, "Strategy Editor - %s###Strategy Editor",
                    node_system ? node_system->name : "None");

            ImGui::SetNextWindowSize(ImVec2(840, 640), ImGuiCond_FirstUseEver);
            if (ImGui::Begin(title_buf, &strategies_window->editor.active))
            {
                ImGui::BeginGroup();
                if (node_system)
                {
                    ImGui::BeginChild("node buttons", ImVec2(150, 0), true);
                    for (Node_Type type = (Node_Type)2;
                         type < NodeTypeCount;
                         type = (Node_Type)(type + 1))
                    {
                        if (type == NodeType_Constant)
                        {
                            ImGui::Text("Manual Input");
                        }
                        else if (type == NodeType_Favorite)
                        {
                            ImGui::Separator();
                            ImGui::Text("Player Variables");
                        }
                        else if (type == NodeType_Team_In_Match)
                        {
                            ImGui::Separator();
                            ImGui::Text("Number Variables");
                        }
                        else if (type == NodeType_Win_Probability)
                        {
                            ImGui::Separator();
                            ImGui::Text("Player Functions");
                        }
                        else if (type == NodeType_Multiply)
                        {
                            ImGui::Separator();
                            ImGui::Text("Number Functions");
                        }
                        else if (type == NodeType_If)
                        {
                            ImGui::Separator();
                            ImGui::Text("Conditionals");
                        }

                        Node_Info info = GetNodeInfo(type);
                        if (ImGui::Button(info.name))
                        {
                            AddNode(node_system, type);
                        }
                    }
                    ImGui::EndChild();

                    ImGui::SameLine();

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                    ImGui::BeginChild("node view", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
                    Calc_Nodes_Result calc = CalcNodes(node_system, &paprika->current_match_comparison);
                    ImGuiNodes(node_system, &strategies_window->editor, &calc);
                    ImGui::EndChild();
                    ImGui::PopStyleVar();
                }
                ImGui::EndGroup();
            }
            ImGui::End();
        }
    }
    ImGui::End();
}

internal void
LogWindow(Paprika_State *paprika, Log_Window *log_window)
{
    ImGui::SetNextWindowSize(ImVec2(350, 180), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Log", &log_window->active))
    {
        ArenaReset(&log_window->view_buffer);

        Line_Buffer_Read_Result read = LineBufferRead(&paprika->log.line_buffer);
        if (read.a_start)
        {
            mem_t a_size = read.a_end - read.a_start;
            MemCopy(read.a_start, ArenaAlloc(&log_window->view_buffer, a_size), a_size);

            if (read.b_start)
            {
                mem_t b_size = read.b_end - read.b_start;
                MemCopy(read.b_start, ArenaAlloc(&log_window->view_buffer, b_size), b_size);
            }

            bool32 auto_scroll = (ImGui::GetScrollMaxY() - ImGui::GetScrollY()) <= 1.0f;

            char *start = (char *)log_window->view_buffer.base;
            ImGui::TextUnformatted(start, start + log_window->view_buffer.used);
            
            if (auto_scroll) ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::End();
}

internal void
ConfigWindow(Paprika_State *paprika, Config_Window *config_window)
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(328, 100), ImVec2(1920, 100));
    if (ImGui::Begin("Configuration", &config_window->active))
    {
        ImGui::InputText("Username", config_window->edit_config.username, sizeof(config_window->edit_config.username));
        ImGui::InputText("Password", config_window->edit_config.password, sizeof(config_window->edit_config.password), ImGuiInputTextFlags_Password);
        if (ImGui::Button("Save"))
        {
            paprika->config = config_window->edit_config;
            SaveConfig(&paprika->platform, &paprika->json_writer, &paprika->config);
        }
    }
    ImGui::End();
}

internal inline void
ActivateConfigWindow(Paprika_State *paprika)
{
    paprika->windows.config_window.edit_config = paprika->config;
    paprika->windows.config_window.active = true;
}

internal f32
GetHistoryIndex(void *wagers, i32 index)
{
    return (f32)((Match_Wager *)wagers + index)->close_balance;
}

internal i32
BinarySearch(u64 *arr, i32 count, u64 needle)
{
    i32 min = 0;
    i32 max = count - 1;
    i32 idx = -1;
    while (min <= max)
    {
        idx = (i32)((min + max) / 2);
        u64 val = arr[idx];
        if (needle == val)
            break;
        if (needle < val)
            max = idx - 1;
        else
            min = idx + 1;
    }
    return idx;
}

internal void
PlotCandlestick(char *label_id,
                i32 count, Matchup_Frame *matchup_frames,
                Match_System *mm,
                ImU32 pos_col, ImU32 neg_col,
                ImU32 pos_wager_col, ImU32 neg_wager_col)
{
    ImDrawList *draw_list = ImPlot::GetPlotDrawList();

    f64 bar_width = 1.0;
    f64 half_bar_width = 0.5 * bar_width;

    u64 timestamps[ArrayCount(Matchup_History::matchup_frames)];
    for (i32 i = 0; i < count; ++i)
        timestamps[i] = matchup_frames[i].wager.timestamp;

    if (ImPlot::IsPlotHovered())
    {
        ImPlotPoint mouse = ImPlot::GetPlotMousePos();
        mouse.x = (f64)(i32)mouse.x;
        //ImPlotTime mouse_time = ImPlot::RoundTime(ImPlotTime::FromDouble(mouse.x), ImPlotTimeUnit_S);
        //mouse.x = mouse_time.ToDouble();

        f32 l = ImPlot::PlotToPixels(mouse.x, mouse.y).x;
        f32 r = ImPlot::PlotToPixels(mouse.x + bar_width, mouse.y).x;
        f32 t = ImPlot::GetPlotPos().y;
        f32 b = t + ImPlot::GetPlotSize().y;

        ImPlot::PushPlotClipRect();
        draw_list->AddRectFilled(ImVec2(l, t), ImVec2(r, b), IM_COL32(128, 128, 128, 64));
        ImPlot::PopPlotClipRect();


        //i32 idx = BinarySearch(timestamps, count, (u64)mouse.x);
        i32 mouse_index = (i32)mouse.x;
        i32 idx = (0 <= mouse_index && mouse_index < count) ? mouse_index : -1;

        if (idx != -1)
        {
            ImGui::BeginTooltip();

            char buf[64];

            Matchup_Frame frame = matchup_frames[idx];
            Match_Wager wager = frame.wager;
            Match *match = GetMatch(&mm->matches, frame.match_id);
            Matchup_Comparison comparison = frame.comparison;

            ImPlot::FormatDateTime(ImPlotTime(wager.timestamp), buf, ArrayCount(buf), ImPlotDateTimeSpec(ImPlotDateFmt_DayMoYr, ImPlotTimeFmt_HrMinS));
            ImGui::Text("Time:   %s", buf);
            ImGui::Text("Start:  $%llu", wager.open_balance);
            ImGui::Text("End:    $%llu", wager.close_balance);
            ImGui::Text("Wager:  $%llu", wager.wager);
            char change_sign = wager.close_balance > wager.open_balance ? '+' : '-';
            ImGui::Text("Change: %c$%lld",
                        change_sign,
                        Abs(wager.close_balance - wager.open_balance));
            ImGui::Text("Player: %s", wager.player ? "Blue" : "Red");

            ImGui::Separator();

            Character *red = GetCharacter(&mm->characters, match->player1id);
            Character *blue = GetCharacter(&mm->characters, match->player2id);

            PlayerFrame((ImGuiID)&match, 128.0f,
                        GetCharacterName(&mm->characters, red->id),
                        red->rank.rank,
                        (u32)(comparison.red_confidence * 100.0f),
                        (u32)(comparison.red_prediction * 100.0f),
                        comparison.vs_wins,
                        ImVec4(0.8f, 0.2f, 0.2f, 0.4f));

            ImGui::SameLine();
            VersusSeparator((ImGuiID)&match + 1, ImVec4(0.3f, 0.3f, 0.3f, 0.0f));
            ImGui::SameLine();

            PlayerFrame((ImGuiID)&match + 2, 128.0f,
                        GetCharacterName(&mm->characters, blue->id),
                        blue->rank.rank,
                        (u32)(comparison.blue_confidence * 100.0f),
                        (u32)((1.0f - comparison.red_prediction) * 100.0f),
                        comparison.vs_count - comparison.vs_wins,
                        ImVec4(0.2f, 0.2f, 0.8f, 0.4f));

            PlayerTotals(*match, false);

            ImGui::Text("Gamemode: %s", GetSaltyBetModeName(comparison.gamemode));
            ImGui::Text("Winner:   %s", match->winner ? "Blue" : "Red");
            
            ImGui::EndTooltip();
        }
    }

    if (ImPlot::BeginItem(label_id))
    {
        ImPlot::GetCurrentItem()->Color = IM_COL32(64, 64, 64, 255);

        if (ImPlot::FitThisFrame())
        {
            for (i32 i = 0; i < count; ++i)
            {
                ImPlot::FitPoint(ImPlotPoint(i, matchup_frames[i].wager.open_balance));
                ImPlot::FitPoint(ImPlotPoint(i, matchup_frames[i].wager.close_balance));
            }
        }

        for (i32 i = 0; i < count; ++i)
        {
            Match_Wager wager = matchup_frames[i].wager;
            if (wager.timestamp)
            {
                ImVec2 open_pos = ImPlot::PlotToPixels(i, wager.open_balance);
                ImVec2 close_pos = ImPlot::PlotToPixels(i + bar_width, wager.close_balance);

                ImVec2 wager_open_pos = ImPlot::PlotToPixels(i + half_bar_width, wager.open_balance);
                ImVec2 wager_close_pos;
                ImU32 color;
                ImU32 wager_color;

                if (wager.open_balance < wager.close_balance)
                {
                    wager_close_pos = ImPlot::PlotToPixels(i + half_bar_width, wager.open_balance + wager.wager);
                    color = pos_col;
                    wager_color = pos_wager_col;
                }
                else
                {
                    wager_close_pos = ImPlot::PlotToPixels(i + half_bar_width, wager.open_balance - wager.wager);
                    color = neg_col;
                    wager_color = neg_wager_col;
                }
                
                draw_list->AddRectFilled(open_pos, close_pos, color);
                if (wager.close_balance != wager.open_balance - wager.wager)
                    draw_list->AddLine(wager_open_pos, wager_close_pos, wager_color);
            }
        }

        ImPlot::EndItem();
    }
}

internal void
HistoryWindow(Paprika_State *paprika, History_Window *history_Window)
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(218, 187), ImVec2(1000000, 1000000));
    ImGui::SetNextWindowSize(ImVec2(816, 618), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("History", &history_Window->active))
    {
        const ImVec4 pos_col = ImVec4(0.000f, 1.000f, 0.294f, 1.000f);
        const ImVec4 neg_col = ImVec4(1.000f, 0.000f, 0.353f, 1.000f);
        const ImVec4 pos_wager_col = ImVec4(0.426f, 0.600f, 0.755f, 1.000f);
        const ImVec4 neg_wager_col = ImVec4(1.000f, 0.000f, 0.353f, 1.000f);

        if (ImPlot::BeginPlot("Wager Chart", ImVec2(-1, -1), ImPlotFlags_NoLegend |
                                                             ImPlotFlags_NoTitle |
                                                             ImPlotFlags_NoMouseText))
        {
            // TODO: Can probably change this to use `Match_Wager`.
            Matchup_Frame matchup_frames[ArrayCount(paprika->history.matchup_frames)];
            CircularBufferCopy(matchup_frames, paprika->history.matchup_frames,
                               sizeof(Matchup_Frame), paprika->history.count,
                               ArrayCount(paprika->history.matchup_frames), paprika->history.write_index);

            f64 y_focus = 0;
            if (paprika->history.count > 0)
                y_focus = (f64)matchup_frames[paprika->history.count - 1].wager.close_balance;
            f64 y_min = Max(y_focus - 25000, 0);

            ImPlot::SetupAxisLimits(ImAxis_X1, 0, 100, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, y_min, y_min + 50000);

            ImPlot::SetupAxes(0, "Balance",
                              ImPlotAxisFlags_NoGridLines |
                              ImPlotAxisFlags_NoTickMarks |
                              ImPlotAxisFlags_NoTickLabels);
            ImPlot::SetupAxisFormat(ImAxis_Y1, "$%.f");

            PlotCandlestick("Wagers",
                            (u32)paprika->history.count,
                            matchup_frames, &paprika->mm,
                            ImGui::GetColorU32(pos_col), ImGui::GetColorU32(neg_col),
                            ImGui::GetColorU32(pos_wager_col), ImGui::GetColorU32(neg_wager_col));

            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}

internal void
MainWindow(Paprika_State *paprika)
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
    f32 padding = 10.0f;
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos = ImVec2(work_pos.x + padding, work_pos.y + work_size.y - padding);
    ImVec2 window_pos_pivot = ImVec2(0.0f, 1.0f);

    ImGui::SetNextWindowBgAlpha(0.25f); // Transparent background
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    if (ImGui::Begin("main", 0, window_flags))
    {
        ImGui::Text("Username: %s", paprika->player.name);
        ImGui::Text("Balance:  $%lld", paprika->player.balance);
        ImGui::Text("Status:   %s", GetPaprikaModeName(paprika->mode));
    }
    ImGui::End();
}

internal void
HandleWindows(Paprika_State *paprika)
{
    ArenaReset(&paprika->windows.strategies_list);
    ArenaAllocString(&paprika->windows.strategies_list, "None");
    for (Node_System_ID ns_id = 1; ns_id <= paprika->node_systems.count; ++ns_id)
    {
        Node_System *node_system = GetNodeSystem(&paprika->node_systems, ns_id);
        ArenaAllocString(&paprika->windows.strategies_list, node_system->name);
    }
    ArenaAlloc(&paprika->windows.strategies_list, 1, 1);

    ImGui::SetCurrentContext(paprika->platform.im_ctx);
    ImPlot::SetCurrentContext(paprika->platform.implot_ctx);

    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save"))
            {
                SaveMatchData(&paprika->platform, &paprika->mm);
                SaveNodeSystems(&paprika->platform, &paprika->node_systems);
                SaveConfig(&paprika->platform, &paprika->json_writer, &paprika->config);
            }

            if (ImGui::MenuItem("Reload data"))
            {
                LoadPaprikaData(paprika, false);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Bot"))
        {
            if (ImGui::MenuItem("Start"))
                ChangeMode(paprika, PaprikaMode_Login);
            if (ImGui::MenuItem("Stop", 0, paprika->mode == PaprikaMode_Disabled))
                ChangeMode(paprika, PaprikaMode_Disabled);
            if (ImGui::MenuItem("Stop after current round", 0, paprika->stopping))
                paprika->stopping = !paprika->stopping;

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows"))
        {
            ImGui::MenuItem("Strategies", 0, &paprika->windows.strategies_window.active);
            ImGui::MenuItem("History", 0, &paprika->windows.history_window.active);
            ImGui::MenuItem("Current Match", 0, &paprika->windows.match_window.active);
            ImGui::MenuItem("Log", 0, &paprika->windows.log_window.active);
            if (ImGui::MenuItem("Configuration", 0, &paprika->windows.config_window.active) &&
                paprika->windows.config_window.active)
            {
                ActivateConfigWindow(paprika);
            }

#ifdef INTERNAL_BUILD
            ImGui::MenuItem("ImGui Demo", 0, &paprika->windows.demo_active);
#endif

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    MainWindow(paprika);

    if (paprika->windows.strategies_window.active)
        StrategiesWindow(paprika, &paprika->windows.strategies_window);

    if (paprika->windows.log_window.active)
        LogWindow(paprika, &paprika->windows.log_window);
    
    if (paprika->windows.match_window.active)
        MatchWindow(paprika, &paprika->windows.match_window);
    
    if (paprika->windows.config_window.active)
        ConfigWindow(paprika, &paprika->windows.config_window);
    
    if (paprika->windows.history_window.active)
        HistoryWindow(paprika, &paprika->windows.history_window);

#ifdef INTERNAL_BUILD
    if (paprika->windows.demo_active)
        ImGui::ShowDemoWindow(&paprika->windows.demo_active);
#endif

    ImGui::Render();
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
}
