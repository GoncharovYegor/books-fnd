#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <functional>
#include <iterator>
#include <ranges>
#include <span>
#include <string>

template <class Predicate, class Value>
concept BinaryLogicalPredicate = requires(const std::remove_reference_t<Predicate>& predicate, const std::remove_reference_t<Value>& lhs, const std::remove_reference_t<Value>& rhs) {
	{ predicate(lhs, rhs) } -> std::same_as<bool>;
};

//.............................................................................
/// Ищем в диапазоне пар по first, возвращаем итератор
template <std::input_iterator InputIterator, typename Value = std::iter_value_t<InputIterator>, BinaryLogicalPredicate<typename Value::first_type> KeyEqual = std::equal_to<typename Value::first_type>>
constexpr InputIterator FindPairIteratorByFirst(InputIterator begin, InputIterator end, const typename Value::first_type& key, KeyEqual comparer = KeyEqual {})
{
	return std::find_if(begin, end, [&](const auto& item) {
		return comparer(key, item.first);
	});
}

template <std::ranges::input_range Container, typename Value = std::ranges::range_value_t<Container>, BinaryLogicalPredicate<typename Value::first_type> KeyEqual = std::equal_to<typename Value::first_type>>
constexpr std::ranges::iterator_t<const Container> FindPairIteratorByFirst(const Container& container, const typename Value::first_type& key, KeyEqual comparer = KeyEqual {})
{
	return FindPairIteratorByFirst(std::begin(container), std::end(container), key, std::move(comparer));
}

//.............................................................................
/// Ищем в диапазоне пар по first, возвращаем second
template <std::input_iterator InputIterator, typename Value = std::iter_value_t<InputIterator>, BinaryLogicalPredicate<typename Value::first_type> KeyEqual = std::equal_to<typename Value::first_type>>
const typename Value::second_type& FindSecond(InputIterator begin, InputIterator end, const typename Value::first_type& key, KeyEqual comparer = KeyEqual {})
{
	const auto it = FindPairIteratorByFirst(begin, end, key, std::move(comparer));
	assert(it != end);
	return it->second;
}

template <std::ranges::input_range Container, typename Value = std::ranges::range_value_t<Container>, BinaryLogicalPredicate<typename Value::first_type> KeyEqual = std::equal_to<typename Value::first_type>>
const typename Value::second_type& FindSecond(const Container& container, const typename Value::first_type& key, KeyEqual comparer = KeyEqual {})
{
	return FindSecond(std::begin(container), std::end(container), key, std::move(comparer));
}

//.............................................................................
/// Ищем в диапазоне пар по first, если нашли - возвращаем second, иначе - defaultValue
template <std::input_iterator InputIterator, typename Value = std::iter_value_t<InputIterator>, BinaryLogicalPredicate<typename Value::first_type> KeyEqual = std::equal_to<typename Value::first_type>>
const typename Value::second_type& FindSecond(InputIterator begin, InputIterator end, const typename Value::first_type& key, const typename Value::second_type& defaultValue, KeyEqual comparer = KeyEqual {})
{
	const auto it = FindPairIteratorByFirst(begin, end, key, std::move(comparer));
	return it != end ? it->second : defaultValue;
}

template <std::ranges::input_range Container, typename Value = std::ranges::range_value_t<Container>, BinaryLogicalPredicate<typename Value::first_type> KeyEqual = std::equal_to<typename Value::first_type>>
const typename Value::second_type& FindSecond(const Container& container, const typename Value::first_type& key, const typename Value::second_type& defaultValue, KeyEqual comparer = KeyEqual {})
{
	return FindSecond(std::begin(container), std::end(container), key, defaultValue, std::move(comparer));
}

//.............................................................................
/// Ищем в диапазоне пар по second, возвращаем итератор
template <std::input_iterator InputIterator, typename Value = std::iter_value_t<InputIterator>, BinaryLogicalPredicate<typename Value::second_type> KeyEqual = std::equal_to<typename Value::second_type>>
InputIterator FindPairIteratorBySecond(InputIterator begin, InputIterator end, const typename Value::second_type& key, KeyEqual comparer = KeyEqual {})
{
	return std::find_if(begin, end, [&](const auto& item) {
		return comparer(key, item.second);
	});
}

template <std::ranges::input_range Container, typename Value = std::ranges::range_value_t<Container>, BinaryLogicalPredicate<typename Value::second_type> KeyEqual = std::equal_to<typename Value::second_type>>
constexpr std::ranges::iterator_t<const Container> FindPairIteratorBySecond(const Container& container, const typename Value::second_type& key, KeyEqual comparer = KeyEqual {})
{
	return FindPairIteratorBySecond(std::begin(container), std::end(container), key, std::move(comparer));
}

//.............................................................................
/// Ищем в диапазоне пар по second, возвращаем first
template <std::input_iterator InputIterator, typename Value = std::iter_value_t<InputIterator>, BinaryLogicalPredicate<typename Value::second_type> KeyEqual = std::equal_to<typename Value::second_type>>
const typename Value::first_type& FindFirst(InputIterator begin, InputIterator end, const typename Value::second_type& key, KeyEqual comparer = KeyEqual {})
{
	const auto it = FindPairIteratorBySecond(begin, end, key, std::move(comparer));
	assert(it != end);
	return it->first;
}

template <std::ranges::input_range Container, typename Value = std::ranges::range_value_t<Container>, BinaryLogicalPredicate<typename Value::second_type> KeyEqual = std::equal_to<typename Value::second_type>>
const typename Value::first_type& FindFirst(const Container& container, const typename Value::second_type& key, KeyEqual comparer = KeyEqual {})
{
	return FindFirst(std::begin(container), std::end(container), key, std::move(comparer));
}

//.............................................................................
/// Ищем в диапазоне пар по second, если нашли - возвращаем first, иначе - defaultValue
template <std::input_iterator InputIterator, typename Value = std::iter_value_t<InputIterator>, BinaryLogicalPredicate<typename Value::second_type> KeyEqual = std::equal_to<typename Value::second_type>>
const typename Value::first_type& FindFirst(InputIterator begin, InputIterator end, const typename Value::second_type& key, const typename Value::first_type& defaultValue, KeyEqual comparer = KeyEqual {})
{
	const auto it = FindPairIteratorBySecond(begin, end, key, std::move(comparer));
	return it != end ? it->first : defaultValue;
}

template <std::ranges::input_range Container, typename Value = std::ranges::range_value_t<Container>, BinaryLogicalPredicate<typename Value::second_type> KeyEqual = std::equal_to<typename Value::second_type>>
const typename Value::first_type& FindFirst(const Container& container, const typename Value::second_type& key, const typename Value::first_type& defaultValue, KeyEqual comparer = KeyEqual {})
{
	return FindFirst(std::begin(container), std::end(container), key, defaultValue, std::move(comparer));
}

//.............................................................................

struct PszComparer
{
	bool operator()(std::string_view lhs, std::string_view rhs) const
	{
		return compare(lhs, rhs);
	}

	bool operator()(std::span<const unsigned char> lhs, std::string_view rhs) const
	{
		return compare(lhs, rhs);
	}

	bool operator()(std::string_view lhs, std::span<const unsigned char> rhs) const
	{
		return compare(lhs, rhs);
	}

private:
	template <typename L, typename R>
	bool compare(L lhs, R rhs) const
	{
		const auto [lit, rit] = std::ranges::mismatch(lhs, rhs, [](const unsigned char lhs, const char rhs) {
			return lhs == static_cast<unsigned char>(rhs);
		});

		return (lit == lhs.end() && rit == rhs.end()) || (lit == lhs.end() && *rit == 0) || (rit == rhs.end() && *lit == 0);
	}
};

struct PszComparerCaseInsensitive
{
	bool operator()(std::string_view lhs, std::string_view rhs) const
	{
		return compare(lhs, rhs);
	}

	bool operator()(std::span<const unsigned char> lhs, std::string_view rhs) const
	{
		return compare(lhs, rhs);
	}

	bool operator()(std::string_view lhs, std::span<const unsigned char> rhs) const
	{
		return compare(lhs, rhs);
	}

private:
	template <typename L, typename R>
	bool compare(L lhs, R rhs) const
	{
		const auto [lit, rit] = std::ranges::mismatch(lhs, rhs, [](const unsigned char lhs, const char rhs) {
			return std::tolower(lhs) == std::tolower(rhs);
		});

		return (lit == lhs.end() && rit == rhs.end()) || (lit == lhs.end() && *rit == 0) || (rit == rhs.end() && *lit == 0);
	}
};
