#pragma once
#include "ScalableControl.h"
#define _USE_MATH_DEFINES
#include "math.h"
#include <gdiplus.h>

namespace PluginGUI
{
	class KnobControl : public ScalableControl
	{
		REGISTER_PLUGIN(KnobControl, ScalableControl, ControlId::Knob)

		void Init(); // Инициализация для вызова из конструктора только!

	public:
		KnobControl(const CRect& Border);
		KnobControl(CPoint startPoint, int size);

		/// <summary>
		/// Изменить границы элемента управления
		/// </summary>
		void SetBorder(const Frame& Border) override;
		/// <summary>
		/// Получить размер ручки
		/// </summary>
		/// <returns></returns>
		int GetSize() const
		{
			return m_Size;
		}

		DEFINE_PROPERTY(Gdiplus::Color, CapColor, Gdiplus::Color(255, 255, 0, 0))
		DEFINE_PROPERTY(int, NotchCount, 0);
		DEFINE_PROPERTY(Gdiplus::Color, RingColor, Gdiplus::Color(255, 190, 190, 190))
		DEFINE_PROPERTY(Gdiplus::Color, IndicatorColor, Gdiplus::Color(255, 255, 168, 48))
		DEFINE_PROPERTY(float, CapPercent, 65)
		DEFINE_PROPERTY(float, GearPercent, 15)
		DEFINE_PROPERTY(float, CapReductionPercent, 5)
		DEFINE_PROPERTY(bool, Scale, true)
		DEFINE_PROPERTY(bool, IndicatorOnGear, true)

		//bool isDirty() const override
		//{
		//	return true;
		//}

	protected:
		CPoint m_StartPoint; // Левая верхняя точка ручки
		int m_Size = 0; // Размер ручки
		const float m_Alpha = (float)M_PI * 30.f / 180.f; // Угол отклонения начала шкалы от вертикали
		const float m_MinAngle = (float)M_PI_2 * 3 - m_Alpha; // Минимальный угол шкалы
		const float m_MaxAngle = (float)(- M_PI_2) + m_Alpha; // Максимальный угол шкалы
		//Gdiplus::Color m_IndicatorColor; // Цвет индикатора
		//Gdiplus::Color m_RingColor = Gdiplus::Color(255, 190, 190, 190); // Цвет кольца
		//int m_NotchCount = 7; // Количество вырезов на ручке в режиме фокуса

		void Draw(bool hasFocus, bool drawSelected) override;

		void DrawScale(Gdiplus::Graphics& g, float x, float y, float radius, float tickSize, float tickThickness, float angle);
		void DrawMetallicRingWithCircularNotches(
			Gdiplus::Graphics& graphics,
			float x,
			float y,
			float radius,
			float ringThickness,
			float rickThickness,
			int notchCount = 6,
			float rotationAngleDegrees = 0.0f,
			float animationProgress = 0.0f);

		void DrawPartialRing(Gdiplus::Graphics& g, float x, float y, float radius, float thickness, Gdiplus::Color fillColor);
		void DrawRingSector(Gdiplus::Graphics& g, float x, float y, float R_outer, float R_inner,
			float startAngleRad, float endAngleRad, Gdiplus::Color fillColor);
		void DrawPartialRing2(Gdiplus::Graphics& g, float x, float y, float radius, float size,
			Gdiplus::Color activeColor, Gdiplus::Color backgroundColor, float angle);
	}; // KnobControl
} // PluginGUI
