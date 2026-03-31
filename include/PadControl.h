#pragma once

#include "Control.h"

namespace PluginGUI
{
	class PadControl : public Control
	{
		REGISTER_PLUGIN(PadControl, Control, ControlId::Pad)

		void Init() {}; // Инициализация для вызова из конструктора только!

	public:
		PadControl(const CRect& Border);

		void Draw(bool hasFocus, bool drawSelected) override;

		virtual UINT GetMinWidth() const override
		{
			return 31;
		}
		virtual UINT GetMinHeight() const override
		{
			return 32;
		}
	};
}