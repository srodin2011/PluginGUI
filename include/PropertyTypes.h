#pragma once
#include <cassert>
#include <string>
#include "GdiplusExtention.h"

#ifdef _DEBUG
#define PROPERTY_TRACE(...) \
        TRACE(__VA_ARGS__)
#else
#define PROPERTY_TRACE(...)
#endif

namespace PluginGUI
{
    /// <summary>
    /// Ведение информации о свойствах контролов. Возможность поиска свойства по id.
    /// Возможность поиска id свойства по имени. Обеспечивает возможность редактирования
    /// свойств через проперти грид.
    /// </summary>

    enum class TypeId
    {
        Bool, Color, WString, Int, Float, Frame
    };

    //class Variant;
    class Frame;

    using Id = int;

    struct PropertyInfo
    {
        Id m_propertyId;
        TypeId m_type;
        std::string m_uiName;

        PropertyInfo() = default;
        PropertyInfo(Id id, TypeId t, const std::string& name)
            : m_propertyId(id), m_type(t), m_uiName(name)
        {
        }
    };

    // Трейт для отображения C++‑типа → TypeId
    template<typename T> struct ToTypeId { static constexpr TypeId value = TypeId::Int; };
    template<> struct ToTypeId<bool> { static constexpr TypeId value = TypeId::Bool; };
    template<> struct ToTypeId<int> { static constexpr TypeId value = TypeId::Int; };
    template<> struct ToTypeId<float> { static constexpr TypeId value = TypeId::Float; };
    template<> struct ToTypeId<std::wstring> { static constexpr TypeId value = TypeId::WString; };
    template<> struct ToTypeId<Gdiplus::Color> { static constexpr TypeId value = TypeId::Color; };
    template<> struct ToTypeId<Frame> { static constexpr TypeId value = TypeId::Frame; };

    // Реестр свойств, привязанных к классу контрола
    template<typename ControlType>
    struct PropertyTable
    {
        static constexpr size_t MAX_PROPERTIES = 256;
        static PropertyInfo* m_entries; // Указатель на массив свойств
        static size_t m_count; // Количество свойств

        /// <summary>
        /// Получить доступ к совйствам. Инициализация при первом обращении
        /// </summary>
        /// <returns></returns>
        static PropertyInfo* entries()
        {
            static PropertyInfo storage[MAX_PROPERTIES] = {};
            return storage;
        }

        /// <summary>
        /// Счетчик свойств
        /// </summary>
        /// <returns></returns>
        static size_t& count()
        {
            static size_t storage = 0;
            return storage;
        }

        /// <summary>
        /// Добавить информацию о новом свойстве
        /// </summary>
        /// <param name="id"></param>
        /// <param name="type"></param>
        /// <param name="uiName"></param>
        static void Add(Id id, TypeId type, const char* uiName)
        {
            PropertyInfo* e = entries();
            e[count()++] = PropertyInfo(id, type, uiName);
        }

        /// <summary>
        /// Найти свойство по id
        /// </summary>
        /// <param name="id"></param>
        /// <returns></returns>
        static const PropertyInfo* Find(Id id)
        {
            PropertyInfo* e = entries();
            for (size_t i = 0; i < count(); ++i)
                if (e[i].m_propertyId == id)
                    return e + i;
            return nullptr;
        }

        /// <summary>
        /// Найти свойство по имени
        /// </summary>
        /// <param name="uiName"></param>
        /// <returns></returns>
        static Id FindIdByName(const std::string& uiName)
        {
            PropertyInfo* e = entries();
            for (size_t i = 0; i < count(); ++i)
                if (e[i].m_uiName == uiName)
                    return e[i].m_propertyId;
            return -1;  // или 0, или optional/Result
        }
    };

    // глобальная функция: найти свойство по Id 
    template<typename ControlType>
    const PropertyInfo* FindProperty(Id id)
    {
        return PropertyTable<ControlType>::Find(id);
    }

    template<typename ControlType>
    Id FindPropertyIdByName(const std::string& uiName)
    {
        return PropertyTable<ControlType>::FindIdByName(uiName);
    }
}