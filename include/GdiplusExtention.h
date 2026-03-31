#pragma once

#include <gdiplus.h>

namespace Gdiplus
{
	inline bool operator==(const Color& c1, const Color& c2) noexcept
	{
		return c1.GetValue() == c2.GetValue();
	}

	inline bool operator!=(const Color& c1, const Color& c2) noexcept
	{
		return c1.GetValue() != c2.GetValue();
	}

	inline bool operator==(const Color& c, ARGB argb) noexcept
	{
		return c.GetValue() == argb;
	}

	inline bool operator==(ARGB argb, const Color& c) noexcept
	{
		return c == argb;
	}
}

