#include "PluginGUI\include\ButtonImage.h"
#include "PluginGUI\include\PluginView.h"

using namespace Gdiplus;

namespace PluginGUI
{
    PLUGINGUI_PROPERTY_TABLE_INSTANTIATE(ButtonImage)

    ButtonImage::ButtonImage(const CRect& border) : Control(border)
    {
        Init();
    }

    void ButtonImage::Init()
    {
        ImagePathChanged.Subscribe([&](std::wstring path)
            {
                LoadImage(path);
            }
        );

        m_HoverBgColor = Color(255, 200, 220, 255); // Светло-голубой фон при hover
    }

    void ButtonImage::CleanupImage()
    {
        if (m_pImage)
        {
            delete m_pImage;
            m_pImage = nullptr;
        }
    }

    void ButtonImage::LoadImage(const std::wstring& path)
    {
        CleanupImage();

        if (!path.empty())
        {
            m_pImage = new Image(path.c_str());
            if (m_pImage->GetLastStatus() != Ok)
            {
                CleanupImage();
            }
        }
        Invalidate();
    }

    void ButtonImage::LoadImageFromResource(UINT nID, LPCTSTR lpszType)
    {
        // Реализация из предыдущего ответа (упрощенная)
        CleanupImage();
        // TODO: загрузка из ресурсов через IStream
        Invalidate();
    }

    // Mouse события
    void ButtonImage::OnMouseMove(UINT nFlags, CPoint p)
    {
        bool wasHover = m_bHover;
        m_bHover = HitTest(p.x, p.y);

        if (m_bHover != wasHover)
        {
            Invalidate();
        }
        Control::OnMouseMove(nFlags, p);
    }

    void ButtonImage::OnMouseLeave()
    {
        if (m_bHover)
        {
            m_bHover = false;
            Invalidate();
        }
        Control::OnMouseLeave();
    }

    void ButtonImage::OnLButtonDown(UINT nFlags, CPoint p)
    {
        if (HitTest(p.x, p.y))
        {
            Pressed = !Pressed;
            Invalidate();
        }
        Control::OnLButtonDown(nFlags, p);
    }

    void ButtonImage::OnLButtonUp(UINT nFlags, CPoint p)
    {
        if (Pressed && HitTest(p.x, p.y))
        {
            // Клик произошел - здесь можно добавить событие
        }
        if (!Toggle)
        {
            Pressed = false;
        }
        Invalidate();
        Control::OnLButtonUp(nFlags, p);
    }

    // Основная функция рисования
    void ButtonImage::Draw(bool hasFocus, bool drawSelected)
    {
        if (m_pContainer)
        {
            Graphics g(оffScreenBitmap.get());
            g.Clear(Color::Transparent);
            g.SetSmoothingMode(SmoothingModeHighQuality);

            if (Selected && !drawSelected) return;

            RectF border_(0.f, 0.f, (float)Border.Width() - 1, (float)Border.Height() - 1);

            // Рисуем кнопку с изображением
            DrawImageButton(g, border_, m_pImage, m_bHover, Pressed, HoverBgColor, PressedColor);

            // Рамка если нужно
            DrawBorder2(g);
        }
    }

    // Статическая функция рисования (как в ButtonControl)
    void ButtonImage::DrawImageButton(Graphics& g, const RectF& rect,
        Image* image, bool hover, bool pressed,
        Color hoverBg, Color pressedBg)
    {
        if (!image) return;

        GraphicsPath path;

        if (pressed)
        {
            SolidBrush presseBrush(pressedBg);
            CreateRoundedRectPath(path, rect, СornerRoundRadius, CornerStyle);
            g.FillPath(&presseBrush, &path);
            //g.FillRectangle(&presseBrush, rect);
        }
        else if (hover)
        {
            SolidBrush hoverBrush(hoverBg);
            CreateRoundedRectPath(path, rect, СornerRoundRadius, CornerStyle);
            g.FillPath(&hoverBrush, &path);
            g.FillRectangle(&hoverBrush, rect);
        }

        if (pressed)
        {
            // Инверсия цветов
            ImageAttributes imgAttr;
            ColorMatrix cm = {
                   -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f,-1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f,-1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                    1.0f, 1.0f, 1.0f, 0.0f, 1.0f
            };
            imgAttr.SetColorMatrix(&cm);

            g.DrawImage(image, rect,
                0.f, 0.f, (float)image->GetWidth(), (float)image->GetHeight(),
                UnitPixel, &imgAttr);
        }
        else
        {
            // Обычное изображение
            g.DrawImage(image, rect);
        }
    }
}
