
// ChildView.cpp: реализация класса CChildView
//
#define NOMINMAX
#include "PluginGUI\include\PluginView.h"
#include "PluginGUI\include\Control.h"
#include "PluginGUI\include\commands.h"
#include "PluginGUI\include\Utils.h"
#include "PluginGUI\include\MouseCalibration.h"
#include "resource.h"
#include <iostream>
#include <future> 
#include <ranges>
#include <algorithm>
#include <vector>
#include <memory> 
#include <cmath> 
using namespace Gdiplus;

namespace PluginGUI
{
	PluginView* PluginView::s_pActiveEditorView = nullptr;

	IMPLEMENT_DYNAMIC(PluginView, CWnd)

	BEGIN_MESSAGE_MAP(PluginView, CWnd)
		ON_MESSAGE(WM_INPUT, OnRawInput)
		ON_WM_PAINT()
		ON_WM_MOUSEMOVE()
		ON_WM_LBUTTONDOWN()
		ON_WM_LBUTTONUP()
		ON_WM_MOUSEWHEEL()
		ON_WM_ERASEBKGND()
		ON_WM_SETCURSOR()
		ON_WM_LBUTTONDBLCLK()
		ON_WM_CREATE()
		ON_MESSAGE(WM_USER + 100, OnUserShowTooltip)
		ON_WM_MOUSELEAVE()
		ON_WM_KEYDOWN()
		ON_WM_KEYUP()
		ON_WM_SYSKEYDOWN()
		ON_WM_SYSKEYUP()
		ON_WM_CONTEXTMENU()
		ON_UPDATE_COMMAND_UI_RANGE(ID_PLUGINGUI_FIRST_CONTROL_COMMAND, ID_PLUGINGUI_LAST_CONTROL_COMMAND, OnUpdateCommand)
		ON_COMMAND_RANGE(ID_PLUGINGUI_FIRST_CONTROL_COMMAND, ID_PLUGINGUI_LAST_CONTROL_COMMAND, OnCommand)
		ON_COMMAND(ID_PLUGINGUI_EDIT_SHOWGRID, OnShowGrid)
		ON_UPDATE_COMMAND_UI(ID_PLUGINGUI_EDIT_SHOWGRID, OnUpdateShowGrid)
		ON_COMMAND(ID_EDITMODE, OnEditMode)
		ON_UPDATE_COMMAND_UI(ID_EDITMODE, OnUpdateEditMode)
		ON_UPDATE_COMMAND_UI_RANGE(ID_PLUGINGUI_ALIGHN_TO_GRID, ID_PLUGINGUI_ALIGHN_BOTTOMS, OnUpdateAlignCommand)
		ON_COMMAND(ID_PLUGINGUI_ALIGHN_LEFTS, OnAlingLefts)
		ON_COMMAND(ID_PLUGINGUI_ALIGHN_TOPS, OnAlingTops)
		ON_COMMAND(ID_PLUGINGUI_ALIGHN_RIGHTS, OnAlingRights)
		ON_COMMAND(ID_PLUGINGUI_ALIGHN_BOTTOMS, OnAlingBottoms)
	END_MESSAGE_MAP()

	LRESULT PluginView::OnRawInput(WPARAM wParam, LPARAM lParam)
	{
		if (wParam == RIM_INPUT)
		{
			UINT bufferSize = 0;
			UINT result = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));

			if (result != ~0U)
			{  // ~0U = (UINT)-1 = ошибка
				BYTE* buffer = new BYTE[bufferSize];
				result = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &bufferSize, sizeof(RAWINPUTHEADER));

				if (result != ~0U)
				{
					RAWINPUT* ri = (RAWINPUT*)buffer;
					if (ri->header.dwType == RIM_TYPEMOUSE)
					{
						RAWMOUSE& mouse = ri->data.mouse;

						if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
						{
							LONG dx = mouse.lLastX;  // Дельта X
							LONG dy = mouse.lLastY;  // Дельта Y ← для knob!
							//TRACE(_T("RawInput: dx=%d, dy=%d, flags=0x%04X\n"), dx, dy, mouse.usFlags);

							LONG eventTime = GetMessageTime();
							MouseCalibration::getInstance().updateCalibration(mouse.lLastY, eventTime);

							if (GetKeyState(VK_LBUTTON) & 0x8000)
							{
								Control* control = GetFocusControl();
								if (control)
								{
									UINT nFlags = 0;

									// Кнопки мыши
									if (GetKeyState(VK_LBUTTON) & 0x8000) nFlags |= MK_LBUTTON;
									if (GetKeyState(VK_RBUTTON) & 0x8000) nFlags |= MK_RBUTTON;
									if (GetKeyState(VK_MBUTTON) & 0x8000) nFlags |= MK_MBUTTON;
									if (GetKeyState(VK_SHIFT) & 0x8000) nFlags |= MK_SHIFT;
									if (GetKeyState(VK_CONTROL) & 0x8000) nFlags |= MK_CONTROL;

									control->onMouseMoveAndLButtonDown(nFlags, dx, dy);
								}
							}
						}						
					}
				}
				delete[] buffer;
			}
		}
		return 0;
	}

	PluginView::PluginView(Gdiplus::Color backColor) :
		m_currentTooltipOwner(nullptr),
		m_BackColor(backColor)
	{
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

		hHandCursor = ::LoadCursor(NULL, IDC_HAND);
		//hHandCursor = AfxGetInstanceHandle() ?
		//	AfxGetApp()->LoadStandardCursor(IDC_HAND) : NULL;
	}

	PluginView::~PluginView()
	{
		m_dragSelectionBitmap.reset(); // На всякий случай
		m_controls.clear();
		//m_sharedTooltip.DestroyWindow();
		GdiplusShutdown(m_gdiplusToken);
	}

	PluginView* PluginView::GetActiveEditorView()
	{
		return s_pActiveEditorView;
	}

	void PluginView::SetEditMode(bool b)
	{
		if (m_editMode != b)
		{
			TRACE(_T("SetEditMode=%s\n"), b ? L"true" : L"false");

			if (b) // Входим в режим редактирования
			{
				s_pActiveEditorView = this;

				Control* pFocused = GetFocusControl();
				if (pFocused)
				{
					pFocused->SetFocus(false);
					SelectControl(pFocused);
				}
				else if (m_lastGotFocuseControl)
				{
					SelectControl(m_lastGotFocuseControl);
				}
			}
			else // Выходим из режима редактирования
			{
				DropControlSelection();
				
				if (m_lastSingleSelectedControl)
				{
					m_lastSingleSelectedControl->SetFocus(true);
				}
				else if (m_lastGotFocuseControl)
				{
					m_lastGotFocuseControl->SetFocus(true);
				}

				if (s_pActiveEditorView == this)
				{
					s_pActiveEditorView = nullptr;  // Сбрасываем активный
				}
			}
			m_editMode = b;

			if (ShowGrid)
			{
				CWnd::Invalidate();
			}
		}
	}

	// Обработка сообщений ---------------------------------------------------------------------------

	/// <summary>
	/// Обработка сообщения поворот колесика мышки
	/// </summary>
	/// <param name="nFlags"></param>
	/// <param name="zDelta"></param>
	/// <param name="p"></param>
	/// <returns></returns>
	BOOL PluginView::OnMouseWheel(UINT nFlags, short zDelta, CPoint p)
	{
		if (GetEditMode())
		{
			return TRUE;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			return pFocused->OnMouseWheel(nFlags, zDelta, p);
		}

		return TRUE;
	}

	/// <summary>
	/// Обработка очистки фона окна
	/// </summary>
	/// <param name="pDC"></param>
	/// <returns></returns>
	BOOL PluginView::OnEraseBkgnd(CDC* pDC)
	{
		//Graphics graphics(pDC->GetSafeHdc());
		//graphics.Clear(m_BackColor);

		return TRUE; // фон обработан
	}

	/// <summary>
	/// Обработка перемещения мышки
	/// </summary>
	/// <param name="pWnd"></param>
	/// <param name="nHitTest"></param>
	/// <param name="message"></param>
	/// <returns></returns>
	BOOL PluginView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
	{
		if (GetEditMode())
		{
			return TRUE;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			pFocused->OnSetCursor(pWnd, nHitTest, message);
			//SetCursor(hHandCursor);
			return TRUE; // Курсор установлен, стандартная обработка не нужна
		}

		// Иначе стандартный курсор
		return CWnd::OnSetCursor(pWnd, nHitTest, message);
	}

	/// <summary>
	/// Обработка двойного клика левой клавишей мыши
	/// </summary>
	/// <param name="nFlags"></param>
	/// <param name="point"></param>
	void PluginView::OnLButtonDblClk(UINT nFlags, CPoint point)
	{
		if (GetEditMode())
		{
			return;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			pFocused->OnLButtonDblClk(nFlags, point);
		}
	}

	void PluginView::OnShowGrid()
	{
		if (GetEditMode())
		{
			ShowGrid = !ShowGrid;
			CWnd::Invalidate();
		}
	}

	void PluginView::OnUpdateShowGrid(CCmdUI* pCmdUI)
	{
		pCmdUI->Enable(GetEditMode());
		pCmdUI->SetCheck(ShowGrid);
	}

	void PluginView::OnEditMode()
	{
		SetEditMode(!GetEditMode());
	}

	void PluginView::OnUpdateEditMode(CCmdUI* pCmdUI)
	{
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(GetEditMode());
	}

	void PluginView::OnUpdateAlignCommand(CCmdUI* pCmdUI)
	{
		pCmdUI->Enable(GetSelectedCount() > 1);
	}

	/// <summary>
	/// Выровнять все выбранные контролы по левой границе
	/// </summary>
	void PluginView::OnAlingLefts()
	{
		if (!m_selectionContainer) return; // Нет селекции

		// Ищем левую границу
		int minLeft = INT_MAX;
		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			if (childSP->Selected)
			{
				const CRect& border = childSP->Border;
				minLeft = std::min(minLeft, (int)border.left);
			}
		}

		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			if (childSP->Selected && childSP->Border.left != minLeft)
			{
				CRect newBorder(CPoint(minLeft, childSP->Border.top), childSP->Border.Size());
				childSP->Invalidate();
				childSP->Border = newBorder;
			}
		}

		UpdateSelectionBoundingRect();
	}

	/// <summary>
	/// Выровнять все выбранные контролы по верхней границе
	/// </summary>
	void PluginView::OnAlingTops()
	{
		if (!m_selectionContainer) return; // Нет селекции

		// Ищем левую границу
		int minTop = INT_MAX;
		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			if (childSP->Selected)
			{
				const CRect& border = childSP->Border;
				minTop = std::min(minTop, (int)border.top);
			}
		}

		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			if (childSP->Selected && childSP->Border.top != minTop)
			{
				CRect newBorder(CPoint(childSP->Border.left, minTop), childSP->Border.Size());
				childSP->Invalidate();
				childSP->Border = newBorder;
			}
		}

		UpdateSelectionBoundingRect();
	}

	/// <summary>
	/// Выровнять все выбранные контролы по правой границе
	/// </summary>
	void PluginView::OnAlingRights()
	{
		if (!m_selectionContainer) return; // Нет селекции

		// Ищем левую границу
		int maxRight = INT_MIN;
		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			if (childSP->Selected)
			{
				const CRect& border = childSP->Border;
				maxRight = std::max(maxRight, (int)border.right);
			}
		}

		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			if (childSP->Selected && childSP->Border.left != maxRight)
			{
				CRect newBorder(CPoint(maxRight - childSP->Border.Width(), childSP->Border.top), childSP->Border.Size());
				childSP->Invalidate();
				childSP->Border = newBorder;
			}
		}

		UpdateSelectionBoundingRect();
	}

	/// <summary>
	/// Выровнять все выбранные контролы по нижней границе
	/// </summary>
	void PluginView::OnAlingBottoms()
	{
		if (!m_selectionContainer) return; // Нет селекции

		// Ищем левую границу
		int maxBottom = INT_MIN;
		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			if (childSP->Selected)
			{
				const CRect& border = childSP->Border;
				maxBottom = std::max(maxBottom, (int)border.bottom);
			}
		}

		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			if (childSP->Selected && childSP->Border.left != maxBottom)
			{
				CRect newBorder(CPoint(childSP->Border.left, maxBottom - childSP->Border.Height()), childSP->Border.Size());
				childSP->Invalidate();
				childSP->Border = newBorder;
			}
		}

		UpdateSelectionBoundingRect();
	}

	void PluginView::OnAlingCenters()
	{
		if (!m_selectionContainer) return; // Нет селекции

		// Ищем левую границу
		int maxBottom = INT_MIN;
		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			if (childSP->Selected)
			{
				const CRect& border = childSP->Border;
				maxBottom = std::max(maxBottom, (int)border.bottom);
			}
		}

		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			if (childSP->Selected && childSP->Border.left != maxBottom)
			{
				CRect newBorder(CPoint(childSP->Border.left, maxBottom - childSP->Border.Height()), childSP->Border.Size());
				childSP->Invalidate();
				childSP->Border = newBorder;
			}
		}

		UpdateSelectionBoundingRect();
	}

	void PluginView::OnAlingMiddles()
	{
	}

	/// <summary>
	/// Обработать перемещение курсора
	/// </summary>
	/// <param name="nFlags"></param>
	/// <param name="p"></param>
	void PluginView::OnMouseMove(UINT nFlags, CPoint p)
	{
		if (GetEditMode())
		{
			if (m_draggMode == DraggingMode::dmNone && m_draggedControl)
			{
				auto pControl = HitTestGlobal(p);
				if (pControl && m_draggedControl != pControl && !pControl->Selected)
				{
					DropControlSelection();
					SelectControl(m_draggedControl);
				}
				StartDragMove();
			}

			if (m_draggMode == DraggingMode::dmMoving)
			{
				if (m_draggPos != p)
				{
					if (GetAsyncKeyState(VK_MENU) & 0x8000)
					{
						// Привязываем ЛК угол объекта к сетке
						CPoint objectCorner = m_selectionBoundingRect.TopLeft();
						CPoint snappedCorner = SnapToGrid(objectCorner + (p - m_draggPos));
						CPoint shift = snappedCorner - objectCorner;

						MoveSelectedControls(shift);
						m_draggPos += shift;
					}
					else
					{
						// Свободное перемещение
						CPoint shift = p - m_draggPos;
						MoveSelectedControls(shift);
						m_draggPos = p;
					}
				}
			}
			else if (m_draggMode == DraggingMode::dmResizing)
			{
				if (m_draggPos != p)
				{
					CPoint delta = p - m_draggPos;

					const CRect& refBorder = m_draggedControl->Border;
					CPoint fixedCorner = refBorder.TopLeft();  // ← Фиксируем ЛК угол!

					CRect referenceNewBorder;
					CPoint trackingShift(0, 0);

					if (GetAsyncKeyState(VK_MENU) & 0x8000)
					{
						// ALT: snapping ПРАВОГО НИЖНЕГО угла
						CPoint bottomRight = refBorder.BottomRight();
						CPoint snappedBR = SnapToGrid(bottomRight + delta);
						referenceNewBorder = CRect(fixedCorner, snappedBR);  // ← ЛК угол не меняется!
						trackingShift = snappedBR - bottomRight; // Для изменения m_draggPos 
					}
					else
					{
						// Free resize: ЛК угол тоже фиксируем
						CSize newSize = refBorder.Size() + CSize(delta.x, delta.y);
						referenceNewBorder = CRect(fixedCorner, newSize);
					}

					// Проверка минимальных размеров
					if (referenceNewBorder.IsRectEmpty() ||
						referenceNewBorder.Width() < m_draggedControl->GetMinWidth() ||
						referenceNewBorder.Height() < m_draggedControl->GetMinHeight())
						return;

					// 1. Применяем к reference объекту
					//CPoint clientTL = fixedCorner;
					//m_selectionContainer->LocalToGlobal(clientTL);
					//m_selectionBoundingRect = CRect(clientTL, referenceNewBorder.Size());

					m_draggedControl->Invalidate();
					m_draggedControl->Border = referenceNewBorder;

					// 2. Синхронный ресайз остальных контролов
					CSize sizeDelta = referenceNewBorder.Size() - refBorder.Size();
					for (auto& childSP: m_selectionContainer->GetChildren())
					{
						Control* control = childSP.get();

						if (control != m_draggedControl && control->Selected)
						{
							// Независимые проверки по X/Y
							int newWidth = control->Border.Width() + sizeDelta.cx;
							int newHeight = control->Border.Height() + sizeDelta.cy;

							// X ось
							if (newWidth < control->GetMinWidth())
								newWidth = control->Border.Width();

							// Y ось  
							if (newHeight < control->GetMinHeight())
								newHeight = control->Border.Height();

							CRect newControlBorder(control->Border.TopLeft(),
								CSize(newWidth, newHeight));

							control->Invalidate();
							control->Border = newControlBorder;
						}
					}

					// 3. ОБНОВЛЯЕМ bounding rect для ВСЕХ объектов!
					UpdateSelectionBoundingRect();

					if (GetAsyncKeyState(VK_MENU) & 0x8000)
					{
						m_draggPos += trackingShift;
					}
					else
					{
						m_draggPos = p;
					}
				}
				//CPoint delta = p - m_draggPos;
				//if (delta.x != 0 || delta.y != 0)
				//{
				//	const CRect& border = m_draggedControl->Border;
				//	CSize newSize = border.Size() + CSize(delta.x, delta.y);  // Увеличиваем размер
				//	CRect newBorder(border.TopLeft(), newSize);              // Новый CRect

				//	if (newBorder.IsRectEmpty() ||
				//		newBorder.Width() < m_draggedControl->GetMinWidth() ||
				//		newBorder.Height() < m_draggedControl->GetMinHeight())
				//	{
				//		return;
				//	}

				//	CPoint clientTL = border.TopLeft();
				//	m_selectionContainer->LocalToGlobal(clientTL);
				//	m_selectionBoundingRect = CRect(clientTL, newBorder.Size());

				//	m_draggedControl->Invalidate();
				//	m_draggedControl->Border = newBorder;
				//	m_draggPos = p;
				//	m_draggedControl->Invalidate();
				//}
			}
			else if(m_draggMode == DraggingMode::dmSelection)
			{
				CRect clientRect;
				GetClientRect(&clientRect);

				CRect newSelRect(m_draggPos, p);
				newSelRect.NormalizeRect();
				newSelRect.IntersectRect(&newSelRect, clientRect);

				// Union старой + новой области
				CRect updateRect = m_oldSelectionRect | newSelRect;
				updateRect.InflateRect(100, 100);

				InvalidateRect(&updateRect, TRUE);
				UpdateWindow();

				m_oldSelectionRect = newSelRect;  // Кэш для следующего раза

				TRACE("OnMouseMove m_oldSelectionRect: (%d,%d)-(%d,%d)\n",
					m_oldSelectionRect.left,
					m_oldSelectionRect.top,
					m_oldSelectionRect.right,
					m_oldSelectionRect.bottom);
			}
			else
			{
				auto pControl = HitTestGlobal(p);
				if (pControl && pControl->Selected)
				{
					CPoint controlLocal = p;
					pControl->m_pContainer->ParentToLocal(controlLocal);
					if (pControl->Border.HitTestCorner(controlLocal))
					{
						SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
					}
					else
					{
						SetCursor(LoadCursor(NULL, IDC_SIZEALL));
					}
				}
				else
				{
					SetCursor(LoadCursor(NULL, IDC_ARROW));
				}
			}

			if (ShowGrid)
			{
				CWnd::Invalidate();
			}

			//UpdateWindow();

			return;
		}

		//Control* pOldFocused = GetFocusControl();
		//if (pOldFocused /*&& pOldFocused->ValueChangeMode*/)
		//{
		//	pOldFocused->OnMouseMove(nFlags, p);
		//}

		//Control* pNewFocused = HitTestControl(p);
		//if (pNewFocused && pOldFocused != pNewFocused/*&& pNewFocused->ValueChangeMode*/)
		//{
		//	pNewFocused->OnMouseMove(nFlags, p);
		//}

		//if (pOldFocused == pNewFocused)
		//	return; // Если фокус не изменился - ничего не делать

		//// Обновляем фокус
		//if (pOldFocused && !(nFlags & MK_LBUTTON))
		//{
		//	pOldFocused->OnMouseLeave();
		//	pOldFocused->SetFocus(false);
		//}
		//if (pNewFocused && !(nFlags & MK_LBUTTON))
		//{
		//	pNewFocused->SetFocus(true);
		//	m_lastGotFocuseControl = pNewFocused;
		//}

		Control* pOldFocused = m_focusedChild;  
		//Control* pNewFocused = HitTestControl(p);
		Control* pNewFocused = HitTestGlobal(p);
		if (pOldFocused)
		{
			pOldFocused->OnMouseMove(nFlags, p);
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
			m_lastGotFocuseControl = pNewFocused;
		}
	}

	/// <summary>
	/// Обработать нажатие левой кнопки мышки
	/// </summary>
	/// <param name="nFlags"></param>
	/// <param name="p"></param>
	void PluginView::OnLButtonDown(UINT nFlags, CPoint p)
	{
		SetCapture();
		ClipCursor(NULL);

		if (GetEditMode())
		{
			bool isCtrl = (nFlags & MK_CONTROL) != 0;
			bool isShift = (nFlags & MK_SHIFT) != 0;
			if (isCtrl || isShift) return;

			auto pControl = HitTestGlobal(p);


			if (pControl)
			{
				if (!pControl->Selected)
				{
					if (m_selectionContainer && m_selectionContainer->GetSelectedCount())
					{
						DropControlSelection();
					}
					SelectControl(pControl);
				}

				//ASSERT(pControl->Selected && m_selectionContainer != nullptr);
				m_draggedControl = pControl;

				// Перед проверкой преобразовываем координаты окна в локальные координаты контрола
				CPoint controlLocal = p;
				pControl->m_pContainer->ParentToLocal(controlLocal);
				if (pControl->Border.HitTestCorner(controlLocal))
				{
					m_draggMode = DraggingMode::dmResizing;
				}
				else
				{
					//StartDragMove();
				}
			}
			else
			{
				DropControlSelection();
				m_draggMode = DraggingMode::dmSelection;
				m_oldSelectionRect.SetRectEmpty();
			}

			m_draggPos = p;

			return;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			if ((nFlags & MK_CONTROL) == 0 && pFocused->СanChangeByMouse())
			{
				pFocused->SetValueChangeMode(true);
			}
			pFocused->OnLButtonDown(nFlags, p);
		}
	}

	/// <summary>
	/// Обработать отпускание левой кнопки мышки
	/// </summary>
	/// <param name="nFlags"></param>
	/// <param name="p"></param>
	void PluginView::OnLButtonUp(UINT nFlags, CPoint p)
	{
		ReleaseCapture();
		ClipCursor(NULL);

		if (GetEditMode())
		{
			if (m_draggMode == DraggingMode::dmSelection)
			{
				// Очистка старой области селекции
				if (!m_oldSelectionRect.IsRectEmpty())
				{
					SelectControls();

					m_oldSelectionRect.InflateRect(4, 4);
					InvalidateRect(&m_oldSelectionRect, FALSE);
					m_oldSelectionRect.SetRectEmpty();
				}
			}
			else if (m_draggMode == DraggingMode::dmMoving)
			{
				// 1. Вычисляем итоговый сдвиг
				CPoint finalShift = m_selectionBoundingRect.TopLeft() -
					m_startSelectionBoundingRect.TopLeft();

				// 2. Находим НОВЫЙ контейнер под курсором
				Control* dropTarget = HitTestGlobal(p);
				IContainer* targetDropContainer = GetTargetContainer(dropTarget);

				// 3. Перемещаем контролы
				MoveSelectedControlsToContainer(finalShift, targetDropContainer);

				// Смена контейнера-владельца селекции
				if (m_selectionContainer != targetDropContainer)
				{
					m_selectionContainer = targetDropContainer;
				}

				// 4. Очистка
				EndDragMove();
			}
			else if (m_draggMode == DraggingMode::dmNone)
			{
				bool isCtrl = (nFlags & MK_CONTROL) != 0;
				bool isShift = (nFlags & MK_SHIFT) != 0;

				Control* targetControl = HitTestGlobal(p);

				if (targetControl)
				{
					IContainer* targetContainer = targetControl->m_pContainer;

					// 1. селекции нет
					//if (!m_selectionContainer)
					//{
					//	SelectControl(targetControl);
					//}
					//// 2: селекция есть и уровки объектов совпадают
					//else 
					if (m_selectionContainer == targetContainer)
					{
						if (isCtrl || isShift)
						{
							if (m_selectionContainer->GetSelectedCount() == 1 && targetControl->Selected)
							{
								DropControlSelection();
							}
							else
							{
								targetControl->Selected = !targetControl->Selected;
								UpdateSelectionBoundingRect();
							}
						}
					}
					// Другой контейнер
					else
					{
						DropControlSelection();
						SelectControl(targetControl); 
					}

					Control* pPrimary = GetSingleSelectedControl();  // nullptr или один контрол
					OnSelectionChanged(pPrimary);  // событие уведомляет MainFrame
				}
			}
			//CWnd::Invalidate();

			m_draggedControl = nullptr;
			m_draggMode = DraggingMode::dmNone;

			return;
		}

		Control* pFocused = GetFocusControl();

		if (pFocused)
		{
			if (pFocused->СanChangeByMouse() && pFocused->ValueChangeMode)
			{
				pFocused->SetValueChangeMode(false);
				if (!pFocused->HitTest(p.x, p.y))
				{
					pFocused->SetFocus(false);
				}
			}
			pFocused->OnLButtonUp(nFlags, p);
		}
	}

	void PluginView::OnMouseLeave()
	{
		if (GetEditMode())
		{
			return;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			pFocused->OnMouseLeave();
		}
	}

	void PluginView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		if (GetEditMode())
		{
			return;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			pFocused->OnKeyDown(nChar, nRepCnt, nFlags);
		}

		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
	}

	void PluginView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		if (GetEditMode())
		{
			return;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			pFocused->OnKeyDown(nChar, nRepCnt, nFlags);
		}

		CWnd::OnKeyUp(nChar, nRepCnt, nFlags);
	}

	void PluginView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		if (GetEditMode())
		{
			return;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			pFocused->OnSysKeyDown(nChar, nRepCnt, nFlags);
		}

		CWnd::OnSysKeyDown(nChar, nRepCnt, nFlags);
	}

	void PluginView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		if (GetEditMode())
		{
			return;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			pFocused->OnSysKeyUp(nChar, nRepCnt, nFlags);
		}

		CWnd::OnSysKeyUp(nChar, nRepCnt, nFlags);
	}

	void PluginView::OnContextMenu(CWnd* pWnd, CPoint point)
	{
		if (GetEditMode())
		{
			return;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			pFocused->OnContextMenu(point);
		}
	}

	void PluginView::OnUpdateCommand(CCmdUI* pCmdUI)
	{
		if (GetEditMode())
		{
			return;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			pFocused->OnUpdateCommand(pCmdUI);
		}
	}

	void PluginView::OnCommand(UINT nID)
	{
		if (GetEditMode())
		{
			return;
		}

		Control* pFocused = GetFocusControl();
		if (pFocused)
		{
			pFocused->OnCommand(nID);
		}
	}

	// Обработчики сообщений ----------------------------------------------------------------------------------------------------------

	BOOL PluginView::PreCreateWindow(CREATESTRUCT& cs) 
	{
		cs.style |= WS_CLIPSIBLINGS | WS_CHILD | CS_DBLCLKS;
		return CWnd::PreCreateWindow(cs);
	}

	/// <summary>
	/// Обработка отрисовки
	/// </summary>
	void PluginView::OnPaint()
	{
		auto time_ms = measureExecutionTime([&]()
			{
				TRACE(_T("Цикл отрисовки -------------------------------------------------- \n"));
				OnPaintNew();
			});
		TRACE("Время выполнения \"OnPaint()\" = %.3f\n", time_ms);
	}

	int PluginView::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		if (CWnd::OnCreate(lpCreateStruct) == -1) return -1;
		m_sharedTooltip.CreateTooltip(m_hWnd);

		RAWINPUTDEVICE rid = { 0x01, 0x02, RIDEV_INPUTSINK , m_hWnd };
		BOOL regOk = RegisterRawInputDevices(&rid, 1, sizeof(rid));
		TRACE("RawInput OnCreate: %d (hWnd=0x%X)\n", regOk, m_hWnd);

		return 0;
	}

	void PluginView::Invalidate(const CRect& rect)
	{
		if (m_hWnd != 0)
		{
			InvalidateRect(rect, false);
			TRACE(_T("Invalidated rect: (%d,%d)-(%d,%d)\n"), rect.left, rect.top, rect.right, rect.bottom);
		}
	}

	void PluginView::GetCursorPos(LPPOINT p)
	{
		::GetCursorPos(p);           // Экранные координаты
		ScreenToClient(p);         // Переводим в локальные
	}

	void PluginView::SetCursorPos(POINT p)
	{
		ClientToScreen(&p);
		::SetCursorPos(p.x, p.y);
	}

	void PluginView::GlobalToLocal(CPoint& p)
	{
	}

	void PluginView::LocalToGlobal(CPoint& p)
	{
	}

	void PluginView::ParentToLocal(CPoint& p)
	{
	}

	void PluginView::LocalToParent(CPoint& p)
	{
	}

	void PluginView::ShowTooltipForControl(Control* pControl)
	{
		if (!pControl) return;

		if (m_sharedTooltip.IsVisible())
		{
			m_sharedTooltip.Hide();
		}

		m_currentTooltipOwner = pControl;
		CRect Border;
		Border.CopyRect((const CRect&)pControl->GetBorder());
		ClientToScreen(&Border);
		m_sharedTooltip.ShowForControl(Border, pControl->GetTooltipText());
	}

	void PluginView::UpdateTooltipForCurrentControl(Control* pControl)
	{
		if (m_currentTooltipOwner == pControl && m_sharedTooltip.IsVisible())
		{
			CRect Border;
			Border.CopyRect((const CRect&)pControl->GetBorder());
			ClientToScreen(&Border);
			m_sharedTooltip.UpdateForControl(Border, pControl->GetTooltipText());
		}
	}

	void PluginView::HideCurrentTooltip()
	{  // ✅ Новый метод
		if (m_sharedTooltip.IsVisible())
		{
			m_sharedTooltip.Hide();
			m_currentTooltipOwner = nullptr;
		}
	}

	LRESULT PluginView::OnUserShowTooltip(WPARAM wParam, LPARAM lParam)
	{
		Control* pControl = (Control*)wParam;
		ShowTooltipForControl(pControl);
		return 0;
	}

	void PluginView::DrawBitmap(const CPaintDC& dc, Bitmap& bitmap, const CRect& Border)
	{
		UINT width = bitmap.GetWidth();
		UINT height = bitmap.GetHeight();

		// Создаём 32-битный DIBSection с альфа-каналом
		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = -(LONG)height; // отрицательное для верхнего левого угла
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32; // 32 бита на пиксель
		bmi.bmiHeader.biCompression = BI_RGB;

		void* pBits = nullptr;
		CPaintDC& _dc = const_cast<CPaintDC&>(dc);

		CDC memDC;
		memDC.CreateCompatibleDC(&_dc);

		HBITMAP hDIB = CreateDIBSection(memDC.GetSafeHdc(), &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
		if (!hDIB || !pBits)
			return;

		HBITMAP hOldBitmap = (HBITMAP)memDC.SelectObject(hDIB);

		// Копируем данные из Gdiplus::Bitmap в DIBSection
		Gdiplus::BitmapData bitmapData;
		Gdiplus::Rect rect(0, 0, width, height);

		if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) == Gdiplus::Ok)
		{
			BYTE* srcPixels = (BYTE*)bitmapData.Scan0;
			BYTE* dstPixels = (BYTE*)pBits;
			int srcStride = bitmapData.Stride;
			int dstStride = width * 4;

			for (UINT y = 0; y < height; y++)
			{
				BYTE* srcRow = srcPixels + y * srcStride;
				BYTE* dstRow = dstPixels + y * dstStride;

				for (UINT x = 0; x < width; x++)
				{
					BYTE B = srcRow[x * 4 + 0];
					BYTE G = srcRow[x * 4 + 1];
					BYTE R = srcRow[x * 4 + 2];
					BYTE A = srcRow[x * 4 + 3];

					float alpha = A / 255.0f;

					// premultiply alpha
					BYTE Rp = static_cast<BYTE>(R * alpha);
					BYTE Gp = static_cast<BYTE>(G * alpha);
					BYTE Bp = static_cast<BYTE>(B * alpha);

					dstRow[x * 4 + 0] = Bp; // Blue
					dstRow[x * 4 + 1] = Gp; // Green
					dstRow[x * 4 + 2] = Rp; // Red
					dstRow[x * 4 + 3] = A;  // Alpha
				}
			}
			// Копируем построчно, учитывая возможный stride
			//for (UINT y = 0; y < height; y++)
			//{
			//	memcpy(dstPixels + y * dstStride, srcPixels + y * srcStride, dstStride);
			//}

			bitmap.UnlockBits(&bitmapData);
		}
		else
		{
			// Если не удалось заблокировать, очистим DIB прозрачным цветом
			memset(pBits, 0, width * height * 4);
		}

		// Выводим с помощью AlphaBlend
		BLENDFUNCTION blend = { 0 };
		blend.BlendOp = AC_SRC_OVER;
		blend.SourceConstantAlpha = 255;
		blend.AlphaFormat = AC_SRC_ALPHA;

		_dc.AlphaBlend(
			Border.left, Border.top,
			Border.Width(), Border.Height(),
			&memDC,
			0, 0,
			width, height,
			blend);

		// Освобождаем ресурсы
		memDC.SelectObject(hOldBitmap);
		DeleteObject(hDIB);
		//HBITMAP hBitmap = NULL;
		//Gdiplus::Status status = m_OffScreenBitmap->GetHBITMAP(Gdiplus::Color::Transparent, &hBitmap);
		//if (status == Gdiplus::Ok && hBitmap != NULL)
		//{
		//	CPaintDC& _dc = const_cast<CPaintDC&>(dc);
		//	// Создаём совместимый DC
		//	CDC memDC;
		//	memDC.CreateCompatibleDC(&_dc);

		//	// Выбираем HBITMAP в memDC
		//	HBITMAP hOldBitmap = (HBITMAP)memDC.SelectObject(hBitmap);

		//	BLENDFUNCTION blend = { 0 };
		//	blend.BlendOp = AC_SRC_OVER;
		//	blend.SourceConstantAlpha = 255;
		//	blend.AlphaFormat = AC_SRC_ALPHA;

		//	_dc.AlphaBlend(
		//		m_Border.left, m_Border.top,
		//		m_Border.Width(), m_Border.Height(),
		//		&memDC,
		//		0, 0,
		//		m_Border.Width(), m_Border.Height(),
		//		blend);

		//	// Копируем из memDC в pDC
		//	//_dc.BitBlt((INT)m_Border.left, (INT)m_Border.top, (INT)m_Border.Width(), (INT)m_Border.Height(), &memDC, 0, 0, SRCCOPY);

		//	// Восстанавливаем старый битмап и удаляем созданный
		//	memDC.SelectObject(hOldBitmap);
		//	DeleteObject(hBitmap);
		//}
	}

	void PluginView::DrawGrid(Gdiplus::Graphics& g, const CRect& areaRect)
	{
		if (!GetEditMode() || !ShowGrid) return;

		Gdiplus::SolidBrush dotBrush(Gdiplus::Color(160, 160, 160));

		// Точно как в SnapToGrid — рисуем ИЗ 0,0 клиентских координат!
		for (int x = 0; x <= areaRect.right; x += GridCellSize.x)
		{
			for (int y = 0; y <= areaRect.bottom; y += GridCellSize.y)
			{
				// Локальные координаты bitmap
				float localX = static_cast<float>(x - areaRect.left);
				float localY = static_cast<float>(y - areaRect.top);

				// Только точки внутри areaRect
				if (localX >= -1.0f && localX <= areaRect.Width() + 1.0f &&
					localY >= -1.0f && localY <= areaRect.Height() + 1.0f)
				{
					g.FillEllipse(&dotBrush, localX - 1, localY - 1, 2.0f, 2.0f);
				}
			}
		}
	}

	void PluginView::DrawSelection(Gdiplus::Graphics& g, const CRect& areaRect)
	{
		//if (m_draggMode != DraggingMode::dmSelection)
		//{
		//	return;
		//}

		//Gdiplus::Color selectColor(Gdiplus::Color::Yellow);
		//Gdiplus::Pen pen(selectColor, 1.f);

		//CPoint pt;
		//GetCursorPos(&pt);
		////::ScreenToClient(GetSafeHwnd(), &pt);

		//CRect clientRect;
		//GetClientRect(&clientRect);

		//CRect selRect(m_draggPos, pt);
		//selRect.NormalizeRect();
		//selRect.IntersectRect(selRect, clientRect/*CRect(0, 0, clientRect.Width(), clientRect.Height())*/);

		//g.DrawRectangle(&pen,
		//	static_cast<float>(selRect.left),
		//	static_cast<float>(selRect.top),
		//	static_cast<float>(selRect.Width()),
		//	static_cast<float>(selRect.Height()));
		if (m_draggMode != DraggingMode::dmSelection ||
			m_oldSelectionRect.IsRectEmpty())
		{
			return;
		}

		Gdiplus::Color selectColor(Gdiplus::Color::Yellow);
		Gdiplus::Pen pen(selectColor, 1.f);

		CRect localRect = m_oldSelectionRect;
		localRect.OffsetRect(-areaRect.left, -areaRect.top);

		// Используем кэшированный rect из OnMouseMove!
		g.DrawRectangle(&pen,
			static_cast<float>(localRect.left),
			static_cast<float>(localRect.top),
			static_cast<float>(localRect.Width() - 1),
			static_cast<float>(localRect.Height() - 1));

		//TRACE("DrawSelectinon m_oldSelectionRect: (%d,%d)-(%d,%d)\n",
		//	m_oldSelectionRect.left,
		//	m_oldSelectionRect.top,
		//	m_oldSelectionRect.right,
		//	m_oldSelectionRect.bottom);
	}

	void PluginView::OnPaintNew()
	{
		RECT r;
		GetUpdateRect(&r, true);

		CPaintDC dc(this);
		Graphics screenGraphics(dc.m_hDC);

		CRect areaRect(r);

		UINT areaWidth = areaRect.Width();
		UINT areaHeight = areaRect.Height();

		auto startBufferCreation = std::chrono::high_resolution_clock::now();

		// 1. Создаём общий буфер (bitmap) для всей области отрисовки
		Gdiplus::Bitmap compositeBitmap(areaWidth, areaHeight, PixelFormat32bppARGB);
		Gdiplus::Graphics compositeGraphics(&compositeBitmap);


		compositeGraphics.Clear(GetBkColor());


		// Создать фон
		//Gdiplus::Bitmap backgroundBitmap(areaWidth, areaHeight, PixelFormat32bppARGB);
		//Graphics backgroundGraphics(&backgroundBitmap);
		//backgroundGraphics.Clear(GetBkColor());

		// Рисуем фон (всю область или часть)
		//compositeGraphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
		//compositeGraphics.DrawImage(&backgroundBitmap,
		//	0, 0,                      // куда рисуем (в буфер)
		//	0, 0, // часть фона (source)
		//	//areaRect.left, areaRect.top, // часть фона (source)
		//	areaWidth, areaHeight,
		//	Gdiplus::UnitPixel);

		auto endBufferCreation = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> durationBufferCreation = endBufferCreation - startBufferCreation;
		TRACE(_T("Время создания буферов: %.3f мс\n"), durationBufferCreation.count());

		DrawGrid(compositeGraphics, areaRect);

		auto startElapsedControls = std::chrono::high_resolution_clock::now();

		bool old = false;

		if (old)
		{
			// 2. Для каждого объекта рисуем на общий буфер нужную часть
			for (auto& pControl : m_controls)
			{
				// Вычисляем пересечение объекта с областью отрисовки
				CRect intersection;
				if (!intersection.IntersectRect(pControl->Border, areaRect))
					continue; // нет пересечения - пропускаем

				// Размеры пересечения
				UINT interWidth = intersection.Width();
				UINT interHeight = intersection.Height();

				// Смещение внутри объекта (откуда брать пиксели)
				int srcX = intersection.left - pControl->Border.left;
				int srcY = intersection.top - pControl->Border.top;

				// Смещение в общем буфере (куда рисовать)
				int destX = intersection.left - areaRect.left;
				int destY = intersection.top - areaRect.top;

				auto timeDrawControl = measureExecutionTime([&]()
					{
						//screenGraphics.DrawImage(&compositeBitmap, (int)areaRect.left, (int)areaRect.top); 
						if (pControl->isDirty())
						{
							bool drawSelected = m_draggMode != DraggingMode::dmMoving;
							pControl->Draw(drawSelected);
							TRACE(_T("Перерисовка объекта \"%s\" drawSelected=%s\n"),
								pControl->Name.c_str(), drawSelected ? _T("true") : _T("false"));
						}
					});
				TRACE("Время выполнения \"Draw\" одного контрола = %.3f\n", timeDrawControl);

				if (pControl->Selected && m_draggMode == DraggingMode::dmMoving) continue;

				// Рисуем часть объекта на общий буфер
				compositeGraphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
				compositeGraphics.DrawImage(pControl->оffScreenBitmap.get(),
					destX, destY,           // куда рисуем в compositeBitmap
					srcX, srcY,             // часть объекта bitmap
					interWidth, interHeight,
					Gdiplus::UnitPixel);
			}
		}
		else
		{
			// 2. ПАРАЛЛЕЛЬНО создаём/обновляем bitmaps контролов
			std::vector<std::future<void>> futures;
			for (auto& pControl : m_controls)
			{
				CRect intersection;
				if (!intersection.IntersectRect(pControl->Border, areaRect))
					continue;

				UINT interWidth = intersection.Width();
				UINT interHeight = intersection.Height();
				int srcX = intersection.left - pControl->Border.left;
				int srcY = intersection.top - pControl->Border.top;

				// 🔥 ПАРАЛЛЕЛЬНО рисуем контрол в его offScreenBitmap!
				futures.push_back(std::async(std::launch::async, [=, &pControl]()
					{
						auto timeDrawControl = measureExecutionTime([&]()
							{
								if (pControl->isDirty())
								{
									pControl->Draw(m_draggMode != DraggingMode::dmMoving);
								}
							});
						TRACE("Worker поток: Draw контрола = %.3fмс\n", timeDrawControl);
					}));
			}

			// Ждём ВСЕ потоки (макс время самого долгого контрола!)
			for (auto& future : futures)
			{
				future.wait();
			}

			// 3. Собираем ГОТОВЫЕ bitmaps в composite (0.1мс/контрол)
			for (auto& pControl : m_controls)
			{
				if (pControl->Selected && m_draggMode == DraggingMode::dmMoving) continue;

				CRect intersection;
				if (!intersection.IntersectRect(pControl->Border, areaRect))
					continue;

				int destX = intersection.left - areaRect.left;
				int destY = intersection.top - areaRect.top;
				int srcX = intersection.left - pControl->Border.left;
				int srcY = intersection.top - pControl->Border.top;
				UINT interWidth = intersection.Width();
				UINT interHeight = intersection.Height();

				compositeGraphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
				compositeGraphics.DrawImage(pControl->GetOffscreenBitmap(),
					destX, destY,
					srcX, srcY,
					interWidth, interHeight,
					Gdiplus::UnitPixel);
			}
		}
		auto endElapsedControls = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> durationElapsedControls = endElapsedControls - startElapsedControls;

		TRACE(_T("Суммарное время отрисовки всех контролов: %.3f мс\n"), durationElapsedControls.count());

		// Рисуем drag контролы
		if (m_draggMode == DraggingMode::dmMoving && m_dragSelectionBitmap)
		{
			// Если битмап создан — рисуем его
			CRect intersection;
			intersection.IntersectRect(m_selectionBoundingRect, areaRect);  // Только видимая часть

			TRACE(_T("m_selectionBoundingRect (%d,%d)-(%d,%d)\n"), m_selectionBoundingRect.left, m_selectionBoundingRect.top, m_selectionBoundingRect.right, m_selectionBoundingRect.bottom);
			TRACE(_T("areaRect (%d,%d)-(%d,%d)\n"), areaRect.left, areaRect.top, areaRect.right, areaRect.bottom);

			if (!intersection.IsRectEmpty())
			{
				// Координаты в буфере compositeGraphics (относительно areaRect)
				int destX = intersection.left - areaRect.left;
				int destY = intersection.top - areaRect.top;
				int srcX = intersection.left - m_selectionBoundingRect.left;
				int srcY = intersection.top - m_selectionBoundingRect.top;
				UINT interWidth = intersection.Width();
				UINT interHeight = intersection.Height();

				compositeGraphics.DrawImage(m_dragSelectionBitmap.get(),
					destX, destY, 
					srcX, srcY,
					interWidth, interHeight,
					Gdiplus::UnitPixel);
			}
		}

		DrawSelection(compositeGraphics, areaRect);

		// Выводим итог на экран
		// Здесь DrawImage более медленный вариант. Например, при перерисовке всего окна DrawImage отрабатывает за 22 мс, а DrawBitmap за 18 мс
		//screenGraphics.DrawImage(&compositeBitmap, (int)areaRect.left, (int)areaRect.top); 
		
		auto time_ms = measureExecutionTime([&]() {
			//screenGraphics.DrawImage(&compositeBitmap, (int)areaRect.left, (int)areaRect.top); 
			DrawBitmap(dc, compositeBitmap, areaRect);
			});
		TRACE("Время выполнения \"DrawBitmap\" = %.3f\n", time_ms);

	}

	void PluginView::StartDragMove()
	{
		m_draggMode = DraggingMode::dmMoving;

		// Создаем битмап 
		CRect bitmapRect = m_selectionBoundingRect;
		m_dragSelectionBitmap.reset();
		m_dragSelectionBitmap = std::make_unique<Gdiplus::Bitmap>(
			bitmapRect.Width(), bitmapRect.Height(), PixelFormat32bppARGB);

		Gdiplus::Graphics dragGraphics(m_dragSelectionBitmap.get());
		dragGraphics.Clear(Color::Transparent);  

		// Перевести bitmapRect
		CPoint bitmapTL = bitmapRect.TopLeft();
		m_selectionContainer->GlobalToLocal(bitmapTL);

		TRACE(_T("StartDragMove(). m_selectionBoundingRect (%d,%d)-(%d,%d)\n"), m_selectionBoundingRect.left, m_selectionBoundingRect.top, m_selectionBoundingRect.right, m_selectionBoundingRect.bottom);

		// Рисуем Selected контролы
		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			Control* child = childSP.get();
			if (child->Selected)
			{
				//CRect childLocalRect;
				//childLocalRect.SetRect(
				//	child->Border.left,
				//	child->Border.top,
				//	child->Border.right,
				//	child->Border.bottom
				//);
				const CRect& border = child->Border;
				CRect childLocalRect(border.TopLeft(), border.Size());
				childLocalRect.OffsetRect(-bitmapTL);

				if (child->isDirty())
				{
					child->Draw(true);
				}
				dragGraphics.DrawImage(child->GetOffscreenBitmap(),
					childLocalRect.left, childLocalRect.top,
					childLocalRect.Width(), childLocalRect.Height());
			}
		}

		if (Control* control = dynamic_cast<Control*>(m_selectionContainer))
		{
			control->SetDirty();  // Рекурсивно вверх!
		}
	}

	void PluginView::EndDragMove()
	{
		//Invalidate(m_selectionBoundingRect);
		m_draggMode = DraggingMode::dmNone;
		m_dragSelectionBitmap.reset();  // УДАЛЯЕМ битмап
		m_startSelectionBoundingRect = m_selectionBoundingRect;
	}

	/// <summary>
	/// Добавить элемент управления к окну
	/// </summary>
	/// <param name="control"></param>
	void PluginView::Add(Control* control)
	{
		if (control)
		{
			if (GetEditMode())
			{
				DropControlSelection();
			}
			control->m_pContainer = this; // Сохранить ссылку на окно
			if (GetEditMode())
			{
				//control->Selected = true;
				//control->DrawBorder = true;
			}
			m_controls.push_back(std::shared_ptr<Control>(control));
			if (m_hWnd != 0)
			{
				UpdateWindow();
			}
		}
	}

	void PluginView::Add(std::shared_ptr<Control> childSP)
	{
		if (childSP.get())
		{
			childSP->m_pContainer = this; // Сохранить ссылку на окно
			m_controls.push_back(childSP);
		}
	}

	void PluginView::Remove(Control* control)
	{
		if (control)
		{
			// Ищем контрол в коллекции
			auto it = std::find_if(m_controls.begin(), m_controls.end(),
				[control](const std::shared_ptr<Control>& c)
				{
					return c.get() == control;
				});

			// Удаляем, если нашли
			if (it != m_controls.end())
			{
				// Нужно перерисовать область на которой был контрол
				control->Invalidate();
				m_controls.erase(it);
				control->m_pContainer = nullptr;
			}
		}
	}

	/// <summary>
	/// Количество контролов
	/// </summary>
	/// <returns></returns>
	size_t PluginView::GetCount() const
	{
		return m_controls.size();
	}

	/// <summary>
	/// Посчитать количество выбранных контролов
	/// </summary>
	/// <returns></returns>
	size_t PluginView::GetSelectedCount() const
	{
		if (!m_selectionContainer) return 0;

		const auto& children = m_selectionContainer->GetChildren();
		return std::count_if(children.begin(), children.end(),
			[](const auto& childSP)
			{
				return childSP->Selected;
			});
	}

	void PluginView::OnChildPropertyChanged(Control* pChild, Id id)
	{
		switch (static_cast<Control::PropertyName>(id))
		{
			case Control::PropertyName::pnSelected:
				break;
			case Control::PropertyName::pnBorder:
				break;
		}
	}

	// Работа с выделением контролов в режиме редактирования --------------------------------------------------------------------

	/// <summary>
	/// Выделить все элементы управления, попавшие в область выделения
	/// </summary>
	/// <returns></returns>
	//bool PluginView::SelectControls(CRect& boundingRectOut)
	//{
	//	bool selected = false;
	//	boundingRectOut.SetRectEmpty();

	//	CPoint pt;
	//	GetCursorPos(&pt);
	//	ScreenToClient(&pt);

	//	CRect selectionRect(
	//		min(m_draggPos.x, pt.x),
	//		min(m_draggPos.y, pt.y),
	//		max(m_draggPos.x, pt.x),
	//		max(m_draggPos.y, pt.y));

	//	auto selectControl = [&](std::shared_ptr<Control>& control)
	//		{
	//			const CRect& ctrlRect = control->Border;
	//			bool fullyInside = selectionRect.PtInRect(ctrlRect.TopLeft()) &&
	//				selectionRect.PtInRect(ctrlRect.BottomRight());

	//			if (fullyInside)
	//			{
	//				control->Selected = true;
	//				selected = true;
	//				boundingRectOut.UnionRect(boundingRectOut, ctrlRect);  // Всегда!
	//			}
	//		};

	//	std::for_each(m_controls.begin(), m_controls.end(), selectControl);
	//	return selected;
	//}

	IContainer* PluginView::GetTargetContainer(Control* control) 
	{
		IContainer* targetContainer = nullptr;
		if (!control)
		{
			targetContainer = this;  // PluginView
		}
		else if (auto container = dynamic_cast<IContainer*>(control))
		{
			targetContainer = container;  // pControl сам контейнер (панель)
		}
		else
		{
			targetContainer = control->m_pContainer;  // Обычный контрол → его панель
		}
		return targetContainer;
	}

	IContainer* PluginView::GetControlOwner(Control* control)
	{
		IContainer* targetContainer = nullptr;
		if (!control)
		{
			targetContainer = this;  // PluginView
		}
		else
		{
			targetContainer = control->m_pContainer;  // Обычный контрол → его панель
		}
		return targetContainer;
	}


	void PluginView::SelectControl(Control* pControl)
	{
		m_selectionContainer = GetControlOwner(pControl);

		const CRect& border = pControl->Border;
		CPoint clientTL = border.TopLeft();
		m_selectionContainer->LocalToGlobal(clientTL);

		m_selectionBoundingRect = CRect(clientTL, border.Size());
		m_startSelectionBoundingRect = m_selectionBoundingRect;
		m_lastSingleSelectedControl = pControl;

		pControl->Selected = true;

		OnSelectionChanged(pControl);
	}

	/// <summary>
	/// Выделить контролы по области, с учетом выбранного контейнера
	/// </summary>
	/// <param name="pControl">Выбранный контрол контейнер, если не nullptr</param>
	/// <param name="boundingRectOut"></param>
	/// <returns></returns>
	bool PluginView::SelectControls()
	{
		int selectedCount = false;
		m_selectionBoundingRect.SetRectEmpty();

		// ТОЛЬКО текущая позиция мыши
		CPoint pt;
		GetCursorPos(&pt);
		//ScreenToClient(&pt);

		m_selectionContainer = GetTargetContainer(m_draggedControl);
		ASSERT(m_selectionContainer != nullptr);

		// Считаем contSelectionRect в координатах m_selectionContainer 
		CPoint cont_draggPos = m_draggPos;
		CPoint cont_pt = pt;
		m_selectionContainer->ParentToLocal(cont_draggPos);
		m_selectionContainer->ParentToLocal(cont_pt);

		CRect contSelectionRect(
			std::min(cont_draggPos.x, cont_pt.x),
			std::min(cont_draggPos.y, cont_pt.y),
			std::max(cont_draggPos.x, cont_pt.x),
			std::max(cont_draggPos.y, cont_pt.y));

		// Убрать селекцию в с экрана
		Invalidate(contSelectionRect);

		Control* lastSelected = nullptr;
		// Ищем пересечения среди детей m_selectionContainer 
		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			Control* child = childSP.get();
			
			// Ищем пересечение контрола с областью селекции
			if (child->Border.IntersectRect(child->Border, contSelectionRect))
			{
				child->Selected = true;
				m_selectionBoundingRect.UnionRect(m_selectionBoundingRect, child->Border);
				lastSelected = child;
				selectedCount++;
			}
		}

		// Сохраняем указатель на выбранный единстренный контрол
		m_lastSingleSelectedControl = selectedCount == 1 ? lastSelected : nullptr;

		m_startSelectionBoundingRect = m_selectionBoundingRect;

		// !!!! m_selectionBoundingRect в локальных ккординатах контейнера. Нужно перевести вглобальные координаты!!!
			// !!!! ПЕРЕВОД В ГЛОБАЛЬНЫЕ КООРДИНАТЫ !!!!
		if (!m_selectionBoundingRect.IsRectEmpty())
		{
			CPoint clientTL = m_selectionBoundingRect.TopLeft();
			m_selectionContainer->LocalToGlobal(clientTL);
			m_selectionBoundingRect = CRect(clientTL, m_selectionBoundingRect.Size());
		}

		OnSelectionChanged(m_lastSingleSelectedControl);

		return selectedCount > 0;
	}

	/// <summary>
	/// Обновить общую область выделенных контролов. Нужно после ресайза.
	/// </summary>
	void PluginView::UpdateSelectionBoundingRect()
	{
		m_selectionBoundingRect.SetRectEmpty();

		if (!m_selectionContainer || m_selectionContainer->GetChildren().size() == 0) return;

		// Union всех границ выделенных контролов
		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			Control* control = childSP.get();

			CPoint clientTL = control->Border.TopLeft();
			m_selectionContainer->LocalToGlobal(clientTL);

			CRect controlGlobalRect(clientTL, control->Border.Size());

			m_selectionBoundingRect |= controlGlobalRect;  // Union!
		}

		m_startSelectionBoundingRect = m_selectionBoundingRect;
	}

	bool PluginView::DropControlSelection()
	{
		if (!m_selectionContainer) return false; // Нет селекции

		bool deselected = false;
		for (auto& childSP : m_selectionContainer->GetChildren())
		{
			Control* child = childSP.get();
			if (child->Selected)
			{
				child->Selected = false;
				deselected = true;
			}
		}

		m_selectionBoundingRect.SetRectEmpty();
		m_startSelectionBoundingRect.SetRectEmpty();
		m_selectionContainer = nullptr;  // Сбрасываем!

		OnSelectionChanged(nullptr);

		return deselected;
	}

	/// <summary>
	/// Сбросить селекцию у все элементов управления
	/// </summary>
	//bool PluginView::DropControlSelection()
	//{
	//	bool deselected = false;

	//	std::for_each(m_controls.begin(), m_controls.end(),
	//		[&](std::shared_ptr<Control>& control)
	//		{
	//			if (control->Selected)
	//			{
	//				control->Selected = false;
	//				//control->DrawBorder = false;
	//				deselected = true;
	//			}
	//		});

	//	return deselected;  // true если снят хотя бы один выделенный
	//}

	//void PluginView::MoveSelectedControls(CPoint shift)
	//{
	//	CRect clientRect;
	//	GetClientRect(&clientRect);
	//	clientRect.DeflateRect(5, 5);  // Отступы от краёв

	//	// Проверяем все выбранные контролы на возможность перемещения
	//	bool canMoveAll = !std::any_of(m_controls.begin(), m_controls.end(),
	//		[&](const std::shared_ptr<Control>& control)
	//		{
	//			if (control->Selected)
	//			{
	//				Frame newRect = control->Border;
	//				newRect.OffsetRect(shift);
	//				return !clientRect.PtInRect(newRect.TopLeft()) ||
	//					!clientRect.PtInRect(newRect.BottomRight());
	//			}
	//			return false;
	//		});

	//	// Если все помещаются — перемещаем
	//	if (canMoveAll)
	//	{
	//		std::for_each(m_controls.begin(), m_controls.end(),
	//			[&](std::shared_ptr<Control>& control)
	//			{
	//				if (control->Selected)
	//				{
	//					Frame newRect = control->Border;
	//					newRect.OffsetRect(shift);
	//					control->Invalidate();
	//					control->Border = newRect;
	//					control->Invalidate();
	//				}
	//			});
	//	}
	//}

	void PluginView::MoveSelectedControls(CPoint shift)
	{
		if (!m_selectionContainer || shift == CPoint(0, 0)) return;

		CRect oldRect = m_selectionBoundingRect;
		m_selectionBoundingRect.OffsetRect(shift);

		// Динамический margin по осям + cap
		int dx = std::min(std::max((int)abs(shift.x) * 2, 8), 30);
		int dy = std::min(std::max((int)abs(shift.y) * 2, 8), 30);

		CRect unionRect = oldRect | m_selectionBoundingRect;
		unionRect.InflateRect(dx, dy);

		InvalidateRect(&unionRect, FALSE);

		TRACE(_T("MoveSelectedControls m_selectionBoundingRect.TopLeft (%d,%d)\n"), m_selectionBoundingRect.left, m_selectionBoundingRect.top);

		//if (!m_selectionContainer) return;

		//CRect oldRect(m_selectionBoundingRect);

		//// 2. Смещаем bounding rect
		//m_selectionBoundingRect.OffsetRect(shift);

		//CRect newRect(m_selectionBoundingRect);

		//CRect total = oldRect | newRect;
		//total.InflateRect(30, 30);
		////InvalidateRect(oldRect | newRect, TRUE);
		//InvalidateRect(total);
		////UpdateWindow();
	}

	/// <summary>
	/// Завершение перемещения. При этом контролы могут поменять контейнер
	/// </summary>
	/// <param name="shift"></param>
	/// <param name="newContainer"></param>
	void PluginView::MoveSelectedControlsToContainer(CPoint shift, IContainer* newContainer)
	{
		if (m_selectionContainer == newContainer)
		{
			// Тот же контейнер, простое перемещение 
			for (auto& childSP : m_selectionContainer->GetChildren())
			{
				if (childSP->Selected)
				{
					const CRect& border = childSP->Border;
					CRect newBorder(border.TopLeft(), border.Size());
					newBorder.OffsetRect(shift);
					childSP->Border = newBorder;
					childSP->SetDirty();
				}
			}
		}
		else
		{
			// 1. Собираем ВСЕ выбранные элементы
			std::vector<std::shared_ptr<Control>> selectedControls;
			for (auto& childSP : m_selectionContainer->GetChildren())
			{
				if (childSP->Selected)
				{
					selectedControls.push_back(childSP);  // refcount=2!
				}
			}

			// 2. Перемещаем каждый элемент
			for (auto& controlSP : selectedControls)
			{
				Control* control = controlSP.get();

				// Переводим координаты контрола в глобальные координаты
				CPoint globalPos = control->Border.TopLeft();
				m_selectionContainer->LocalToGlobal(globalPos);
				globalPos += shift;

				CSize borderSize(control->Border.Size());

				m_selectionContainer->Remove(control);
				newContainer->Add(controlSP);

				// Переводим координаты контрола в локальные нового контейнера
				newContainer->GlobalToLocal(globalPos);
				control->Border = CRect(globalPos, borderSize);
				control->SetDirty();
			}
			//for (auto& childSP : m_selectionContainer->GetChildren())
			//{
			//	if (childSP->Selected)
			//	{
			//		Control* control = childSP.get();

			//		control->Invalidate();

			//		// Переводим координаты контрола в глобальные координаты
			//		CPoint globalPos = control->Border.TopLeft();
			//		m_selectionContainer->LocalToGlobal(globalPos);
			//		globalPos += shift;

			//		// Переводим координаты контрола в локальные нового контейнера
			//		newContainer->GlobalToLocal(globalPos);
			//		control->Border = CRect(globalPos, control->Border.Size());

			//		// 3. Перемещаем
			//		m_selectionContainer->Remove(control);
			//		newContainer->Add(control);  

			//		//control->Invalidate();
			//	}
			//}
		}
	}


	/// <summary>
	/// Найти контрол, который содержит точку, и находиется по иерархии максимально "глубоко".
	/// </summary>
	/// <param name="clientPt">Точка в координатах клиентской области окна</param>
	/// <returns></returns>
	Control* PluginView::HitTestGlobal(CPoint clientPt)
	{
		return HitTestRecursive(this, clientPt, m_draggMode == DraggingMode::dmMoving);
	}

	/// <summary>
	/// Найти контрол, который содержит точку, на данном уровне иерархии и провалиться глубже, если это IContainer
	/// </summary>
	/// <param name="container">Контейнер контролов</param>
	/// <param name="pt">Точка в координатах контейнера</param>
	/// <returns></returns>
	Control* PluginView::HitTestRecursive(IContainer* container, CPoint pt, bool skipSelected)
	{
		auto& children = container->GetChildren();
		for (auto it = children.rbegin(); it != children.rend(); ++it)
		{
			Control* child = it->get();
			if (skipSelected && child->Selected) continue;

			if (child->Border.PtInRect(pt))
			{
				if (IContainer* subContainer = dynamic_cast<IContainer*>(child))
				{
					CPoint childLocalPt = pt;  // Копируем
					subContainer->ParentToLocal(childLocalPt);  // container → child
					if (Control* deepest = HitTestRecursive(subContainer, childLocalPt, skipSelected))
					{
						return deepest;
					}
				}
				return child;
			}
		}
		return nullptr;
	}

	CPoint PluginView::SnapToGrid(CPoint point) const
	{
		CRect clientRect;
		GetClientRect(&clientRect);

		// Округление к сетке
		int gridX = static_cast<int>(std::floor(static_cast<float>(point.x) / GridCellSize.x + 0.5f));
		int gridY = static_cast<int>(std::floor(static_cast<float>(point.y) / GridCellSize.y + 0.5f));

		// Координаты узла сетки
		CPoint snappedPoint(gridX * GridCellSize.x, gridY * GridCellSize.y);

		// Ограничение в границы клиента
		snappedPoint.x = std::clamp(snappedPoint.x, 0L, clientRect.right);
		snappedPoint.y = std::clamp(snappedPoint.y, 0L, clientRect.bottom);

		return snappedPoint;
	}

	bool PluginView::IsControlSelected(Control* pControl) const
	{
		if (!pControl || !m_selectionContainer)
			return false;

		return pControl->Selected &&
			std::any_of(m_selectionContainer->GetChildren().begin(),
				m_selectionContainer->GetChildren().end(),
				[pControl](const auto& childSP)
				{
					return childSP.get() == pControl && childSP->Selected;
				});
	}

	Control* PluginView::GetSingleSelectedControl() const
	{
		if (!m_selectionContainer)
			return nullptr;

		const auto& children = m_selectionContainer->GetChildren();
		Control* pSingle = nullptr;

		for (const auto& childSP : children)
		{
			Control* pCtrl = childSP.get();
			if (pCtrl && pCtrl->Selected)
			{
				if (pSingle == nullptr)
					pSingle = pCtrl;
				else
					return nullptr;  // уже второй выбранный → нет единого
			}
		}

		return pSingle;
	}
}

