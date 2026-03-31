#include "PluginGUI\include\PropertyRegistry.h"
#include "PluginGUI\include\ButtonControl.h"

namespace PluginGUI
{
    void RegisterControlProperties(PropertyRegistry& reg)
    {
        // id — просто ваши PropertyName / Names значения
        using Id = int;

        reg.Register(static_cast<Id>(Control::PropertyName::pnBaseColor), {
            _T("The control's background color. Used as the base color for all elements."),
            std::nullopt,
            false  // редакируемое
            });

        reg.Register(static_cast<Id>(Control::PropertyName::pnBorder), {
            _T("Control bounds relative to a window or panel."),
            std::nullopt,
            false
            });

        reg.Register(static_cast<Id>(Control::PropertyName::pnName), {
            _T("Control name"),
            std::nullopt,
            false
            });

        reg.Register(static_cast<Id>(Control::PropertyName::pnFocus), {
            _T("Does it have focus control?"),
            std::nullopt,
            true
            });

        reg.Register(static_cast<Id>(Control::PropertyName::pnValueChangeMode), {
            _T("Is it possible to change the control value by moving the mouse?"),
            std::nullopt,
            true
            });

        reg.Register(static_cast<Id>(Control::PropertyName::pnSelected), {
            _T("Is the control selected?"),
            std::nullopt,
            true
            });
    }

    void RegisterButtonControlProperties(PropertyRegistry& reg)
    {
        // id — просто ваши PropertyName / Names значения
        using Id = int;

        reg.Register(static_cast<Id>(ButtonControl::PropertyName::pnOnColor), {
            _T("Button color when pressed."),
            std::nullopt,
            false  // редакируемое
            });

        reg.Register(static_cast<Id>(ButtonControl::PropertyName::pnOffColor), {
            _T("Button color when released."),
            std::nullopt,
            false
            });

        reg.Register(static_cast<Id>(ButtonControl::PropertyName::pnTextColor), {
            _T("Button text color."),
            std::nullopt,
            false
            });

        reg.Register(static_cast<Id>(ButtonControl::PropertyName::pnText), {
            _T("Button text."),
            std::nullopt,
            false
            });
    }
}