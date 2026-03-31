#pragma once

namespace PluginGUI
{
    class Control;

    //using ControlId = unsigned int;

    enum class ControlId : uint32_t
    {
        Button = 1,
        Slider = 2,
        Knob = 3,
        CheckBox = 4,
        Lable = 5,
        Pad = 6,
        Envelop = 7,
        ButtonImage = 8,
        ButtonRadioImage = 9
        // Добавляйте новые вручную
    };
}
