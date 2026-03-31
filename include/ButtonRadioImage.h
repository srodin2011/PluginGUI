#pragma once

#include "Control.h"

namespace PluginGUI
{
    using namespace Gdiplus;

    class ButtonRadioImage : public Control
    {
        REGISTER_PLUGIN(ButtonRadioImage, Control, ControlId::ButtonRadioImage)

        void Init(); // Инициализация для вызова из конструктора только!

    public:
        enum class СornerStyle
        {
            WithoutRounding,
            RoundedEdges,
            EachButtonRounding
        };

        ButtonRadioImage(const CPoint& p);

        // Свойства группы
        DEFINE_PROPERTY_WITH_EVENТ(std::wstring, ImagePath, L"")
        DEFINE_PROPERTY_WITH_EVENТ(int, ButtonCount, 3)              // Кол-во кнопок
        DEFINE_PROPERTY_WITH_EVENТ(int, Spacing, 5)                  // Интервал между кнопками
        DEFINE_PROPERTY_WITH_EVENТ(int, ButtonH, 32)                  
        DEFINE_PROPERTY_WITH_EVENТ(int, ButtonW, 32)                  
        DEFINE_PROPERTY_WITH_EVENТ(bool, OrientationVertical, true)  // Вертикаль/горизонталь
        DEFINE_PROPERTY(Gdiplus::Color, HoverBgColor, Gdiplus::Color(255, 200, 220, 255))
        DEFINE_PROPERTY(Gdiplus::Color, PressedColor, Gdiplus::Color(255, 180, 180, 255))
        DEFINE_PROPERTY(int, SelectedIndex, -1)           // Выбранная кнопка (-1 = ни одна)
        DEFINE_PROPERTY(СornerStyle, CrStyle, СornerStyle::WithoutRounding)
        DEFINE_PROPERTY(int, СornerRoundRadius, 5)

        void Draw(bool hasFocus, bool drawSelected) override;
        void OnLButtonDown(UINT, CPoint p) override;
        void OnMouseMove(UINT nFlags, CPoint p) override;
        void OnMouseLeave() override;
        void OnLButtonUp(UINT, CPoint p) override;

        virtual UINT GetMinWidth() const override;
        virtual UINT GetMinHeight() const override;

        // Загрузка изображения
        void LoadImage(const std::wstring& path);

    protected:
        Image* m_pImage = nullptr;
        int m_hoverIndex = -1;      // Кнопка под мышью
        int m_pressIndex = -1;      // Нажатая кнопка

        void CleanupImage();
        int HitTestButton(int x, int y);  // Какая кнопка под мышью
        CRect GetButtonRect(int index) const;   // Rect конкретной кнопки
        int CalculateTotalSize() const;         // Общий размер группы

    private:
            void DrawSingleButton(Graphics& g, const Gdiplus::RectF& rect, Image* image,
            bool hover, bool pressed, bool selected,
            const Color& hoverBg, const Color& pressedBg, int buttonIndex, int buttonW, int buttonH);
    };
}
