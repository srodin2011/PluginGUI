#pragma once
#include <unordered_map>
#include <string>
#include <optional>
#include <utility>  // pair

#include "PluginGUI\include\Control.h"

//#include <windows.h>    // для PropertyId / Names, или заменить на int

namespace PluginGUI
{
    /// <summary>
    /// Ведение реестра метаданных - дополнительных характеристик свойств,
    /// уточняющих работу с ними.
    /// </summary>
    class PropertyRegistry
    {
    public:
        using TypeId = const std::type_info*;  // RTTI ключ
        using Key = std::pair<TypeId, Id>;

        // lazy‑инициализируемый единый экземпляр
        static PropertyRegistry& Instance();

        struct PropertyMetadata
        {
            std::wstring description;   // развёрнутая подсказка
            std::optional<std::pair<float, float>> range;  // min/max для числовых
            bool readonly = false;     // редактируемость
        };

        // Чтение по id (с автоматической инициализацией при первом вызове)
        std::optional<PropertyRegistry::PropertyMetadata> Get(const Control* pControl, int id);

        // Внутреннее добавление (для регистрационных функций)
        void Register(TypeId type, Id id, const PropertyMetadata& meta);

    private:
        struct KeyHash
        {
            size_t operator()(const Key& k) const noexcept
            {
                size_t h1 = std::hash<std::string_view>{}(k.first->name());
                size_t h2 = std::hash<Id>{}(k.second);
                return h1 ^ (h2 << 1);
            }
        };

        // Конструктор / деструктор — приватные, чтобы не создавать новые экземпляры
        PropertyRegistry() = default;
        ~PropertyRegistry() = default;

        // Заполняет реестр один раз при первом обращении
        void InitializeIfNecessary();

        mutable bool m_initialized = false;
        mutable std::unordered_map<Key, PropertyMetadata, KeyHash> m_registry;

        // Друзья, чтобы могли использовать Register извне
        friend void RegisterControlProperties(PropertyRegistry& reg);
        friend void RegisterButtonControlProperties(PropertyRegistry& reg);
        friend void RegisterButtonImageProperties(PropertyRegistry& reg);
    };
}