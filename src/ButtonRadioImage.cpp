#include "PluginGUI\\include\\ButtonImage.h"
#include "PluginGUI\\include\\ButtonRadioImage.h"
#include "PluginGUI\\include\\PluginView.h"
#include "PluginGUI\\include\\Utils.h"

namespace PluginGUI
{
    ButtonRadioImage::ButtonRadioImage(const CPoint& p) : Control()
    {
        Border = Frame(p.x, p.y, p.x, p.y);
        Init();
    }

    void ButtonRadioImage::Init()
    {
        auto recaltulateBorder = [&](int)
            {
                int w = GetMinWidth();
                int h = GetMinHeight();

                Border = Frame(Border.left, Border.top, Border.left + w, Border.top + h);
            };

        ImagePathChanged.Subscribe([&](std::wstring path)
            {
                LoadImage(path);
            }
        );
        ButtonCountChanged.Subscribe(recaltulateBorder);
        SpacingChanged.Subscribe(recaltulateBorder);
        ButtonHChanged.Subscribe(recaltulateBorder);
        ButtonWChanged.Subscribe(recaltulateBorder);
        OrientationVerticalChanged.Subscribe(recaltulateBorder);
    }

    int ButtonRadioImage::HitTestButton(int x, int y)
    {
        for (int i = 0; i < ButtonCount; i++)
        {
            if (GetButtonRect(i).PtInRect(CPoint(x, y)))
            {
                return i;
            }
        }
        return -1;
    }

    CRect ButtonRadioImage::GetButtonRect(int index) const
    {
        if (index < 0 || index >= ButtonCount) return CRect(0, 0, 0, 0);

        int btnWidth = ButtonW;
        int btnHeight = ButtonH;

        if (OrientationVertical)
        {
            // Вертикальное расположение ОТНОСИТЕЛЬНО Border (0,0)
            int top = index * (btnHeight + Spacing);
            return CRect(0,           // ← left = 0 (относительно Border.left)
                top,
                btnWidth,
                top + btnHeight);  // ← bottom = top + высота (БЕЗ Spacing!)
        }
        else
        {
            // Горизонтальное расположение ОТНОСИТЕЛЬНО Border (0,0)
            int left = index * (btnWidth + Spacing);
            return CRect(left,        // ← left растет вправо
                0,           // ← top = 0 (относительно Border.top)
                left + btnWidth,
                btnHeight);
        }
    }

    int ButtonRadioImage::CalculateTotalSize() const
    {
        int singleSize = 32; // Минимальный размер кнопки
        if (OrientationVertical)
        {
            return ButtonCount * ButtonH + (ButtonCount - 1) * Spacing;
        }
        else
        {
            return ButtonCount * ButtonW + (ButtonCount - 1) * Spacing;
        }
    }

    UINT ButtonRadioImage::GetMinWidth() const
    {
        return OrientationVertical ? ButtonW : CalculateTotalSize();
    }

    UINT ButtonRadioImage::GetMinHeight() const
    {
        return OrientationVertical ? CalculateTotalSize() : ButtonH;
    }

    void ButtonRadioImage::LoadImage(const std::wstring& path)
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

    void ButtonRadioImage::CleanupImage()
    {
        if (m_pImage)
        {
            delete m_pImage;
            m_pImage = nullptr;
        }
    }

    void ButtonRadioImage::OnMouseMove(UINT nFlags, CPoint p)
    {
        int hit = HitTestButton(p.x - Border.left, p.y - Border.top);
        if (hit != m_hoverIndex)
        {
            m_hoverIndex = hit;
            Invalidate();
        }
        Control::OnMouseMove(nFlags, p);
    }

    void ButtonRadioImage::OnMouseLeave()
    {
        if (m_hoverIndex != -1)
        {
            m_hoverIndex = -1;
            Invalidate();
        }
        Control::OnMouseLeave();
    }

    void ButtonRadioImage::OnLButtonDown(UINT nFlags, CPoint p)
    {
        int hit = HitTestButton(p.x - Border.left, p.y - Border.top);
        if (hit >= 0)
        {
            SelectedIndex = hit;  // Выбираем эту кнопку
            m_pressIndex = hit;
            Invalidate();
        }
        Control::OnLButtonDown(nFlags, p);
    }

    void ButtonRadioImage::OnLButtonUp(UINT nFlags, CPoint p)
    {
        //m_pressIndex = -1;
        //Invalidate();
        Control::OnLButtonUp(nFlags, p);
    }

    void ButtonRadioImage::Draw(bool hasFocus, bool drawSelected)
    {
        if (m_pContainer)
        {
            Graphics g(оffScreenBitmap.get());
            g.Clear(Color::Transparent);
            g.SetSmoothingMode(SmoothingModeHighQuality);

            if (Selected && !drawSelected) return;

            // Рисуем каждую кнопку группы
            for (int i = 0; i < ButtonCount; i++)
            {
                CRect btnRect = GetButtonRect(i);
                RectF gdiRect((float)btnRect.left, (float)btnRect.top, (float)btnRect.Width(), (float)btnRect.Height());

                bool isHover = (i == m_hoverIndex);
                bool isPressed = (i == m_pressIndex);
                bool isSelected = (i == SelectedIndex);

                DrawSingleButton(g, gdiRect, m_pImage, isHover, isPressed, isSelected,
                    HoverBgColor, PressedColor, i, ButtonW, ButtonH);
            }

            DrawBorder2(g);
        }
    }

    void ButtonRadioImage::DrawSingleButton(Graphics& g, const RectF& rect, Image* image,
        bool hover, bool pressed, bool selected,
        const Color& hoverBg, const Color& pressedBg,
        int buttonIndex, int buttonW, int buttonH)
    {
        GraphicsPath path;

        if (CrStyle == СornerStyle::EachButtonRounding)
        {
            CreateRoundedRectPath(path, rect, СornerRoundRadius, CornerMask::All);
        }
        else if(CrStyle == СornerStyle::RoundedEdges)
        {
            CornerMask mask = CornerMask::None;
            if (buttonIndex == 0)
            {
                mask = OrientationVertical ? CornerMask::Top : CornerMask::Left;
            }
            else if (buttonIndex == ButtonCount - 1)
            {
                mask = OrientationVertical ? CornerMask::Bottom : CornerMask::Right;
            }
            CreateRoundedRectPath(path, rect, СornerRoundRadius, mask);
        }
        else
        {
            path.AddRectangle(rect);
        }

        // Фон
        if (selected)
        {
            SolidBrush pressedBrush(pressedBg);
            g.FillPath(&pressedBrush, &path);
            //g.FillRectangle(&pressedBrush, rect);
        }
        else if (hover)
        {
            SolidBrush hoverBrush(hoverBg);
            g.FillPath(&hoverBrush, &path);
            //g.FillRectangle(&hoverBrush, rect);
        }

        // Индикатор выбранности (рамка)
 /*       if (selected)
        {
            Pen selectPen(Color(255, 255, 255, 255), 2);
            g.DrawRectangle(&selectPen, rect);
        }*/

        if (!image) return;

        // ✅ Source rect с использованием ButtonW/ButtonH
        UINT frameWidth = buttonW;
        UINT frameHeight = buttonH;

        // Поскольку расположены ПО ВЕРТИКАЛИ:
        // X = 0 (всегда первый кадр по горизонтали)
        // Y = buttonIndex * ButtonH
        Gdiplus::Rect srcRect(0, buttonIndex * frameHeight, frameWidth, frameHeight);

        if (pressed)
        {
            // Инверсия + вырезка
            ImageAttributes imgAttr;
            ColorMatrix cm = {
                -1.0f, 0, 0, 0, 0,
                 0,-1.0f, 0, 0, 0,
                 0, 0,-1.0f, 0, 0,
                 0, 0, 0, 1, 0,
                 1, 1, 1, 0, 1
            };
            imgAttr.SetColorMatrix(&cm);

            g.DrawImage(image, rect,
                (float)srcRect.X, (float)srcRect.Y, (float)srcRect.Width, (float)srcRect.Height,
                UnitPixel, &imgAttr);
        }
        else
        {
            // Обычная вырезка
            g.DrawImage(image, rect,
                (float)srcRect.X, (float)srcRect.Y, (float)srcRect.Width, (float)srcRect.Height,
                UnitPixel);
        }
    }

}
