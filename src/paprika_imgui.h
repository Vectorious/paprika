#pragma once


#include "paprika.h"


struct Log_Window
{
    bool active;

    Memory_Arena view_buffer;
};

struct Config_Window
{
    bool active;

    Paprika_Config edit_config;
};

struct Current_Match_Window
{
    bool active;

    i64 wager;
    int selected_strategy;
};

struct Node_Editor_Window
{
    bool active;

    Node_Connector_Reference in_line;
    Node_ID dragging_node;
};

struct Strategies_Window
{
    bool active;

    Node_System_ID selected_system;
    Node_System_ID renaming_system;

    Node_Editor_Window editor;
};

struct History_Window
{
    bool active;
};

struct Paprika_Windows
{
    bool demo_active;
    Current_Match_Window match_window;
    Strategies_Window strategies_window;
    Config_Window config_window;
    History_Window history_window;
    Log_Window log_window;

    Memory_Arena strategies_list;
};


#include "imgui_nodes.h"
