//#include "pch.h"
#include "PluginGUI\include\ScalableControl.h"
#include "PluginGUI\include\Utils.h"
#include "PluginGUI\include\MouseCalibration.h"

using namespace Gdiplus;

namespace PluginGUI
{
	PLUGINGUI_PROPERTY_TABLE_INSTANTIATE(ScalableControl)

	ScalableControl::ScalableControl(const CRect& Border) :
		Control(Border)
	{
		Init();
	}

	void ScalableControl::Init()
	{
		m_LastCursorPos.x = m_LastCursorPos.y = 0;

		m_ResetToDefValueAnimation = std::make_unique<Animation>(
			// При старте анимации
			[&]() {
				m_SaveValue = m_Value;
			},
			// При обновлении анимации
			[&](float progrees) {
				SetValue(m_SaveValue > m_DefaultValue
					? m_DefaultValue + (m_SaveValue - m_DefaultValue) * progrees
					: m_SaveValue + (m_DefaultValue - m_SaveValue) * progrees);
			},
			// При завершении анимации
			[&]() {
			},
			500);

		// Задаем функцию инвалидации по умолчанию
		m_ResetToDefValueAnimation->SetInvalidateCallback([&]() {
			Invalidate();
			});
	}

	/// <summary>
	/// Реакция на перемещение мышки
	/// </summary>
	/// <param name="p">Новая позиция мышки</param>
	void ScalableControl::OnMouseMove(UINT nFlags, CPoint p)
	{
		if (nFlags & MK_LBUTTON)
		{
			CPoint screenPt;
			GetCursorPos(&screenPt);

			int screenHeight = GetSystemMetrics(SM_CYSCREEN);
			if (screenPt.y <= EDGE_ZONE || screenPt.y >= screenHeight - EDGE_ZONE)
			{
				return;
			}

			if (lastMouseY != -1)
			{
				int screenDelta = -(screenPt.y - lastMouseY);
				if (screenDelta != 0)
				{
					onMouseDelta((float)screenDelta, nFlags);  // screenDelta → pixelDelta
				}
			}
			lastMouseY = screenPt.y;
		}
	}

	//void ScalableControl::OnMouseMove(UINT nFlags, CPoint p)
	//{
	//	if (СanChangeByMouse() && GetValueChangeMode())
	//	{
	//		CPoint screenPt;
	//		GetCursorPos(&screenPt);

	//		int y = screenPt.y;
	//		//int y = p.y;

	//		if (lastMouseY != -1)
	//		{
	//			int deltaY = -(y - lastMouseY);

	//			double curVal = GetValue();
	//			double newVal = curVal + deltaY * Sensitivity * (nFlags & MK_CONTROL ? 0.1f : 1.0f)/*0.05f*/;

	//			// Ограничиваем значение в диапазоне
	//			if (newVal < GetMinVal())
	//			{
	//				newVal = GetMinVal();
	//			}
	//			else if (newVal > GetMaxVal())
	//			{
	//				newVal = GetMaxVal();
	//			}

	//			lastMouseY = screenPt.y;
	//			//lastMouseY = p.y;

	//			if (newVal != curVal)
	//			{
	//				SetValue(newVal);
	//				Invalidate();
	//			}
	//		}
	//		else
	//		{
	//			lastMouseY = y;
	//		}
	//	}
	//	else
	//	{
	//		Control::OnMouseMove(nFlags, p);
	//	}
	//}

	/// <summary>
	/// Реакция на поворот колесика мышки
	/// </summary>
	/// <param name="nFlags"></param>
	/// <param name="zDelta">Размер поворота колесика</param>
	/// <param name="p"></param>
	/// <returns></returns>
	BOOL ScalableControl::OnMouseWheel(UINT nFlags, short zDelta, CPoint p)
	{
		if (GetFocus())
		{
			// Прокрутка по рискам шкалы ручки
			float curVal = GetValue();
			int numWheelTurns = zDelta / WHEEL_DELTA; // Вычислить на сколько "щелчков" колесико повернуто
			float scaleStepValue = (GetMaxVal() - GetMinVal()) / GetScaleSteps(); // Вычислить изменение значения, соответствующее одному шагу шкалы
			float newVal = curVal + numWheelTurns * scaleStepValue * (nFlags & MK_CONTROL ? 0.1f : 1.0f); // Здесь 30 - количество "щелчков", соответствующих всей шкале

			if ((nFlags & MK_CONTROL) == 1)
			{
				float numOfSteps = roundToDecimals(newVal / scaleStepValue, 1); // Округлить шаги до 0.1
				int newNumOfSteps = (int)(numWheelTurns < 0 ? ceil(numOfSteps) : floor(numOfSteps)); // Округлить до ближайшего целого
				newVal = GetMinVal() + newNumOfSteps * scaleStepValue; // Перевести шаги в новое значение
			}

			// Ограничиваем значение в диапазоне
			if (newVal < GetMinVal())
			{
				newVal = GetMinVal();
			}
			else if (newVal > GetMaxVal())
			{
				newVal = GetMaxVal();
			}

			if (newVal != curVal)
			{
				SetValue(newVal);
				Invalidate();
			}
			return true;
		}

		return false;
	}

	void ScalableControl::OnLButtonDblClk(UINT nFlags, CPoint point)
	{
		//StartAnimation(!m_On);
	}

	void ScalableControl::OnLButtonDown(UINT nFlags, CPoint p)
	{
		if (nFlags & MK_CONTROL)
		{
			m_ResetToDefValueAnimation->StartAnimation(m_Value < m_DefaultValue);
		}
		Control::OnLButtonDown(nFlags, p);
	}

	//void ScalableControl::onMouseMoveAndLButtonDown(UINT nFlags, LONG dx, LONG dy)
	//{
	//	if (СanChangeByMouse() && GetValueChangeMode() && dy != 0)
	//	{
	//		double pixelsPerRange = Sensitivity;
	//		
	//		// Конвертируем тики в эквивалентные пиксели экрана
	//		double pixelDelta = (double)dy / MouseCalibration::getInstance().getTicksPerPixel();

	//		// Нормализуем: 200px = полный диапазон (-1..+1)
	//		double normalizedDelta = -pixelDelta / pixelsPerRange;

	//		double range = GetMaxVal() - GetMinVal();
	//		double newVal = GetValue() + normalizedDelta * range * (nFlags & MK_SHIFT ? 0.1f : 1.0f);
	//		SetValue(std::clamp(newVal, GetMinVal(), GetMaxVal()));
	//		Invalidate();
	//	}
	//}

	void ScalableControl::onMouseMoveAndLButtonDown(UINT nFlags, LONG dx, LONG dy)
	{
		if (dy == 0) return;

		CPoint screenPt;
		GetCursorPos(&screenPt);
		int screenDelta = abs(screenPt.y - lastMouseY);
		//if (screenDelta != 0)
		//{
		//	return;
		//}

		int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		if (screenPt.y <= EDGE_ZONE - 1 || screenPt.y >= screenHeight - EDGE_ZONE )
		{
			float pixelDelta = (float)(-dy) / MouseCalibration::getInstance().getTicksPerPixel();
			onMouseDelta(pixelDelta, nFlags);  // rawTicks → pixelDelta
			lastMouseY = screenPt.y;
		}
	}

	void ScalableControl::onMouseDelta(float pixelDelta, UINT nFlags)
	{
		if (!СanChangeByMouse() || !GetValueChangeMode()) return;

		int pixelsPerRange = Sensitivity;  // 200px = полный диапазон
		float normalizedDelta = pixelDelta / pixelsPerRange;

		float range = GetMaxVal() - GetMinVal();
		float modifier = (nFlags & MK_SHIFT) ? 0.1f : 1.0f;

		float newVal = GetValue() + normalizedDelta * range * modifier;
		SetValue(std::clamp(newVal, MinVal, MaxVal));
		Invalidate();
	}

	//void ScalableControl::DrawLable(
	//	Graphics& g,               // Контекст GDI+
	//	int width, int height,     // Общий размер области
	//	const WCHAR* text,
	//	TextAlignmentStyle alignment = TextAlignmentStyle::Left)
	//{
	//	// Размеры "кнопки"
	//	int buttonWidth = 10;
	//	int buttonHeight = 25;//18;
	//	int buttonY = (height - buttonHeight) / 2;
	//	int buttonX = 4;

	//	// Рисуем текст справа от кнопки
	//	FontFamily fontFamily(L"Segoe UI");
	//	Gdiplus::Font font(&fontFamily, 12, FontStyleRegular, UnitPixel);
	//	SolidBrush textBrush(Color(255, 255, 255, 255)); // Белый

	//	// Область для текста: от кнопки до правого края
	//	//RectF textRect(
	//	//    (REAL)(buttonX + buttonWidth + 8),  // textX
	//	//    0,                                  // от верха
	//	//    (REAL)(width - (buttonX + buttonWidth + 8)),  // ширина до правого края
	//	//    (REAL)height);                      // полная высота

	//	RectF textRect(0, 0, Border.Width() - 1, Border.Height() - 1);


	//	// Настройка выравнивания
	//	StringFormat stringFormat;
	//	switch (alignment)
	//	{
	//		case TextAlignmentStyle::Center:
	//			stringFormat.SetAlignment(StringAlignmentCenter);
	//			stringFormat.SetLineAlignment(StringAlignmentCenter);
	//			break;
	//		case TextAlignmentStyle::Right:
	//			stringFormat.SetAlignment(StringAlignmentFar);
	//			stringFormat.SetLineAlignment(StringAlignmentCenter);
	//			break;
	//		case TextAlignmentStyle::Left:
	//		default:
	//			stringFormat.SetAlignment(StringAlignmentNear);
	//			stringFormat.SetLineAlignment(StringAlignmentCenter);
	//			break;
	//	}

	//	g.DrawString(text, -1, &font, textRect, &stringFormat, &textBrush);
	//}

} // PluginGUI
