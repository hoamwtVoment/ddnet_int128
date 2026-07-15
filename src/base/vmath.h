/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_VMATH_H
#define BASE_VMATH_H

#include "math.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

// ------------------------------------

template<Numeric T>
class vector2_base
{
public:
	// No unions: T may be non-trivial (e.g. wcoord / i128 fixed-point).
	// Texture UV code uses dedicated GL_STexCoord / STexCoord types with u,v fields.
	T x, y;

	constexpr vector2_base() = default;
	constexpr vector2_base(T nx, T ny) :
		x(nx), y(ny)
	{
	}

	// Convert between world (wcoord) and screen/render (float) vectors.
	template<Numeric U>
		requires(!std::same_as<T, U>)
	vector2_base(const vector2_base<U> &Other) :
		x(static_cast<T>(Other.x)), y(static_cast<T>(Other.y))
	{
	}

	constexpr vector2_base operator-() const { return vector2_base(-x, -y); }
	constexpr vector2_base operator-(const vector2_base &vec) const { return vector2_base(x - vec.x, y - vec.y); }
	constexpr vector2_base operator+(const vector2_base &vec) const { return vector2_base(x + vec.x, y + vec.y); }
	constexpr vector2_base operator*(const T rhs) const { return vector2_base(x * rhs, y * rhs); }
	constexpr vector2_base operator*(const vector2_base &vec) const { return vector2_base(x * vec.x, y * vec.y); }
	constexpr vector2_base operator/(const T rhs) const { return vector2_base(x / rhs, y / rhs); }
	constexpr vector2_base operator/(const vector2_base &vec) const { return vector2_base(x / vec.x, y / vec.y); }

	constexpr vector2_base &operator+=(const vector2_base &vec)
	{
		x += vec.x;
		y += vec.y;
		return *this;
	}
	constexpr vector2_base &operator-=(const vector2_base &vec)
	{
		x -= vec.x;
		y -= vec.y;
		return *this;
	}
	constexpr vector2_base &operator*=(const T rhs)
	{
		x *= rhs;
		y *= rhs;
		return *this;
	}
	constexpr vector2_base &operator*=(const vector2_base &vec)
	{
		x *= vec.x;
		y *= vec.y;
		return *this;
	}
	constexpr vector2_base &operator/=(const T rhs)
	{
		x /= rhs;
		y /= rhs;
		return *this;
	}
	constexpr vector2_base &operator/=(const vector2_base &vec)
	{
		x /= vec.x;
		y /= vec.y;
		return *this;
	}

	constexpr bool operator==(const vector2_base &vec) const { return x == vec.x && y == vec.y; } // TODO: do this with an eps instead
	constexpr bool operator!=(const vector2_base &vec) const { return x != vec.x || y != vec.y; }

	constexpr T &operator[](const int index) { return index ? y : x; }
	constexpr const T &operator[](const int index) const { return index ? y : x; }

	// Scale by float when T is not float (e.g. wcoord) — avoids dual user-defined conversion on scalars
	template<typename U = T>
	vector2_base operator*(float rhs) const
		requires(!std::floating_point<U>)
	{
		return vector2_base(x * rhs, y * rhs);
	}
	template<typename U = T>
	vector2_base operator/(float rhs) const
		requires(!std::floating_point<U>)
	{
		return vector2_base(x / rhs, y / rhs);
	}
	template<typename U = T>
	vector2_base &operator*=(float rhs)
		requires(!std::floating_point<U>)
	{
		x *= rhs;
		y *= rhs;
		return *this;
	}
	template<typename U = T>
	vector2_base &operator/=(float rhs)
		requires(!std::floating_point<U>)
	{
		x /= rhs;
		y /= rhs;
		return *this;
	}
};

template<Numeric T>
	requires(!std::floating_point<T>)
inline vector2_base<T> operator*(float lhs, const vector2_base<T> &vec)
{
	return vec * lhs;
}

template<Numeric T>
constexpr vector2_base<T> rotate(const vector2_base<T> &a, float angle)
{
	angle = angle * pi / 180.0f;
	float s = std::sin(angle);
	float c = std::cos(angle);
	return vector2_base<T>(static_cast<T>(c * a.x - s * a.y), static_cast<T>(s * a.x + c * a.y));
}

template<Numeric T>
constexpr T dot(const vector2_base<T> a, const vector2_base<T> &b)
{
	return a.x * b.x + a.y * b.y;
}

template<std::floating_point T>
inline float length(const vector2_base<T> &a)
{
	return std::sqrt(dot(a, a));
}

template<std::integral T>
inline float length(const vector2_base<T> &a)
{
	return std::sqrt(static_cast<float>(dot(a, a)));
}

inline float length(const vector2_base<wcoord> &a)
{
	const double dx = a.x.to_double();
	const double dy = a.y.to_double();
	return static_cast<float>(std::sqrt(dx * dx + dy * dy));
}

constexpr float length_squared(const vector2_base<float> &a)
{
	return dot(a, a);
}

inline float length_squared(const vector2_base<wcoord> &a)
{
	const double dx = a.x.to_double();
	const double dy = a.y.to_double();
	return static_cast<float>(dx * dx + dy * dy);
}

template<Numeric T>
inline auto distance(const vector2_base<T> a, const vector2_base<T> &b)
{
	return length(a - b);
}

// Mixed world/render vectors: compute in double (common for m_Pos vs camera float)
template<Numeric T, Numeric U>
	requires(!std::same_as<T, U>)
inline float distance(const vector2_base<T> &a, const vector2_base<U> &b)
{
	const double dx = static_cast<double>(a.x) - static_cast<double>(b.x);
	const double dy = static_cast<double>(a.y) - static_cast<double>(b.y);
	return static_cast<float>(std::sqrt(dx * dx + dy * dy));
}

template<Numeric T>
inline auto distance_squared(const vector2_base<T> a, const vector2_base<T> &b)
{
	return length_squared(a - b);
}

template<Numeric T, Numeric U>
	requires(!std::same_as<T, U>)
inline float distance_squared(const vector2_base<T> &a, const vector2_base<U> &b)
{
	const double dx = static_cast<double>(a.x) - static_cast<double>(b.x);
	const double dy = static_cast<double>(a.y) - static_cast<double>(b.y);
	return static_cast<float>(dx * dx + dy * dy);
}

constexpr float angle(const vector2_base<float> &a)
{
	if(a.x == 0 && a.y == 0)
		return 0.0f;
	else if(a.x == 0)
		return a.y < 0 ? -pi / 2 : pi / 2;
	float result = std::atan(a.y / a.x);
	if(a.x < 0)
		result = result + pi;
	return result;
}

inline float angle(const vector2_base<wcoord> &a)
{
	const float ax = a.x.to_float();
	const float ay = a.y.to_float();
	if(ax == 0 && ay == 0)
		return 0.0f;
	else if(ax == 0)
		return ay < 0 ? -pi / 2 : pi / 2;
	float result = std::atan(ay / ax);
	if(ax < 0)
		result = result + pi;
	return result;
}

template<Numeric T>
constexpr vector2_base<T> normalize_pre_length(const vector2_base<T> &v, T len)
{
	if(len == 0)
		return vector2_base<T>();
	return vector2_base<T>(v.x / len, v.y / len);
}

inline vector2_base<wcoord> normalize_pre_length(const vector2_base<wcoord> &v, float len)
{
	if(len == 0.0f)
		return vector2_base<wcoord>();
	return vector2_base<wcoord>(v.x / len, v.y / len);
}

inline vector2_base<float> normalize(const vector2_base<float> &v)
{
	float divisor = length(v);
	if(divisor == 0.0f)
		return vector2_base<float>(0.0f, 0.0f);
	float l = 1.0f / divisor;
	return vector2_base<float>(v.x * l, v.y * l);
}

inline vector2_base<wcoord> normalize(const vector2_base<wcoord> &v)
{
	float divisor = length(v);
	if(divisor == 0.0f)
		return vector2_base<wcoord>(0, 0);
	float l = 1.0f / divisor;
	return vector2_base<wcoord>(v.x * l, v.y * l);
}

inline vector2_base<float> direction(float angle)
{
	return vector2_base<float>(std::cos(angle), std::sin(angle));
}

inline vector2_base<wcoord> direction_w(float angle)
{
	return vector2_base<wcoord>(std::cos(angle), std::sin(angle));
}

inline vector2_base<float> random_direction()
{
	return direction(random_angle());
}

// Screen/render coordinates (float). World game coordinates use wvec2.
typedef vector2_base<float> vec2;
// World coordinates: fixed-point i128, 1 unit = 1 pixel, 32 px = 1 tile
typedef vector2_base<wcoord> wvec2;
typedef vector2_base<bool> bvec2;
typedef vector2_base<int> ivec2;

inline vec2 ToVec2(const wvec2 &v)
{
	return vec2(v.x.to_float(), v.y.to_float());
}
inline wvec2 ToWVec2(const vec2 &v)
{
	return wvec2(v.x, v.y);
}
inline wvec2 ToWVec2(float x, float y)
{
	return wvec2(x, y);
}

template<Numeric T>
constexpr bool closest_point_on_line(vector2_base<T> line_pointA, vector2_base<T> line_pointB, vector2_base<T> target_point, vector2_base<T> &out_pos)
{
	vector2_base<T> AB = line_pointB - line_pointA;
	T SquaredMagnitudeAB = dot(AB, AB);
	if(SquaredMagnitudeAB > 0)
	{
		vector2_base<T> AP = target_point - line_pointA;
		T APdotAB = dot(AP, AB);
		T t = APdotAB / SquaredMagnitudeAB;
		out_pos = line_pointA + AB * std::clamp(t, (T)0, (T)1);
		return true;
	}
	else
	{
		return false;
	}
}

constexpr int intersect_line_circle(const vec2 LineStart, const vec2 LineEnd, const vec2 CircleCenter, float Radius, vec2 aIntersections[2])
{
	vec2 Delta = LineEnd - LineStart;
	vec2 Offset = LineStart - CircleCenter;

	// A * Time^2 + B * Time + c == 0
	float A = length_squared(Delta);
	float B = 2.0f * dot(Offset, Delta);
	float C = dot(Offset, Offset) - Radius * Radius;

	float Discriminant = B * B - 4.0f * A * C;
	if(Discriminant < 0.0f || A == 0.0f)
	{
		// no intersection
		return 0;
	}
	else if(Discriminant == 0.0f)
	{
		// tangent
		float Time = -B / (2.0f * A);
		aIntersections[0] = LineStart + Delta * Time;
		return 1;
	}
	else
	{
		Discriminant = std::sqrt(Discriminant);
		float Time1 = (-B - Discriminant) / (2.0f * A);
		float Time2 = (-B + Discriminant) / (2.0f * A);

		aIntersections[0] = LineStart + Delta * Time1;
		aIntersections[1] = LineStart + Delta * Time2;

		return 2;
	}
}

// ------------------------------------
template<Numeric T>
class vector3_base
{
public:
	union
	{
		T x, r, h, u;
	};
	union
	{
		T y, g, s, v;
	};
	union
	{
		T z, b, l, w;
	};

	constexpr vector3_base() = default;
	constexpr vector3_base(T nx, T ny, T nz) :
		x(nx), y(ny), z(nz)
	{
	}

	constexpr vector3_base operator-(const vector3_base &vec) const { return vector3_base(x - vec.x, y - vec.y, z - vec.z); }
	constexpr vector3_base operator-() const { return vector3_base(-x, -y, -z); }
	constexpr vector3_base operator+(const vector3_base &vec) const { return vector3_base(x + vec.x, y + vec.y, z + vec.z); }
	constexpr vector3_base operator*(const T rhs) const { return vector3_base(x * rhs, y * rhs, z * rhs); }
	constexpr vector3_base operator*(const vector3_base &vec) const { return vector3_base(x * vec.x, y * vec.y, z * vec.z); }
	constexpr vector3_base operator/(const T rhs) const { return vector3_base(x / rhs, y / rhs, z / rhs); }
	constexpr vector3_base operator/(const vector3_base &vec) const { return vector3_base(x / vec.x, y / vec.y, z / vec.z); }

	constexpr vector3_base &operator+=(const vector3_base &vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		return *this;
	}
	constexpr vector3_base &operator-=(const vector3_base &vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		return *this;
	}
	constexpr vector3_base &operator*=(const T rhs)
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}
	constexpr vector3_base &operator*=(const vector3_base &vec)
	{
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
		return *this;
	}
	constexpr vector3_base &operator/=(const T rhs)
	{
		x /= rhs;
		y /= rhs;
		z /= rhs;
		return *this;
	}
	constexpr vector3_base &operator/=(const vector3_base &vec)
	{
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
		return *this;
	}

	constexpr bool operator==(const vector3_base &vec) const { return x == vec.x && y == vec.y && z == vec.z; } // TODO: do this with an eps instead
	constexpr bool operator!=(const vector3_base &vec) const { return x != vec.x || y != vec.y || z != vec.z; }
};

template<Numeric T>
inline T distance(const vector3_base<T> &a, const vector3_base<T> &b)
{
	return length(a - b);
}

template<Numeric T>
constexpr T dot(const vector3_base<T> &a, const vector3_base<T> &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

template<Numeric T>
constexpr vector3_base<T> cross(const vector3_base<T> &a, const vector3_base<T> &b)
{
	return vector3_base<T>(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x);
}

//
inline float length(const vector3_base<float> &a)
{
	return std::sqrt(dot(a, a));
}

inline vector3_base<float> normalize(const vector3_base<float> &v)
{
	float divisor = length(v);
	if(divisor == 0.0f)
		return vector3_base<float>(0.0f, 0.0f, 0.0f);
	float l = 1.0f / divisor;
	return vector3_base<float>(v.x * l, v.y * l, v.z * l);
}

typedef vector3_base<float> vec3;
typedef vector3_base<bool> bvec3;
typedef vector3_base<int> ivec3;

// ------------------------------------

template<Numeric T>
class vector4_base
{
public:
	union
	{
		T x, r, h;
	};
	union
	{
		T y, g, s;
	};
	union
	{
		T z, b, l;
	};
	union
	{
		T w, a;
	};

	constexpr vector4_base() = default;
	constexpr vector4_base(T nx, T ny, T nz, T nw) :
		x(nx), y(ny), z(nz), w(nw)
	{
	}

	constexpr vector4_base operator+(const vector4_base &vec) const { return vector4_base(x + vec.x, y + vec.y, z + vec.z, w + vec.w); }
	constexpr vector4_base operator-(const vector4_base &vec) const { return vector4_base(x - vec.x, y - vec.y, z - vec.z, w - vec.w); }
	constexpr vector4_base operator-() const { return vector4_base(-x, -y, -z, -w); }
	constexpr vector4_base operator*(const vector4_base &vec) const { return vector4_base(x * vec.x, y * vec.y, z * vec.z, w * vec.w); }
	constexpr vector4_base operator*(const T rhs) const { return vector4_base(x * rhs, y * rhs, z * rhs, w * rhs); }
	constexpr vector4_base operator/(const vector4_base &vec) const { return vector4_base(x / vec.x, y / vec.y, z / vec.z, w / vec.w); }
	constexpr vector4_base operator/(const T vec) const { return vector4_base(x / vec, y / vec, z / vec, w / vec); }

	constexpr vector4_base &operator+=(const vector4_base &vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		w += vec.w;
		return *this;
	}
	constexpr vector4_base &operator-=(const vector4_base &vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		w -= vec.w;
		return *this;
	}
	constexpr vector4_base &operator*=(const T rhs)
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
		w *= rhs;
		return *this;
	}
	constexpr vector4_base &operator*=(const vector4_base &vec)
	{
		x *= vec.x;
		y *= vec.y;
		z *= vec.z;
		w *= vec.w;
		return *this;
	}
	constexpr vector4_base &operator/=(const T rhs)
	{
		x /= rhs;
		y /= rhs;
		z /= rhs;
		w /= rhs;
		return *this;
	}
	constexpr vector4_base &operator/=(const vector4_base &vec)
	{
		x /= vec.x;
		y /= vec.y;
		z /= vec.z;
		w /= vec.w;
		return *this;
	}

	constexpr bool operator==(const vector4_base &vec) const { return x == vec.x && y == vec.y && z == vec.z && w == vec.w; } // TODO: do this with an eps instead
	constexpr bool operator!=(const vector4_base &vec) const { return x != vec.x || y != vec.y || z != vec.z || w != vec.w; }
};

typedef vector4_base<float> vec4;
typedef vector4_base<bool> bvec4;
typedef vector4_base<int> ivec4;
typedef vector4_base<uint8_t> ubvec4;

#endif
