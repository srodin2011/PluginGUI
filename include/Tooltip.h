#pragma once
#include <afxwin.h>
#include <string>
#include <memory>
#include <gdiplus.h>

namespace PluginGUI
{
    class CCustomTooltip : public CWnd
    {
    private:
        HWND hParent;
        std::wstring currentText;
        bool bVisible;
        CRect Border{ 0, 0, 0, 0 };
        std::unique_ptr<Gdiplus::Bitmap> оffScreenBitmap = nullptr; // Bitmap для двойной буферизации

    protected:
        afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
        afx_msg void OnPaint();
        afx_msg void OnDestroy();
        afx_msg BOOL OnEraseBkgnd(CDC* pDC);
        afx_msg void OnSize(UINT nType, int cx, int cy);
        DECLARE_MESSAGE_MAP()

        void CalculateSizeAndPositionForControl(const CRect& controlBorderScreen);
        void SetBorder(const CRect& Border);

    public:
        CCustomTooltip();
        virtual ~CCustomTooltip();

        bool CreateTooltip(HWND hParentWnd);
        bool IsVisible() const { return bVisible; }

        // 1. ПОКАЗАТЬ тултип
        void ShowForControl(const CRect& controlBorderScreen, const std::wstring& text);

        // 2. СКРЫТЬ тултип
        void Hide();

        // 3. ОБНОВИТЬ текст+позицию
        void UpdateForControl(const CRect& controlBorderScreen, const std::wstring& text);
    };
}