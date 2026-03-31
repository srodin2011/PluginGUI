//#include "pch.h"
#define NOMINMAX

#include "PluginGUI\include\KnobControl.h"
#include "PluginGUI\include\PluginView.h"
#include "PluginGUI\include\Utils.h"
#include "Clipper2\include\clipper2\clipper.h"
#include <algorithm>

//using namespace Clipper2Lib;
using namespace Gdiplus;

namespace PluginGUI
{
	PLUGINGUI_PROPERTY_TABLE_INSTANTIATE(KnobControl)

	KnobControl::KnobControl(const CRect& Border):
		KnobControl(Border.TopLeft(), std::min(Border.Width(), Border.Height()))
	{
		Init();
	}

	KnobControl::KnobControl(CPoint startPoint, int size) :
		m_StartPoint(startPoint),
		m_Size(size),
		ScalableControl(CRect(startPoint, CSize(size, size)))
	{
		Init();
	}

	/// <summary>
	/// Инициализация
	/// </summary>
	void KnobControl::Init()
	{
	}

	/// <summary>
	/// Изменить границы элемента управления
	/// </summary>
	void KnobControl::SetBorder(const Frame& border)
	{
		m_StartPoint = border.TopLeft();
		m_Size = std::min(border.Width(), border.Height());
		ScalableControl::SetBorder(CRect(m_StartPoint, CSize(m_Size, m_Size)));
	}
	/// <summary>
	/// Проверить возможность получения фокуса элементом управления
	/// </summary>
	/// <param name="x">Координата проверяемой точки по оси Х</param>
	/// <param name="y">Координата проверяемой точки по оси Y</param>
	/// <returns>Возвращает true, если точка в фокусе</returns>
	//bool KnobControl::HitTest(int x, int y)
	//{
	//	if (x >= m_StartPoint.x && x <= m_StartPoint.x + m_Size - 1 && y >= m_StartPoint.y && y <= m_StartPoint.y + m_Size - 1)
	//		return true;
	//	return false;
	//}

	constexpr double scale = 1000.0;

	/// <summary>
	/// Функция для приближенного преобразования окружности в многоугольник (Path64)
	/// </summary>
	/// <param name="cx"></param>
	/// <param name="cy"></param>
	/// <param name="r"></param>
	/// <param name="numSides"></param>
	/// <returns></returns>
	static Clipper2Lib::Paths64 CircleToPolygon(double cx, double cy, double r, int numSides = 64)
	{
		Clipper2Lib::Paths64 result;
		Clipper2Lib::Path64 polygon;
		polygon.reserve(numSides);

		for (int i = 0; i < numSides; i++)
		{
			double angle = 2.0 * M_PI * i / numSides;
			double x = cx + r * cos(angle);
			double y = cy + r * sin(angle);
			polygon.push_back(Clipper2Lib::Point64(static_cast<int64_t>(x * scale), static_cast<int64_t>(y * scale)));
		}
		// Закрываем контур, добавляя первую точку в конец (Clipper2Lib не требует, но можно)
		polygon.push_back(polygon.front());

		result.push_back(polygon);
		return result;
	}

	/// <summary>
	/// Рисуем фаску по краям ручки
	/// </summary>
	/// <param name="graphics">Ссылка на Graphics</param>
	/// <param name="center">Сентральная точка</param>
	/// <param name="outerRadius">Внешний радиус фаски (рисуем фаску внутрь ручки)</param>
	/// <param name="bevelWidth"></param>
	/// <param name="baseColor">Базовый цвет</param>
	/// <param name="lightAngleDeg">Угол освещения в градусах</param>
	static void DrawBevel(Graphics& graphics, PointF center, float outerRadius, float bevelWidth, Color baseColor, float lightAngleDeg)
	{
		const int rings = 3;
		float ringWidth = bevelWidth / rings;
		int segments = 360; // Чем больше, тем глаже

		// Основные цвета для колец
		Color ringColors[rings] = {
			Color(baseColor.GetA(), std::max(0, baseColor.GetR() - 60), std::max(0, baseColor.GetG() - 50), std::max(0, baseColor.GetB() - 50)),
			Color(baseColor.GetA(), std::max(0, baseColor.GetR() - 30), std::max(0, baseColor.GetG() - 15), std::max(0, baseColor.GetB() - 15)),
			Color(baseColor.GetA(), std::min(255, baseColor.GetR() + 10), std::min(255, baseColor.GetG() + 10), std::min(255, baseColor.GetB() + 10)),
		};

		float lightAngleRad = lightAngleDeg * 3.14159265f / 180.0f;

		for (int ring = 0; ring < rings; ++ring)
		{
			float rOuter = outerRadius - ring * ringWidth;
			float rInner = rOuter - ringWidth;

			for (int i = 0; i < segments; ++i)
			{
				float angle1 = 2.0f * 3.14159265f * i / segments;
				float angle2 = 2.0f * 3.14159265f * (i + 1) / segments;

				// Яркость блика на этом угле (косинусная зависимость)
				float highlight = float(cos(angle1 - lightAngleRad));
				highlight = (highlight + 1.0f) / 2.0f; // от 0 (тень) до 1 (блик)
				float contrast = 0.7f + 0.3f * ring / (rings - 1);
				highlight = pow(highlight, 1.5f) * contrast;

				int r = int(ringColors[ring].GetR() + highlight * (baseColor.GetR() + 60 - ringColors[ring].GetR()));
				int g = int(ringColors[ring].GetG() + highlight * (baseColor.GetG() + 40 - ringColors[ring].GetG()));
				int b = int(ringColors[ring].GetB() + highlight * (baseColor.GetB() + 40 - ringColors[ring].GetB()));

				Color arcColor(255, std::min(255, std::max(0, r)), std::min(255, std::max(0, g)), std::min(255, std::max(0, b)));

				// Строим путь сектора
				GraphicsPath path;
				PointF pts[4] = {
					PointF(center.X + rInner * cos(angle1), center.Y + rInner * sin(angle1)),
					PointF(center.X + rOuter * cos(angle1), center.Y + rOuter * sin(angle1)),
					PointF(center.X + rOuter * cos(angle2), center.Y + rOuter * sin(angle2)),
					PointF(center.X + rInner * cos(angle2), center.Y + rInner * sin(angle2))
				};
				path.AddLine(pts[0], pts[1]);
				path.AddLine(pts[1], pts[2]);
				path.AddLine(pts[2], pts[3]);
				path.CloseFigure();

				SolidBrush brush(arcColor);
				graphics.FillPath(&brush, &path);
			}
		}
	}

	/// <summary>
	/// Отрисовка ручки с круговыми насечками при фокусировании
	/// </summary>
	/// <param name="graphics">Графический контекст, по которому нужно рисовать</param>
	/// <param name="x">Центр ручки по оси X</param>
	/// <param name="y">Центр ручки по оси Y</param>
	/// <param name="radius">Радиус ручки управления</param>
	/// <param name="ringThickness">Толщина кольца вокруг ручки управления</param>
	/// <param name="notchCount">Количесво насечек на кольце</param>
	/// <param name="rotationAngle">Поворот ручки (звисит от текущего значения параметра), в радианах</param>
	/// <param name="animationProgress">Прогресс анимации в диапазоне от 0 до 1</param>
	void KnobControl::DrawMetallicRingWithCircularNotches(
		Graphics& graphics,
		float x,
		float y,
		float radius,
		float ringThickness,
		float rickThickness,
		int notchCount,
		float rotationAngle,
		float animationProgress) // угол поворота в градусах
	{
		float outerRadius = radius + ringThickness; // Внешний радиус кольца
		float notchRadius = notchCount > 0 ? radius / 2.0f * (7.0f / notchCount) : 0; // Радиус выреза на внешнем кольце
		float innerRadius = radius - notchRadius * 0.15f * animationProgress; // Внутренний радиус кольца
		float notchCenterRadius = outerRadius + notchRadius * (1 - animationProgress * 0.3f); // Радиус, на котором находятся центры всех насечек (окружностей)

		//float outerRadius = radius + ringThickness; // Внешний радиус кольца
		//float notchRadius = radius / 2.0f; // Радиус выреза на внешнем кольце
		//float innerRadius = radius - notchRadius * 0.15f * animationProgress; // Внутренний радиус кольца
		//float notchCenterRadius = outerRadius + notchRadius * (1 - animationProgress * 0.3f); // Радиус, на котором находятся центры всех насечек (окружностей)


		//GraphicsPath ringPath;
		//ringPath.SetFillMode(FillModeAlternate);

		//ringPath.AddEllipse(RectF(
		//	(REAL)(x - outerRadius),
		//	(REAL)(y - outerRadius),
		//	(REAL)(outerRadius * 2),
		//	(REAL)(outerRadius * 2)));

		// Создаем полигоны для внешеней окружности кольца
		Clipper2Lib::Paths64 mainPoly = CircleToPolygon(x, x, outerRadius);


		//Region ringRegion(&ringPath);

		double rotationRad = rotationAngle/* * M_PI / 180.0*/;

		Clipper2Lib::Paths64 cutPolys; // полигону, в которые будут преобразованы насечки

		for (int i = 0; i < notchCount; i++)
		{
			double baseAngle = 2 * M_PI / notchCount;
			double angle = m_MinAngle - baseAngle * i - baseAngle / 2.0 - rotationRad;

			float notchCenterX = x + notchCenterRadius * cosf((float)angle);
			float notchCenterY = y - notchCenterRadius * sinf((float)angle);

			//GraphicsPath notchPath;

			//REAL notchX = notchCenterX - notchRadius;
			//REAL notchY = notchCenterY - notchRadius;
			//REAL notchSize = notchRadius * 2;

			//notchPath.AddEllipse(RectF(notchX, notchY, notchSize, notchSize));

			// Было
			//ringRegion.Exclude(&notchPath);

			// Стало
			//Region cutRegion(&notchPath);
			//ringRegion.Exclude(&cutRegion);
			//notchPath.Reverse();
			//ringPath.AddPath(&notchPath, TRUE);

			// Преобразуем одину насечку в полигон и добавим список полигонов
			Clipper2Lib::Paths64 p = CircleToPolygon(notchCenterX, notchCenterY, notchRadius);
			cutPolys.insert(cutPolys.end(), p.begin(), p.end());
		}

		Color capColor = CapColor;
		Color outerColor1 = AdjustColor(m_RingColor, -0.008f, 0.38f, 2.1f, 1.f);
		Color outerColor2 = AdjustColor(m_RingColor, 0.f, 0.53f, 0.5f, 1.f);
		//Color outerColor1 = AdjustBrightness(baseColor, 1.5f);
		//Color outerColor2 = AdjustBrightness(baseColor, 0.22f);

		RectF outerRect(
			(REAL)(x - outerRadius),
			(REAL)(y - outerRadius),
			(REAL)(outerRadius * 2),
			(REAL)(outerRadius * 2));

		LinearGradientBrush ringBrush(
			outerRect,
			outerColor1,
			outerColor2,
			//Color(255, 141, 150, 155),
			//Color(255, 35, 39, 42),
			90.f + m_Alpha * 180.f / (float)M_PI,
			LinearGradientModeBackwardDiagonal);

		//RectF outerRect(
		//	(REAL)(x - outerRadius),
		//	(REAL)(y - outerRadius),
		//	(REAL)(outerRadius * 2),
		//	(REAL)(outerRadius * 2));

		//LinearGradientBrush ringBrush(
		//	outerRect,
		//	outerColor1,
		//	outerColor2,
		//	(REAL)(90.f + m_Alpha * 180.f / M_PI)
		//	//LinearGradientModeBackwardDiagonal
		//);

		//graphics.FillPath(&ringBrush, &ringPath);
		//graphics.FillRegion(&ringBrush, &ringRegion);

		Color borderColor(255, 0, 0, 0);
		Pen borderPen(borderColor, 1.0f); // Перо для вшешних границ
		Pen scalePen(ScaleColor, 1.0f); // Перо для вшешних границ
		// graphics.DrawPath(&pen, &ringPath); - Пока выключили


		//!!!!!!!!!!!!!!!
		// Выполняем вычитание: внешнее кольцо минус насечки
		Clipper2Lib::Paths64 ringBorder = Difference(mainPoly, cutPolys, Clipper2Lib::FillRule::NonZero);

		// Преобразуем solution в GraphicsPath для GDI+
		GraphicsPath boredPath;
		for (const Clipper2Lib::Path64& polygon : ringBorder)
		{
			if (polygon.empty()) continue;

			PointF startPoint(static_cast<float>(polygon[0].x / scale), static_cast<float>(polygon[0].y / scale));
			boredPath.StartFigure();
			boredPath.AddLine(startPoint, startPoint); // Начинаем фигуру

			for (size_t i = 1; i < polygon.size(); ++i)
			{
				PointF pt(static_cast<float>(polygon[i].x / scale), static_cast<float>(polygon[i].y / scale));
				PointF lastPt;
				if (boredPath.GetLastPoint(&lastPt) == Ok)
				{
					boredPath.AddLine(lastPt.X, lastPt.Y, pt.X, pt.Y);
				}
				else
				{
					// Если не удалось получить последнюю точку, начинаем линию от startPoint
					boredPath.AddLine(startPoint.X, startPoint.Y, pt.X, pt.Y);
				}
			}
			boredPath.CloseFigure();
		}

		// Отрисовываем внешнюю границу кольца
		graphics.FillPath(&ringBrush, &boredPath);
		graphics.DrawPath(&scalePen, &boredPath);

		// Здесь нужно продолжить описание



		Color innerColor1 = capColor;
		Color innerColor2 = AdjustBrightness(capColor, 0.39f);

		// Внутренний круг - основной металлический круг
		Gdiplus::RectF innerRect(x - innerRadius, y - innerRadius, innerRadius * 2, innerRadius * 2);

		// Внутренняя часть ручки
		if (true)
		{
			float metallicGlareRadius = innerRadius;
			Bitmap bmp((int)(metallicGlareRadius * 2), (int)(metallicGlareRadius * 2), PixelFormat32bppARGB);

			Color _baseColor = AdjustColor(capColor, 0.0028f, 1.446f, 1.14f, 0.691f);
			Color _lightColorNoFocus = AdjustColor(capColor, 0.00278f, 1.419f, 2.72f, 1.f);
			Color _lightColorWithFocus = AdjustColor(capColor, 0.0175f, 1.986f, 2.967f, 1.f);
			//Color _lightColor = AdjustColor(baseColor, 0.00278, 1.419f, 2.72f, 1.f);

			Color _lightColor = InterpolateColors(_lightColorNoFocus, _lightColorWithFocus, animationProgress);

			//Color _baseColor = AdjustColor(baseColor, -0.0022f, 1.f, 1.14f, 1.f);
			//Color _lightColor = AdjustColor(baseColor, 0.0133, 2.f, 3.39f, 1.f);

			//Color baseColor(255, 120, 40, 70);
			//Color lightColor(255, 255, 220, 230);

			float diagAngle = (float)(M_PI + M_PI_2 + m_Alpha);
			float sharpnessEdge = 10.0f;
			float sharpnessCenter = 2.0f;
			float highlightPower = 0.4f;

			const float rignsFactor = 0.75;
			float ringsCount = metallicGlareRadius * rignsFactor;      // <--- Количество колец (теперь удобно менять)
			float ringsFreq = (float)ringsCount;
			float ringsAmp = 0.06f;

			for (int _y = 0; _y < metallicGlareRadius * 2; ++_y)
			{
				for (int _x = 0; _x < metallicGlareRadius * 2; ++_x)
				{
					float dx = _x - metallicGlareRadius;
					float dy = _y - metallicGlareRadius;
					float dist = sqrt(dx * dx + dy * dy);

					if (dist > metallicGlareRadius) continue;

					float angle = atan2(dy, dx);

					float t = dist / metallicGlareRadius;
					float sharpness = sharpnessCenter + (sharpnessEdge - sharpnessCenter) * t;

					float highlight = pow(fabs(cos(angle - diagAngle)), sharpness);

					// --- Кольца ---
					float rings = sin(dist / metallicGlareRadius * ringsFreq * (float)M_PI) * ringsAmp;

					float finalHighlight = highlight * highlightPower + rings;

					// Ограничим диапазон
					if (finalHighlight < 0) finalHighlight = 0;
					if (finalHighlight > 1) finalHighlight = 1;

					BYTE r = BYTE(_baseColor.GetR() + finalHighlight * (_lightColor.GetR() - _baseColor.GetR()));
					BYTE gcol = BYTE(_baseColor.GetG() + finalHighlight * (_lightColor.GetG() - _baseColor.GetG()));
					BYTE b = BYTE(_baseColor.GetB() + finalHighlight * (_lightColor.GetB() - _baseColor.GetB()));

					bmp.SetPixel(_x, _y, Color(255, r, gcol, b));
				}
			}
			graphics.DrawImage(&bmp, x - metallicGlareRadius, y - metallicGlareRadius);

		}

		/*
		LinearGradientBrush baseBrush(
			innerRect,
			innerColor1,
			innerColor2,
			//Color(255, 180, 180, 180), // светло-серый
			//Color(255, 70, 70, 70),    // темно-серый
			LinearGradientMode::LinearGradientModeForwardDiagonal);

		graphics.FillEllipse(&baseBrush, innerRect);

		// Блик основного круга
		GraphicsPath glarePath;
		glarePath.AddEllipse(innerRect);

		PathGradientBrush highlightBrush(&glarePath);
		highlightBrush.SetCenterColor(AdjustBrightness(baseColor, 3.1f)); // светлый центр
	//	highlightBrush.SetCenterColor(Color(180, 255, 255, 255)); // светлый центр
		Color surroundColors[] = { Color(0, 0, 0, 0) };
		INT count = 1;
		highlightBrush.SetSurroundColors(surroundColors, &count);
		PointF center((REAL)x, (REAL)(y - innerRadius / 3));
		highlightBrush.SetCenterPoint(center);

		graphics.FillEllipse(&highlightBrush, innerRect);

		*/

		// Рисуем границу между ручной и внешним кольцом
		//DrawBlurredCircle(graphics, x, y, innerRadius, borderColor);
		DrawBevel(graphics, PointF(x, y), innerRadius, 3, capColor, m_Alpha * 180 / (float)M_PI - 90);
		graphics.DrawEllipse(&borderPen, innerRect);


		//Pen borderPen1(AdjustBrightness(baseColor, 1.2f), 1.0f); // Перо подсветки края ручки
		//Gdiplus::Rect innerRect1(innerRect.X + 1, innerRect.Y + 1, innerRect.Width - 2, innerRect.Height - 2);
	//	graphics.DrawEllipse(&borderPen1, innerRect1);




		//glarePath.Reverse();
		float indicatorR;
		float indicatorLen;
		if (IndicatorOnGear)
		{
			indicatorR = innerRadius * 4.f / 5.f; // (int)(size * 0.33);
			indicatorLen = innerRadius / 10.f;
		}
		else
		{
			indicatorR = innerRadius + (outerRadius - innerRadius) / 2; // (int)(size * 0.33);
			indicatorLen = (outerRadius - innerRadius - 2) / 2.f;
		}

		// --- Индикатор ---
		float indAngle = m_MinAngle - rotationAngle; // 135 градусов

		float ix = x + cos(indAngle) * indicatorR;
		float iy = y - sin(indAngle) * indicatorR;
		float indicatorSize = std::sqrt(innerRadius / 10.f);
		//SolidBrush indBrush(IndicatorColor/*Color(255, 255, 180, 40)*/);
		//graphics.FillEllipse(&indBrush, ix - indicatorSize, iy - indicatorSize, indicatorSize * 2, indicatorSize * 2);

		float x1 = x + cos(indAngle) * (indicatorR - indicatorLen / 2.f);
		float y1 = y - sin(indAngle) * (indicatorR - indicatorLen / 2.f);
		float x2 = x + cos(indAngle) * (indicatorR + indicatorLen / 2.f);
		float y2 = y - sin(indAngle) * (indicatorR + indicatorLen / 2.f);

		Pen indicatorRisk(IndicatorColor, rickThickness);
		graphics.DrawLine(&indicatorRisk, x1, y1, x2, y2);

	}

	/// <summary>
	/// Отрисовка шкалы. Нужно ввести параметры:
	/// 1.Сколько маленьких делений внутри больших
	/// 2.Рассояние между внутренним радиусом и началом шкалы
	/// 3.Сколько делений на шкале
	/// </summary>
	/// <param name="g"></param>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <param name="radius"></param>
	/// <param name="thickness"></param>
	void KnobControl::DrawScale(Graphics& g, float x, float y, float radius, float tickSize, float tickThickness, float angle)
	{
		float R_outer = radius; //size / 2;

		// --- Деления (вокруг ручки!) ---
		//int nTicks = 31;
		int nTicks = ScaleBigSteps * ScaleSmallSteps + 1;
		float tickLenShort = tickSize * 2.0f / 3.0f;
		float tickLenLong = tickSize;
		float R_tickStart = R_outer + 1; // чуть за пределами ободка
		Pen tickPenSort(ScaleColor/*Color(200, 20, 20, 20)*/, tickThickness / 2.f);
		Pen tickPenSortIndicated(IndicatorColor, tickThickness / 2.f);
		Pen tickPenLong(ScaleColor/*Color(255, 20, 20, 20)*/, tickThickness);
		Pen tickPenLongIndicated(IndicatorColor, tickThickness);

		// Рисуем риски шкалы
		for (int i = 0; i < nTicks; ++i) {
			float tickAngle = m_MinAngle - (2 * (float)M_PI - m_Alpha * 2) * i / (nTicks - 1); // Вычислить угол риски шкалы
			//int len = (i % 5 == 0) ? tickLenLong : tickLenShort;
			float len = (i % ScaleSmallSteps == 0) ? tickLenLong : tickLenShort;
			float x1 = x + cos(tickAngle) * (R_tickStart - len + tickLenLong + 2);
			float y1 = y - sin(tickAngle) * (R_tickStart - len + tickLenLong);
			float x2 = x + cos(tickAngle) * (R_tickStart + tickLenLong);
			float y2 = y - sin(tickAngle) * (R_tickStart + tickLenLong);
			Pen* tickPen;
			if ((i == 0 && roundToDecimals(angle) == roundToDecimals(tickAngle - m_MinAngle) || roundToDecimals(angle) == roundToDecimals(m_MinAngle - m_MaxAngle) && i == nTicks - 1) * !FullRuler ||
				FullRuler && roundToDecimals(angle) >= roundToDecimals(m_MinAngle - tickAngle))
			{
				//tickPen = ((i % 5 == 0) ? &tickPenLongIndicated : &tickPenSortIndicated);
				tickPen = ((i % ScaleSmallSteps == 0) ? &tickPenLongIndicated : &tickPenSortIndicated);
			}
			else
			{
				//tickPen = ((i % 5 == 0) ? &tickPenLong : &tickPenSort);
				tickPen = ((i % ScaleSmallSteps == 0) ? &tickPenLong : &tickPenSort);
			}

			g.DrawLine(tickPen, x1, y1, x2, y2);
		}
	}

	void KnobControl::DrawPartialRing2(Graphics& g, float x, float y, float radius, float size,
		Color activeColor, Color backgroundColor, float angle)
	{
		float R_outer = radius + size;
		float R_inner = radius;

		// --- Активная часть: от m_MinAngle до angle ---
		if (angle > 0)
		{
			DrawRingSector(g, x, y, R_outer, R_inner, m_MinAngle, m_MinAngle - angle, activeColor);
		}

		// --- Фоновая часть: от angle до m_MaxAngle ---
		//if (m_MinAngle - angle < m_MaxAngle)
		{
			DrawRingSector(g, x, y, R_outer, R_inner, m_MinAngle - angle, m_MaxAngle, backgroundColor);
		}
	}

	void KnobControl::DrawRingSector(Graphics& g, float x, float y, float R_outer, float R_inner,
		float startAngleRad, float endAngleRad, Color fillColor)
	{
		// Классические радианы -> GDI+ градусы:
		// 1. Инвертируем знак (против часовой -> по часовой)
		// 2. Переводим в градусы
		float startAngleDeg = (float)((2 * M_PI - startAngleRad) * 180.0 / M_PI);
		float endAngleDeg = (float)((2 * M_PI - endAngleRad) * 180.0 / M_PI);

		// Вычисление углового прохода (GDI+ по часовой)
		float sweepAngle = endAngleDeg - startAngleDeg;
		if (sweepAngle < 0) sweepAngle += 360.0;
		if (sweepAngle > 360.0) sweepAngle -= 360.0;
		// Создание пути кольцевого сектора
		GraphicsPath sectorPath;

		// 1. Внешняя дуга
		sectorPath.AddArc(x - R_outer, y - R_outer, R_outer * 2, R_outer * 2,
			startAngleDeg, sweepAngle);

		// 2. Линия к внутреннему радиусу (конец дуги)
		sectorPath.AddLine(
			(REAL)(x + R_outer * cos(endAngleRad)),
			(REAL)(y - R_outer * sin(endAngleRad)),
			(REAL)(x + R_inner * cos(endAngleRad)),
			(REAL)(y - R_inner * sin(endAngleRad)));

		// 3. Внутренняя дуга (обратно, отрицательный sweep)
		sectorPath.AddArc(x - R_inner, y - R_inner, R_inner * 2, R_inner * 2,
			endAngleDeg, -sweepAngle);

		// 4. Линия обратно к началу
		sectorPath.AddLine(
			(REAL)(x + R_inner * cos(startAngleRad)),
			(REAL)(y - R_inner * sin(startAngleRad)),
			(REAL)(x + R_outer * cos(startAngleRad)),
			(REAL)(y - R_outer * sin(startAngleRad)));

		sectorPath.CloseFigure();

		SolidBrush brush(fillColor);
		g.FillPath(&brush, &sectorPath);
	}

	void KnobControl::DrawPartialRing(Graphics& g, float x, float y, float radius, float thickness, Color fillColor)
	{
		// Если углы заданы в радианах и идут по часовой, 
		// переведем их к градусам и подготовим угловой проход

		// Углы в градусах для GDI+ (0 градусов — по оси X вправо, углы идут против часовой стрелки)
		// Учтем, что в вашей системе углы могут считать иначе, поэтому
		// делаем перевод:
		// Угол 0 в математике (радиан) соответствует 0 градусов,
		// но в GDI+ 0 градусов — по оси X вправо, и углы идут против часовой
		// Для корректного вращения используем отрицательные углы
		float R_outer = radius + thickness;
		float R_inner = radius;

		float startAngleDeg = (float)(- m_MinAngle * 180.0 / M_PI);
		float endAngleDeg = (float)(- m_MaxAngle * 180.0 / M_PI);

		// Вычислим Sweep (угловой проход)
		// Углы могут быть в разном порядке, поэтому
		float sweepAngle = endAngleDeg - startAngleDeg;
		if (sweepAngle <= 0)
			sweepAngle += 360.0;

		// Создадим GraphicsPath для кольцевого сектора
		GraphicsPath ringPath;

		// Внешняя дуга (по часовой стрелке)
		ringPath.AddArc(x - R_outer, y - R_outer, R_outer * 2, R_outer * 2, startAngleDeg, sweepAngle);
		// Линия к внутреннему радиусу в конце дуги
		float innerArcStartX = x + R_inner * cos(m_MaxAngle);
		float innerArcStartY = y - R_inner * sin(m_MaxAngle);
		ringPath.AddLine(
			(REAL)(x + R_outer * cos(m_MaxAngle)),
			(REAL)(y - R_outer * sin(m_MaxAngle)),
			innerArcStartX,
			innerArcStartY);
		// Внутренняя дуга (против часовой стрелки)
		// Но AddArc идет по часовой, поэтому рисуем с обратным sweep: -sweepAngle
		ringPath.AddArc(x - R_inner, y - R_inner, R_inner * 2, R_inner * 2, endAngleDeg, -sweepAngle);
		// Линия обратно к началу внешней дуги
		ringPath.AddLine(
			REAL(x + R_inner * cos(m_MinAngle)),
			REAL(y - R_inner * sin(m_MinAngle)),
			REAL(x + R_outer * cos(m_MinAngle)),
			REAL(y - R_outer * sin(m_MinAngle)));

		ringPath.CloseFigure();

		SolidBrush brush(fillColor);
		g.FillPath(&brush, &ringPath);
	}


	void KnobControl::Draw(bool hasFocus, bool drawSelected)
	{
		if (m_pContainer)
		{
			Graphics g(оffScreenBitmap.get());

			g.Clear(Color::Transparent);
			g.SetSmoothingMode(SmoothingModeHighQuality);

			if (Selected && !drawSelected) return;

			// Пример: рисуем ручку размером 200x200 в центре
			float size = (float)m_Size; // min(clientRect.Width(), clientRect.Height()) - 40;
			float x = size / 2;
			float y = size / 2;

			//double metallicCirclePercent = 65.0f;
			//double metallicCirclePercent = 65.0 + (55.0 - 65.0) * m_FocusAnimationProgress;
			//double metallicRingPercent = 15.0f;
			//double scalePercent = 100.0 - metallicCirclePercent - metallicRingPercent;

			float scalePercent = 100.0f - CapPercent - GearPercent;

			float radius = size / 2.0f;
			float capRadiusOriginal = radius * CapPercent / 100.f;
			float capRadiusRedution = capRadiusOriginal * CapReductionPercent * m_FocusAnimationProgress / 100.f;
			float capRadius = capRadiusOriginal - capRadiusRedution;
			float ringThickness = radius * GearPercent / 100.f + capRadiusRedution;
			float scaleTickSize = radius * scalePercent / 100.f;
			float scaleTickThickness = scaleTickSize / 10.f;

			//int metallicCircleRadius = size / 2.0 * metallicCirclePercent / 100.0;
			//int ringThickness = size / 2.0 * metallicRingPercent / 100.0;
			//int scaleThickness = size / 2.0 * scalePercent / 100.0;

			float angle = (float)((M_PI * 2 - 2 * m_Alpha) * ((m_Value - m_MinVal) / (m_MaxVal - m_MinVal)));

			// Шкала или заливка
			if (Scale)
			{
				DrawScale(g, x, y, capRadius + ringThickness, scaleTickSize, scaleTickThickness, angle);
			}
			else
			{
				//DrawPartialRing(memGraphics, x, y, metallicCircleRadius + ringThickness, scaleThickness, Color(180, 124, 176, 255));
				DrawPartialRing2(g, x, y, capRadius, scaleTickSize + ringThickness, Color(180, 124, 176, 255), Color(255, 100, 97, 95), angle);
				//DrawPartialRing2(memGraphics, x, y, capRadius + ringThickness, scaleThickness, Color(180, 124, 176, 255), Color(255, 100, 97, 95), angle);
			}
			// Сначала рисуем кольцо с выемками
			DrawMetallicRingWithCircularNotches(g, x, y, capRadius, ringThickness, scaleTickThickness, m_NotchCount, angle, m_FocusAnimationProgress);
			// Нарисовать внешнюю рамку
			DrawBorder2(g);
		}
	}
}


