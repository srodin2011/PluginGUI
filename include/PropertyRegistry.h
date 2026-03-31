#pragma once
#include <unordered_map>
#include <string>
#include <optional>
#include <utility>  // pair

#include "PluginGUI\include\Control.h"

//#include <windows.h>    // для PropertyId / Names, или заменить на int

namespace PluginGUI
{
    class PropertyRegistry
    {
    public:
        // lazy‑инициализируемый единый экземпляр
        static PropertyRegistry& Instance();

        struct PropertyMetadata
        {
            std::wstring description;   // развёрнутая подсказка
            std::optional<std::pair<float, float>> range;  // min/max для числовых
            bool readonly = false;     // редактируемость
        };

        // Чтение по id (с автоматической инициализацией при первом вызове)
        std::optional<PropertyMetadata> Get(int id);

        // Внутреннее добавление (для регистрационных функций)
        void Register(Id id, const PropertyMetadata& meta);

    private:
        // Конструктор / деструктор — приватные, чтобы не создавать новые экземпляры
        PropertyRegistry() = default;
        ~PropertyRegistry() = default;

        // Заполняет реестр один раз при первом обращении
        void InitializeIfNecessary();

        mutable bool m_initialized = false;
        mutable std::unordered_map<int, PropertyMetadata> m_registry;

        // Друзья, чтобы могли использовать Register извне
        friend void RegisterControlProperties(PropertyRegistry& reg);
        friend void RegisterButtonControlProperties(PropertyRegistry& reg);
    };
}