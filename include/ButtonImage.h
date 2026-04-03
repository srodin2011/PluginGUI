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
        enum class PropertyName
        {
            pnImagePath = Base::PropertyName::pnLastName,
            pnHoverBgColor,
            pnPressedColor,
            pnPressed,
            pnToggle,
            pnCornerStyle,
            pnСornerRoundRadius,
            pnLastName
        };

        ButtonImage(const CRect& Border);

        // Свойства для изображения
        DEFINE_PROPERTY_WITH_EVENТ_NEW(std::wstring, ImagePath, L"", ButtonImage, PropertyName::pnImagePath)
        DEFINE_PROPERTY_NEW(Gdiplus::Color, HoverBgColor, Gdiplus::Color(255, 200, 220, 255), ButtonImage, PropertyName::pnHoverBgColor)
        DEFINE_PROPERTY_NEW(Gdiplus::Color, PressedColor, Gdiplus::Color(255, 200, 220, 255), ButtonImage, PropertyName::pnPressedColor)
        DEFINE_PROPERTY_NEW(bool, Pressed, false, ButtonImage, PropertyName::pnPressed)
        DEFINE_PROPERTY_NEW(bool, Toggle, true, ButtonImage, PropertyName::pnToggle)
        DEFINE_PROPERTY_NEW(CornerMask, CornerStyle, CornerMask::None, ButtonImage, PropertyName::pnCornerStyle)
        DEFINE_PROPERTY_NEW(int, СornerRoundRadius, 5, ButtonImage, PropertyName::pnСornerRoundRadius)

        // Состояния мыши (public для доступа из PluginView)
        bool IsHover() const { return m_bHover; }

        void Draw(bool hasFocus, bool drawSelected) override;
        void OnLButtonDown(UINT, CPoint p) override;
        void OnMouseMove(UINT nFlags, CPoint p) override;
        void OnMouseLeave() override;
        void OnLButtonUp(UINT, CPoint p) override;

        virtual UINT GetMinWidth() const override { return 32; }
        virtual UINT GetMinHeight() const override { return 32; }

        size_t GetPropertyCount() const override
        {
            // pnLastName — уже общий индекс по всей иерархии
            return static_cast<size_t>(PropertyName::pnLastName);
        }

        IMPLEMENT_OVERRIDE_FIND_PROPERTY(ButtonImage)

    protected:
        Gdiplus::Image* m_pImage = nullptr;
        bool m_bHover = false;
        bool m_bPressed = false;

        void CleanupImage();

        // Загрузка изображения
        void LoadImage(const std::wstring& path);
        void LoadImageFromResource(UINT nID, LPCTSTR lpszType);

        virtual Variant doGetPropertyValue(Id id) const override;
        virtual bool doSetPropertyValue(Id id, const Variant& value) override;

    private:
        void DrawImageButton(Gdiplus::Graphics& g, const Gdiplus::RectF& rect,
            Gdiplus::Image* image, bool hover, bool pressed,
            Gdiplus::Color hoverBg, Gdiplus::Color pressedBg);
    };
}

