#pragma once


#include "paprika_types.h"


enum Json_Type
{
    JsonType_Null = 0,
    JsonType_String,
    JsonType_Num,
    JsonType_Bool,
    JsonType_Array,
    JsonType_Object,

    JsonType_Error,

    JsonTypeCount,
};

struct Json_Object
{
    u32 count;

    union
    {
        struct Json_Object_Entry *root;
        Arena_Offset root_offset;
    };
};

struct Json_Array
{
    u32 count;

    union
    {
        struct Json_Array_Entry *root;
        Arena_Offset root_offset;
    };
};

enum Json_Num_Type
{
    JsonNumType_Int,
    JsonNumType_Float,
};

struct Json_Num
{
    Json_Num_Type type;
    union {
        i64 i;
        f64 f;
    };
};

struct Json_Value
{
    Json_Type type;
    union
    {
        char *str;
        Json_Num num;
        Json_Array arr;
        Json_Object obj;

        Arena_Offset offset;
    };
};

struct Json_Array_Entry
{
    Json_Value value;

    union
    {
        Json_Array_Entry *next;
        Arena_Offset next_offset;
    };
};

struct Json_Object_Entry
{
    union
    {
        char *name;
        Arena_Offset name_offset;
    };

    union
    {
        Json_Object_Entry *next;
        Arena_Offset next_offset;
    };

    Json_Value value;
};

/*
    Json Reading
*/

struct Json_Reader
{
    Growable_Memory_Arena arena;

    mem_t error_offset;

    char *read;
    char *read_start;
};

internal inline char *
JsonGetStr(Json_Reader *json, mem_t offset)
{
    return GrowableArenaGetString(&json->arena, offset);
}

internal inline Json_Array_Entry *
JsonGetArrEntry(Json_Reader *json, mem_t offset)
{
    return GrowableArenaGetStruct(&json->arena, Json_Array_Entry, offset);
}

internal inline Json_Array_Entry *
JsonGetNextArrEntry(Json_Reader *json, Json_Array_Entry *entry)
{
    return JsonGetArrEntry(json, entry->next_offset);
}

internal inline Json_Object_Entry *
JsonGetObjEntry(Json_Reader *json, mem_t offset)
{
    return GrowableArenaGetStruct(&json->arena, Json_Object_Entry, offset);
}

internal inline Json_Object_Entry *
JsonGetNextObjEntry(Json_Reader *json, Json_Object_Entry *entry)
{
    return JsonGetObjEntry(json, entry->next_offset);
}

internal mem_t
JsonParseString(Json_Reader *json)
{
    mem_t result = 0;

    if (*json->read == '"')
    {
        ++json->read;
    }
    else
    {
        json->error_offset = json->read - json->read_start;
        return 0;
    }

    char buf[2048];
    char *write = buf;
    char *end = buf + ArrayCount(buf);
    bool32 escape_next = false;
    bool32 found_end = false;
    while (*json->read)
    {
        if (*json->read == '\\')
        {
            escape_next = true;
        }
        else if (*json->read == '"' && !escape_next)
        {
            *write = '\0';
            ++json->read;
            result = GrowableArenaAllocString(&json->arena, buf);
            found_end = true;
            break;
        }
        else
        {
            if (write >= end)
            {
                json->error_offset = json->read - json->read_start;
                return 0;
            }

            *write++ = *json->read;
            escape_next = false;
        }

        ++json->read;
    }

    if (!found_end)
    {
        json->error_offset = json->read - json->read_start;
        return 0;
    }

    return result;
}

internal void
JsonParseWhitespace(Json_Reader *json)
{
    while (*json->read == ' ' || *json->read == '\n' || *json->read == '\t' ||
           *json->read == '\r')
        ++json->read;
}

internal i64
JsonParseBool(Json_Reader *json)
{
    i64 result = 0;
    if (StringCmpLen(json->read, "true", 4))
    {
        json->read += 4;
        result = 1;
    }
    else if (StringCmpLen(json->read, "false", 5))
    {
        json->read += 5;
        result = 0;
    }
    else
    {
        json->error_offset = json->read - json->read_start;
    }
    return result;
}

internal void
JsonParseNull(Json_Reader *json)
{
    if (StringCmpLen(json->read, "null", 4))
        json->read += 4;
    else
        json->error_offset = json->read - json->read_start;
}

internal Json_Num
JsonParseNum(Json_Reader *json)
{
    char *start = json->read;

    bool32 is_float = false;

    if (*json->read == '-')
        ++json->read;

    while (*json->read)
    {
        if (*json->read == '.')
        {
            if ('0' > *(json->read+1) || *(json->read+1) > '9' || is_float)
            {
                json->error_offset = json->read + 1 - json->read_start;
                return {};
            }    

            is_float = true;
            ++json->read;
        }
        else if ('0' > *json->read || *json->read > '9')
        {
            break;
        }

        ++json->read;
    }

    Json_Num result;
    if (is_float)
    {
        result.type = JsonNumType_Float;
        result.f = StringToF64(start, json->read);
    }
    else
    {
        result.type = JsonNumType_Int;
        result.i = StringToI64(start, json->read);
    }
    return result;
}

internal Json_Value JsonParseValue(Json_Reader *json);

internal Json_Array
JsonParseArray(Json_Reader *json)
{
    Json_Array result = {};

    if (*json->read != '[')
    {
        json->error_offset = json->read - json->read_start;
        return result;
    }
    ++json->read;

    mem_t last_entry = 0;

    bool32 found_end = false;

    while (*json->read)
    {
        JsonParseWhitespace(json);
        if (*json->read == ']')
        {
            found_end = true;
            ++json->read;
            break;
        }
        mem_t entry_offset = GrowableArenaAllocStructAndZero(&json->arena, Json_Array_Entry);

        Json_Value entry_value = JsonParseValue(json);
        Json_Array_Entry *entry = JsonGetArrEntry(json, entry_offset);
        entry->value = entry_value;
        
        if (entry->value.type == JsonType_Error)
            break;

        if (last_entry)
            JsonGetArrEntry(json, last_entry)->next_offset = entry_offset;
        else
            result.root_offset = entry_offset;

        ++result.count;
        last_entry = entry_offset;

        JsonParseWhitespace(json);
        if (*json->read == ']')
        {
            found_end = true;
            ++json->read;
            break;
        }
        else if (*json->read == ',')
        {
            ++json->read;
        }
        else
        {
            json->error_offset = json->read - json->read_start;
            break;
        }
    }

    if (!found_end && !json->error_offset)
        json->error_offset = json->read - json->read_start;

    return result;
}

internal Json_Object
JsonParseObject(Json_Reader *json)
{
    Json_Object result = {};

    if (*json->read != '{')
    {
        json->error_offset = json->read - json->read_start;
        return result;
    }
    ++json->read;

    mem_t last_entry = 0;

    bool32 found_end = false;

    while (*json->read)
    {
        JsonParseWhitespace(json);
        if (*json->read == '}')
        {
            found_end = true;
            ++json->read;
            break;
        }

        mem_t entry_offset = GrowableArenaAllocStructAndZero(&json->arena, Json_Object_Entry);
        mem_t name_offset = JsonParseString(json);
        JsonGetObjEntry(json, entry_offset)->name_offset = name_offset;
        JsonParseWhitespace(json);

        if (*json->read != ':')
        {
            json->error_offset = json->read - json->read_start;
            break;
        }

        ++json->read;
        JsonParseWhitespace(json);

        Json_Value entry_value = JsonParseValue(json);
        Json_Object_Entry *entry = JsonGetObjEntry(json, entry_offset);
        entry->value = entry_value;

        if (entry->value.type == JsonType_Error)
            break;

        if (last_entry)
            JsonGetObjEntry(json, last_entry)->next_offset = entry_offset;
        else
            result.root_offset = entry_offset;

        ++result.count;
        last_entry = entry_offset;

        JsonParseWhitespace(json);
        if (*json->read == '}')
        {
            found_end = true;
            ++json->read;
            break;
        }
        else if (*json->read == ',')
        {
            ++json->read;
        }
        else
        {
            json->error_offset = json->read - json->read_start;
            break;
        }
    }
    
    if (!found_end && !json->error_offset)
        json->error_offset = json->read - json->read_start;

    return result;
}

internal Json_Value
JsonParseValue(Json_Reader *json)
{
    Json_Value result;
    switch (*json->read)
    {
        case '"': {
            result.type = JsonType_String;
            result.offset = JsonParseString(json);
        } break;

        case '{': {
            result.type = JsonType_Object;
            result.obj = JsonParseObject(json);
        } break;

        case '[': {
            result.type = JsonType_Array;
            result.arr = JsonParseArray(json);
        } break;

        case 't':
        case 'f': {
            result.type = JsonType_Bool;
            result.num.i = JsonParseBool(json);
        } break;

        case 'n': {
            result.type = JsonType_Null;
            result.num.i = 0;
            JsonParseNull(json);
        } break;

        default: {
            result.type = JsonType_Num;
            result.num = JsonParseNum(json);
        } break;
    }

    if (json->error_offset)
    {
        Panic("Json parser error");
        result.type = JsonType_Error;
    }

    return result;
}

internal inline void
JsonReaderInit(Json_Reader *json)
{
    GrowableArenaReset(&json->arena);
    GrowableArenaAlloc(&json->arena, 1, true);
}

internal void JsonResolveObj(Json_Reader *json, Json_Object *obj);

internal void
JsonResolveArr(Json_Reader *json, Json_Array *arr)
{
    arr->root = JsonGetArrEntry(json, arr->root_offset);
    Json_Array_Entry *entry = arr->root;
    while (entry)
    {
        if (entry->value.type == JsonType_String)
            entry->value.str = JsonGetStr(json, entry->value.offset);
        else if (entry->value.type == JsonType_Array)
            JsonResolveArr(json, &entry->value.arr);
        else if (entry->value.type == JsonType_Object)
            JsonResolveObj(json, &entry->value.obj);
        
        entry->next = JsonGetNextArrEntry(json, entry);
        entry = entry->next;
    }
}

internal void
JsonResolveObj(Json_Reader *json, Json_Object *obj)
{
    obj->root = JsonGetObjEntry(json, obj->root_offset);
    Json_Object_Entry *entry = obj->root;
    while (entry)
    {
        entry->name = JsonGetStr(json, entry->name_offset);
        if (entry->value.type == JsonType_String)
            entry->value.str = JsonGetStr(json, entry->value.offset);
        else if (entry->value.type == JsonType_Array)
            JsonResolveArr(json, &entry->value.arr);
        else if (entry->value.type == JsonType_Object)
            JsonResolveObj(json, &entry->value.obj);
        
        entry->next = JsonGetNextObjEntry(json, entry);
        entry = entry->next;
    }
}

internal inline Json_Value
JsonParse(Json_Reader *json, char *s)
{
    JsonReaderInit(json);
    json->read_start = s;
    json->read = s;
    Json_Value result = JsonParseValue(json);

    if (result.type == JsonType_String)
        result.str = JsonGetStr(json, result.offset);
    else if (result.type == JsonType_Array)
        JsonResolveArr(json, &result.arr);
    else if (result.type == JsonType_Object)
        JsonResolveObj(json, &result.obj);

    return result;
}


/*
    Json Writing
*/


struct Json_Writer
{
    Memory_Arena arena;
    i32 indent_depth;
};

internal inline void
JsonWrite(Json_Writer *writer, char *src, bool32 indent = false, bool32 newline = false)
{
    if (newline)
        ArenaPushString(&writer->arena, "\n");

    for (i32 i = 0; indent && i < writer->indent_depth; ++i)
        ArenaPushString(&writer->arena, "    ");

    ArenaPushString(&writer->arena, src);
}

internal inline void
JsonDumpNull(Json_Writer *writer)
{
    JsonWrite(writer, "null");
}

internal inline void
JsonDumpString(Json_Writer *writer, char *str)
{
    JsonWrite(writer, "\"");
    JsonWrite(writer, str);
    JsonWrite(writer, "\"");
}

internal inline void
JsonDumpNum(Json_Writer *writer, Json_Num num)
{
    char buf[32];
    if (num.type == JsonNumType_Int)
        sprintf(buf, "%lld", num.i);
    else
        sprintf(buf, "%f", num.f);
    JsonWrite(writer, buf);
}

internal inline void
JsonDumpBool(Json_Writer *writer, i64 val)
{
    if (val)
        JsonWrite(writer, "true");
    else
        JsonWrite(writer, "false");
}

internal void JsonDumpValue(Json_Writer *writer, Json_Value value);

internal inline void
JsonDumpArray(Json_Writer *writer, Json_Array array)
{
    JsonWrite(writer, "[\n", true);

    ++writer->indent_depth;
    Json_Array_Entry *entry = array.root;
    while (entry)
    {
        JsonDumpValue(writer, entry->value);

        entry = entry->next;

        if (entry)
            JsonWrite(writer, ",");
        JsonWrite(writer, "\n");
    }
    --writer->indent_depth;

    JsonWrite(writer, "]", true);
}

internal void
JsonDumpObject(Json_Writer *writer, Json_Object obj)
{
    JsonWrite(writer, "{\n", true);

    ++writer->indent_depth;
    Json_Object_Entry *entry = obj.root;
    while (entry)
    {
        JsonWrite(writer, "\"", true);
        JsonWrite(writer, entry->name);
        JsonWrite(writer, "\": ");

        JsonDumpValue(writer, entry->value);

        entry = entry->next;

        if (entry)
            JsonWrite(writer, ",");

        JsonWrite(writer, "\n");
    }
    --writer->indent_depth;

    JsonWrite(writer, "}", true);
}

internal void
JsonDumpValue(Json_Writer *writer, Json_Value value)
{
    switch (value.type)
    {
        case JsonType_Null:   JsonDumpNull(writer);              break;
        case JsonType_String: JsonDumpString(writer, value.str); break;
        case JsonType_Num:    JsonDumpNum(writer, value.num);    break;
        case JsonType_Bool:   JsonDumpBool(writer, value.num.i); break;

        case JsonType_Array:
        {
            JsonWrite(writer, "\n");
            JsonDumpArray(writer, value.arr);
        } break;

        case JsonType_Object:
        {
            JsonWrite(writer, "\n");
            JsonDumpObject(writer, value.obj);
        } break;

        DefaultPanic;
    }
}

internal void
JsonDump(Json_Writer *writer, Json_Object obj)
{
    ArenaReset(&writer->arena);
    writer->indent_depth = 0;
    JsonDumpObject(writer, obj);
    JsonWrite(writer, "\n");
}
