#pragma once

#include <afxwin.h>
#include <chrono>
#include <gdiplus.h>

namespace PluginGUI
{
	/// <summary>
	/// Выполнить функцию и измерить время ее выполнения в миллисекундах.
	/// Примеры использования
	/// 1.Если функция возвращает значение
	/// auto [result, time_ms] = measureExecutionTime([&]() {
	///		return someFunction(...);
	///	});
	/// 2.Если функция не возвращает значение
	/// auto time_ms = measureExecutionTime([&]() {
	///		someFunction(...);
	///	});
	/// </summary>
	/// <typeparam name="Func"></typeparam>
	/// <param name="func"></param>
	/// <returns></returns>
	template<typename Func>
	auto measureExecutionTime(Func&& func) {
		auto start = std::chrono::high_resolution_clock::now();

		if constexpr (std::is_void_v<std::invoke_result_t<Func>>) {
			std::forward<Func>(func)();
			auto end = std::chrono::high_resolution_clock::now();
			// duration в миллисекундах, преобразуем в double
			std::chrono::duration<double, std::milli> duration = end - start;
			return duration.count();
		}
		else {
			auto result = std::forward<Func>(func)();
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> duration = end - start;
			return std::make_pair(result, duration.count());
		}
	}

	float roundToDecimals(float value, int decimals = 2);
	void DrawBitmap(CPaintDC& dc, Gdiplus::Bitmap* bitmap, const CRect& Border, bool useLayered = false);

	static int sign(int x) 
	{ 
		return (x > 0) ? 1 : ((x < 0) ? -1 : 0); 
	}

	void CreateRoundedRectPath(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, int radius);

	enum class CornerMask : UINT
	{
		None = 0,
		TopLeft = 1 << 0,    // Бит 0
		TopRight = 1 << 1,    // Бит 1
		BottomRight = 1 << 2,    // Бит 2  
		BottomLeft = 1 << 3,    // Бит 3

		Top = TopLeft | TopRight,
		Bottom = BottomRight | BottomLeft,
		Left = TopLeft | BottomLeft,
		Right = TopRight | BottomRight,
		All = 0xF
	};

	void CreateRoundedRectPath(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, int radius, CornerMask cornerMask);
}

