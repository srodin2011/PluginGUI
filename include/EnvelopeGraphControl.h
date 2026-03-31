#pragma once

#include "Control.h"
#include "Event.h"
#include <numbers>

namespace PluginGUI
{
    class EnvelopeGraphControl : public Control
    {
        using Base = Control;
        REGISTER_PLUGIN(EnvelopeGraphControl, Control, ControlId::Envelop)

        void Init() {} // Инициализация для вызова из конструктора только!

    public:
        enum class CurveType
        {
            Power,
            Bezier
        };

        struct Point
        {
            float x, y;
            bool isMain;  // true для основных точек, false для промежуточных
            float relativeX = 0.0f; // Отношение p1 к центру p0 и p2
            float relativeY = 0.0f; // Отношение p1 к центру p0 и p2
            CurveType curve = CurveType::Power;
            bool onAscendingCurve = true;
            bool selected = false;
            float dragRelativeX = 0.0f; // Координата по x относительно перемещаемой точки
            float dragRelativeY = 0.0f; // Координата по y относительно перемещаемой точки

            Point() :
                x(0.f),
                y(0.f),
                isMain(true),
                relativeX(0.f),
                relativeY(0.f),
                curve(CurveType::Power),
                onAscendingCurve(true)
            {
            }

            Point(float x_, float y_, bool isMain_ = true):
                x(x_),
                y(y_),
                isMain(isMain_),
                relativeX(0.f),
                relativeY(0.f),
                curve(CurveType::Power),
                onAscendingCurve(true)
            {
            }
            Point(float x_, float y_, float relativeX_, float relativeY_, CurveType curve_) :
                x(x_),
                y(y_),
                isMain(false),
                relativeX(relativeX_),
                relativeY(relativeY_),
                curve(curve_),
                onAscendingCurve(true)
            {
            }
        };

        EnvelopeGraphControl(const CRect& Border);

        void SetPoints(std::vector<Point> points);

        // Основные точки огибающей (пользователь редактирует только их)
        //void SetMainPoints(const std::vector<Point>& points);
        //const std::vector<Point>& GetMainPoints() const { return m_mainPoints; }

        // Настройки
        DEFINE_PROPERTY(Gdiplus::Color, LineColor, Gdiplus::Color(255, 0, 180, 255))
        DEFINE_PROPERTY(Gdiplus::Color, BackgroundColor, Gdiplus::Color(255, 33, 39, 44))
        DEFINE_PROPERTY(Gdiplus::Color, GridColor, Gdiplus::Color(100, 50, 84, 102))
        DEFINE_PROPERTY(Gdiplus::Color, UnderCurveColor, Gdiplus::Color(125, 8, 153, 213))
        DEFINE_PROPERTY(Gdiplus::Color, MainPointColor, Gdiplus::Color(255, 173, 215, 255))
        DEFINE_PROPERTY(Gdiplus::Color, InterPointColor, Gdiplus::Color(255, 0, 255, 255))
        DEFINE_PROPERTY(Gdiplus::Color, SelectedPointColor, Gdiplus::Color::Red)
        DEFINE_PROPERTY(Gdiplus::Color, HoverColor, Gdiplus::Color(125, 59, 110, 137))
        DEFINE_PROPERTY(Gdiplus::Color, CrossColor, Gdiplus::Color(125, 180, 180, 180))
        DEFINE_PROPERTY(Gdiplus::Color, SelectColor, Gdiplus::Color::Yellow)
        DEFINE_PROPERTY(int, HorizontalDivisions, 8)
        DEFINE_PROPERTY(int, VerticalDivisions, 8)
        DEFINE_PROPERTY(bool, ShowGrid, true)
        DEFINE_PROPERTY(float, XScaleMsPerUnit, 1.0f)  // для ADRS: мс на единицу X
        DEFINE_PROPERTY(bool, FixedRightPoint, true)
        DEFINE_PROPERTY(bool, SynchronizeEdgeValues, true)
        DEFINE_PROPERTY(CurveType, Curve, CurveType::Power)
        DEFINE_PROPERTY(bool, ShowTrackingCross​, true)

    public:
        void Draw(bool hasFocus, bool drawSelected) override;
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
        void OnContextMenu(CPoint point) override;
        void OnUpdateCommand(CCmdUI* pCmdUI) override;
        void OnCommand(UINT nID) override;

        IMPLEMENT_OVERRIDE_FIND_PROPERTY(EnvelopeGraphControl)

    private:
        float m_graphW = 1.f;
        float m_graphH = 1.f;
        std::vector<Point> m_allPoints;  // основные + промежуточные
        Point* m_draggedPoint = nullptr;
        bool m_dragActive = false;
        int m_draggedPointIndex = -1;
        int m_hoveredPointIndex = -1;
        const double distributionFunctionBase = 40.f/*40*/;
        int m_lastMouseX = -1;
        int m_lastMouseY = -1;
        bool m_selectActive = false;
        int m_selectX = -1;
        int m_selectY = -1;
        bool m_AltPressed = false;

        // Пересчёт всех точек из основных
        void RecalculatePoints();

        // Поиск ближайшей точки к позиции мыши
        int FindNearestPoint(float x, float y, float threshold = 0.05f);
        int FindPrevMainPoint(float x) const;

        // Конвертация координат
        CPoint ToScreen(float x, float y) const;
        std::pair<float, float> ToGraph(int screenX, int screenY) const;
        std::pair<float, float> SnapToGrid(float x, float y) const;

        // Рисование
        void DrawGrid(Gdiplus::Graphics& g);
        void DrawCurve(Gdiplus::Graphics& g);
        void DrawPoints(Gdiplus::Graphics& g);
        void DrawCross(Gdiplus::Graphics& g);
        void DrawSelection(Gdiplus::Graphics& g);


        Gdiplus::PointF ToScreenPointF(float x, float y) const;
        Point hermite_spline(Point p0, Point p1, Point p2, float t);

        void crossSegment(Point A, Point B, Point& A1, Point& B1);
        Point reflectBySegment(Point A, Point B, Point P);
        Point reflectByCenter(Point A, Point B, Point P);
        Point symmetricByCenter(Point A, Point B, Point P);

        Point CalculateСurvePoint(int p1Index, float t = 0.5f) const;
        Point CalculateСurvePoint(const Point& p0, const Point& p1, const Point& p2, float t = 0.5f) const;
        Point CalculatePowerPoint(const Point& p0, const Point& p1, const Point& p2, float t = 0.5f) const;
        Point CalculateBezierPoint(const Point& p0, const Point& p1, const Point& p2, float t = 0.5f) const;

        // Управление точками
        void MovePoint(int index, const Point& p, bool recursion = false);
        void UpdateRelative(Point& p1, const Point& p0, const Point& p2);
        void RestoreP1ByRelative(Point& p1, const Point& p0, const Point& p2);
        void UpdateCurveDirections();

        double distributionFunction(double x, double n) const;
        double invertDistributionFunction(double y, double n) const;

        void DeleteMainPoint(int index);
        void ChangeCurve(int index, CurveType curve);

        void FlipVertical();
        void FlipHorizontal();
        void FlipByCenter();
 
        bool SelectPoints();
        bool IsSelectedPoints() const;
        void MoveSelectedPoints(float dx, float dy);
        void DropPointSelection();
        void DeleteSelectedPoints();

        void InitDragRelative();
        void DropDragRelative();

        int FindNextPoint(int currentIdx, int diffIdx, bool isMain);

        void AdaptiveSplineRemapping(int index);
   };
}