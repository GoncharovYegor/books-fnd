#pragma once

#include <algorithm>
#include <filesystem>
#include <vector>

#include <QDataStream>
#include <QHash>
#include <QIODevice>
#include <QString>
#include <QVariant>

#include "platform/StrUtil.h"

namespace HomeCompa::Util {

template <typename T>
bool InBounds(const T value, std::convertible_to<T> auto min, std::convertible_to<T> auto max)
{
	return !(value < static_cast<T>(min)) && (value < static_cast<T>(max));
}

template <typename T>
static bool Set(T& dst, T value)
{
	if (dst == value)
		return false;

	dst = std::move(value);
	return true;
}

template <typename T, std::invocable F>
bool Set(T& oldValue, T newValue, const F& f)
{
	if (oldValue == newValue)
		return false;

	oldValue = std::move(newValue);
	f();
	return true;
}

template <typename T, std::invocable BeforeF, std::invocable AfterF>
bool Set(T& oldValue, T newValue, const BeforeF& before, const AfterF& after)
{
	if (oldValue == newValue)
		return false;

	before();
	oldValue = std::move(newValue);
	after();
	return true;
}

// из отсортированного диапазона делает вектор диапазонов из идущих подряд элементов
template <typename InputIterator, typename Value = typename std::iterator_traits<InputIterator>::value_type>
std::vector<std::pair<Value, Value>> CreateRanges(InputIterator begin, InputIterator end)
{
	assert(std::is_sorted(begin, end));
	if (begin == end)
		return {};

	std::vector<std::pair<Value, Value>> result {
		{ *begin, *begin + 1 }
	};
	for (++begin; begin != end; ++begin)
		if (*begin == result.back().second)
			++result.back().second;
		else
			result.emplace_back(*begin, *begin + 1);

	return result;
}

template <typename Container, typename ValueType = typename Container::value_type>
std::vector<std::pair<ValueType, ValueType>> CreateRanges(const Container& container)
{
	return Util::CreateRanges(std::cbegin(container), std::cend(container));
}

template <class InputIt1, class InputIt2>
bool Intersect(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
{
	assert(std::is_sorted(first1, last1) && std::is_sorted(first2, last2));
	while (first1 != last1 && first2 != last2)
	{
		if (*first1 < *first2)
		{
			++first1;
			continue;
		}
		if (*first2 < *first1)
		{
			++first2;
			continue;
		}
		return true;
	}
	return false;
}

template <typename Container1, typename Container2>
bool Intersect(const Container1& container1, const Container2& container2)
{
	return Intersect(std::cbegin(container1), std::cend(container1), std::cbegin(container2), std::cend(container2));
}

template <typename T>
void FixSize(T& width, T& height, const T max)
{
	if (width > max || height > max)
	{
		if (width > height)
		{
			height = height * max / width;
			width  = max;
		}
		else
		{
			width  = width * max / height;
			height = max;
		}
	}
}

template <typename T>
QString ToQString(const T& str) = delete;

template <>
inline QString ToQString<std::string>(const std::string& str)
{
	return QString::fromStdString(str);
}

template <>
inline QString ToQString<QString>(const QString& str)
{
	return str;
}

template <>
inline QString ToQString<std::wstring>(const std::wstring& str)
{
	return QString::fromStdWString(str);
}

template <>
inline QString ToQString<std::wstring_view>(const std::wstring_view& str)
{
	return QString::fromStdWString(std::wstring(str));
}

template <>
inline QString ToQString<std::pair<std::wstring, std::wstring>>(const std::pair<std::wstring, std::wstring>& str)
{
	return QString("%1/%2").arg(QString::fromStdWString(str.first), QString::fromStdWString(str.second));
}

template <>
inline QString ToQString<std::pair<QString, QString>>(const std::pair<QString, QString>& str)
{
	return QString("%1/%2").arg(str.first, str.second);
}

template <>
inline QString ToQString<std::filesystem::path>(const std::filesystem::path& str)
{
	return Platform::PathToString(str);
}

template <class T>
[[nodiscard]] T FakeCopyInit(T) noexcept = delete;

template <class T = void>
struct CaseInsensitiveComparer
{
	[[nodiscard]] constexpr bool operator()(const T& lhs, const T& rhs) const noexcept(noexcept(FakeCopyInit<bool>(lhs < rhs)))
	{
		return QString::compare(ToQString(lhs), ToQString(rhs), Qt::CaseInsensitive) < 0;
	}
};

template <>
struct CaseInsensitiveComparer<void>
{
	template <class L, class R>
	[[nodiscard]] constexpr auto operator()(L&& lhs, R&& rhs) const noexcept(noexcept(static_cast<L&&>(lhs) < static_cast<R&&>(rhs))) -> decltype(static_cast<L&&>(lhs) < static_cast<R&&>(rhs))
	{
		return QString::compare(ToQString(static_cast<L&&>(lhs)), ToQString(static_cast<R&&>(rhs)), Qt::CaseInsensitive) < 0;
	}

	using is_transparent = int;
};

template <typename T>
struct CaseInsensitiveHash
{
	size_t operator()(const T& value) const
	{
		return std::hash<QString>()(ToQString(value));
	}
};

template <typename First, typename Second>
struct PairHash
{
	size_t operator()(const std::pair<First, Second>& value) const
	{
		return std::rotl(std::hash<First>()(value.first), 1) | std::hash<Second>()(value.second);
	}
};

template <typename... Args>
struct TupleHash
{
	size_t operator()(const std::tuple<Args...>& value) const
	{
		size_t result = 42;

		const auto update = [&]<typename T>(const T& e) {
			result = std::rotl(result, 1) | std::hash<T>()(e);
		};

		std::apply(
			[&]<typename... T>(const T&... e) {
				((update(e)), ...);
			},
			value
		);

		return result;
	}
};

struct VariantHash
{
	size_t operator()(const QVariant& var) const
	{
		QByteArray bytes;
		{
			QDataStream stream(&bytes, QIODevice::WriteOnly);
			stream << var.typeName() << var;
		}

		return qHash(bytes);
	}
};

struct QStringViewHash
{
	using is_transparent = void;

	size_t operator()(const QStringView bv) const noexcept
	{
		return qHash(bv);
	}

	size_t operator()(const QString& str) const noexcept
	{
		return qHash(str);
	}
};

struct QStringViewEqual
{
	using is_transparent = void;

	bool operator()(const QStringView lhs, const QStringView rhs) const noexcept
	{
		return lhs == rhs;
	}

	bool operator()(const QString& lhs, const QString& rhs) const noexcept
	{
		return lhs == rhs;
	}

	bool operator()(const QString& lhs, const QStringView rhs) const noexcept
	{
		return lhs == rhs;
	}

	bool operator()(const QStringView lhs, const QString& rhs) const noexcept
	{
		return lhs == rhs;
	}
};

} // namespace HomeCompa::Util
