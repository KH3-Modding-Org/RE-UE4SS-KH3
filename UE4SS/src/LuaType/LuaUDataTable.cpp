#include <LuaType/LuaUDataTable.hpp>
#include <Unreal/Engine/UDataTable.hpp>
#include <Helpers/String.hpp>

#include "LuaLibrary.hpp"
#include "UnrealDef.hpp"

namespace RC::LuaType
{
    std::string wstring_to_string(const std::wstring& wstr)
    {
        using convert_type = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_type, wchar_t> converter;

        try
        {
            return converter.to_bytes(wstr);
        }
        catch (const std::exception& e)
        {
            printf("Error converting wstring to string: %s\n", e.what());
            return "";
        }
    }

    UDataTable::UDataTable(Unreal::UDataTable* object) : RemoteObjectBase<Unreal::UDataTable, UDataTableName>(object)
    {
    }

    auto UDataTable::construct(const LuaMadeSimple::Lua& lua, Unreal::UDataTable* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        add_to_global_unreal_objects_map(unreal_object);

        LuaType::UDataTable lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaType::UObject::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::UDataTable>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto UDataTable::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = UObject::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto UDataTable::setup_metamethods(BaseObject&) -> void
    {
        // UDataTable has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto UDataTable::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        table.add_pair("IsValid",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           auto& lua_object = lua.get_userdata<UDataTable>();

                           lua.set_bool(lua_object.get_remote_cpp_object());

                           return 1;
                       });

        table.add_pair("GetRowNames",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           auto& lua_object = lua.get_userdata<UDataTable>();
                           Unreal::UDataTable* cpp_object = lua_object.get_remote_cpp_object();

                           Unreal::TArray<Unreal::FName> row_names;
                           try
                           {
                               row_names = cpp_object->GetRowNames();
                           }
                           catch (const std::exception& e)
                           {
                               lua.throw_error(e.what());
                           }

                           // Map array to LUA table
                           auto return_array_table = lua.prepare_new_table();
                           for (auto [index, name] : row_names | std::views::enumerate)
                           {
                               return_array_table.add_key(index);
                               return_array_table.add_value(to_utf8_string(name.ToString()).c_str());
                               return_array_table.fuse_pair();
                           }
                           return_array_table.make_local();
                           return 1;
                       }
                );

        table.add_pair("FindRow",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           auto& lua_object = lua.get_userdata<UDataTable>();
                           Unreal::UDataTable* cpp_object = lua_object.get_remote_cpp_object();

                           StringType param_row_name{};
                           StringType param_context_string{};

                           // P1 (RowName), string
                           if (lua.is_string())
                           {
                               param_row_name = ensure_str(lua.get_string());
                           }
                           else
                           {
                               lua.throw_error("'UDataTable::HasRowWithName' - Could not load parameter for \"RowName\"");
                           }

                           // P2 (ContextString), string
                           // Used for error handling
                           if (lua.is_string())
                           {
                               param_context_string = ensure_str(lua.get_string());
                           }
                           else
                           {
                               lua.throw_error("'UDataTable::HasRowWithName' - Could not load parameter for \"ContextString\"");
                           }

                           try
                           {
                               Unreal::FName fname_row = FName(param_row_name, Unreal::FNAME_Find);
                               // TODO: Can't use the StringType due to a stack overflow identified by the compiler.  
                               auto parsed_context_string = FromCharTypePtr<TCHAR>(param_context_string.c_str());
                               // TODO: The type passed to FindRow is not super clear... The template only denotes generic type T
                               // This is giving a lot of errors as a result...
                               auto rows = cpp_object->FindRow<UScriptStruct*>(
                                       fname_row,
                                       parsed_context_string);
                           }
                           catch (const std::exception& e)
                           {
                               lua.throw_error(e.what());
                           }
                           return 1;
                       }
                );

        if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
        {
            table.add_pair("type",
                           [](const LuaMadeSimple::Lua& lua) -> int {
                               lua.set_string(ClassName::ToString());
                               return 1;
                           });

            // If this is the final object then we also want to finalize creating the table
            // If not then it's the responsibility of the overriding object to call 'make_global()'
            // table.make_global(ClassName::ToString());
        }
    }

} // namespace RC::LuaType