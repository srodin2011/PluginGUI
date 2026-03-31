#include "PluginGUI\include\Panel.h"
#include "PluginGUI\include\PluginView.h"
#include <algorithm>

namespace PluginGUI
{
    PLUGINGUI_PROPERTY_TABLE_INSTANTIATE(Panel)

    using namespace Gdiplus;

    Panel::Panel(const CRect& border) : Control(border)
    {
        Init();
    }

    Panel::~Panel()
    {
        m_children.clear();
    }

    void Panel::Init()
    {
        BaseColor = Gdiplus::Color(40, 50, 50, 50); // Полупрозрачный фон
    }

    /// <summary>
    /// Найти дочерний контрол по координате
    /// </summary>
    /// <param name="pt"></param>
    /// <returns></returns>
    Control* Panel::HitTestControl(const CPoint& pt) const
    {
        auto it = std::find_if(m_children.rbegin(), m_children.rend(),
            [&pt](const std::shared_ptr<Control>& p)
            {
                return p && p->HitTest(pt.x, pt.y);
            });
        return (it != m_children.rend()) ? it->get() : nullptr;
    }

    /// <summary>
    /// Получить дочерний контрол, получивший фокус
    /// </summary>
    /// <returns></returns>
    Control* Panel::GetFocusControl() const
    {
        if (m_focusedChild && m_focusedChild->Focus)
        {
            return m_focusedChild;
        }

        auto it = std::find_if(m_children.begin(), m_children.end(),
            [](const std::shared_ptr<Control>& control)
            {
                return control && control->Focus;
            });

        m_focusedChild = (it != m_children.end()) ? it->get() : nullptr;
        return m_focusedChild;
    }

    // === IContainer реализация ===
    void Panel::Add(Control* control)
    {
        if (control)
        {
            m_children.push_back(std::shared_ptr<Control>(control));
            control->m_pContainer = this;
            m_isDirty = true;
        }
    }

    void Panel::Add(std::shared_ptr<Control> childSP)
    {
        if (childSP.get())
        {
            m_children.push_back(childSP);
            childSP->m_pContainer = this;
            m_isDirty = true;
        }
    }

    void Panel::Remove(Control* child)
    {
        // Ищем контрол в коллекции
        auto it = std::find_if(m_children.begin(), m_children.end(),
            [child](const std::shared_ptr<Control>& c)
            {
                return c.get() == child;
            });

        // Удаляем, если нашли
        if (it != m_children.end())
        {
            child->Invalidate(); // Нужно перерисовать область на которой был контрол
            m_children.erase(it);
            child->m_pContainer = nullptr;
            m_isDirty = true;
        }
    }

    size_t Panel::GetCount() const
    {
        return m_children.size();
    }

    size_t Panel::GetSelectedCount() const
    {
        return std::count_if(GetChildren().begin(), GetChildren().end(),
            [](const auto& childSP)
            {
                return childSP->Selected;  
            });
    }

    void Panel::Invalidate(const CRect& rect)
    {
        m_isDirty = true;
        CRect invalidRect = rect;
        invalidRect.OffsetRect(Border.TopLeft()); // Переводим в коордитаты родителя
        if (m_pContainer)
        {
            m_pContainer->Invalidate(invalidRect);  // Делегируем вверх по иерархии
        }
    }

    void Panel::GetCursorPos(LPPOINT p) 
    {
        if (m_pContainer)
        {
            m_pContainer->GetCursorPos(p);
            p->x -= Border.left;
            p->y -= Border.top;
        }
    }

    void Panel::SetCursorPos(POINT p)
    {
        if (m_pContainer)
        {
            p.x += Border.left;
            p.y += Border.top;
            m_pContainer->SetCursorPos(p);
        }
    }

    void Panel::OnChildPropertyChanged(Control* pChild, Id id)
    {
        if (m_pContainer)
        {
            m_pContainer->OnChildPropertyChanged(pChild, id);
        }
    }

    void Panel::GlobalToLocal(CPoint& p)
    {
        if (m_pContainer == this)
        {
            int a = 10;
        }

        if (m_pContainer)
        {
            m_pContainer->GlobalToLocal(p);
            p.x -= Border.left;
            p.y -= Border.top;
        }
    }

    void Panel::LocalToGlobal(CPoint& p)
    {
        if (m_pContainer)
        {
            p.x += Border.left;
            p.y += Border.top;
            m_pContainer->LocalToGlobal(p);
        }
    }

    void Panel::ParentToLocal(CPoint& p)
    {
        p.x -= Border.left;
        p.y -= Border.top;
    }

    void Panel::LocalToParent(CPoint& p)
    {
        p.x += Border.left;
        p.y += Border.top;
    }

    void Panel::ShowTooltipForControl(Control* pControl)
    {
        if (m_pContainer)
        {
            m_pContainer->ShowTooltipForControl(pControl);
        }
    }

    void Panel::UpdateTooltipForCurrentControl(Control* pControl)
    {
        if (m_pContainer)
        {
            m_pContainer->UpdateTooltipForCurrentControl(pControl);
        }
    }

    void Panel::HideCurrentTooltip()
    {
        if (m_pContainer)
        {
            m_pContainer->HideCurrentTooltip();
        }
    }

    // === Control переопределения ===

    void Panel::OnMouseMove(UINT nFlags, CPoint p)
    {
        // Переводим координаты родителя в клиентские координаты панели
        CPoint clientP = p - Border.TopLeft();

        Control* pOldFocused = m_focusedChild;
        Control* pNewFocused = HitTestControl(clientP);
        if (pOldFocused)
        {
            pOldFocused->OnMouseMove(nFlags, clientP);
        }

        if (pOldFocused == pNewFocused) return;

        // Нужно ли вызывать OnMouseMove для pNewFocused?

        // Смена фокуса
        if (pOldFocused && !(nFlags & MK_LBUTTON))
        {
            pOldFocused->OnMouseLeave();
            pOldFocused->SetFocus(false);
        }
        if (pNewFocused && !(nFlags & MK_LBUTTON))
        {
            pNewFocused->SetFocus(true);
            m_focusedChild = pNewFocused;
        }

        Control::OnMouseMove(nFlags, p);  // Базовый класс
    }

    BOOL Panel::OnMouseWheel(UINT nFlags, short zDelta, CPoint p)
    {
        // 1. Родитель → Клиентские координаты ПАНЕЛИ
        CPoint clientP = p - Border.TopLeft();

        // 2. ТОЛЬКО focused контрол!
        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            return pFocused->OnMouseWheel(nFlags, zDelta, clientP);
        }

        // 3. Базовый класс
        return Control::OnMouseWheel(nFlags, zDelta, clientP);
    }

    void Panel::OnLButtonDown(UINT nFlags, CPoint p)
    {
        // 1. Родитель → Клиентские координаты ПАНЕЛИ
        CPoint clientP = p - Border.TopLeft();

        // 2. ТОЧНАЯ копия PluginView логики!
        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            if ((nFlags & MK_CONTROL) == 0 && pFocused->СanChangeByMouse())
            {
                pFocused->SetValueChangeMode(true);
            }
            pFocused->OnLButtonDown(nFlags, clientP);
        }

        // 3. Базовый класс
        Control::OnLButtonDown(nFlags, p);
    }

    void Panel::OnLButtonUp(UINT nFlags, CPoint p)
    {
        // 1. Родитель → Клиентские координаты ПАНЕЛИ
        CPoint clientP = p - Border.TopLeft();

        // 2. ТОЧНАЯ копия PluginView!
        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            if (pFocused->СanChangeByMouse() && pFocused->ValueChangeMode)
            {
                pFocused->SetValueChangeMode(false);
                if (!pFocused->HitTest(clientP.x, clientP.y))
                {
                    pFocused->SetFocus(false);
                }
            }
            pFocused->OnLButtonUp(nFlags, clientP);
        }

        // 3. Базовый класс
        Control::OnLButtonUp(nFlags, p);
    }

    void Panel::OnMouseLeave()
    {
        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            pFocused->OnMouseLeave();
        }

        Control::OnMouseLeave();
    }

    void Panel::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            pFocused->OnKeyDown(nChar, nRepCnt, nFlags);
        }

        Control::OnKeyDown(nChar, nRepCnt, nFlags);
    }

    void Panel::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            pFocused->OnKeyDown(nChar, nRepCnt, nFlags);
        }

        Control::OnKeyUp(nChar, nRepCnt, nFlags);
    }

    void Panel::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            pFocused->OnSysKeyDown(nChar, nRepCnt, nFlags);
        }

        Control::OnSysKeyDown(nChar, nRepCnt, nFlags);
    }

    void Panel::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            pFocused->OnSysKeyUp(nChar, nRepCnt, nFlags);
        }

        Control::OnSysKeyUp(nChar, nRepCnt, nFlags);
    }

    void Panel::OnContextMenu(CPoint point)
    {
        // 1. Родитель → Клиентские координаты ПАНЕЛИ
        CPoint clientP = point - Border.TopLeft();

        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            pFocused->OnContextMenu(clientP);
        }

        Control::OnContextMenu(point);
    }

    void Panel::OnUpdateCommand(CCmdUI* pCmdUI)
    {
        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            pFocused->OnUpdateCommand(pCmdUI);
        }

        Control::OnUpdateCommand(pCmdUI);
    }

    void Panel::OnCommand(UINT nID)
    {
        Control* pFocused = GetFocusControl();
        if (pFocused)
        {
            pFocused->OnCommand(nID);
        }

        Control::OnCommand(nID);
    }

    void Panel::Draw(bool hasFocus, bool drawSelected)
    {
        if (m_pContainer)
        {
            Graphics g(оffScreenBitmap.get());
            g.Clear(Color::Transparent);
            g.SetSmoothingMode(SmoothingModeHighQuality);

            if (Selected && !drawSelected) return;

            Gdiplus::Color selectColor(Gdiplus::Color::White);
            Gdiplus::Pen pen(selectColor, 1.f);

            // Дочерние контролы
            for (const auto& child : m_children)
            {
                child->Draw(hasFocus, drawSelected);
                Gdiplus::Bitmap* childBmp = child->GetOffscreenBitmap();
                if (childBmp)
                {
                    const CRect& childRect = child->Border;
                    g.DrawImage(childBmp, childRect.left, childRect.top,
                        childRect.Width(), childRect.Height());
                }
            }

            CPoint clienTL = Border.TopLeft();
            ParentToLocal(clienTL);
            g.DrawRectangle(&pen,
                static_cast<float>(clienTL.x),
                static_cast<float>(clienTL.y),
                static_cast<float>(Border.Width() - 1),
                static_cast<float>(Border.Height() - 1));

            DrawBorder2(g);
        }
    }

    void Panel::Invalidate()
    {
        m_isDirty = true;
        for (const auto& child : m_children)
        {
            child->Invalidate();
        }
        Control::Invalidate();
    }
} // namespace PluginGUI
