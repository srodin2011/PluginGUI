
#pragma once

#include "framework.h" // Стандартные компоненты MFC
#include "Tooltip.h"
#include "Container.h"
#include <vector>
#include <memory>

#include "Control.h"
#include "Event.h"

namespace PluginGUI
{
	//class PluginControl;
	class PluginView : public CWnd, public IContainer
	{
		DECLARE_DYNAMIC(PluginView)

	public:
		using SelectionChangedEvent = Event<Control*>;
		SelectionChangedEvent OnSelectionChanged;  // событие

		PluginView(Gdiplus::Color backColor);
		virtual ~PluginView();

		Gdiplus::Color GetBkColor() const
		{
			return m_BackColor;
		}

		static PluginView* GetActiveEditorView();
		static bool GetEditMode() { return s_pActiveEditorView ? s_pActiveEditorView->m_editMode : false; }
		void SetEditMode(bool b);

		bool GetShowGrid() const { return m_showGrid; }
		void SetShowGrid(bool b)
		{
			if (m_showGrid != b)
			{
				m_showGrid = b;
				CWnd::Invalidate();
			}
		}
		__declspec(property(get = GetShowGrid, put = SetShowGrid)) bool ShowGrid;

		const CPoint& GetGridCellSize() const { return m_gridCellSize; }
		void SetGridCellSize(CPoint p)
		{
			if (m_gridCellSize != p)
			{
				m_gridCellSize = p;
				CWnd::Invalidate();
			}
		}
		__declspec(property(get = GetGridCellSize, put = SetGridCellSize)) CPoint GridCellSize;

		// IContainer
		const std::vector<std::shared_ptr<Control>>& GetChildren() const { return m_controls; }

		void Add(Control* control) override;
		void Add(std::shared_ptr<Control> childSP) override;
		void Remove(Control* control) override;
		size_t GetCount() const override;
		size_t GetSelectedCount() const override;
		void OnChildPropertyChanged(Control* pChild, Id id) override;

		void Invalidate(const CRect& rect) override;
		void GetCursorPos(LPPOINT p) override;
		void SetCursorPos(POINT) override;

		// Преобразование глобальные координаты <=> локальные координаты
		void GlobalToLocal(CPoint& p) override;
		void LocalToGlobal(CPoint& p) override;

		// Преобразование координаты родителя <=> локальные координаты
		void ParentToLocal(CPoint& p) override;
		void LocalToParent(CPoint& p) override;

		void ShowTooltipForControl(Control* pControl) override;
		void UpdateTooltipForCurrentControl(Control* pControl) override;
		void HideCurrentTooltip() override;

		Control* GetSingleSelectedControl() const;

	protected:
		CCustomTooltip m_sharedTooltip;
		Control* m_currentTooltipOwner;

		virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;

		HCURSOR hHandCursor;

		enum class DraggingMode
		{
			dmNone,
			dmMoving,
			dmResizing,
			dmSelection
		};

		afx_msg void OnPaint();
		afx_msg void OnMouseMove(UINT, CPoint);
		afx_msg void OnLButtonDown(UINT, CPoint);
		afx_msg void OnLButtonUp(UINT, CPoint);
		afx_msg BOOL OnMouseWheel(UINT, short, CPoint);
		afx_msg BOOL OnEraseBkgnd(CDC*);
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		afx_msg LRESULT OnUserShowTooltip(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnRawInput(WPARAM wParam, LPARAM lParam);
		afx_msg void OnMouseLeave();
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		afx_msg void OnUpdateCommand(CCmdUI* pCmdUI);
		afx_msg void OnCommand(UINT nID);
		afx_msg void OnShowGrid();
		afx_msg void OnUpdateShowGrid(CCmdUI* pCmdUI);
		afx_msg void OnEditMode();
		afx_msg void OnUpdateEditMode(CCmdUI* pCmdUI);
		afx_msg void OnUpdateAlignCommand(CCmdUI* pCmdUI);
		afx_msg void OnAlingLefts();
		afx_msg void OnAlingTops();
		afx_msg void OnAlingRights();
		afx_msg void OnAlingBottoms();
		afx_msg void OnAlingCenters();
		afx_msg void OnAlingMiddles();
		DECLARE_MESSAGE_MAP()

		std::vector<std::shared_ptr<Control>> m_controls;
		mutable Control* m_focusedChild = nullptr;
		ULONG_PTR m_gdiplusToken = 0;
		Gdiplus::Color m_BackColor;
		bool m_editMode;
		static PluginView* s_pActiveEditorView;
		bool m_showGrid = false;
		CPoint m_gridCellSize = { 8, 8 };
		DraggingMode m_draggMode = DraggingMode::dmNone;
		CPoint m_draggPos;
		bool m_selectActive = false;
		int m_selectX = -1;
		int m_selectY = -1;
		Control* m_draggedControl = nullptr;
		Control* m_lastGotFocuseControl = nullptr;
		Control* m_lastSingleSelectedControl = nullptr;
		IContainer* m_selectionContainer = nullptr;
		CRect m_selectionBoundingRect;
		CRect m_startSelectionBoundingRect;
		std::unique_ptr<Gdiplus::Bitmap> m_dragSelectionBitmap = nullptr;
		CRect m_oldSelectionRect;


		void OnPaintNew();
		void DrawGrid(Gdiplus::Graphics& g, const CRect& areaRect);
		void DrawSelection(Gdiplus::Graphics& g, const CRect& areaRect);
		static void DrawBitmap(const CPaintDC& dc, Gdiplus::Bitmap& bitmap, const CRect& Border);
		void StartDragMove();
		void EndDragMove();

		//bool SelectControls(CRect& boundingRectOut);
		IContainer* GetTargetContainer(Control* control);
		IContainer* GetControlOwner(Control* control);
		void SelectControl(Control* pControl);
		bool SelectControls();
		void UpdateSelectionBoundingRect();
		bool DropControlSelection();
		void MoveSelectedControls(CPoint shift);
		void MoveSelectedControlsToContainer(CPoint shift, IContainer* newContainer);
		bool IsControlSelected(Control* pControl) const;

		Control* GetFocusControlOld() const
		{
			auto it = std::find_if(m_controls.begin(), m_controls.end(),
				[](const std::shared_ptr<Control>& p) { return p && p->GetFocus(); });
			auto focusedControl = (it != m_controls.end()) ? *it : nullptr;
			return focusedControl.get();
		}

		Control* GetFocusControl() const
		{
			if (m_focusedChild && m_focusedChild->Focus)
			{
				return m_focusedChild;
			}

			auto it = std::find_if(m_controls.begin(), m_controls.end(),
				[](const std::shared_ptr<Control>& control)
				{
					return control && control->Focus;
				});

			m_focusedChild = (it != m_controls.end()) ? it->get() : nullptr;
			return m_focusedChild;
		}

		Control* HitTestControl(const CPoint& pt) const
		{
			auto it = std::find_if(m_controls.rbegin(), m_controls.rend(),
				[&pt](const std::shared_ptr<Control>& p) { return p && p->HitTest(pt.x, pt.y); });
			auto focusedControl = (it != m_controls.rend()) ? *it : nullptr;
			return focusedControl.get();
		}

		Control* HitTestGlobal(CPoint globalPt);
		static Control* HitTestRecursive(IContainer* container, CPoint pt, bool skipSelected = false);

		CPoint SnapToGrid(CPoint point) const;
	};
} // PluginGUI
