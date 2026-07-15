/* World coordinate type: fixed-point backed by i128 (__int128).
 *
 * Unit: 1.0 == 1 pixel (same as historical float coords).
 * Tile size: 32 pixels per tile (unchanged).
 * Storage: value is (pixels * 2^FRAC_BITS) as i128.
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

	constexpr wcoord() :
		raw(I128(0))
	{
	}
	explicit constexpr wcoord(i128 Raw, bool /*as_raw*/) :
		raw(Raw)
	{
	}

	constexpr wcoord(int v) :
		raw(I128(v) << FRAC_BITS)
	{
	}
	constexpr wcoord(unsigned v) :
		raw(I128(static_cast<int64_t>(v)) << FRAC_BITS)
	{
	}
	constexpr wcoord(long v) :
		raw(I128(static_cast<int64_t>(v)) << FRAC_BITS)
	{
	}
	constexpr wcoord(int64_t v) :
		raw(I128(v) << FRAC_BITS)
	{
	}
	wcoord(float v) :
		raw(i128_from_double(static_cast<double>(v) * static_cast<double>(1 << FRAC_BITS)))
	{
	}
	wcoord(double v) :
		raw(i128_from_double(v * static_cast<double>(1 << FRAC_BITS)))
	{
	}

	static constexpr wcoord FromRaw(i128 Raw) { return wcoord(Raw, true); }

	// Integer pixels as i128 (ho_tp / far lands — not limited to int64)
	static constexpr wcoord FromIntegerPixels(i128 Pixels) { return FromRaw(Pixels << FRAC_BITS); }

	constexpr i128 Raw() const { return raw; }

	// Integer pixel part as full i128 (not truncated to int64)
	i128 to_i128_pixels() const { return raw >> FRAC_BITS; }

	explicit operator bool() const { return raw != I128(0); }
	// Implicit conversions for tile indices / legacy int pixel APIs
	operator int() const { return to_int(); }
	operator int64_t() const { return to_int64(); }
	// Implicit float/double for game code that still uses float math at boundaries
	operator float() const { return to_float(); }
	operator double() const { return to_double(); }

	int to_int() const { return static_cast<int>(to_int64()); }
	int64_t to_int64() const { return i128_to_int64(raw >> FRAC_BITS); }
	float to_float() const { return static_cast<float>(to_double()); }
	double to_double() const
	{
		return i128_to_double(raw) / static_cast<double>(1 << FRAC_BITS);
	}

	int64_t round_to_int64() const
	{
		const i128 half = I128(1) << (FRAC_BITS - 1);
		if(raw >= I128(0))
			return i128_to_int64((raw + half) >> FRAC_BITS);
		return i128_to_int64((raw - half) >> FRAC_BITS);
	}

	// Truncates to int32 range; prefer round_to_int64() for large world coords.
	int round_to_int() const
	{
		return static_cast<int>(round_to_int64());
	}

	wcoord operator+() const { return *this; }
	wcoord operator-() const { return FromRaw(-raw); }

	wcoord &operator+=(wcoord o)
	{
		raw = raw + o.raw;
		return *this;
	}
	wcoord &operator-=(wcoord o)
	{
		raw = raw - o.raw;
		return *this;
	}
	wcoord &operator*=(wcoord o)
	{
		raw = (raw * o.raw) >> FRAC_BITS;
		return *this;
	}
	wcoord &operator/=(wcoord o)
	{
		if(o.raw == I128(0))
		{
			raw = I128(0);
			return *this;
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

	friend wcoord operator+(wcoord a, wcoord b) { return FromRaw(a.raw + b.raw); }
	friend wcoord operator-(wcoord a, wcoord b) { return FromRaw(a.raw - b.raw); }
	friend wcoord operator*(wcoord a, wcoord b) { return FromRaw((a.raw * b.raw) >> FRAC_BITS); }
	friend wcoord operator/(wcoord a, wcoord b)
	{
		if(b.raw == I128(0))
			return wcoord(0);
		return FromRaw((a.raw << FRAC_BITS) / b.raw);
	}

	friend wcoord operator+(wcoord a, int b) { return a + wcoord(b); }
	friend wcoord operator+(int a, wcoord b) { return wcoord(a) + b; }
	friend wcoord operator-(wcoord a, int b) { return a - wcoord(b); }
	friend wcoord operator-(int a, wcoord b) { return wcoord(a) - b; }
	friend wcoord operator*(wcoord a, int b) { return FromRaw(a.raw * I128(b)); }
	friend wcoord operator*(int a, wcoord b) { return FromRaw(b.raw * I128(a)); }
	friend wcoord operator/(wcoord a, int b)
	{
		if(b == 0)
			return wcoord(0);
		return FromRaw(a.raw / I128(b));
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

	friend bool operator==(wcoord a, wcoord b) { return a.raw == b.raw; }
	friend bool operator!=(wcoord a, wcoord b) { return a.raw != b.raw; }
	friend bool operator<(wcoord a, wcoord b) { return a.raw < b.raw; }
	friend bool operator>(wcoord a, wcoord b) { return a.raw > b.raw; }
	friend bool operator<=(wcoord a, wcoord b) { return a.raw <= b.raw; }
	friend bool operator>=(wcoord a, wcoord b) { return a.raw >= b.raw; }

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
	const int64_t Px = World.to_int64();
	if(Px >= 0)
		return static_cast<int>(Px / WORLD_TILE_SIZE);
	return static_cast<int>((Px - (WORLD_TILE_SIZE - 1)) / WORLD_TILE_SIZE);
}

inline wcoord TileToWorld(int Tile)
{
	return wcoord(static_cast<int64_t>(Tile) * WORLD_TILE_SIZE);
}

// Tile index as i128 → world pixels (tile * 32). Returns false if pixel or fixed-point overflows i128.
inline bool TileI128ToWorld(i128 Tile, wcoord *pOut)
{
	if(!pOut)
		return false;
	const i128 Scale = I128(WORLD_TILE_SIZE);
	const i128 Pixels = Tile * Scale;
	// Detect multiply overflow: Pixels / 32 must equal Tile
	if(Scale != I128(0) && Pixels / Scale != Tile)
		return false;
	// Detect fixed-point shift overflow
	const i128 Raw = Pixels << wcoord::FRAC_BITS;
	if((Raw >> wcoord::FRAC_BITS) != Pixels)
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
	return a.raw < I128(0) ? -a : a;
}

#endif // BASE_WORLD_COORD_H
