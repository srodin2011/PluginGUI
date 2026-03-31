#pragma once

#include "Control.h"
#include "Animation.h"
#include "Event.h"

namespace PluginGUI
{
	class ScalableControl : public Control
	{
		void Init(); // Инициализация для вызова из конструктора только!

	public:
		DEFINE_PROPERTY_WITH_EVENТ(float, Value, 0.0f)
		DEFINE_PROPERTY(float, DefaultValue, 0.0f)
		DEFINE_PROPERTY(float, MinVal, 0.0f)
		DEFINE_PROPERTY(float, MaxVal, 1.0f)
		DEFINE_PROPERTY(bool, FullRuler, true)
		DEFINE_PROPERTY(int, ScaleBigSteps, 6)
		DEFINE_PROPERTY(int, ScaleSmallSteps, 5)
		DEFINE_PROPERTY(Gdiplus::Color, ScaleColor, Gdiplus::Color::Black)
		DEFINE_PROPERTY(int, Sensitivity, 200) // Перемещение в пикселях, соотвествующее всему диапазону значений

		virtual bool СanChangeByMouse() const
		{
			return true;
		}

		int GetScaleSteps() const
		{
			return ScaleBigSteps * ScaleSmallSteps;
		}

		void OnMouseMove(UINT nFlags, CPoint p) override;
		BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint p) override;
		void OnLButtonDblClk(UINT nFlags, CPoint point) override;
		void OnLButtonDown(UINT, CPoint p) override;
		void onMouseMoveAndLButtonDown(UINT nFlags, LONG dx, LONG dy) override;

		/// <summary>
		/// Должна ли выполняться анимация при изменении фокуса
		/// </summary>
		/// <returns></returns>
		bool AnimateOnFocusChange() const override
		{
			return true;
		}

	protected:
		ScalableControl(const CRect& Border);
		ScalableControl() : Control() { Init(); }

		std::unique_ptr <Animation> m_ResetToDefValueAnimation = nullptr;
		float m_SaveValue;
		POINT m_LastCursorPos{ 0, 0 };
		int lastMouseY = -1; // Последнее положение мышки по оси Y (необходимо для определения величины изменения значения
		const int EDGE_ZONE = 10;

		/// <summary>
		/// Установить для элемента управления состояние изменения значения (мышкой)
		/// </summary>
		/// <param name="b"></param>
		void SetValueChangeMode(const bool& b) override
		{
			Control::SetValueChangeMode(b);
			//ShowCursor(!b);
			if (!b)
			{
				lastMouseY = -1;
				RestoreMousePos();
			}
			else
			{
				GetCursorPos(&m_LastCursorPos);
			}
		}

		/// <summary>
		/// Нужно ли восстанавливать позицию мышки после изменения значения перемещениеем мышки
		/// </summary>
		/// <returns></returns>
		virtual void RestoreMousePos() 
		{
			POINT p = { m_LastCursorPos.x, m_LastCursorPos.y };
			SetCursorPos(p);
		}

		void OnChangeValueProgress(int progress)
		{
			m_FocusAnimationProgress = m_FocusAnimation->AnimatingForward ? progress : (1.0f - progress);
		}

		void onMouseDelta(float pixelDelta, UINT nFlags);
	};
}
