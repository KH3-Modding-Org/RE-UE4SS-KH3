#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaUDataTable.hpp>
#include <Unreal/Engine/UDataTable.hpp>

#include "LuaLibrary.hpp"

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

        table.add_pair("HasRowWithName",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           auto& lua_object = lua.get_userdata<UDataTable>();
                           Unreal::UDataTable* cpp_object = lua_object.get_remote_cpp_object();

                           StringType param_row_name{};

                           Unreal::TArray<Unreal::FName> row_names;
                           try
                           {
                               row_names = cpp_object->GetRowNames();
                           }
                           catch (const std::exception& e)
                           {
                               lua.throw_error(e.what());
                           }

                           // P1 (RowName), stirng
                           if (lua.is_string())
                           {
                               param_row_name = ensure_str(lua.get_string());
                           }
                           else
                           {
                                lua.throw_error("'UDataTable::HasRowWithName' - Could not load parameter for \"RowName\"");
                           }

                           int index = 1;
                           for (int i = 0; i < row_names.Num(); ++i)
                           {
                               Unreal::FName row_name = row_names[i];
                               auto fstring_row_name = row_name.ToString();

                               if (fstring_row_name == to_wstring(param_row_name))
                               {
                                   // Matched!
                                   lua.set_bool(true);
                                   return 1;
                               }

                               index += 1;
                           }

                           // Did not find row with given row name
                           lua.set_bool(false);
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