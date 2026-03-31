#pragma once

#include "framework.h" // Стандартные компоненты MFC

namespace PluginGUI
{
    class Frame : public CRect
    {
    public:
        Frame(int l = 0, int t = 0, int r = 0, int b = 0, int cornerSize = 10)
            : CRect(l, t, r, b), m_CornerSize(cornerSize)
        {
        }

        Frame(const CRect& rect, int cornerSize = 10)
            : CRect(rect), m_CornerSize(cornerSize)
        {
        }

        Frame(const CPoint& pointTL, const CSize& size, int cornerSize = 10)
            : CRect(pointTL, size), m_CornerSize(cornerSize)
        {
        }

        // Проверка принадлежности точки угловой треугольной зоне (правый нижний угол)
        bool HitTestCorner(const CPoint& point) const
        {
            // Проверяем, что точка внутри квадрата cornerSize_ в правом нижнем углу
            if (point.x < right - m_CornerSize || point.x > right ||
                point.y < bottom - m_CornerSize || point.y > bottom)
                return false;

            // Координаты относительно правого нижнего угла
            int dx = right - point.x;
            int dy = bottom - point.y;

            // Треугольник: dx + dy <= cornerSize_
            return (dx + dy) <= m_CornerSize;
        }

        Frame& operator=(const Frame& rect)
        {
            CRect::operator=(rect);
            return *this;
        }

        // operator= от CRect (критично!)
        Frame& operator=(const CRect& rect)
        {
            CRect::operator=(rect);
            return *this;
        }

        bool operator==(const Frame& other) const
        {
            return (CRect::operator==(other) && CornerSize == other.CornerSize);
        }

        bool operator!=(const Frame& other) const
        {
            return !operator==(other);
        }

        int GetCornerSize() const
        {
            return m_CornerSize;
        }
        void SetCornerSize(int size)
        {
            m_CornerSize = size;
        }
        __declspec(property(get = GetCornerSize, put = SetCornerSize)) int CornerSize;

    private:
        int m_CornerSize; // размер зоны треугольника в пикселях
    };
}