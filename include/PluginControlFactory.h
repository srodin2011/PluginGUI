#pragma once

#include <map>
#include "ControlId.h"

namespace PluginGUI
{
	/// <summary>
	/// Фабрика элементов управления. Регистрация новых элементов управления. Создание экземпляров элементов управления 
	/// </summary>
	class PluginControlFactory
	{
		static constexpr size_t MAX_CONTROLS = 256;
		using CreatorFn = Control * (*)();
		CreatorFn m_Creators[MAX_CONTROLS]{};
		//std::map<ControlId, Control* (*)()> m_Creators;

		// Приватный конструктор для синглтона
		PluginControlFactory() = default;
		// Конструктор копирования запрещен
		PluginControlFactory(const PluginControlFactory&) = delete;
		// Присваивание запрещено
		PluginControlFactory& operator=(const PluginControlFactory&) = delete;

	public:
		/// <summary>
		/// Добавить новый класс элемента управления в фабрику
		/// </summary>
		/// <typeparam name="C">Класс элемента управления</typeparam>
		//template <class C> void Add()
		//{
		//	m_Creators[C::pluginClassId] = &C::Create;
		//}
		template<ControlId ID, class C>
		void Add()
		{
			static_assert(static_cast<uint32_t>(ID) < MAX_CONTROLS, "ID overflow");
			m_Creators[static_cast<uint32_t>(ID)] = &C::Create;
		}

		/// <summary>
		/// Получить доступ к единственному экземпляру фабрики
		/// </summary>
		static PluginControlFactory& Instance()
		{
			static PluginControlFactory instance;
			return instance;
		}

		/// <summary>
		/// Создать новый экземпляр элемента управления по его уникальному идентификатору. 
		/// </summary>
		/// <param name="id">Идентификатор элемента управления</param>
		/// <returns>Указатель на экземпляр элемента управления</returns>
		//Control* Create(ControlId id) const
		//{
		//	auto it = m_Creators.find(id);
		//	return (it != m_Creators.end()) ? it->second() : nullptr;
		//}
		Control* Create(ControlId id) const
		{
			uint32_t idx = static_cast<uint32_t>(id);
			return idx < MAX_CONTROLS && m_Creators[idx] ? m_Creators[idx]() : nullptr;
		}
	};
} // PluginGUI