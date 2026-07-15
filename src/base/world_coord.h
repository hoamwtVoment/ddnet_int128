/* World coordinate type: fixed-point backed by i128 (__int128).
 *
 * Unit: 1.0 == 1 pixel (same as historical float coords).
 * Tile size: 32 pixels per tile (unchanged).
 *
 * Storage modes:
 * - Fixed-point (default): raw = pixels * 2^FRAC_BITS (sub-pixel physics near the map)
 * - Integer-pixel: raw = integer pixels when |pixels| is too large for the fixed shift
 *   (supports up to ~2^127 pixels, i.e. tile (2^127-1)/32 for ho_tp far lands)
 */
#ifndef BASE_WORLD_COORD_H
#define BASE_WORLD_COORD_H

#include "int128.h"

// Pixels per map tile — must stay 32.
inline constexpr int WORLD_TILE_SIZE = 32;

class wcoord
{
public:
	static constexpr int FRAC_BITS = 20;

	i128 raw;
	// When true, `raw` is integer pixels (no FRAC scale). When false, fixed-point.
	bool m_IntPixels;

	constexpr wcoord() :
		raw(I128(0)), m_IntPixels(false)
	{
	}
	explicit constexpr wcoord(i128 Raw, bool IntPixels) :
		raw(Raw), m_IntPixels(IntPixels)
	{
	}

	constexpr wcoord(int v) :
		raw(I128(v) << FRAC_BITS), m_IntPixels(false)
	{
	}
	constexpr wcoord(unsigned v) :
		raw(I128(static_cast<int64_t>(v)) << FRAC_BITS), m_IntPixels(false)
	{
	}
	constexpr wcoord(long v) :
		raw(I128(static_cast<int64_t>(v)) << FRAC_BITS), m_IntPixels(false)
	{
	}
	constexpr wcoord(int64_t v) :
		raw(I128(v) << FRAC_BITS), m_IntPixels(false)
	{
	}
	wcoord(float v) :
		raw(i128_from_double(static_cast<double>(v) * static_cast<double>(1 << FRAC_BITS))), m_IntPixels(false)
	{
	}
	wcoord(double v) :
		raw(i128_from_double(v * static_cast<double>(1 << FRAC_BITS))), m_IntPixels(false)
	{
	}

	static constexpr wcoord FromRaw(i128 Raw) { return wcoord(Raw, false); }
	static constexpr wcoord FromRawIntPixels(i128 Pixels) { return wcoord(Pixels, true); }

	// Integer pixels as i128. Uses fixed-point when it fits; otherwise integer-pixel mode
	// so positions up to ~2^127 pixels (tile (2^127-1)/32) are representable.
	static wcoord FromIntegerPixels(i128 Pixels)
	{
		const i128 Shifted = Pixels << FRAC_BITS;
		if((Shifted >> FRAC_BITS) == Pixels)
			return FromRaw(Shifted);
		return FromRawIntPixels(Pixels);
	}

	constexpr i128 Raw() const { return raw; }
	constexpr bool IsIntPixels() const { return m_IntPixels; }

	// Integer pixel part as full i128
	i128 to_i128_pixels() const
	{
		if(m_IntPixels)
			return raw;
		return raw >> FRAC_BITS;
	}

	explicit operator bool() const { return raw != I128(0); }
	operator int() const { return to_int(); }
	operator int64_t() const { return to_int64(); }
	operator float() const { return to_float(); }
	operator double() const { return to_double(); }

	int to_int() const { return static_cast<int>(to_int64()); }
	int64_t to_int64() const { return i128_to_int64(to_i128_pixels()); }
	float to_float() const { return static_cast<float>(to_double()); }
	double to_double() const
	{
		if(m_IntPixels)
			return i128_to_double(raw);
		return i128_to_double(raw) / static_cast<double>(1 << FRAC_BITS);
	}

	i128 round_to_i128_pixels() const
	{
		if(m_IntPixels)
			return raw;
		const i128 half = I128(1) << (FRAC_BITS - 1);
		if(raw >= I128(0))
			return (raw + half) >> FRAC_BITS;
		return (raw - half) >> FRAC_BITS;
	}

	int64_t round_to_int64() const
	{
		return i128_to_int64(round_to_i128_pixels());
	}

	int round_to_int() const
	{
		return static_cast<int>(round_to_int64());
	}

	// Normalize both to integer-pixel mode for mixed arithmetic / compares
	static wcoord CanonicalAdd(wcoord a, wcoord b)
	{
		if(!a.m_IntPixels && !b.m_IntPixels)
			return FromRaw(a.raw + b.raw);
		return FromIntegerPixels(a.round_to_i128_pixels() + b.round_to_i128_pixels());
	}
	static wcoord CanonicalSub(wcoord a, wcoord b)
	{
		if(!a.m_IntPixels && !b.m_IntPixels)
			return FromRaw(a.raw - b.raw);
		return FromIntegerPixels(a.round_to_i128_pixels() - b.round_to_i128_pixels());
	}

	wcoord operator+() const { return *this; }
	wcoord operator-() const
	{
		if(m_IntPixels)
			return FromRawIntPixels(-raw);
		return FromRaw(-raw);
	}

	wcoord &operator+=(wcoord o) { return *this = CanonicalAdd(*this, o); }
	wcoord &operator-=(wcoord o) { return *this = CanonicalSub(*this, o); }
	wcoord &operator*=(wcoord o)
	{
		if(m_IntPixels || o.m_IntPixels)
		{
			// Scale in double space is wrong for huge ints; integer*integer for int mode
			if(m_IntPixels && o.m_IntPixels)
				return *this = FromIntegerPixels(raw * o.raw);
			return *this = FromIntegerPixels(i128_from_double(to_double() * o.to_double()));
		}
		raw = (raw * o.raw) >> FRAC_BITS;
		return *this;
	}
	wcoord &operator/=(wcoord o)
	{
		if(o.raw == I128(0) && !o.m_IntPixels)
		{
			raw = I128(0);
			m_IntPixels = false;
			return *this;
		}
		if(m_IntPixels || o.m_IntPixels)
		{
			const double D = o.to_double();
			if(D == 0.0)
			{
				raw = I128(0);
				m_IntPixels = false;
				return *this;
			}
			return *this = FromIntegerPixels(i128_from_double(to_double() / D));
		}
		raw = (raw << FRAC_BITS) / o.raw;
		return *this;
	}
	wcoord &operator*=(int o)
	{
		raw = raw * I128(o);
		return *this;
	}
	wcoord &operator/=(int o)
	{
		if(o == 0)
		{
			raw = I128(0);
			m_IntPixels = false;
			return *this;
		}
		raw = raw / I128(o);
		return *this;
	}
	wcoord &operator+=(float o) { return *this = *this + wcoord(o); }
	wcoord &operator-=(float o) { return *this = *this - wcoord(o); }
	wcoord &operator*=(float o) { return *this = *this * wcoord(o); }
	wcoord &operator/=(float o) { return *this = *this / wcoord(o); }
	wcoord &operator+=(double o) { return *this = *this + wcoord(o); }
	wcoord &operator-=(double o) { return *this = *this - wcoord(o); }
	wcoord &operator*=(double o) { return *this = *this * wcoord(o); }
	wcoord &operator/=(double o) { return *this = *this / wcoord(o); }
	wcoord &operator+=(int o) { return *this = *this + wcoord(o); }
	wcoord &operator-=(int o) { return *this = *this - wcoord(o); }

	friend wcoord operator+(wcoord a, wcoord b) { return CanonicalAdd(a, b); }
	friend wcoord operator-(wcoord a, wcoord b) { return CanonicalSub(a, b); }
	friend wcoord operator*(wcoord a, wcoord b)
	{
		wcoord r = a;
		r *= b;
		return r;
	}
	friend wcoord operator/(wcoord a, wcoord b)
	{
		wcoord r = a;
		r /= b;
		return r;
	}

	friend wcoord operator+(wcoord a, int b) { return a + wcoord(b); }
	friend wcoord operator+(int a, wcoord b) { return wcoord(a) + b; }
	friend wcoord operator-(wcoord a, int b) { return a - wcoord(b); }
	friend wcoord operator-(int a, wcoord b) { return wcoord(a) - b; }
	friend wcoord operator*(wcoord a, int b)
	{
		wcoord r = a;
		r *= b;
		return r;
	}
	friend wcoord operator*(int a, wcoord b) { return b * a; }
	friend wcoord operator/(wcoord a, int b)
	{
		wcoord r = a;
		r /= b;
		return r;
	}
	friend wcoord operator/(int a, wcoord b) { return wcoord(a) / b; }

	friend wcoord operator+(wcoord a, float b) { return a + wcoord(b); }
	friend wcoord operator+(float a, wcoord b) { return wcoord(a) + b; }
	friend wcoord operator-(wcoord a, float b) { return a - wcoord(b); }
	friend wcoord operator-(float a, wcoord b) { return wcoord(a) - b; }
	friend wcoord operator*(wcoord a, float b) { return a * wcoord(b); }
	friend wcoord operator*(float a, wcoord b) { return wcoord(a) * b; }
	friend wcoord operator/(wcoord a, float b) { return a / wcoord(b); }
	friend wcoord operator/(float a, wcoord b) { return wcoord(a) / b; }

	friend wcoord operator+(wcoord a, double b) { return a + wcoord(b); }
	friend wcoord operator+(double a, wcoord b) { return wcoord(a) + b; }
	friend wcoord operator-(wcoord a, double b) { return a - wcoord(b); }
	friend wcoord operator-(double a, wcoord b) { return wcoord(a) - b; }
	friend wcoord operator*(wcoord a, double b) { return a * wcoord(b); }
	friend wcoord operator*(double a, wcoord b) { return wcoord(a) * b; }
	friend wcoord operator/(wcoord a, double b) { return a / wcoord(b); }
	friend wcoord operator/(double a, wcoord b) { return wcoord(a) / b; }

	// Compare by actual pixel value (mode-independent)
	friend bool operator==(wcoord a, wcoord b)
	{
		if(a.m_IntPixels == b.m_IntPixels)
			return a.raw == b.raw;
		return a.round_to_i128_pixels() == b.round_to_i128_pixels();
	}
	friend bool operator!=(wcoord a, wcoord b) { return !(a == b); }
	friend bool operator<(wcoord a, wcoord b)
	{
		if(!a.m_IntPixels && !b.m_IntPixels)
			return a.raw < b.raw;
		return a.round_to_i128_pixels() < b.round_to_i128_pixels();
	}
	friend bool operator>(wcoord a, wcoord b) { return b < a; }
	friend bool operator<=(wcoord a, wcoord b) { return !(b < a); }
	friend bool operator>=(wcoord a, wcoord b) { return !(a < b); }

	friend bool operator==(wcoord a, int b) { return a == wcoord(b); }
	friend bool operator!=(wcoord a, int b) { return a != wcoord(b); }
	friend bool operator<(wcoord a, int b) { return a < wcoord(b); }
	friend bool operator>(wcoord a, int b) { return a > wcoord(b); }
	friend bool operator<=(wcoord a, int b) { return a <= wcoord(b); }
	friend bool operator>=(wcoord a, int b) { return a >= wcoord(b); }
	friend bool operator==(int a, wcoord b) { return wcoord(a) == b; }
	friend bool operator!=(int a, wcoord b) { return wcoord(a) != b; }
	friend bool operator<(int a, wcoord b) { return wcoord(a) < b; }
	friend bool operator>(int a, wcoord b) { return wcoord(a) > b; }
	friend bool operator<=(int a, wcoord b) { return wcoord(a) <= b; }
	friend bool operator>=(int a, wcoord b) { return wcoord(a) >= b; }

	friend bool operator==(wcoord a, float b) { return a == wcoord(b); }
	friend bool operator!=(wcoord a, float b) { return a != wcoord(b); }
	friend bool operator<(wcoord a, float b) { return a < wcoord(b); }
	friend bool operator>(wcoord a, float b) { return a > wcoord(b); }
	friend bool operator<=(wcoord a, float b) { return a <= wcoord(b); }
	friend bool operator>=(wcoord a, float b) { return a >= wcoord(b); }
	friend bool operator==(float a, wcoord b) { return wcoord(a) == b; }
	friend bool operator!=(float a, wcoord b) { return wcoord(a) != b; }
	friend bool operator<(float a, wcoord b) { return wcoord(a) < b; }
	friend bool operator>(float a, wcoord b) { return wcoord(a) > b; }
	friend bool operator<=(float a, wcoord b) { return wcoord(a) <= b; }
	friend bool operator>=(float a, wcoord b) { return wcoord(a) >= b; }
};

inline int WorldToTile(wcoord World)
{
	const i128 Px = World.to_i128_pixels();
	const i128 T = Px / I128(WORLD_TILE_SIZE);
	// For negative, match previous toward -inf behavior when needed
	if(Px >= I128(0))
		return static_cast<int>(i128_to_int64(T));
	// only for small negative near map
	return static_cast<int>(i128_to_int64((Px - I128(WORLD_TILE_SIZE - 1)) / I128(WORLD_TILE_SIZE)));
}

inline wcoord TileToWorld(int Tile)
{
	return wcoord(static_cast<int64_t>(Tile) * WORLD_TILE_SIZE);
}

// Tile index as i128 → world pixels (tile * 32).
// Supports full signed i128 pixel range (max tile = (2^127-1)/32).
inline bool TileI128ToWorld(i128 Tile, wcoord *pOut)
{
	if(!pOut)
		return false;
	const i128 Scale = I128(WORLD_TILE_SIZE);
	const i128 Pixels = Tile * Scale;
	// Detect multiply overflow
	if(Scale != I128(0) && Pixels / Scale != Tile)
		return false;
	*pOut = wcoord::FromIntegerPixels(Pixels);
	return true;
}

inline wcoord TileCenterToWorld(int Tile)
{
	return wcoord(static_cast<int64_t>(Tile) * WORLD_TILE_SIZE + WORLD_TILE_SIZE / 2);
}

inline wcoord absolute(wcoord a)
{
	return a < wcoord(0) ? -a : a;
}

#endif // BASE_WORLD_COORD_H
