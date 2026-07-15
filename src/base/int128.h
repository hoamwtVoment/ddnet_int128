/* Portable signed 128-bit integer for world coordinates.
 * Uses compiler __int128 when available; soft implementation on MSVC.
 * Soft i128 is a trivial aggregate so it can live in vector unions. */
#ifndef BASE_INT128_H
#define BASE_INT128_H

#include <cstdint>

#if defined(__SIZEOF_INT128__)
#define DDNET_HAS_NATIVE_INT128 1
#endif

#if defined(DDNET_HAS_NATIVE_INT128)

using i128 = __int128;
using u128 = unsigned __int128;

inline constexpr i128 I128(int64_t v) { return static_cast<i128>(v); }

inline int64_t i128_to_int64(i128 v) { return static_cast<int64_t>(v); }

inline double i128_to_double(i128 v)
{
	if(v < 0)
		return -i128_to_double(-v);
	const u128 u = static_cast<u128>(v);
	return static_cast<double>(static_cast<uint64_t>(u)) +
	       static_cast<double>(static_cast<uint64_t>(u >> 64)) * 18446744073709551616.0;
}

inline i128 i128_from_double(double v)
{
	if(v < 0)
		return -i128_from_double(-v);
	constexpr double Scale = 18446744073709551616.0; // 2^64
	if(v < Scale)
		return static_cast<i128>(static_cast<uint64_t>(v));
	const uint64_t hi = static_cast<uint64_t>(v / Scale);
	const uint64_t lo = static_cast<uint64_t>(v - static_cast<double>(hi) * Scale);
	return (static_cast<i128>(hi) << 64) | static_cast<i128>(lo);
}

#else // soft i128 for MSVC — trivial aggregate

struct i128
{
	uint64_t lo;
	int64_t hi;
};

inline constexpr i128 I128(int64_t v)
{
	return i128{static_cast<uint64_t>(v), v < 0 ? int64_t(-1) : int64_t(0)};
}

inline constexpr i128 i128_from_parts(uint64_t Lo, int64_t Hi)
{
	return i128{Lo, Hi};
}

inline constexpr bool operator==(i128 a, i128 b) { return a.lo == b.lo && a.hi == b.hi; }
inline constexpr bool operator!=(i128 a, i128 b) { return !(a == b); }
inline constexpr bool operator<(i128 a, i128 b)
{
	if(a.hi != b.hi)
		return a.hi < b.hi;
	return a.lo < b.lo;
}
inline constexpr bool operator>(i128 a, i128 b) { return b < a; }
inline constexpr bool operator<=(i128 a, i128 b) { return !(b < a); }
inline constexpr bool operator>=(i128 a, i128 b) { return !(a < b); }

inline constexpr i128 operator-(i128 a)
{
	i128 r;
	r.lo = ~a.lo + 1;
	r.hi = ~a.hi;
	if(r.lo == 0)
		r.hi += 1;
	return r;
}

inline constexpr i128 operator+(i128 a, i128 b)
{
	i128 r;
	r.lo = a.lo + b.lo;
	r.hi = a.hi + b.hi + (r.lo < a.lo ? 1 : 0);
	return r;
}
inline constexpr i128 operator-(i128 a, i128 b) { return a + (-b); }

inline constexpr i128 operator<<(i128 a, int s)
{
	if(s <= 0)
		return a;
	if(s >= 128)
		return i128{0, 0};
	if(s >= 64)
		return i128{0, static_cast<int64_t>(a.lo << (s - 64))};
	return i128{a.lo << s, static_cast<int64_t>((static_cast<uint64_t>(a.hi) << s) | (a.lo >> (64 - s)))};
}

inline constexpr i128 operator>>(i128 a, int s)
{
	if(s <= 0)
		return a;
	if(s >= 128)
		return a.hi < 0 ? i128{~uint64_t(0), int64_t(-1)} : i128{0, 0};
	if(s >= 64)
		return i128{static_cast<uint64_t>(a.hi >> (s - 64)), a.hi >> 63};
	return i128{(a.lo >> s) | (static_cast<uint64_t>(a.hi) << (64 - s)), a.hi >> s};
}

inline i128 operator*(i128 a, i128 b)
{
	const bool Neg = (a.hi < 0) != (b.hi < 0);
	if(a.hi < 0)
		a = -a;
	if(b.hi < 0)
		b = -b;

	auto Mul64 = [](uint64_t x, uint64_t y, uint64_t *pHi) -> uint64_t {
		const uint64_t x0 = x & 0xffffffffull, x1 = x >> 32;
		const uint64_t y0 = y & 0xffffffffull, y1 = y >> 32;
		const uint64_t p00 = x0 * y0;
		const uint64_t p01 = x0 * y1;
		const uint64_t p10 = x1 * y0;
		const uint64_t p11 = x1 * y1;
		uint64_t mid = (p00 >> 32) + (p01 & 0xffffffffull) + (p10 & 0xffffffffull);
		const uint64_t lo = (p00 & 0xffffffffull) | (mid << 32);
		*pHi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
		return lo;
	};

	uint64_t p0h = 0;
	const uint64_t p0l = Mul64(a.lo, b.lo, &p0h);
	uint64_t p1h = 0;
	const uint64_t p1l = Mul64(a.lo, static_cast<uint64_t>(b.hi), &p1h);
	uint64_t p2h = 0;
	const uint64_t p2l = Mul64(static_cast<uint64_t>(a.hi), b.lo, &p2h);
	(void)p1h;
	(void)p2h;

	uint64_t rlo = p0l;
	uint64_t rhi = p0h;
	rhi += p1l;
	rhi += p2l;

	i128 out{rlo, static_cast<int64_t>(rhi)};
	return Neg ? -out : out;
}

inline i128 operator/(i128 a, i128 b)
{
	if(b == i128{0, 0})
		return i128{0, 0};
	const bool Neg = (a.hi < 0) != (b.hi < 0);
	if(a.hi < 0)
		a = -a;
	if(b.hi < 0)
		b = -b;

	i128 q{0, 0}, r{0, 0};
	for(int bit = 127; bit >= 0; --bit)
	{
		r = r << 1;
		const i128 shifted = a >> bit;
		if(shifted.lo & 1ull)
			r.lo |= 1ull;
		if(r >= b)
		{
			r = r - b;
			q = q + (i128{1, 0} << bit);
		}
	}
	return Neg ? -q : q;
}

inline i128 operator%(i128 a, i128 b) { return a - (a / b) * b; }

inline i128 &operator+=(i128 &a, i128 b) { return a = a + b; }
inline i128 &operator-=(i128 &a, i128 b) { return a = a - b; }
inline i128 &operator*=(i128 &a, i128 b) { return a = a * b; }
inline i128 &operator/=(i128 &a, i128 b) { return a = a / b; }
inline i128 &operator<<=(i128 &a, int s) { return a = a << s; }
inline i128 &operator>>=(i128 &a, int s) { return a = a >> s; }

inline int64_t i128_to_int64(i128 v)
{
	// Truncate to low 64 bits (valid for our world-pixel ranges)
	return static_cast<int64_t>(v.lo);
}

inline double i128_to_double(i128 v)
{
	constexpr double Scale = 18446744073709551616.0;
	if(v.hi < 0)
	{
		const i128 t = -v;
		return -(static_cast<double>(t.lo) + static_cast<double>(static_cast<uint64_t>(t.hi)) * Scale);
	}
	return static_cast<double>(v.lo) + static_cast<double>(static_cast<uint64_t>(v.hi)) * Scale;
}

inline i128 i128_from_double(double v)
{
	if(v < 0)
		return -i128_from_double(-v);
	constexpr double Scale = 18446744073709551616.0;
	if(v < Scale)
		return i128{static_cast<uint64_t>(v), 0};
	const int64_t hi = static_cast<int64_t>(v / Scale);
	const uint64_t lo = static_cast<uint64_t>(v - static_cast<double>(hi) * Scale);
	return i128{lo, hi};
}

#endif // soft i128

#endif // BASE_INT128_H
