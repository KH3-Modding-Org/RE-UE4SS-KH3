#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaUDataTable.hpp>
#include <LuaType/LuaTArray.hpp>
#include <Unreal/Engine/UDataTable.hpp>

namespace RC::LuaType
{
    UDataTable::UDataTable(Unreal::UDataTable* object)
        : RemoteObjectBase<Unreal::UDataTable, UDataTableName>(object)
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

        table.add_pair("GetRowNames", [](const LuaMadeSimple::Lua& lua) -> int {
            // TODO: May not have overloads
            std::string error_overload_not_found{R"(
No overload found for function 'UDataTable.GetRowNames'.
            )"};

            auto& lua_object = lua.get_userdata<UDataTable>();
            auto row_names = lua_object.get_remote_cpp_object()->GetRowNames();
            //LuaType::TArray::construct(row_names);
            return 1;
        });
    }

} // namespace RC::LuaType