#pragma once

#include "Control.h"
//#include "Descriptor.h"

//#include "PluginControlFactory.h"

namespace PluginGUI
{
	class ButtonControl : public Control
	{
		using Base = Control;
		REGISTER_PLUGIN(ButtonControl, Control, ControlId::Button)

		void Init(); // »нициализаци€ дл€ вызова из конструктора только!

	public:
		enum class PropertyName
		{
			pnOn = Base::PropertyName::pnLastName,
			pnOnColor,
			pnOffColor,
			pnTextColor,
			pnText,
			pnLastName
		};

		ButtonControl(const CRect& Border);

		DEFINE_PROPERTY_NEW(bool, On, false, ButtonControl, PropertyName::pnOn)
		DEFINE_PROPERTY_NEW(Gdiplus::Color, OnColor, Gdiplus::Color(255, 255, 255, 255), ButtonControl, PropertyName::pnOnColor)
		DEFINE_PROPERTY_NEW(Gdiplus::Color, OffColor, Gdiplus::Color(255, 0, 0, 0), ButtonControl, PropertyName::pnOffColor)
		DEFINE_PROPERTY_NEW(Gdiplus::Color, TextColor, Gdiplus::Color(255, 255, 255, 255), ButtonControl, PropertyName::pnTextColor)
		DEFINE_PROPERTY_NEW(std::wstring, Text, L"", ButtonControl, PropertyName::pnText)

		void Draw(bool hasFocus, bool drawSelected) override;
		void OnLButtonDown(UINT, CPoint p) override;

		virtual UINT GetMinWidth() const override
		{
			return 31;
		}
		virtual UINT GetMinHeight() const override
		{
			return 32;
		}

		//void SetText(const std::string& newText);
		//const std::string& GetText() const { return m_Text; }

		size_t GetPropertyCount() const override
		{
			// pnLastName Ч уже общий индекс по всей иерархии
			return static_cast<size_t>(PropertyName::pnLastName);
		}

		IMPLEMENT_OVERRIDE_FIND_PROPERTY(ButtonControl)
		//DECLARE_DESCRIPTOR()
	protected:
		virtual Variant doGetPropertyValue(Id id) const override;
		virtual bool doSetPropertyValue(Id id, const Variant& value) override;
	};
}