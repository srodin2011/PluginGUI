#include "PluginGUI\include\PropertyRegistry.h"

namespace PluginGUI
{
    /// <summary>
    /// Создание реестра при первом обращении
    /// </summary>
    /// <returns></returns>
    PropertyRegistry& PropertyRegistry::Instance()
    {
        static PropertyRegistry s_instance;
        return s_instance;
    }

    /// <summary>
    /// Получить метаданные свойства с учетом иерархии
    /// </summary>
    /// <param name="pControl"></param>
    /// <param name="id"></param>
    /// <returns></returns>
    std::optional<PropertyRegistry::PropertyMetadata> PropertyRegistry::Get(const Control* pControl, int id)
    {
        if (!pControl) return std::nullopt;
        InitializeIfNecessary();

        // Находим владелеца свойства через иерархию
        const std::type_info& ownerType = pControl->GetPropertyOwnerType(id);

        Key key{ &ownerType, id };
        auto it = m_registry.find(key);
        return it != m_registry.end() ? std::make_optional(it->second) : std::nullopt;
    }

    /// <summary>
    /// Внести информацию в реестр
    /// </summary>
    /// <param name="type"></param>
    /// <param name="id"></param>
    /// <param name="meta"></param>
    void PropertyRegistry::Register(TypeId type, Id id, const PropertyMetadata& meta)
    {
        auto key = std::make_pair(type, id);
        m_registry[key] = meta;
    }

    /// <summary>
    /// Инициализация метаданных для всех контролах
    /// </summary>
    void PropertyRegistry::InitializeIfNecessary() 
    {
        if (m_initialized)
            return;

        RegisterControlProperties(*this);
        RegisterButtonControlProperties(*this);
        RegisterButtonImageProperties(*this);

        m_initialized = true;
    }
}