#pragma once

#include "Control.h"
#include "Utils.h"

namespace PluginGUI
{
	class LableControl : public Control
	{
		REGISTER_PLUGIN(LableControl, Control, ControlId::Lable)

		void Init(); // Инициализация для вызова из конструктора только!

	public:
		enum class TextAlignmentStyle
		{
			Near,
			Center,
			Far
		};

		LableControl(const CRect& Border, const std::wstring& text);
		~LableControl();

		void OnMouseMove(UINT nFlags, CPoint p) override;
		void OnMouseLeave() override;

		void Draw(bool hasFocus, bool drawSelected) override;

		virtual UINT GetMinWidth() const override
		{
			return 31;
		}
		virtual UINT GetMinHeight() const override
		{
			return 32;
		}

		DEFINE_PROPERTY(std::wstring, Text, L"")
		DEFINE_PROPERTY(TextAlignmentStyle, TextAlignmentH, TextAlignmentStyle::Near)
		DEFINE_PROPERTY(TextAlignmentStyle, TextAlignmentV, TextAlignmentStyle::Near)
		DEFINE_PROPERTY(std::wstring, FontName, L"Segoe UI")
		DEFINE_PROPERTY(int, FontSize, 10)
		DEFINE_PROPERTY(Gdiplus::Color, TextColor, Gdiplus::Color(255, 255, 255, 255))
		DEFINE_PROPERTY(Gdiplus::Color, BackColor, Gdiplus::Color(255, 23, 23, 23))
		DEFINE_PROPERTY(Gdiplus::Color, HoverColor, Gdiplus::Color(255, 255, 255, 255))
		DEFINE_PROPERTY(Gdiplus::Color, HoverBackColor, Gdiplus::Color(255, 23, 23, 23))
		DEFINE_PROPERTY(CornerMask, CornerStyle, CornerMask::All)
		DEFINE_PROPERTY(int, СornerRoundRadius, 5)

		//bool isDirty() const override
		//{
		//	return true;
		//}

	protected:
		bool m_bHover = false;

		void DrawLable(Gdiplus::Graphics& g, int width, int height, const WCHAR* text);
	};
}