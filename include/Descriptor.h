#pragma once
#include <string>

namespace PluginGUI
{
    struct ControlDescriptor
    {
        struct NameInfo
        {
            std::string m_name;  // property name
            int         m_id;     // property ID
        };

        ControlDescriptor* m_base;     // base control descriptor
        std::string        m_typename; // type name
        bool               m_custom;   // supports custom properties?
        const NameInfo*    m_names;    // predefined name table

        // number of predefined names
        int NameCount() const noexcept;
        // find ID by name
        int NameId(std::string name) const noexcept;
        // supports custom properties?
        bool SupportCustom() const;
        // type name
        std::string TypeName() const { return m_typename; }
    };

    #define DECLARE_DESCRIPTOR()                        \
        public:                                         \
        static ControlDescriptor m_descr;                \
        virtual const ControlDescriptor* getDescriptor() const; \
        protected:                                      \
        static ControlDescriptor::NameInfo m_stdNames[];

    // internal macro: start descriptor definition
    #define BEGIN_DESCRIPTOR_(cname,baseref,typeName,custom) \
        const ControlDescriptor* cname::getDescriptor() const   \
            { return &m_descr; }                       \
        ControlDescriptor cname::m_descr =                   \
            { baseref, #typeName, custom, m_stdNames };    \
        ControlDescriptor::NameInfo cname::m_stdNames[] = {

    // start descriptor definition (with base class)
    #define BEGIN_DESCRIPTOR(cname,typeName,custom)  \
        BEGIN_DESCRIPTOR_(cname,&Base::m_descr,typeName,custom)

    #define NAME_INFO(name,id) { #name, static_cast<int>(id) },

    // end descriptor definition
    #define END_DESCRIPTOR() { "", -1 } };

}
