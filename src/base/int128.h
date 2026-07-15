/* Portable signed 128-bit integer for world coordinates.
 * Uses compiler __int128 when available; soft implementation on MSVC.
 * Soft path uses int64 fast-paths whenever values fit (normal + large maps). */
#ifndef BASE_INT128_H
#define BASE_INT128_H

#include <cstdint>
#include <limits>

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
	// Fast path: values that fit in int64 after truncate (covers all practical world fixed-point)
	if(v < 9223372036854775808.0) // 2^63
		return static_cast<i128>(static_cast<int64_t>(v));
	constexpr double Scale = 18446744073709551616.0; // 2^64
	if(v < Scale)
		return static_cast<i128>(static_cast<uint64_t>(v));
	const uint64_t hi = static_cast<uint64_t>(v / Scale);
	const uint64_t lo = static_cast<uint64_t>(v - static_cast<double>(hi) * Scale);
	return (static_cast<i128>(hi) << 64) | static_cast<i128>(lo);
}

#else // soft i128 for MSVC

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

// True when value is representable as signed int64 (two's complement layout)
inline constexpr bool i128_is_i64(i128 v)
{
	return (v.hi == 0 && static_cast<int64_t>(v.lo) >= 0) ||
	       (v.hi == -1 && static_cast<int64_t>(v.lo) < 0);
}

inline constexpr int64_t i128_as_i64(i128 v)
{
	return static_cast<int64_t>(v.lo);
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
	if(i128_is_i64(a) && i128_as_i64(a) != std::numeric_limits<int64_t>::min())
		return I128(-i128_as_i64(a));
	i128 r;
	r.lo = ~a.lo + 1;
	r.hi = ~a.hi;
	if(r.lo == 0)
		r.hi += 1;
	return r;
}

inline constexpr i128 operator+(i128 a, i128 b)
{
	if(i128_is_i64(a) && i128_is_i64(b))
	{
		const int64_t aa = i128_as_i64(a);
		const int64_t bb = i128_as_i64(b);
		// Detect overflow of signed add
		if((bb > 0 && aa > std::numeric_limits<int64_t>::max() - bb) ||
			(bb < 0 && aa < std::numeric_limits<int64_t>::min() - bb))
		{
			// fall through to wide add
		}
		else
			return I128(aa + bb);
	}
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
	if(i128_is_i64(a) && s < 63)
	{
		const int64_t v = i128_as_i64(a);
		// if shift stays in i64 range
		if(v >= 0 && v <= (std::numeric_limits<int64_t>::max() >> s))
			return I128(v << s);
		if(v < 0 && v >= (std::numeric_limits<int64_t>::min() >> s))
			return I128(v << s);
	}
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
	if(i128_is_i64(a))
		return I128(i128_as_i64(a) >> (s >= 63 ? 63 : s));
	if(s >= 64)
		return i128{static_cast<uint64_t>(a.hi >> (s - 64)), a.hi >> 63};
	return i128{(a.lo >> s) | (static_cast<uint64_t>(a.hi) << (64 - s)), a.hi >> s};
}

inline i128 operator*(i128 a, i128 b)
{
	// Fast path: both fit in i64 and product fits in i64
	if(i128_is_i64(a) && i128_is_i64(b))
	{
		const int64_t aa = i128_as_i64(a);
		const int64_t bb = i128_as_i64(b);
		if(aa == 0 || bb == 0)
			return I128(0);
		// Check product fits in int64
		if(aa > 0)
		{
			if(bb > 0)
			{
				if(aa <= std::numeric_limits<int64_t>::max() / bb)
					return I128(aa * bb);
			}
			else
			{
				if(bb >= std::numeric_limits<int64_t>::min() / aa)
					return I128(aa * bb);
			}
		}
		else
		{
			if(bb > 0)
			{
				if(aa >= std::numeric_limits<int64_t>::min() / bb)
					return I128(aa * bb);
			}
			else
			{
				if(aa != std::numeric_limits<int64_t>::min() && bb != std::numeric_limits<int64_t>::min() &&
					-aa <= std::numeric_limits<int64_t>::max() / -bb)
					return I128(aa * bb);
			}
		}
		// Product needs full 128 bits — use unsigned 64x64->128
		const bool Neg = (aa < 0) != (bb < 0);
		uint64_t ua = static_cast<uint64_t>(aa < 0 ? -aa : aa);
		uint64_t ub = static_cast<uint64_t>(bb < 0 ? -bb : bb);
		const uint64_t a0 = ua & 0xffffffffull, a1 = ua >> 32;
		const uint64_t b0 = ub & 0xffffffffull, b1 = ub >> 32;
		const uint64_t p00 = a0 * b0;
		const uint64_t p01 = a0 * b1;
		const uint64_t p10 = a1 * b0;
		const uint64_t p11 = a1 * b1;
		uint64_t mid = (p00 >> 32) + (p01 & 0xffffffffull) + (p10 & 0xffffffffull);
		const uint64_t lo = (p00 & 0xffffffffull) | (mid << 32);
		const uint64_t hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
		i128 out{lo, static_cast<int64_t>(hi)};
		return Neg ? -out : out;
	}

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

	// Fast path: both fit in i64
	if(i128_is_i64(a) && i128_is_i64(b))
	{
		const int64_t bb = i128_as_i64(b);
		if(bb != 0)
			return I128(i128_as_i64(a) / bb);
	}

	const bool Neg = (a.hi < 0) != (b.hi < 0);
	if(a.hi < 0)
		a = -a;
	if(b.hi < 0)
		b = -b;

	// If divisor fits in u64 and dividend hi is 0, use 64-bit div
	if(b.hi == 0 && a.hi == 0 && b.lo != 0)
		return Neg ? -I128(static_cast<int64_t>(a.lo / b.lo)) : I128(static_cast<int64_t>(a.lo / b.lo));

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

inline i128 operator%(i128 a, i128 b)
{
	if(i128_is_i64(a) && i128_is_i64(b))
	{
		const int64_t bb = i128_as_i64(b);
		if(bb != 0)
			return I128(i128_as_i64(a) % bb);
	}
	return a - (a / b) * b;
}

inline i128 &operator+=(i128 &a, i128 b) { return a = a + b; }
inline i128 &operator-=(i128 &a, i128 b) { return a = a - b; }
inline i128 &operator*=(i128 &a, i128 b) { return a = a * b; }
inline i128 &operator/=(i128 &a, i128 b) { return a = a / b; }
inline i128 &operator<<=(i128 &a, int s) { return a = a << s; }
inline i128 &operator>>=(i128 &a, int s) { return a = a >> s; }

inline int64_t i128_to_int64(i128 v)
{
	return static_cast<int64_t>(v.lo);
}

inline double i128_to_double(i128 v)
{
	if(i128_is_i64(v))
		return static_cast<double>(i128_as_i64(v));
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
	// Fast path for values that fit int64 (covers world fixed-point for huge maps)
	if(v < 9223372036854775808.0)
		return I128(static_cast<int64_t>(v));
	constexpr double Scale = 18446744073709551616.0;
	if(v < Scale)
		return i128{static_cast<uint64_t>(v), 0};
	const int64_t hi = static_cast<int64_t>(v / Scale);
	const uint64_t lo = static_cast<uint64_t>(v - static_cast<double>(hi) * Scale);
	return i128{lo, hi};
}

#endif // soft i128

// --- decimal string helpers (native + soft) ---

// Parse optional sign + digits (integer only). Returns false on empty/invalid/overflow-ish junk.
inline bool i128_from_decimal_str(const char *pStr, i128 *pOut)
{
	if(!pStr || !pOut)
		return false;
	while(*pStr == ' ' || *pStr == '\t')
		pStr++;
	bool Neg = false;
	if(*pStr == '-')
	{
		Neg = true;
		pStr++;
	}
	else if(*pStr == '+')
		pStr++;
	if(*pStr < '0' || *pStr > '9')
		return false;

	i128 V = I128(0);
	const i128 Ten = I128(10);
	bool Any = false;
	while(*pStr >= '0' && *pStr <= '9')
	{
		Any = true;
		// V = V * 10 + digit (reject if string is absurdly long — still limited by i128 range in practice)
		V = V * Ten + I128(*pStr - '0');
		pStr++;
	}
	// optional fractional part for convenience: ignore after '.'
	if(*pStr == '.')
	{
		pStr++;
		while(*pStr >= '0' && *pStr <= '9')
			pStr++;
	}
	while(*pStr == ' ' || *pStr == '\t')
		pStr++;
	if(!Any || *pStr != '\0')
		return false;
	*pOut = Neg ? -V : V;
	return true;
}

// Write signed decimal into pBuf (always NUL-terminated if BufSize > 0).
inline void i128_to_decimal_str(i128 V, char *pBuf, size_t BufSize)
{
	if(!pBuf || BufSize == 0)
		return;
	if(BufSize == 1)
	{
		pBuf[0] = '\0';
		return;
	}
	if(V == I128(0))
	{
		pBuf[0] = '0';
		pBuf[1] = '\0';
		return;
	}
	bool Neg = V < I128(0);
	if(Neg)
		V = -V;
	char aTmp[64];
	int N = 0;
	const i128 Ten = I128(10);
	while(V > I128(0) && N < 63)
	{
		const i128 Dig = V % Ten;
		aTmp[N++] = static_cast<char>('0' + static_cast<int>(i128_to_int64(Dig)));
		V = V / Ten;
	}
	size_t Out = 0;
	if(Neg && Out + 1 < BufSize)
		pBuf[Out++] = '-';
	while(N > 0 && Out + 1 < BufSize)
		pBuf[Out++] = aTmp[--N];
	pBuf[Out] = '\0';
}

#endif // BASE_INT128_H
