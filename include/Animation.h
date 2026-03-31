#pragma once

#include "framework.h" // Стандартные компоненты MFC
#include <mutex>
#include <unordered_map>
#include <functional>

namespace PluginGUI
{
	class Animation
	{
	public:
		Animation(std::function<void()> onStartAnimation, std::function<void(float)> onUpdateAnimation, std::function<void()> onStopAnimation, int duration = 200);
		virtual ~Animation()
		{
			StopAnimationTimer();
		}

		void StartAnimation(bool forward);

		int GetAnimatingForward() const
		{
			return m_AnimatingForward;
		}
		void SetAnimatingForward(bool forward)
		{
			m_AnimatingForward = forward;
		}
		__declspec(property(get = GetAnimatingForward, put = SetAnimatingForward)) int AnimatingForward;

		/// <summary>
		/// Установить функцию отрисовки при анимации
		/// </summary>
		/// <param name="callback"></param>
		void SetInvalidateCallback(std::function<void()> callback) // Установка callback для перерисовки
		{
			OnInvalidate = callback;
		}

		/// <summary>
		/// Запущена ли анимация
		/// </summary>
		/// <returns></returns>
		bool AnimationStarted() const
		{
			return m_TimerId != 0;
		}

	protected:
		bool m_AnimatingForward; // Направление анимации
		const ULONGLONG m_AnimationDuration; // Время анимации в миллисекундах
		UINT_PTR m_TimerId; // Идентификатор таймера
		ULONGLONG m_AnimationStartTime; // Начальное время таймера
		std::mutex m_TimerMutex; // Объект блокировки при работе с таймером

		std::function<void()> OnInvalidate; // Метод для перерисовки при анимации
		std::function<void(float)> OnUpdateAnimation; // Метод для перерисовки при анимации
		std::function<void()> OnStartAnimation; // Метод для перерисовки при анимации
		std::function<void()> OnStopAnimation; // Метод для перерисовки при анимации

		// Статическая карта для связи таймера и экземпляра класса 
		static inline std::unordered_map<UINT_PTR, Animation*> m_TimerInstanceMap;

		static VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD);

		void StartAnimationTimer();
		void UpdateAnimation(); // Метод для обновления анимации, вызывается из таймера
		void StopAnimationTimer();
	}; // Animation
} // PluginGUI