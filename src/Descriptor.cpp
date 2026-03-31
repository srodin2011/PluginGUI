#include "PluginGUI\include\Descriptor.h"

namespace PluginGUI
{
    //////////////////////////////////////////////////////////////////////////
    // ControlDescriptor}

    /// <summary>
    /// number of predefined names
    /// </summary>
    /// <returns></returns>
    int ControlDescriptor::NameCount() const noexcept
    {
        int count = 0;
        for (const NameInfo* p = m_names; !p->m_name.empty(); ++p)
            ++count;
        return m_base ? count + m_base->NameCount() : count;
    }

    /// <summary>
    /// find ID by name
    /// </summary>
    /// <param name="name">property name</param>
    /// <returns>name ID</returns>
    int ControlDescriptor::NameId(std::string name) const noexcept
    {
        for (const NameInfo* p = m_names; !p->m_name.empty(); ++p)
        {
            if (p->m_name == name)  // C++17: view без копирования!
                return p->m_id;
        }

        return m_base ? m_base->NameId(name) : -1;
    }

    /// <summary>
    /// supports custom properties?
    /// </summary>
    /// <returns></returns>
    bool ControlDescriptor::SupportCustom() const
    {
        return m_custom || m_base && m_base->SupportCustom();
    }
}