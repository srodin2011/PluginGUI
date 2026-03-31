#pragma once
#include "ScalableControl.h"
#define _USE_MATH_DEFINES
#include "math.h"

namespace PluginGUI
{
	class SliderControl : public ScalableControl
	{
		REGISTER_PLUGIN(SliderControl, ScalableControl, ControlId::Slider)

		void Init(); // »нициализаци€ дл€ вызова из конструктора только!

	public:
		SliderControl(const CRect& Border);

		int GetScaleSteps() const
		{
			return ScaleBigSteps * ScaleSmallSteps;
		}

	protected:
		void Draw(bool hasFocus, bool drawSelected) override;
		void DrawControlHandle(Gdiplus::Graphics& g, float x, float y, float width, float height, float animationProgress);
		void DrawSliderScale(Gdiplus::Graphics& g, float x, float y, float width, float height, float handleY, float animationProgress);

		float GetHandleWidth() const
		{ 
			return (float)Border.Width() * 28.0f / 38.0f;
		}
		float GetHandleHeight() const
		{
			return GetHandleWidth() * 2;
		}
		float GetHandleX() const
		{
			return Border.Width() / 2.0f;
		}
		float GetHandleY() const
		{
			float scaleH = Border.Height() - GetHandleHeight(); // ¬ысота шкалы
			return GetHandleHeight() / 2.0f + scaleH * (1 - (float)(m_Value / (m_MaxVal - m_MinVal)));
		}

		/// <summary>
		/// Ќужно ли восстанавливать позицию мышки после изменени€ значени€ перемещениеем мышки
		/// </summary>
		/// <returns></returns>
		virtual void RestoreMousePos();
	};
}
