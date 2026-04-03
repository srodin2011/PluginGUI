#include "PluginGUI\include\framework.h"
#include "PluginGUI\include\Control.h"
#include "PluginGUI\include\PluginView.h"
#include "PluginGUI\include\Animation.h"

//#include "ScalableControl.h"
//#include <afxdrawmanager.h>
#include <algorithm>

using namespace Gdiplus;

namespace PluginGUI 
{
	/// <summary>
	/// Инициализация элемента управления
	/// </summary>
	/// <param name="pluginView"></param>
	/// <param name="border"></param>
	Control::Control(const CRect& border)
	{
		Init();
		Border = Frame(border);
	}

	Control::Control()
	{
		Init();
	}

	Control::~Control()
	{
	}

	/// <summary>
	/// Инициализзация объектов элемента управления
	/// </summary>
	void Control::Init()
	{
		Border = Frame(0, 0, 0, 0);

		// Подписываемся на событие - изменение границ контрола
		BorderChanged.Subscribe([&](const Frame& frame)
			{
				if (оffScreenBitmap &&
					оffScreenBitmap->GetWidth() == frame.Width() &&
					оffScreenBitmap->GetHeight() == frame.Height())
				{
					m_isDirty = false;
					return;  // Размеры совпадают - НЕ пересоздаём!
				}

				if (оffScreenBitmap)
				{
					оffScreenBitmap.reset();
				}
				оffScreenBitmap = std::make_unique<Gdiplus::Bitmap>(frame.Width(), frame.Height(), PixelFormat32bppARGB);
			}
		);

		// Подписываемся на событие - изменение фокуса контрола
		FocusChanged.Subscribe([&](bool focus)
			{
				OnFocusChanged(focus);
			}
		);

		m_FocusAnimationProgress = 0.0f;

		m_FocusAnimation = std::make_unique<Animation>(
			// Начать 
			[&]() {
			},
			// Обновить
			[&](float progrees) {
				m_FocusAnimationProgress = progrees;
			},
			// Остановить
			[&]() {
			});

		// Задаем функцию инвалидации по умолчанию
		m_FocusAnimation->SetInvalidateCallback([&]() {
			Invalidate();
			});
	}

	/// <summary>
	/// Изменить границы элемента управления. Влечет за собой пересоздание подложки для отрисовки
	/// </summary>
	/// <param name="border"></param>
	//void Control::SetBorder(const Frame& border_)
	//{
	//	if (Border != border_)
	//	{
	//		Border = border_;
	//		if (оffScreenBitmap)
	//		{
	//			оffScreenBitmap.reset();
	//		}
	//		оffScreenBitmap = std::make_unique<Gdiplus::Bitmap>(border_.Width(), border_.Height(), PixelFormat32bppARGB);
	//	}
	//}

	/// <summary>
	/// Обработчик смены фокуса
	/// </summary>
	/// <param name="focused">Получаем или теряем фокус</param>
	void Control::OnFocusChanged(bool focused)
	{
		if (AnimateOnFocusChange())
		{
			m_FocusAnimation->StartAnimation(focused);
		}
	}

	/// <summary>
	/// Отрисовать элемет управления
	/// </summary>
	/// <param name="dc"></param>
	void Control::Draw(bool drawSelected)
	{
		Draw(m_HasFocus, drawSelected);
		m_isDirty = false; // Сбрасываем только после перерисовки
	}

	///// <summary>
	///// Изменить фокус элемента упрвления
	///// </summary>
	///// <param name="focused"></param>
	//void Control::SetFocus(bool focused)
	//{
	//	OnFocusChanged(focused);
	//	m_HasFocus = focused;
	//}

	/// <summary>
	/// Реакция на перемещение мышки
	/// </summary>
	/// <param name="p">Новая позиция мышки</param>
	void Control::OnMouseMove(UINT nFlags, CPoint p)
	{
	}

	/// <summary>
	/// Реакция на поворот колесика мышки
	/// </summary>
	/// <param name="nFlags"></param>
	/// <param name="zDelta"></param>
	/// <param name="p"></param>
	/// <returns></returns>
	BOOL Control::OnMouseWheel(UINT nFlags, short zDelta, CPoint p)
	{
		return false;
	}

	/// <summary>
	/// Реакция на нажатие левой кнопки мышки
	/// </summary>
	/// <param name=""></param>
	/// <param name="p"></param>
	void Control::OnLButtonDown(UINT, CPoint p)
	{
		if (m_pContainer)
		{
			m_pContainer->ShowTooltipForControl(this);
		}
	}

	/// <summary>
	/// Реакция на отпускание левой кнопки мышки
	/// </summary>
	/// <param name=""></param>
	/// <param name=""></param>
	void Control::OnLButtonUp(UINT, CPoint)
	{
		if (m_pContainer)
		{
			m_pContainer->HideCurrentTooltip();
		}
	}

	/// <summary>
	/// Реакция на двойной щелчек левой кнопкой мышки
	/// </summary>
	/// <param name="nFlags"></param>
	/// <param name="point"></param>
	void Control::OnLButtonDblClk(UINT nFlags, CPoint point)
	{
	}

	void Control::OnMouseLeave()
	{
	}

	void Control::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
	}

	void Control::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
	}

	void Control::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
	}

	void Control::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
	}

	void Control::onMouseMoveAndLButtonDown(UINT nFlags, LONG dx, LONG dy)
	{
	}

	BOOL Control::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return TRUE;
	}

	void Control::OnContextMenu(CPoint point)
	{
	}

	void Control::OnUpdateCommand(CCmdUI* pCmdUI)
	{
	}

	void Control::OnCommand(UINT nID)
	{
	}


	/// <summary>
	/// Отрисовать внешнюю рамку для показа и изменения позиции и/или размеров элемента управления
	/// </summary>
	/// <param name="g">Ссылка на Graphics</param>
	void Control::DrawBorder2(Graphics& g)
	{
		if (Selected)
		{
			// Рисуем внешнюю рамку
			Pen borderPen(m_BorderColor, 1.f);
			//g.SetSmoothingMode(SmoothingModeNone);
			//borderPen.SetAlignment(Gdiplus::PenAlignmentInset);
			g.DrawRectangle(&borderPen, 0.f, 0.f, (float)Border.Width() - 1, (float)Border.Height() - 1);
			//g.DrawRectangle(&borderPen, 0, 0, Border.Width() - 1, Border.Height() - 1);

			// Рисуем правый нижний уголок рамки
			SolidBrush brush(m_BorderColor);
			GraphicsPath path;
			Gdiplus::Point points[] =
			{
				Gdiplus::Point(Border.Width() - 1, Border.Height() - 1),
				Gdiplus::Point(Border.Width() - 1 - Border.CornerSize, Border.Height() - 1),
				Gdiplus::Point(Border.Width() - 1, Border.Height() - 1 - Border.CornerSize),
			};
			path.AddLines(points, 3);
			g.FillPath(&brush, &path);
		}
	}

	/// <summary>
	/// Не используем!!!
	/// </summary>
	/// <param name="dc"></param>
	void Control::DrawBitmap(const CPaintDC& dc)
	{
		if (!оffScreenBitmap)
			return;

		UINT width = оffScreenBitmap->GetWidth();
		UINT height = оffScreenBitmap->GetHeight();

		// Создаём 32-битный DIBSection с альфа-каналом
		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = -(LONG)height; // отрицательное для верхнего левого угла
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32; // 32 бита на пиксель
		bmi.bmiHeader.biCompression = BI_RGB;

		void* pBits = nullptr;
		CPaintDC& _dc = const_cast<CPaintDC&>(dc);

		CDC memDC;
		memDC.CreateCompatibleDC(&_dc);

		HBITMAP hDIB = CreateDIBSection(memDC.GetSafeHdc(), &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
		if (!hDIB || !pBits)
			return;

		HBITMAP hOldBitmap = (HBITMAP)memDC.SelectObject(hDIB);

		// Копируем данные из Gdiplus::Bitmap в DIBSection
		Gdiplus::BitmapData bitmapData;
		Gdiplus::Rect rect(0, 0, width, height);

		if (оffScreenBitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) == Gdiplus::Ok)
		{
			BYTE* srcPixels = (BYTE*)bitmapData.Scan0;
			BYTE* dstPixels = (BYTE*)pBits;
			int srcStride = bitmapData.Stride;
			int dstStride = width * 4;

			for (UINT y = 0; y < height; y++)
			{
				BYTE* srcRow = srcPixels + y * srcStride;
				BYTE* dstRow = dstPixels + y * dstStride;

				for (UINT x = 0; x < width; x++)
				{
					BYTE B = srcRow[x * 4 + 0];
					BYTE G = srcRow[x * 4 + 1];
					BYTE R = srcRow[x * 4 + 2];
					BYTE A = srcRow[x * 4 + 3];

					float alpha = A / 255.0f;

					// premultiply alpha
					BYTE Rp = static_cast<BYTE>(R * alpha);
					BYTE Gp = static_cast<BYTE>(G * alpha);
					BYTE Bp = static_cast<BYTE>(B * alpha);

					dstRow[x * 4 + 0] = Bp; // Blue
					dstRow[x * 4 + 1] = Gp; // Green
					dstRow[x * 4 + 2] = Rp; // Red
					dstRow[x * 4 + 3] = A;  // Alpha
				}
			}
			// Копируем построчно, учитывая возможный stride
			//for (UINT y = 0; y < height; y++)
			//{
			//	memcpy(dstPixels + y * dstStride, srcPixels + y * srcStride, dstStride);
			//}

			оffScreenBitmap->UnlockBits(&bitmapData);
		}
		else
		{
			// Если не удалось заблокировать, очистим DIB прозрачным цветом
			memset(pBits, 0, static_cast<size_t>(width * height * 4));
		}

		// Выводим с помощью AlphaBlend
		BLENDFUNCTION blend = { 0 };
		blend.BlendOp = AC_SRC_OVER;
		blend.SourceConstantAlpha = 255;
		blend.AlphaFormat = AC_SRC_ALPHA;

		_dc.AlphaBlend(
			Border.left, Border.top,
			Border.Width(), Border.Height(),
			&memDC,
			0, 0,
			width, height,
			blend);

		// Освобождаем ресурсы
		memDC.SelectObject(hOldBitmap);
		DeleteObject(hDIB);
		//HBITMAP hBitmap = NULL;
		//Gdiplus::Status status = m_OffScreenBitmap->GetHBITMAP(Gdiplus::Color::Transparent, &hBitmap);
		//if (status == Gdiplus::Ok && hBitmap != NULL)
		//{
		//	CPaintDC& _dc = const_cast<CPaintDC&>(dc);
		//	// Создаём совместимый DC
		//	CDC memDC;
		//	memDC.CreateCompatibleDC(&_dc);

		//	// Выбираем HBITMAP в memDC
		//	HBITMAP hOldBitmap = (HBITMAP)memDC.SelectObject(hBitmap);

		//	BLENDFUNCTION blend = { 0 };
		//	blend.BlendOp = AC_SRC_OVER;
		//	blend.SourceConstantAlpha = 255;
		//	blend.AlphaFormat = AC_SRC_ALPHA;

		//	_dc.AlphaBlend(
		//		m_Border.left, m_Border.top,
		//		m_Border.Width(), m_Border.Height(),
		//		&memDC,
		//		0, 0,
		//		m_Border.Width(), m_Border.Height(),
		//		blend);

		//	// Копируем из memDC в pDC
		//	//_dc.BitBlt((INT)m_Border.left, (INT)m_Border.top, (INT)m_Border.Width(), (INT)m_Border.Height(), &memDC, 0, 0, SRCCOPY);

		//	// Восстанавливаем старый битмап и удаляем созданный
		//	memDC.SelectObject(hOldBitmap);
		//	DeleteObject(hBitmap);
		//}
	}

	void Control::SetTooltipText(const std::wstring& text)
	{
		m_tooltipText = text;
		if (m_pContainer)
		{
			m_pContainer->UpdateTooltipForCurrentControl(this);
		}
	}

	void Control::HideTooltip()
	{
		if (m_pContainer)
		{
			m_pContainer->HideCurrentTooltip();
		}
	}

	// Определение статической переменной
	//std::unordered_map<UINT_PTR, PlugingControl*> PlugingControl::m_TimerInstanceMap;

	// Структура для HSL
	struct HSL {
		float h; // Hue [0..1]
		float s; // Saturation [0..1]
		float l; // Lightness [0..1]
	};

	// Конвертация из Color в HSL
	static HSL ColorToHSL(const Gdiplus::Color& color)
	{
		HSL hsl{};

		// Нормируем RGB в [0..1]
		float r = color.GetR() / 255.0f;
		float g = color.GetG() / 255.0f;
		float b = color.GetB() / 255.0f;

		float max = r > g ? (r > b ? r : b) : (g > b ? g : b);
		float min = r < g ? (r < b ? r : b) : (g < b ? g : b);
		float delta = max - min;

		// Lightness
		hsl.l = (max + min) / 2.0f;

		if (delta == 0.0f)
		{
			// Оттенок и насыщенность равны 0 для оттенков серого
			hsl.h = 0.0f;
			hsl.s = 0.0f;
			return hsl;
		}

		// Saturation
		if (hsl.l < 0.5f)
			hsl.s = delta / (max + min);
		else
			hsl.s = delta / (2.0f - max - min);

		// Hue
		if (max == r)
			hsl.h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
		else if (max == g)
			hsl.h = (b - r) / delta + 2.0f;
		else // max == b
			hsl.h = (r - g) / delta + 4.0f;

		hsl.h /= 6.0f; // Нормируем в [0..1]

		return hsl;
	}

	// Вспомогательная функция для HSL->RGB
	float HueToRGB(float p, float q, float t)
	{
		if (t < 0.f) t += 1.f;
		if (t > 1.f) t -= 1.f;
		if (t < 1.f / 6.f) return p + (q - p) * 6.f * t;
		if (t < 1.f / 2.f) return q;
		if (t < 2.f / 3.f) return p + (q - p) * (2.f / 3.f - t) * 6.f;
		return p;
	}

	// Конвертация из HSL в Color (с альфой из исходного цвета)
	static Gdiplus::Color HSLToColor(const HSL& hsl, BYTE alpha = 255)
	{
		float r, g, b;

		if (hsl.s == 0.0f)
		{
			// Оттенки серого
			r = g = b = hsl.l;
		}
		else
		{
			float q = (hsl.l < 0.5f) ? (hsl.l * (1.0f + hsl.s)) : (hsl.l + hsl.s - hsl.l * hsl.s);
			float p = 2.0f * hsl.l - q;

			r = HueToRGB(p, q, hsl.h + 1.0f / 3.0f);
			g = HueToRGB(p, q, hsl.h);
			b = HueToRGB(p, q, hsl.h - 1.0f / 3.0f);
		}

		// Приводим к диапазону 0..255 с округлением и ограничением
		BYTE R = static_cast<BYTE>(std::clamp(std::round(r * 255.0f), 0.0f, 255.0f));
		BYTE G = static_cast<BYTE>(std::clamp(std::round(g * 255.0f), 0.0f, 255.0f));
		BYTE B = static_cast<BYTE>(std::clamp(std::round(b * 255.0f), 0.0f, 255.0f));

		return Gdiplus::Color(alpha, R, G, B);
	}

	// Функция изменения яркости
	Gdiplus::Color AdjustBrightness(const Gdiplus::Color& color, float brightnessFactor)
	{
		HSL hsl = ColorToHSL(color);
		hsl.l *= brightnessFactor;
		if (hsl.l > 1.f) hsl.l = 1.f;
		if (hsl.l < 0.f) hsl.l = 0.f;
		return HSLToColor(hsl, color.GetA());
	}

	Gdiplus::Color AdjustColor(const Gdiplus::Color& color, float hueShift, float saturationValue, float lightnessValue, float alpha)
	{
		HSL hsl = ColorToHSL(color);

		// Применяем Hue с нормализацией
		hsl.h += hueShift;
		hsl.h = fmodf(hsl.h, 1.0f);
		if (hsl.h < 0.0f)
			hsl.h += 1.0f;

		// Применяем Saturation
		if (hsl.s == 0.0f)
			hsl.s = saturationValue; // абсолютное значение
		else
			hsl.s *= saturationValue; // если хотите, можно сделать смешивание

		// Применяем Lightness
		if (hsl.l == 0.0f)
			hsl.l = lightnessValue; // абсолютное значение
		else
			hsl.l *= lightnessValue;

		// Ограничиваем Saturation и Lightness
	#if __cplusplus >= 201703L
		hsl.s = std::clamp(hsl.s, 0.0f, 1.0f);
		hsl.l = std::clamp(hsl.l, 0.0f, 1.0f);
	#else
		if (hsl.s > 1.f) hsl.s = 1.f;
		else if (hsl.s < 0.f) hsl.s = 0.f;

		if (hsl.l > 1.f) hsl.l = 1.f;
		else if (hsl.l < 0.f) hsl.l = 0.f;
	#endif

		// Корректируем Alpha
		int a = static_cast<int>(color.GetA() * alpha + 0.5f);
		if (a > 255) a = 255;
		if (a < 0) a = 0;
		BYTE newAlpha = static_cast<BYTE>(a);

		return HSLToColor(hsl, newAlpha);
	}

	// Функция для вычисления разницы Hue с учётом цикличности
	float HueDifference(float h1, float h2)
	{
		float diff = h2 - h1;
		if (diff > 0.5f)
			diff -= 1.0f;
		else if (diff < -0.5f)
			diff += 1.0f;
		return diff;
	}

	// Функция интерполяции цвета в HSL и Alpha
	Gdiplus::Color InterpolateColors(const Gdiplus::Color& c1, const Gdiplus::Color& c2, float t)
	{
		if (t <= 0.0f) return c2;
		if (t >= 1.0f) return c1;

		// Переводим цвета в HSL
		HSL hsl1 = ColorToHSL(c1);
		HSL hsl2 = ColorToHSL(c2);

		// Интерполяция Hue с учётом цикличности
		float hueDiff = HueDifference(hsl1.h, hsl2.h);
		float hueInterp = hsl1.h + hueDiff * t;
		hueInterp = fmod(hueInterp + 1.0f, 1.0f); // Корректировка в [0..1)

		// Линейная интерполяция Saturation и Lightness с ограничениями
		float satInterp = std::clamp(hsl1.s + (hsl2.s - hsl1.s) * t, 0.0f, 1.0f);
		float lightInterp = std::clamp(hsl1.l + (hsl2.l - hsl1.l) * t, 0.0f, 1.0f);

		// Интерполяция Alpha
		float alpha1 = c1.GetA() / 255.0f;
		float alpha2 = c2.GetA() / 255.0f;
		float alphaInterp = std::clamp(alpha1 + (alpha2 - alpha1) * t, 0.0f, 1.0f);

		// Вычисляем сдвиг Hue относительно исходного цвета
		float hueShift = HueDifference(hsl1.h, hueInterp);

		// Для насыщенности и яркости передаём абсолютные значения (не множители)
		// Предполагается, что AdjustColor умеет работать с абсолютными значениями
		// Если нет - нужно изменить AdjustColor или интерфейс

		// Чтобы сохранить интерфейс с множителями, можно сделать:
		float saturationFactor = (hsl1.s > 0.001f) ? (satInterp / hsl1.s) : satInterp;
		float lightnessFactor = (hsl1.l > 0.001f) ? (lightInterp / hsl1.l) : lightInterp;

		// Ограничиваем разумные пределы
		saturationFactor = std::clamp(saturationFactor, 0.0f, 5.0f);
		lightnessFactor = std::clamp(lightnessFactor, 0.0f, 5.0f);

		return AdjustColor(c1, hueShift, saturationFactor, lightnessFactor, alphaInterp);
	}

	// Работа со свойствами

	//enum class Names
	//{
	//	pnBaseColor,
	//	pnBorder,
	//	pnName,
	//	pnFocus,
	//	pnValueChangeMode,
	//	pnSelected,
	//	pnLastName
	//};

	Variant Control::doGetPropertyValue(Id id) const
	{
		switch (id)
		{
			case static_cast<int>(PropertyName::pnBaseColor):
				return BaseColor;

			case static_cast<int>(PropertyName::pnBorder):
				return Border;

			case static_cast<int>(PropertyName::pnName):
				return Name;

			case static_cast<int>(PropertyName::pnFocus):
				return Focus;

			case static_cast<int>(PropertyName::pnValueChangeMode):
				return ValueChangeMode;

			case static_cast<int>(PropertyName::pnSelected):
				return Selected;

			default:
				// Если класс‑наследник не знает id, можем либо вернуть дефолт, либо бросить
				return Variant{};   // или throw std::invalid_argument
		}
	}

	bool Control::doSetPropertyValue(Id id, const Variant& value)
	{
		return std::visit(
			[this, id](const auto& v) -> bool
			{
				using T = std::decay_t<decltype(v)>;

				switch (id)
				{
					case static_cast<int>(PropertyName::pnBaseColor):
						if constexpr (std::is_same_v<T, Gdiplus::Color>)
						{
							BaseColor = v;
							return true;
						}
						return false;

					case static_cast<int>(PropertyName::pnBorder):
						if constexpr (std::is_same_v<T, Frame>)
						{
							Border = v;
							return true;
						}
						return false;

					case static_cast<int>(PropertyName::pnName):
						if constexpr (std::is_same_v<T, std::wstring>)
						{
							Name = v;
							return true;
						}
						return false;

					case static_cast<int>(PropertyName::pnFocus):
						if constexpr (std::is_same_v<T, bool>)
						{
							Focus = v;
							return true;
						}
						return false;

					case static_cast<int>(PropertyName::pnValueChangeMode):
						if constexpr (std::is_same_v<T, bool>)
						{
							ValueChangeMode = v;
							return true;
						}
						return false;

					case static_cast<int>(PropertyName::pnSelected):
						if constexpr (std::is_same_v<T, bool>)
						{
							Selected = v;
							return true;
						}
						return false;

					default:
						return false;
				}
			},
			value
		);
	}

	Variant Control::GetPropertyValue(Id id) const
	{
		const PropertyInfo* pPropInfo = FindProperty(id);
		if (!pPropInfo)
			throw std::invalid_argument("Invalid property id");

		switch (pPropInfo->m_type)
		{
			case TypeId::Bool:
				return GetPropertyValue<bool>(id);
			case TypeId::Color:
				return GetPropertyValue<Gdiplus::Color>(id);
			case TypeId::WString:
				return GetPropertyValue<std::wstring>(id);
			case TypeId::Int:
				return GetPropertyValue<int>(id);
			case TypeId::Float:
				return GetPropertyValue<float>(id);
			case TypeId::Frame:
				return GetPropertyValue<Frame>(id);
			default:
				throw std::invalid_argument("Unknown property type");
		}
	}
}