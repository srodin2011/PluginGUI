#pragma once
#include "Control.h"
#include "Container.h"
#include <vector>
#include <memory>

namespace PluginGUI
{
    class Panel : public Control, public IContainer
    {
    public:
        Panel(const CRect& border);
        virtual ~Panel();

        // IContainer интерфейс
        const std::vector<std::shared_ptr<Control>>& GetChildren() const { return m_children; }

        void Add(Control* child) override;
        void Add(std::shared_ptr<Control> childSP);
        void Remove(Control* child) override;
        size_t GetCount() const override;
        size_t GetSelectedCount() const override;
        void Invalidate(const CRect& rect) override;
        void GetCursorPos(LPPOINT p) override;
        void SetCursorPos(POINT p) override;
        void OnChildPropertyChanged(Control* pChild, Id id) override;

        // ѕреобразование глобальные координаты <=> локальные координаты
        void GlobalToLocal(CPoint& p) override;
        void LocalToGlobal(CPoint& p) override;

        // ѕреобразование координаты родител€ <=> локальные координаты
        void ParentToLocal(CPoint& p) override;
        void LocalToParent(CPoint& p) override;

        void ShowTooltipForControl(Control* pControl) override;
        void UpdateTooltipForCurrentControl(Control* pControl) override;
        void HideCurrentTooltip() override;

        // ѕереопределение Control методов
        void Draw(bool hasFocus, bool drawSelected) override;
        void OnMouseMove(UINT nFlags, CPoint point) override;
        BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint p);
        void OnLButtonDown(UINT nFlags, CPoint p);
        void OnLButtonUp(UINT nFlags, CPoint point) override;
        void OnMouseLeave() override;
        void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
        void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
        void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
        void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
        void OnContextMenu(CPoint point);
        void OnUpdateCommand(CCmdUI* pCmdUI);
        void OnCommand(UINT nID);
        void Invalidate() override;

    protected:
        std::vector<std::shared_ptr<Control>> m_children;
        mutable Control* m_focusedChild = nullptr;

        void Init();

        Control* HitTestControl(const CPoint& pt) const;
        Control* GetFocusControl() const;

        using Control::GetCursorPos;
        using Control::SetCursorPos;
    };

} // namespace PluginGUI
