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
        }
        else if (hover)
        {
            SolidBrush hoverBrush(hoverBg);
            CreateRoundedRectPath(path, rect, СornerRoundRadius, CornerStyle);
            g.FillPath(&hoverBrush, &path);
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

    //------------------------------------------------------------------------------
    // Работа со свойствами 

    Variant ButtonImage::doGetPropertyValue(Id id) const
    {
        switch (id)
        {
            case static_cast<int>(PropertyName::pnImagePath):
                return ImagePath;

            case static_cast<int>(PropertyName::pnHoverBgColor):
                return HoverBgColor;

            case static_cast<int>(PropertyName::pnPressedColor):
                return PressedColor;

            case static_cast<int>(PropertyName::pnPressed):
                return Pressed;

            case static_cast<int>(PropertyName::pnToggle):
                return Toggle;

            case static_cast<int>(PropertyName::pnCornerStyle):
                return static_cast<int>(CornerStyle);

            case static_cast<int>(PropertyName::pnСornerRoundRadius):
                return СornerRoundRadius;

            default:
                return Base::doGetPropertyValue(id);
        }
    }

    bool ButtonImage::doSetPropertyValue(Id id, const Variant& value)
    {
        return std::visit(
            [this, id](const auto& v) -> bool
            {
                using T = std::decay_t<decltype(v)>;

                switch (id)
                {
                    case static_cast<int>(PropertyName::pnImagePath):
                        if constexpr (std::is_same_v<T, std::wstring>)
                        {
                            ImagePath = v;
                            return true;
                        }
                        return false;

                    case static_cast<int>(PropertyName::pnHoverBgColor):
                        if constexpr (std::is_same_v<T, Gdiplus::Color>)
                        {
                            HoverBgColor = v;
                            return true;
                        }
                        return false;

                    case static_cast<int>(PropertyName::pnPressedColor):
                        if constexpr (std::is_same_v<T, Gdiplus::Color>)
                        {
                            PressedColor = v;
                            return true;
                        }
                        return false;

                    case static_cast<int>(PropertyName::pnPressed):
                        if constexpr (std::is_same_v<T, bool>)
                        {
                            Pressed = v;
                            return true;
                        }
                        return false;

                    case static_cast<int>(PropertyName::pnToggle):
                        if constexpr (std::is_same_v<T, bool>)
                        {
                            Toggle = v;
                            return true;
                        }
                        return false;

                    case static_cast<int>(PropertyName::pnCornerStyle):
                        if constexpr (std::is_same_v<T, int>)
                        {
                            CornerStyle = static_cast<CornerMask>(v);
                            return true;
                        }
                        return false;

                    case static_cast<int>(PropertyName::pnСornerRoundRadius):
                        if constexpr (std::is_same_v<T, int>)
                        {
                            СornerRoundRadius = v;
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
