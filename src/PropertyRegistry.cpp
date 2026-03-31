#include "PluginGUI\include\PropertyRegistry.h"

namespace PluginGUI
{
    // Единый статический объект
    PropertyRegistry& PropertyRegistry::Instance()
    {
        static PropertyRegistry s_instance;
        return s_instance;
    }

    std::optional<PropertyRegistry::PropertyMetadata> PropertyRegistry::Get(int id) 
    {
        InitializeIfNecessary();

        auto it = m_registry.find(id);
        if (it == m_registry.end())
        {
            static PropertyMetadata s_empty{};
            return s_empty;
        }
        return it->second;
    }

    void PropertyRegistry::Register(Id id, const PropertyMetadata& meta)
    {
        m_registry[id] = meta;
    }

    void PropertyRegistry::InitializeIfNecessary() 
    {
        if (m_initialized)
            return;

        RegisterControlProperties(*this);
        RegisterButtonControlProperties(*this);

        // сюда можно добавить RegisterSliderProperties, RegisterLabelProperties и т.п.

        m_initialized = true;
    }
}