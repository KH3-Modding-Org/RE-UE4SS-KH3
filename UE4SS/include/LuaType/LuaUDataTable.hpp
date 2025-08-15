#pragma once

#include <LuaType/LuaUObject.hpp>

namespace RC::Unreal
{
    class UDataTable;
}

namespace RC::LuaType
{
    struct UDataTableName
    {
        constexpr static const char* ToString()
        {
            return "UDataTable";
        }
    };
    class UDataTable : public RemoteObjectBase<Unreal::UDataTable, UDataTableName>
    {
    public:
        using Super = UObject;

    private:
        explicit UDataTable(Unreal::UDataTable* object);

    public:
        UDataTable() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::UDataTable*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
