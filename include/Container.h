#pragma once

#include "framework.h" // Стандартные компоненты MFC
#include <vector>
#include <memory>

namespace PluginGUI
{
	class Control;

	class IContainer
	{
	public:
		using Id = int;

		virtual const std::vector<std::shared_ptr<Control>>& GetChildren() const = 0;

		virtual void Add(Control* child) = 0;
		virtual void Add(std::shared_ptr<Control> childSP) = 0;
		virtual void Remove(Control* child) = 0;
		virtual size_t GetCount() const = 0;
		virtual size_t GetSelectedCount() const = 0;
		virtual void OnChildPropertyChanged(Control* pChild, Id id) = 0;

		virtual void Invalidate(const CRect& rect) = 0;

		// Локальные координаты
		virtual void GetCursorPos(LPPOINT p) = 0;
		virtual void SetCursorPos(POINT) = 0;

		// Преобразование глобальные координаты <=> локальные координаты
		virtual void GlobalToLocal(CPoint& p) = 0;
		virtual void LocalToGlobal(CPoint& p) = 0;

		// Преобразование координаты родителя <=> локальные координаты
		virtual void ParentToLocal(CPoint& p) = 0;
		virtual void LocalToParent(CPoint& p) = 0;

		virtual void ShowTooltipForControl(Control* pControl) = 0;
		virtual void UpdateTooltipForCurrentControl(Control* pControl) = 0;
		virtual void HideCurrentTooltip() = 0;
	};
}