#pragma once

#include "Control.h"
#include "Event.h"

//class MyControl;
namespace PluginGUI
{
	class CheckBoxControl : public Control
	{
		REGISTER_PLUGIN(CheckBoxControl, Control, ControlId::CheckBox)

		void Init(); // Инициализация для вызова из конструктора только!

	public:
		CheckBoxControl(const CRect& Border);
		~CheckBoxControl();

		Event<bool> ValueChanged;

		/// <summary>
		/// Получить/установить значение On
		/// </summary>
		bool GetOn() const
		{
			return m_On;
		}
		void SetOn(bool on)
		{
			if (m_On != on)
			{
				m_On = on;
				ValueChanged(on);
			}
		}

		/// <summary>
		/// Получить текст
		/// </summary>
		const std::wstring& GetText() const
		{
			return m_Text;
		}
		/// <summary>
		/// Установить текст
		/// </summary>
		void SetText(const std::wstring& text)
		{
			m_Text = text;
		}
		/// <summary>
		/// Текст контрола
		/// </summary>
		__declspec(property(get = GetText, put = SetText)) std::wstring Text;

		/// <summary>
		/// Получить/установить значение OnColor
		/// </summary>
		Gdiplus::Color GetOnColor() const
		{
			return m_OnColor;
		}
		void SetOnColor(Gdiplus::Color color)
		{
			m_OnColor = color;
		}

		/// <summary>
		/// Получить/установить значение OffColor
		/// </summary>
		Gdiplus::Color GetOffColor() const
		{
			return m_OffColor;
		}
		void SetOffColor(Gdiplus::Color color)
		{
			m_OffColor = color;
		}

		void Draw(bool hasFocus, bool drawSelected) override;
		void OnLButtonDown(UINT, CPoint p) override;
		//void OnStopAnimation() override;

		virtual UINT GetMinWidth() const override
		{
			return 31;
		}
		virtual UINT GetMinHeight() const override
		{
			return 32;
		}

	protected:
		//Gdiplus::Bitmap* m_OffScreenBitmap = nullptr;
		Gdiplus::Color m_OnColor;
		Gdiplus::Color m_OffColor;
		Gdiplus::Color m_TextColor;
		std::wstring m_Text;
		bool m_On = false;

		void DrawCustomCheckBox(Gdiplus::Graphics& g, int width, int height, const WCHAR* text, Gdiplus::Color highlightColor);

		std::unique_ptr<Animation> m_ClickAnimation = nullptr;
		float m_ClickAnimationProgress;
	};
}