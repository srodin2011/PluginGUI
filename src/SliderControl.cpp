//#include "pch.h"
#include "PluginGUI\include\SliderControl.h"
#include "PluginGUI\include\PluginView.h"

using namespace Gdiplus;

namespace PluginGUI
{
	/// <summary>
	/// Конструктор ползунка
	/// </summary>
	/// <param name="pluginView">Ссылка на плагин</param>
	/// <param name="border">Границы ползунка</param>
	SliderControl::SliderControl(const CRect& Border) :
		ScalableControl(Border)
	{
		Init();
	}

	/// <summary>
	/// Инициализация
	/// </summary>
	void SliderControl::Init()
	{
	}

	/// <summary>
	/// Функция отрисовки ползунка
	/// </summary>
	/// <param name="dc"></param>
	/// <param name="hasFocus"></param>
	void SliderControl::Draw(bool hasFocus, bool drawSelected)
	{
		if (m_pContainer)
		{
			Graphics g(оffScreenBitmap.get());
			g.Clear(Color::Transparent);
			g.SetSmoothingMode(SmoothingModeHighQuality/*SmoothingModeAntiAlias*/);

			if (Selected && !drawSelected) return;

			float handleW = GetHandleWidth();
			float handleH = GetHandleHeight();

			float handleX = GetHandleX();
			float handleY = GetHandleY();

			DrawSliderScale(g, 0, handleH / 2, (float)Border.Width(), (float)Border.Height() - handleH, handleY, m_FocusAnimationProgress);
			DrawControlHandle(g, handleX, handleY, handleW, handleH, m_FocusAnimationProgress);
			DrawBorder2(g);
		}
	}

	void GetRoundRectPath(GraphicsPath* path, const RectF& rect, float radius)
	{
		float diameter = radius * 2;
		if (diameter > rect.Width)
		{
			diameter = rect.Width;
		}
		if (diameter > rect.Height) 
		{ 
			diameter = rect.Height; 
		}

		float right = rect.X + rect.Width;
		float bottom = rect.Y + rect.Height;

		RectF corner(rect.X, rect.Y, diameter, diameter);

		path->Reset();

		// Верхний левый угол
		path->AddArc(corner, 180, 90);

		// Верхняя сторона
		path->AddLine(rect.X + radius, rect.Y, right - radius, rect.Y);

		// Верхний правый угол
		corner.X = right - diameter;
		path->AddArc(corner, 270, 90);

		// Правая сторона
		path->AddLine(right, rect.Y + radius, right, bottom - radius);

		// Нижний правый угол
		corner.Y = bottom - diameter;
		path->AddArc(corner, 0, 90);

		// Нижняя сторона
		path->AddLine(right - radius, bottom, rect.X + radius, bottom);

		// Нижний левый угол
		corner.X = rect.X;
		path->AddArc(corner, 90, 90);

		// Левая сторона
		path->AddLine(rect.X, bottom - radius, rect.X, rect.Y + radius);

		path->CloseFigure();
	}

	/// <summary>
	/// Отрисовка ручки слайдера
	/// </summary>
	/// <param name="g">Graphics</param>
	/// <param name="_x">Центр ручки по оси X</param>
	/// <param name="_y">Центр ручки по оси Y</param>
	/// <param name="width">Ширина ручки</param>
	/// <param name="height">Высота ручки</param>
	/// <param name="animationProgress">Прогресс анимации [0;1]</param>
	void SliderControl::DrawControlHandle(Gdiplus::Graphics& g, float _x, float _y, float width, float height, float animationProgress)
	{
		using namespace Gdiplus;

		float focusScalingFactor = 0.1f;
		float scalingFactor = (1 - animationProgress * focusScalingFactor);
		float sliderHandleW = width * scalingFactor;
		float sliderHandleH = height * scalingFactor;
		float sliderHandleHalfH = sliderHandleH / 2.0f;

		float x = _x - width * scalingFactor / 2.0f;
		float y = _y - height * scalingFactor / 2.0f;

		float slopeHandelH = sliderHandleHalfH * 0.7f;
		float controlHandleH = sliderHandleHalfH * 0.3f;

		g.SetSmoothingMode(SmoothingModeAntiAlias);

		Color base(255, 45, 112, 37);
		//Color base(255, 146, 149, 158);

		Color topHandleSlopeTopColor = AdjustColor(base, -0.00463f, 0.3526f, 1.0414f, 1.0f);
		Color topHandleSlopeLowerColor = AdjustColor(base, -0.006f, 1.184f, 1.545f, 1.0f);
		Color lowerHandleSlopeTopColor = AdjustColor(base, -0.006f, 0.643f, 0.393f, 1.0f);
		Color lowerHandleSlopeLowerColor = AdjustColor(base, -0.002f, 0.316f, 0.959f, 1.0f);
		//Color topHandleSlopeTopColor = AdjustBrightness(base, 1.f);
		//Color topHandleSlopeLowerColor = AdjustBrightness(base, 1.5f);
		//Color lowerHandleSlopeTopColor = AdjustBrightness(base, 0.5f);
		//Color lowerHandleSlopeLowerColor = AdjustBrightness(lowerHandleSlopeTopColor, 3.f);

		// Цвета (подберите под свою палитру)
		//Color topHandleSlopeTopColor(255, 146, 149, 158);    // Верхний скат ручки, верхний цвет 
		//Color topHandleSlopeLowerColor(255, 216, 219, 228); // Верхний скат ручки, нижний цвет 
		//Color lowerHandleSlopeTopColor(255, 52, 55, 64);    // Нижний скат ручки, верхний цвет 
		//Color lowerHandleSlopeLowerColor(255, 131, 134, 143); // Нижний скат ручки, нижний цвет 

		Color topControlHandleTopColor = AdjustColor(base, -0.006f, 0.667f, 1.38f, 1.0f);
		Color topControlHandleLowerColor = AdjustColor(base, -0.006f, 0.437f, 1.179f, 1.0f);
		Color lowerControlHandleTopColor = AdjustColor(base, -0.006f, 0.416f, 1.151f, 1.0f);
		Color lowerControlHandleLowerColor = AdjustColor(base, -0.002f, 0.316f, 0.959f, 1.0f);

		//Color topControlHandleTopColor = AdjustBrightness(base, 1.3f);
		//Color topControlHandleLowerColor = AdjustBrightness(base, 1.3f);
		//Color lowerControlHandleTopColor = AdjustBrightness(base, 1.1f);
		//Color lowerControlHandleLowerColor = AdjustBrightness(base, 1.f);

		//Color topControlHandleTopColor(255, 194, 197, 206);    // Верхний скат ручки, верхний цвет 
		//Color topControlHandleLowerColor(255, 164, 167, 176);    // Верхний скат ручки, верхний цвет 
		//Color lowerControlHandleTopColor(255, 162, 165, 172);    // Верхний скат ручки, верхний цвет 
		//Color lowerControlHandleLowerColor(255, 132, 135, 144);    // Верхний скат ручки, верхний цвет 

		//Color bottomSlopeColor(255, 218, 221, 230);  // Светлый (центр)
		//Color topColor2(255, 52, 55, 64);    // Тёмно-серый (края)
		//Color bottomColor2(255, 131, 134, 143);  // Светлый (центр)


		Color lineLight = AdjustColor(base, 0.0f, 0.222f, 0.872f, 1.0f);    // Белая линия
		Color lineDark = AdjustColor(base, -0.005f, 0.240f, 0.359f, 1.0f);        // Тёмная линия
		//Color lineLight(255, 122, 125, 132);    // Белая линия
		//Color lineDark(255, 48, 51, 58);        // Тёмная линия
		//Color borderColor(255, 80, 80, 90);     // Рамка

		Color handleBorderTopColor = AdjustColor(base, 0.0027f, 0.99f, 0.994f, 1.0f);
		Color handleBorderLowerColor = AdjustColor(base, 0.001f, 0.995f, 0.677f, 1.0f);    

		// Обводка ручки, если установлен фокус
		RectF нandleBorder((REAL)(_x - width / 2.0f), (REAL)(_y - sliderHandleH / 2.0f), (REAL)width, (REAL)sliderHandleH);
		LinearGradientBrush linGrBrush(
			нandleBorder,
			handleBorderTopColor,
			handleBorderLowerColor,
			LinearGradientModeVertical);
		GraphicsPath handleBorderPath;
		GetRoundRectPath(&handleBorderPath, нandleBorder, (width - sliderHandleW) / 2.0f);
		g.FillPath(&linGrBrush, &handleBorderPath);

		// Опционально: обводка контура чёрным цветом толщиной 1
		//Pen pen(Color(255, 0, 0, 0), 1);
		//graphics.DrawPath(&pen, &path);


		// Верхний скат ручки
		RectF topHandleSlopeRect((REAL)x, (REAL)y, (REAL)sliderHandleW, (REAL)slopeHandelH);
		LinearGradientBrush topHandleSlopeBrush(
			topHandleSlopeRect,
			topHandleSlopeTopColor, topHandleSlopeLowerColor,
			LinearGradientModeVertical
		);
		g.FillRectangle(&topHandleSlopeBrush, topHandleSlopeRect);

		// Верхняя половина ручки
		RectF topControlHandleRect((REAL)x, (REAL)(y + slopeHandelH), (REAL)sliderHandleW, (REAL)controlHandleH);
		LinearGradientBrush topControlHandleBrush(
			topControlHandleRect,
			topControlHandleTopColor, topControlHandleLowerColor,
			LinearGradientModeVertical
		);
		g.FillRectangle(&topControlHandleBrush, topControlHandleRect);

		// Нижняя половина ручки
		RectF lowerControlHandleRect((REAL)x, (REAL)(y + slopeHandelH + controlHandleH), (REAL)sliderHandleW, (REAL)controlHandleH);
		LinearGradientBrush lowerControlHandleBrush(
			lowerControlHandleRect,
			lowerControlHandleTopColor, lowerControlHandleLowerColor,
			LinearGradientModeVertical
		);
		g.FillRectangle(&lowerControlHandleBrush, lowerControlHandleRect);

		// Нижний скат ручки
		RectF lowerHandleSlopeRect((REAL)x, (REAL)(y + slopeHandelH + controlHandleH * 2), (REAL)sliderHandleW, (REAL)slopeHandelH);
		LinearGradientBrush lowerHandleSlopeBrush(
			lowerHandleSlopeRect,
			lowerHandleSlopeTopColor, lowerHandleSlopeLowerColor,
			LinearGradientModeVertical
		);
		g.FillRectangle(&lowerHandleSlopeBrush, lowerHandleSlopeRect);

		// Тёмная горизонтальная линия между половинами
		Pen darkPen(lineDark, 1.0f);
		g.DrawLine(&darkPen, x, y + sliderHandleHalfH - 1, x + sliderHandleW, y + sliderHandleHalfH - 1);

		// Белая горизонтальная линия между половинами
		Pen lightPen(lineLight, 1.0f);
		g.DrawLine(&lightPen, x, y + sliderHandleHalfH, x + sliderHandleW, y + sliderHandleHalfH);

		Pen borderPen(lowerHandleSlopeLowerColor, 1.5f);
		RectF borderRect((REAL)x, (REAL)y, (REAL)sliderHandleW, (REAL)sliderHandleH);
		g.DrawRectangle(&borderPen, borderRect);

		//// Тёмная рамка по периметру
		//Pen borderPen(borderColor, 2.0f);
		//g.DrawRectangle(&borderPen, x, y, thumbW, thumbH);
	}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="graphics"></param>
	/// <param name="start"></param>
	/// <param name="end"></param>
	/// <param name="length"></param>
	/// <param name="thickness"></param>
	/// <param name="capRadius"></param>
	/// <param name="top"></param>
	/// <param name="color"></param>
	static void DrawVerticalLineWithCustomCaps(Graphics& graphics, PointF start, PointF end, float thickness, float capRadius, bool top, Color color)
	{
		// Вектор перпендикулярный линии
		float halfThick = thickness / 2;
		float px = halfThick;
		float py = 0;

		GraphicsPath path;

		// Прямоугольник линии без концов
		PointF points[4] = {
			PointF(start.X - px, start.Y),
			PointF(start.X + px, start.Y),
			PointF(start.X + px, end.Y),
			PointF(start.X - px, end.Y)
		};
		path.AddPolygon(points, 4);

		if (top)
		{
			RectF topArcRect(start.X - thickness / 2.0f, start.Y - thickness / 4.0f, thickness, thickness / 2.0f);
			path.AddArc(topArcRect, 180, 180);
		}
		else
		{
			RectF bottomArcRect(start.X - thickness / 2.0f, end.Y - thickness / 4.0f, thickness, thickness / 2.0f);
			path.AddArc(bottomArcRect, 0, 180);
		}

		SolidBrush brush(color);
		graphics.FillPath(&brush, &path);
	}

	/// <summary>
	/// Отрисовка шкалы слайдера
	/// </summary>
	/// <param name="g">Graphics</param>
	/// <param name="x">Левая координата X</param>
	/// <param name="y">Левая координата Y</param>
	/// <param name="width">Ширина шкалы</param>
	/// <param name="height">Высота шкалы</param>
	/// <param name="handleY">Координата по оси Y центра ручки слайдера</param>
	/// <param name="animationProgress">Прогресс анимации [0;1]</param>
	void SliderControl::DrawSliderScale(Gdiplus::Graphics& g, float x, float y, float width, float height, float handleY, float animationProgress)
	{
		using namespace Gdiplus;

		Color trackColor(255, 40, 40, 40);     // Почти чёрная центральная линия
		Color trackHightlitedColor(255, 255, 255, 255);     // Почти чёрная центральная линия

		// Деления
		int tickCount = ScaleBigSteps * ScaleSmallSteps + 1; // Как на образце
		float tickLongLen = width / 2; // Длина деления
		float tickShortLen = tickLongLen * 2.0f / 3.0f; // Длина деления
		float railDefaultWidth = width / 6;
		float railFocusedWidth = width / 8;
		float railWidth = railDefaultWidth - (railDefaultWidth - railFocusedWidth) * animationProgress;

		Pen tickPen(ScaleColor, 1);

		float cx = x + width / 2;

		for (int i = 0; i < tickCount; ++i) {
			float yy = y + i * (height - 1) / (tickCount - 1);
			bool longLine = i % ScaleSmallSteps == 0;
			float tickLen = longLine ? tickLongLen : tickShortLen;
			g.DrawLine(&tickPen, cx - tickLen, yy, cx - railWidth / 2 - 4, yy);
			g.DrawLine(&tickPen, cx + railWidth / 2 + 4, yy, cx + tickLen, yy);
		}

		DrawVerticalLineWithCustomCaps(g, PointF(cx, y), PointF(cx, handleY), railWidth, railWidth * 2.0f / 3.0f, true, ScaleColor);
		DrawVerticalLineWithCustomCaps(g, PointF(cx, handleY), PointF(cx, y + height), railWidth, railWidth * 2.0f / 3.0f, false, trackHightlitedColor);
	}

	void SliderControl::RestoreMousePos() 
	{
		if (m_pContainer)
		{
			POINT p = { Border.left + (int)GetHandleX(), Border.top + (int)GetHandleY() };
			SetCursorPos(p);
		}
	}
}