#pragma once

#include <list>
#include <functional>
#include <algorithm>

namespace PluginGUI
{
    template<typename... Args>
    class Event
    {
    private:
        using Callback = std::function<void(Args...)>;
        std::list<Callback*> m_Observers;

    public:
        // Тип для указателя на Callback
        using CallbackPtr = Callback*;

        /// <summary>
        /// Добавить подписку на событие
        /// </summary>
        /// <param name="cb">Функция обратного вызова</param>
        /// <returns>указатель для последующей отписки</returns>
        Callback* Subscribe(const Callback& cb)
        {
            auto* ptr = new Callback(cb);
            m_Observers.push_back(ptr);
            return ptr;
        }

        // Отписка по указателю

        /// <summary>
        /// Отписателя от события по указателю
        /// </summary>
        /// <param name="cbPtr">Указатель подписки</param>
        void Unsubscribe(Callback* cbPtr)
        {
            m_Observers.remove(cbPtr);
            delete cbPtr;
        }

        /// <summary>
        /// Вызов события (уведомление всех подписчиков) 
        /// </summary>
        /// <param name="...args">Аргументы события</param>
        void operator()(Args... args) const
        {
            for (auto* cb : m_Observers)
            {
                (*cb)(args...);
            }
        }

        // Деструктор для очистки памяти
        ~Event()
        {
            for (auto* cb : m_Observers)
            {
                delete cb;
            }
            m_Observers.clear();
        }
    };
} // PluginGUI