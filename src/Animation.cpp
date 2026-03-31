//#include <afxwin.h> Убрал
#include "PluginGUI\include\Animation.h"

namespace PluginGUI
{
	Animation::Animation(std::function<void()> onStartAnimation, std::function<void(float)> onUpdateAnimation, std::function<void()> onStopAnimation, int duration):
		OnStartAnimation(onStartAnimation),
		OnUpdateAnimation(onUpdateAnimation),
		OnStopAnimation(onStopAnimation),
		m_AnimatingForward(false),
		m_AnimationDuration(duration), // миллисекунд
		m_TimerId(0),
		m_AnimationStartTime(0)
	{
	}

	/// <summary>
	/// Запустить анимацию
	/// </summary>
	/// <param name="forward">Направление анимации</param>
	void Animation::StartAnimation(bool forward)
	{
		//std::lock_guard<std::mutex> lock(m_TimerMutex); // Заблокировать поток на основе m_Mutex
		std::scoped_lock lock(m_TimerMutex); // Заблокировать поток на основе m_Mutex

		m_AnimatingForward = forward; // Запомнить направление анимации
		m_AnimationStartTime = GetTickCount64(); // Запомнить теущее значение таймера 
		if (OnStartAnimation)
		{
			OnStartAnimation(); // Вызвать обработчик события - запуск таймера
		}
		StartAnimationTimer(); // Запустить таймер
	}

	/// <summary>
	/// Запустить таймер
	/// </summary>
	void Animation::StartAnimationTimer()
	{
		if (m_TimerId == 0)
		{
			m_TimerId = SetTimer(NULL, 0, 16, TimerProc); // Создать таймер
			m_TimerInstanceMap[m_TimerId] = this; // Запомнить экземпляр класса, соответствующий таймеру, в карте таймеров
		}
	}

	/// <summary>
	/// Остановить анимацию
	/// </summary>
	void Animation::StopAnimationTimer()
	{
		if (m_TimerId != 0)
		{
			KillTimer(NULL, m_TimerId); // Удалить таймер как объект ОС
			m_TimerInstanceMap.erase(m_TimerId); // Удалить таймер из карты таймеров
			m_TimerId = 0; // Удалить идентификатор таймера
			if (OnStopAnimation)
			{
				OnStopAnimation(); // Вызвать обработчик события - остановка таймера
			}
		}
	}

	/// <summary>
	/// Oбновить анимацию, вызывается из таймера
	/// </summary>
	void Animation::UpdateAnimation()
	{
		//std::lock_guard<std::mutex> lock(m_TimerMutex); // Заблокировать поток на основе m_Mutex
		std::scoped_lock lock(m_TimerMutex); // Заблокировать поток на основе m_Mutex

		ULONGLONG now = GetTickCount64(); // Получить теущее значение таймера
		ULONGLONG elapsed = now - m_AnimationStartTime; // Определить интервал времени прошедшего с момента запуска стаймера

		if (elapsed >= m_AnimationDuration) // Остановить таймер
		{
			if (OnUpdateAnimation)
			{
				OnUpdateAnimation(m_AnimatingForward ? 1.0f : 0.0f); // Вызвать обработчик события - обновление таймера
			}
			StopAnimationTimer();
		}
		else
		{
			// Вычислить новое значение прогресса анимации (значение в интервале [0;1])
			float t = static_cast<float>(elapsed) / m_AnimationDuration;
			if (OnUpdateAnimation)
			{
				OnUpdateAnimation(m_AnimatingForward ? t : (1.0f - t));
			}
		}

		// Вызовите метод перерисовки элемента
		if (OnInvalidate)
		{
			OnInvalidate();
		}
	}


	/// <summary>
	/// Статическая функция таймера, вызавается по истечении заданного интервала в милисекундах
	/// </summary>
	/// <param name=""></param>
	/// <param name=""></param>
	/// <param name="idEvent"></param>
	/// <param name=""></param>
	/// <returns></returns>
	VOID CALLBACK Animation::TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
	{
		auto it = m_TimerInstanceMap.find(idEvent); // Найти таймер по идентификатору
		if (it != m_TimerInstanceMap.end())
		{
			it->second->UpdateAnimation(); // Вызвать обработчик для соответствующего экземпляра класса
		}
	}
}