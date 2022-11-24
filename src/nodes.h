#pragma once


#include "paprika.h"


enum Node_Value_Type : u8
{
    NodeValueType_None = 0,

    NodeValueType_Number,
    NodeValueType_Player,

    NodeValueTypeCount,
};

// TODO: For now, these simple boundaries between sections is fine, but when
// new node types are added, the sections will need to be put into arrays.
enum Node_Type : u8
{
    NodeType_None = 0,

    NodeType_Output,

    // Manual input
    NodeType_Constant,

    // Player variables
    NodeType_Favorite,
    NodeType_Upset,

    // Number variables
    NodeType_Team_In_Match,
    NodeType_Versus_Count,
    NodeType_Balance,

    // Player functions
    NodeType_Win_Probability,
    NodeType_Confidence,

    // Number functions
    NodeType_Multiply,
    NodeType_Divide,
    NodeType_Add,
    NodeType_Subtract,
    NodeType_Not,
    NodeType_Xor,
    NodeType_Greater,
    NodeType_Less,
    NodeType_Equals,
    NodeType_Clamp,

    // Conditionals
    NodeType_If,
    NodeType_If_Player,

    NodeTypeCount,
};

typedef u8 Node_ID;
typedef u8 Node_Connector_ID;

struct Node_Connector_Reference
{
    Node_ID node;
    Node_Connector_ID connector;
};

struct Node
{
    Node_Type type;

    union
    {
        Node_ID connections[4];
        f32 value;
    };

    Vector2 position;
};

struct Node_System
{
    char name[32];
    Node nodes[100];
};

struct Calc_Nodes_Out
{
    i64 wager;
    u8 player;
};

internal inline bool32
operator==(Node_Connector_Reference a, Node_Connector_Reference b)
{
    return a.node == b.node && a.connector == b.connector;
}

internal inline bool32
operator!=(Node_Connector_Reference a, Node_Connector_Reference b)
{
    return !(a == b);
}

internal inline Node_Connector_Reference
CreateNodeConnectorReference(Node_ID node, Node_Connector_ID connector)
{
    Node_Connector_Reference result;
    result.node = node;
    result.connector = connector;
    return result;
}


#define NodeMethod(name) f64 name(f64 *input, Matchup_Comparison *comparison, Calc_Nodes_Out *out)
typedef NodeMethod(Node_Method_Type);


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

internal NodeMethod(NodeMethodOutput)
{
    f64 wager = input[0];
    f64 player = input[1];
    out->wager = (i64)wager;
    out->player = (u8)player;
    return wager;
}

internal NodeMethod(NodeMethodMultiply)
{
    f64 a = input[0];
    f64 b = input[1];
    f64 result = a * b;
    return result;
}

internal NodeMethod(NodeMethodWinProbability)
{
    f64 a = input[0];
    f64 result = a == 0.0f ? comparison->red_prediction : 1.0f - comparison->red_prediction;
    return result;
}

internal NodeMethod(NodeMethodGreaterThan)
{
    f64 a = input[0];
    f64 b = input[1];
    f64 result = a > b ? 1.0f : 0.0f;
    return result;
}

internal NodeMethod(NodeMethodLessThan)
{
    f64 a = input[0];
    f64 b = input[1];
    f64 result = a < b ? 1.0f : 0.0f;
    return result;
}

internal NodeMethod(NodeMethodEquals)
{
    f64 a = input[0];
    f64 b = input[1];
    f64 result = Abs(a - b) <= EPSILON;
    return result;
}

internal NodeMethod(NodeMethodAdd)
{
    f64 a = input[0];
    f64 b = input[1];
    f64 result = a + b;
    return result;
}

internal NodeMethod(NodeMethodDivide)
{
    f64 a = input[0];
    f64 b = input[1];
    f64 result = a / b;
    return result;
}

internal NodeMethod(NodeMethodIf)
{
    f64 a = input[0];
    f64 b = input[1];
    f64 c = input[2];
    f64 result = a > 0.0f ? b : c;
    return result;
}

internal NodeMethod(NodeMethodBank)
{
    return comparison->balance;
}

internal NodeMethod(NodeMethodClamp)
{
    f64 a = input[0];
    f64 b = input[1];
    f64 c = input[2];
    f64 result = Clamp(a, b, c);
    return result;
}

internal NodeMethod(NodeMethodFavorite)
{
    if (comparison->red_prediction > 0.5f)
        return 0.0f;
    else
        return 1.0f;
}

internal NodeMethod(NodeMethodUpset)
{
    if (comparison->red_prediction > 0.5f)
        return 1.0f;
    else
        return 0.0f;
}

internal NodeMethod(NodeMethodConfidence)
{
    f64 a = input[0];
    f64 result = a == 0.0f ? comparison->red_confidence : comparison->blue_confidence;
    return result;
}

internal NodeMethod(NodeMethodTeamInMatch)
{
    f64 result = (f64)comparison->team_in_matchup;
    return result;
}

internal NodeMethod(NodeMethodVersusCount)
{
    f64 result = (f64)comparison->vs_count;
    return result;
}

internal NodeMethod(NodeMethodNot)
{
    f64 a = input[0];
    f64 result = a == 0.0;
    return result;
}

internal NodeMethod(NodeMethodXor)
{
    f64 a = input[0];
    f64 b = input[1];
    f64 result = (a == 0.0 && b != 0.0) || (b == 0.0 && a != 0.0);
    return result;
}

internal NodeMethod(NodeMethodSubtract)
{
    f64 a = input[0];
    f64 b = input[1];
    f64 result = a - b;
    return result;
}

#pragma clang diagnostic pop


struct Node_Info
{
    Node_Value_Type input_type[ArrayCount(Node::connections)];
    Node_Value_Type output_type;
    u8 input_count;
    
    char *name;
    Node_Method_Type *method;
};

internal Node_Info 
GetNodeInfo(Node_Type type)
{
    Assert(type);

    Node_Info result;
    switch (type)
    {
        case NodeType_Output:
            result = {{NodeValueType_Number, NodeValueType_Player},
                      NodeValueType_None, 2, "Output", NodeMethodOutput};
            break;
            
        case NodeType_Constant:
            result = {{},
                      NodeValueType_Number, 0, "Constant", 0};
            break;

        case NodeType_Favorite:
            result = {{},
                      NodeValueType_Player, 0, "Favorite", NodeMethodFavorite};
            break;

        case NodeType_Upset:
            result = {{},
                      NodeValueType_Player, 0, "Upset", NodeMethodUpset};
            break;

        case NodeType_Team_In_Match:
            result = {{},
                      NodeValueType_Number, 0, "Team In Match", NodeMethodTeamInMatch};
            break;

        case NodeType_Versus_Count:
            result = {{},
                      NodeValueType_Number, 0, "Versus Count", NodeMethodVersusCount};
            break;
       
        case NodeType_Balance:
            result = {{},
                      NodeValueType_Number, 0, "Balance", NodeMethodBank};
            break;

        case NodeType_Win_Probability:
            result = {{NodeValueType_Player},
                      NodeValueType_Number, 1, "Win Probability", NodeMethodWinProbability};
            break;

        case NodeType_Confidence:
            result = {{NodeValueType_Player},
                      NodeValueType_Number, 1, "Player Confidence", NodeMethodConfidence};
            break;
         
        case NodeType_Multiply:
            result = {{NodeValueType_Number, NodeValueType_Number},
                      NodeValueType_Number, 2, "Multiply", NodeMethodMultiply};
            break;

        case NodeType_Divide:
            result = {{NodeValueType_Number, NodeValueType_Number},
                      NodeValueType_Number, 2, "Divide", NodeMethodDivide};
            break;

        case NodeType_Add:
            result = {{NodeValueType_Number, NodeValueType_Number},
                      NodeValueType_Number, 2, "Add", NodeMethodAdd};
            break;

        case NodeType_Subtract:
            result = {{NodeValueType_Number, NodeValueType_Number},
                      NodeValueType_Number, 2, "Subtract", NodeMethodSubtract};
            break;

        case NodeType_Not:
            result = {{NodeValueType_Number},
                      NodeValueType_Number, 1, "Not", NodeMethodNot};
            break;
        
        case NodeType_Xor:
            result = {{NodeValueType_Number, NodeValueType_Number},
                      NodeValueType_Number, 2, "Exclusive Or", NodeMethodXor};
            break;

        case NodeType_Greater:
            result = {{NodeValueType_Number, NodeValueType_Number},
                      NodeValueType_Number, 2, "Greater Than", NodeMethodGreaterThan};
            break;

        case NodeType_Less:
            result = {{NodeValueType_Number, NodeValueType_Number},
                      NodeValueType_Number, 2, "Less Than", NodeMethodLessThan};
            break;

        case NodeType_Equals:
            result = {{NodeValueType_Number, NodeValueType_Number},
                      NodeValueType_Number, 2, "Equals", NodeMethodEquals};
            break;

        case NodeType_Clamp:
            result = {{NodeValueType_Number, NodeValueType_Number, NodeValueType_Number},
                      NodeValueType_Number, 3, "Clamp", NodeMethodClamp};
            break;

        case NodeType_If:
            result = {{NodeValueType_Number, NodeValueType_Number, NodeValueType_Number},
                      NodeValueType_Number, 3, "If", NodeMethodIf};
            break;

        case NodeType_If_Player:
            result = {{NodeValueType_Number, NodeValueType_Player, NodeValueType_Player},
                      NodeValueType_Player, 3, "If (Player)", NodeMethodIf};
            break;

        case NodeType_None:
        case NodeTypeCount:
            Panic("Invalid type.");
    }

    return result;
}

internal inline u8
Base64DecodeChar(char c)
{
    u8 result;
    if ('A' <= c && c <= 'Z')
        result = (u8)(c - 'A');
    else if ('a' <= c && c <= 'z')
        result = (u8)(c - 'a' + 26);
    else if ('0' <= c && c <= '9')
        result = (u8)(c - '0' + 52);
    else if (c == '+')
        result = 62;
    else if (c == '/')
        result = 63;
    else
        Panic("Invalid Base64 character.");
    return result;
}

// NOTE: Non-padded
// TODO: Add check for write overflow.
internal void
Base64Encode(u8 *data, mem_t size, char *dst)
{
    const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const u8 six_bits = 63;
    u8 *fast_end = data + size - (size % 3);
    u8 *end = data + size;

    while (data < fast_end)
    {
        u8 a = *data++;
        u8 b = *data++;
        u8 c = *data++;

        u32 packed = (u32)(a | (b << 8) | (c << 16));

        u8 a6 = packed & six_bits;
        u8 b6 = (packed >> 6) & six_bits;
        u8 c6 = (packed >> 12) & six_bits;
        u8 d6 = (packed >> 18) & six_bits;

        *dst++ = table[a6];
        *dst++ = table[b6];
        *dst++ = table[c6];
        *dst++ = table[d6];
    }

    u32 bytes_remaining = (u32)(end - data);
    if (bytes_remaining)
    {
        u8 a = *data++;
        u8 b = 0;
        if (bytes_remaining > 1)
            b = *data++;

        u32 packed = (u32)(a | (b << 8));

        u8 a6 = packed & six_bits;
        u8 b6 = (packed >> 6) & six_bits;
        u8 c6 = (packed >> 12) & six_bits;

        *dst++ = table[a6];
        *dst++ = table[b6];
        if (bytes_remaining > 1)
            *dst++ = table[c6];
    }

    *dst = '\0';
}

internal void
Base64Decode(u8 *dst, mem_t size, char *src)
{
    const u8 eight_bits = 0xFF;
    u8 *fast_end = dst + size - (size % 3);
    u8 *end = dst + size;
    while (dst < fast_end)
    {
        u8 a6 = Base64DecodeChar(*src++);
        u8 b6 = Base64DecodeChar(*src++);
        u8 c6 = Base64DecodeChar(*src++);
        u8 d6 = Base64DecodeChar(*src++);

        u32 packed = (u32)(a6 | (b6 << 6) | (c6 << 12) | (d6 << 18));

        u8 a = packed & eight_bits;
        u8 b = (packed >> 8) & eight_bits;
        u8 c = (packed >> 16) & eight_bits;

        *dst++ = a;
        *dst++ = b;
        *dst++ = c;
    }

    u32 bytes_remaining = (u32)(end - dst);
    if (bytes_remaining)
    {
        u8 a6 = Base64DecodeChar(*src++);
        u8 b6 = Base64DecodeChar(*src++);
        u8 c6 = 0;
        if (bytes_remaining > 1)
            c6 = Base64DecodeChar(*src++);

        u32 packed = (u32)(a6 | (b6 << 6) | (c6 << 12));

        u8 a = packed & eight_bits;
        u8 b = (packed >> 8) & eight_bits;

        *dst++ = a;
        if (bytes_remaining > 1)
            *dst++ = b;
    }
}

internal inline Node *
GetNode(Node_System *nodes, Node_ID id)
{
    Assert(id && id <= ArrayCount(nodes->nodes));
    return nodes->nodes + id - 1;
}

internal inline Node_ID
GetNodeConnector(Node_System *nodes, Node_Connector_Reference conref)
{
    Assert(conref.connector);
    Node_ID result = GetNode(nodes, conref.node)->connections[conref.connector - 1];
    return result;
}

internal Node_ID
AddNode(Node_System *nodes, Node_Type type)
{
    Node_ID result = 0;
    Node *node = nodes->nodes;
    for (Node_ID id = 1; id <= ArrayCount(nodes->nodes); ++id)
    {
        if (node->type == NodeType_None)
        {
            result = id;
            node->type = type;
            node->value = 0.0f;
            node->position = V2();
            MemZero(node->connections, sizeof(node->connections));
            break;
        }
        ++node;
    }
    return result;
}

internal void
NodeSystemInit(Node_System *nodes)
{
    StringCopy("Untitled nodes", nodes->name);
    for (Node *node = nodes->nodes;
         node < nodes->nodes + ArrayCount(nodes->nodes);
         ++node)
    {
        node->type = NodeType_None;
    }
    AddNode(nodes, NodeType_Output);
}

internal inline void
RemoveNode(Node_System *nodes, Node_ID id)
{
    if (id > 1)
    {
        Node *node = GetNode(nodes, id);
        node->type = NodeType_None;
        node->value = 0.0f;
        node->position = V2();
        MemZero(node->connections, sizeof(node->connections));
    }
}

internal bool32
ConnectNodes(Node_System *nodes, Node_Connector_Reference input, Node_Connector_Reference output)
{
    bool32 result = input.connector != 0 && output.connector == 0;
    if (result)
    {
        Node *input_node = GetNode(nodes, input.node);
        Node_Info input_info = GetNodeInfo(input_node->type);

        if (output.node)
        {
            Node *output_node = GetNode(nodes, output.node);
            Node_Info output_info = GetNodeInfo(output_node->type);
    
            result = input_info.input_type[input.connector - 1] == output_info.output_type;
            if (result)
            {
                input_node->connections[input.connector - 1] = output.node;
            }
        }
        else
        {
            input_node->connections[input.connector - 1] = 0;
        }
    }
    return result;
}

struct Node_State
{
    u8 resolved;

    f64 value;
};

struct Calc_Nodes_Result
{
    u32 node_count;
    bool32 all_nodes_resolved;

    Node_State states[ArrayCount(Node_System::nodes)];

    Calc_Nodes_Out out;
};

internal u32
CountNodes(Node_System *system, Node_ID *out_ids)
{
    u32 count = 0;

    for (Node_ID id = 1; id <= ArrayCount(system->nodes); ++id)
    {
        if (GetNode(system, id)->type)
        {
            out_ids[count++] = id;
        }
    }

    return count;
}

internal Calc_Nodes_Result
CalcNodes(Node_System *nodes, Matchup_Comparison *comparison)
{
    Calc_Nodes_Result result = {};

    Node_ID ids[ArrayCount(nodes->nodes)];
    result.node_count = CountNodes(nodes, ids);

    i32 cleaned_count = 1;
    bool32 still_dirty = true;

    f64 input_values[ArrayCount(nodes->nodes->connections)];

    while (still_dirty && cleaned_count)
    {
        still_dirty = false;
        cleaned_count = 0;

        for (u32 idx = 0; idx < result.node_count; ++idx)
        {
            Node_ID id = ids[idx];
            Node_State *state = result.states + id - 1;
            if (!state->resolved)
            {
                Node *node = GetNode(nodes, id);
                Node_Info info = GetNodeInfo(node->type);

                bool32 dirty_connections = false;
                for (u32 connection_idx = 0;
                     connection_idx < info.input_count;
                     ++connection_idx)
                {
                    Node_ID con_id = node->connections[connection_idx];
                    if (con_id)
                    {
                        Node *con_node = GetNode(nodes, con_id);
                        Node_State *con_state = result.states + con_id - 1;

                        if (con_node->type == NodeType_None)
                            node->connections[connection_idx] = 0;

                        if (!con_state->resolved)
                        {
                            still_dirty = true;
                            dirty_connections = true;
                            break;
                        }

                        input_values[connection_idx] = con_state->value;
                    }
                    else
                    {
                        still_dirty = true;
                        dirty_connections = true;
                    }
                }

                if (!dirty_connections)
                {
                    if (info.method)
                        state->value = info.method(input_values, comparison, &result.out);
                    else
                        state->value = node->value;

                    state->resolved = true;
                    ++cleaned_count;
                }
            }
        }
    }

    result.all_nodes_resolved = !still_dirty;
    return result;
}
