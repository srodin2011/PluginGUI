#include "PluginGUI\include\ButtonControl.h"
#include "PluginGUI\include\PluginView.h"
#include "PluginGUI\include\Utils.h"

using namespace Gdiplus;

namespace PluginGUI
{
    /// <summary>
    /// Конструктор класса
    /// </summary>
    /// <param name="pluginView">Ссылка плагин</param>
    /// <param name="border">Границы элемента управления</param>
    ButtonControl::ButtonControl(const CRect& Border) :
        Control(Border)
    {
    }

    void ButtonControl::Init()
    {
    }

    // Функция рисования кнопки
    // isPressed - состояние кнопки (нажата или нет)
    // rect - прямоугольник кнопки
    // text - текст кнопки
    static void DrawButton(Graphics& g, const RectF& rect, const WCHAR* text, bool isPressed)
    {
        Color borderColor(255, 30, 60, 90);
        Color shadowColor(100, 0, 0, 0);
        Color textColor(255, 255, 255, 255);

        int radius = 10;
        GraphicsPath path;
        CreateRoundedRectPath(path, rect, radius);

        // Создаем градиент для 3D эффекта
        Color topColor = isPressed ? Color(255, 60, 90, 130) : Color(255, 120, 170, 220);
        Color bottomColor = isPressed ? Color(255, 30, 50, 90) : Color(255, 70, 130, 180);

        LinearGradientBrush brush(rect, topColor, bottomColor, LinearGradientModeVertical);

        // Рисуем тень (вдавленность)
        if (isPressed)
        {
            SolidBrush shadowBrush(shadowColor);
            RectF shadowRect(rect.X + 2, rect.Y + 2, rect.Width, rect.Height);
            GraphicsPath shadowPath;
            CreateRoundedRectPath(shadowPath, shadowRect, radius);
            g.FillPath(&shadowBrush, &shadowPath);
        }

        // Заполняем кнопку градиентом
        g.FillPath(&brush, &path);

        // Рисуем рамку
        Pen borderPen(borderColor, 2);
        g.DrawPath(&borderPen, &path);

        // Рисуем текст
        FontFamily fontFamily(L"Segoe UI");
        Gdiplus::Font font(&fontFamily, 14, FontStyleRegular, UnitPixel);
        SolidBrush textBrush(textColor);

        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);

        PointF offset = isPressed ? PointF(1.0f, 1.0f) : PointF(0.0f, 0.0f);

        RectF layoutRect(
            (REAL)rect.X + offset.X,
            (REAL)rect.Y + offset.Y,
            (REAL)rect.Width,
            (REAL)rect.Height);

        g.DrawString(text, -1, &font, layoutRect, &format, &textBrush);
        //// Цвета для состояний
        //Color bgColorNormal(255, 70, 130, 180);      // Синий светлый
        //Color bgColorPressed(255, 40, 90, 130);      // Синий темный
        //Color borderColorNormal(255, 30, 60, 90);    // Темно-синий
        //Color borderColorPressed(255, 20, 40, 60);   // Еще темнее
        //Color textColor(255, 255, 255, 255);         // Белый

        //// Выбор цвета в зависимости от состояния
        //Color bgColor = isPressed ? bgColorPressed : bgColorNormal;
        //Color borderColor = isPressed ? borderColorPressed : borderColorNormal;

        //// Заливка фона
        //SolidBrush bgBrush(bgColor);
        //g.FillRectangle(&bgBrush, rect);

        //// Рисуем рамку
        //Pen borderPen(borderColor, 2);
        //g.DrawRectangle(&borderPen, rect);

        //// Рисуем текст
        //FontFamily fontFamily(L"Segoe UI");
        //Gdiplus::Font font(&fontFamily, 14, FontStyleRegular, UnitPixel);
        //SolidBrush textBrush(textColor);

        //// Вычисляем позицию текста (центрируем)
        //RectF layoutRect(
        //    (REAL)0,
        //    (REAL)0,
        //    (REAL)rect.Width-1,
        //    (REAL)rect.Height-1);

        //StringFormat format;
        //format.SetAlignment(StringAlignmentCenter);
        //format.SetLineAlignment(StringAlignmentCenter);

        //// Если кнопка нажата, сдвигаем текст на 1 пиксель вниз и вправо для эффекта вдавленности
        //PointF offset = isPressed ? PointF(1.0f, 1.0f) : PointF(0.0f, 0.0f);

        //RectF textRect = layoutRect;
        //textRect.X += offset.X;
        //textRect.Y += offset.Y;

        //g.DrawString(text, -1, &font, textRect, &format, &textBrush);
    }
    //void ButtonControl::DrawCustomCheckBox(
    //    Graphics& g,               // Контекст GDI+
    //    int width, int height,     // Общий размер области
    //    const WCHAR* text,         // Текст
    //    Color highlightColor       // Цвет подсветки кнопки
    //)
    //{
    //    // Размеры "кнопки"
    //    int buttonWidth = 10;
    //    int buttonHeight = 18;
    //    int buttonY = (height - buttonHeight) / 2;
    //    int buttonX = 4;

    //    //// Рисуем фон (тёмно-серый)
    //    //SolidBrush bgBrush(Color(255, 62, 71, 82)); // #3E4752
    //    //g.FillRectangle(&bgBrush, x, y, width, height);

    //    // Рисуем кнопку с подсветкой
    //    SolidBrush buttonBrush(highlightColor);
    //    g.FillRectangle(&buttonBrush, buttonX, buttonY, buttonWidth, buttonHeight);

    //    // Чёрная рамка вокруг кнопки
    //    Pen buttonBorderPen(Color(255, 0, 0, 0), 1);
    //    g.DrawRectangle(&buttonBorderPen, buttonX, buttonY, buttonWidth, buttonHeight);

    //    // Рамка, ограничивающая контрол 
    //    if (GetDrawBorder())
    //    {
    //        Pen borderPen(m_BorderColor, 1);
    //        g.DrawRectangle(&borderPen, 0, 0, width - 1, height - 1);
    //    }

    //    // Рисуем текст справа от кнопки
    //    FontFamily fontFamily(L"Segoe UI");
    //    Gdiplus::Font font(&fontFamily, 12, FontStyleRegular, UnitPixel);
    //    SolidBrush textBrush(Color(255, 255, 255, 255)); // Белый
    //    int textX = buttonX + buttonWidth + 8;
    //    int textY = (height - 16) / 2; // Вертикальное выравнивание

    //    g.DrawString(text, -1, &font, PointF((REAL)textX, (REAL)textY), &textBrush);
    //}

    //void ButtonControl::NewFunction(const CPaintDC& dc)
    //{
    //    HBITMAP hBitmap = NULL;
    //    Status status = m_OffScreenBitmap->GetHBITMAP(Color::Transparent, &hBitmap);
    //    if (status == Ok && hBitmap != NULL)
    //    {
    //        CPaintDC& _dc = const_cast<CPaintDC&>(dc);
    //        // Создаём совместимый DC
    //        CDC memDC;
    //        memDC.CreateCompatibleDC(&_dc);

    //        // Выбираем HBITMAP в memDC
    //        HBITMAP hOldBitmap = (HBITMAP)memDC.SelectObject(hBitmap);

    //        // Копируем из memDC в pDC
    //        _dc.BitBlt((INT)m_Border.left, (INT)m_Border.top, (INT)m_Border.Width(), (INT)m_Border.Height(), &memDC, 0, 0, SRCCOPY);

    //        // Восстанавливаем старый битмап и удаляем созданный
    //        memDC.SelectObject(hOldBitmap);
    //        DeleteObject(hBitmap);
    //    }
    //}

    void ButtonControl::Draw(bool hasFocus, bool drawSelected)
    {
        if (m_pContainer)
        {
            Graphics g(оffScreenBitmap.get());

            g.Clear(Color::Transparent);
            g.SetSmoothingMode(SmoothingModeHighQuality/*SmoothingModeAntiAlias*/);

            if (Selected && !drawSelected) return;

            RectF border_(0.f, 0.f, (float)Border.Width() - 1, (float)Border.Height() - 1);

            DrawButton(g, border_, Text.c_str(), On);
            DrawBorder2(g);
        }
    }

    void ButtonControl::OnLButtonDown(UINT, CPoint p)
    {
        On = !On;
        Invalidate();
    }

    //------------------------------------------------------------------------------
    // Работа со свойствами 

    Variant ButtonControl::doGetPropertyValue(Id id) const
    {
        switch (id)
        {
            case static_cast<int>(PropertyName::pnOn):
                return On;

            case static_cast<int>(PropertyName::pnOnColor):
                return OnColor;

            case static_cast<int>(PropertyName::pnOffColor):
                return OffColor;

            case static_cast<int>(PropertyName::pnTextColor):
                return TextColor;

            case static_cast<int>(PropertyName::pnText):
                return Text;

            default:
                return Base::doGetPropertyValue(id);   
        }
    }

    bool ButtonControl::doSetPropertyValue(Id id, const Variant& value)
    {
        return std::visit(
            [this, id](const auto& v) -> bool
            {
                using T = std::decay_t<decltype(v)>;

                switch (id)
                {
                    case static_cast<int>(PropertyName::pnOn):
                        if constexpr (std::is_same_v<T, bool>)
                        {
                            On = v;
                            return true;
                        }
                        return false;

                    case static_cast<int>(PropertyName::pnOnColor):
                        if constexpr (std::is_same_v<T, Gdiplus::Color>)
                        {
                            OnColor = v;
                            return true;
                        }
                        return false;

                    case static_cast<int>(PropertyName::pnOffColor):
                        if constexpr (std::is_same_v<T, Gdiplus::Color>)
                        {
                            OffColor = v;
                            return true;
                        }
                        return false;

                    case static_cast<int>(PropertyName::pnTextColor):
                        if constexpr (std::is_same_v<T, Gdiplus::Color>)
                        {
                            TextColor = v;
                            return true;
                        }
                        return false;

                    case static_cast<int>(PropertyName::pnText):
                        if constexpr (std::is_same_v<T, std::wstring>)
                        {
                            Text = v;
                            return true;
                        }
                        return false;

                    default:
                        return Base::doSetPropertyValue(id, v);
                }
            },
            value
        );
    }

}