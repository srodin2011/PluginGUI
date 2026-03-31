#pragma once

#include "Control.h"
#include "Event.h"
#include <numbers>

namespace PluginGUI
{
    class EnvelopeGraphControl2 : public Control
    {
    public:
        struct Point
        {
            double x, y;
            bool isMain;  // true для основных точек, false для промежуточных
        };
        enum class Axis { X, Y };

        EnvelopeGraphControl2(const CRect& border);

        // Основные точки огибающей (пользователь редактирует только их)
        //void SetMainPoints(const std::vector<Point>& points);
        //const std::vector<Point>& GetMainPoints() const { return m_mainPoints; }

        // Настройки
        DEFINE_PROPERTY_WITH_EVENТ(Gdiplus::Color, LineColor, Gdiplus::Color(255, 0, 180, 255))
        DEFINE_PROPERTY_WITH_EVENТ(Gdiplus::Color, BackgroundColor, Gdiplus::Color(255, 33, 39, 44))
        DEFINE_PROPERTY_WITH_EVENТ(Gdiplus::Color, GridColor, Gdiplus::Color(100, 50, 84, 102))
        DEFINE_PROPERTY_WITH_EVENТ(Gdiplus::Color, UnderCurveColor, Gdiplus::Color(125, 8, 153, 213))
        DEFINE_PROPERTY_WITH_EVENТ(Gdiplus::Color, MainPointColor, Gdiplus::Color(255, 173, 215, 255))
        DEFINE_PROPERTY_WITH_EVENТ(Gdiplus::Color, InterPointColor, Gdiplus::Color(255, 0, 255, 255))
        DEFINE_PROPERTY_WITH_EVENТ(Gdiplus::Color, FocusColor, Gdiplus::Color(125, 59, 110, 137))
        DEFINE_PROPERTY(int, HorizontalDivisions, 8)
        DEFINE_PROPERTY(int, VerticalDivisions, 8)
        DEFINE_PROPERTY(bool, ShowGrid, true)
        DEFINE_PROPERTY(double, XScaleMsPerUnit, 1.0f)  // для ADRS: мс на единицу X
        DEFINE_PROPERTY(bool, fixedRightPoint, false)
        DEFINE_PROPERTY(bool, synchronizeEdgeValues​, true)

    protected:
        bool HitTest(int x, int y) override;
        void Invalidate() override;
        void Draw(const CPaintDC& dc, bool hasFocus) override;
        void OnMouseMove(UINT nFlags, CPoint p) override;
        void OnLButtonDown(UINT nFlags, CPoint p) override;
        void OnLButtonUp(UINT nFlags, CPoint p) override;
        void OnLButtonDblClk(UINT nFlags, CPoint point) override;
        bool СanChangeByMouse() const override { return true; }
        BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) override;
        void OnMouseLeave() override;
        void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) override;
        void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) override;
        void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) override;
        void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) override;

    private:
        std::vector<Point> m_allPoints;  // основные + промежуточные
        Point* m_draggedPoint;
        Point* m_draggedMainPoint;
        bool m_dragActive = false;
        int m_draggedMainIndex = -1;  
        int m_draggedAllPointsIndex = -1;
        int m_hoveredPointIndex = -1;
        const double distributionFunctionBase = 100./*1.00000007f*//*40*/;
        int m_lastMouseX = -1;
        int m_lastMouseY = -1;

        // Пересчёт всех точек из основных
        void RecalculatePointsOld();
        void RecalculatePoints();

        // Поиск ближайшей точки к позиции мыши
        int FindNearestPoint(float x, float y) const;
        //int FindNearestPoint(double x, double y, double threshold = 0.1f/*0.05f*/);
        int FindPrevMainPoint(double x) const;

        // Конвертация координат
        CPoint ToScreen(double x, double y) const;
        std::pair<double, double> ToGraph(double screenX, double screenY) const;
        std::pair<double, double> SnapToGrid(double x, double y);
        double SmoothMap(double t, Axis axis) const;
        float GetAdaptiveThreshold(Axis axis, float pointPos) const;
        float GetPointScore(float pointX, float pointY, float graphX, float graphY) const;


        // Рисование
        void DrawGrid(Gdiplus::Graphics& g);
        void DrawCurve(Gdiplus::Graphics& g);
        void DrawPoints(Gdiplus::Graphics& g);

        std::vector<Gdiplus::PointF> CalculateBezierPoints();
        Gdiplus::PointF ToScreenPointF(double x, double y) const;
        Gdiplus::PointF CalculateBezierPointAtT(const Point& p0,
            const Point& p2,
            const Point& p1,
            double t) const
        {
            // ★ ТОЧКА НА КРИВОЙ при t=0.5 ★
            double x = (1 - t) * (1 - t) * p0.x + 2 * t * (1 - t) * p1.x + t * t * p2.x;
            double y = (1 - t) * (1 - t) * p0.y + 2 * t * (1 - t) * p1.y + t * t * p2.y;
            return ToScreenPointF(x, y);
        }
        Point SerumEnvPoint(double t, Point p0, Point p1, Point p2);
        Point hermite_spline(double t, Point p0, Point p1, Point p2);

        void crossSegment(Point A, Point B, Point& A1, Point& B1);
        Point reflectBySegment(Point A, Point B, Point P);
        Point reflectByCenter(Point A, Point B, Point P);
        Point symmetricByCenter(Point A, Point B, Point P);

        Point CalculateСurvePoint(double t, Point p0, Point p1, Point p2);
        Point CalculatePowerPoint(double t, Point p0, Point p1, Point p2);
        Point CalculateIntermediatePoint(double t, Point p0, Point p1, Point p2);
        Point RestoreP1ByCurvePoint(double t, Point p0, Point pCurve, Point p2);
        Point RestoreP1ByIntemediatePoint(double t, Point p0, Point pCurve, Point p2);


        ///---------------------------
        void MovePoint(int index, Point p, bool recursion = false);
        void UpdateMidpoint(Point& pMid, const Point& pNeighborMain, const Point& pMain, const Point& pNewMain);


    };
}