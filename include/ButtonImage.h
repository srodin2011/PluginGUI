#pragma once

#include "Control.h"
#include "Utils.h"

namespace PluginGUI
{
    class ButtonImage : public Control
    {
        typedef Control Base;
        REGISTER_PLUGIN(ButtonImage, Control, ControlId::ButtonImage) // Новый ID

        void Init(); // Инициализация для вызова из конструктора только!

    public:
        ButtonImage(const CRect& Border);

        // Свойства для изображения
        DEFINE_PROPERTY_WITH_EVENТ(std::wstring, ImagePath, L"")
        DEFINE_PROPERTY(Gdiplus::Color, HoverBgColor, Gdiplus::Color(255, 200, 220, 255))
        DEFINE_PROPERTY(Gdiplus::Color, PressedColor, Gdiplus::Color(255, 200, 220, 255))
        DEFINE_PROPERTY(bool, Pressed, false)
        DEFINE_PROPERTY(bool, Toggle, true)
        DEFINE_PROPERTY(CornerMask, CornerStyle, CornerMask::None)
        DEFINE_PROPERTY(int, СornerRoundRadius, 5)

        // Состояния мыши (public для доступа из PluginView)
        bool IsHover() const { return m_bHover; }

        void Draw(bool hasFocus, bool drawSelected) override;
        void OnLButtonDown(UINT, CPoint p) override;
        void OnMouseMove(UINT nFlags, CPoint p) override;
        void OnMouseLeave() override;
        void OnLButtonUp(UINT, CPoint p) override;

        virtual UINT GetMinWidth() const override { return 32; }
        virtual UINT GetMinHeight() const override { return 32; }

    protected:
        Gdiplus::Image* m_pImage = nullptr;
        bool m_bHover = false;
        bool m_bPressed = false;

        void CleanupImage();

        // Загрузка изображения
        void LoadImage(const std::wstring& path);
        void LoadImageFromResource(UINT nID, LPCTSTR lpszType);

    private:
        void DrawImageButton(Gdiplus::Graphics& g, const Gdiplus::RectF& rect,
            Gdiplus::Image* image, bool hover, bool pressed,
            Gdiplus::Color hoverBg, Gdiplus::Color pressedBg);
    };
}

