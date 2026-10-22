#pragma once

#include <ranges>
#include <string>

#include "ICommand.h"

namespace HomeCompa::DB {

class IQuery;
template <typename T>
T GetImpl(const IQuery& query, size_t index) = delete;

class IQuery : public ICommand
{
public:
	class Iterator
	{
	public:
		using iterator_concept  = std::input_iterator_tag;
		using iterator_category = std::input_iterator_tag;
		using value_type        = IQuery;
		using difference_type   = std::ptrdiff_t;
		using reference         = IQuery&;

		struct Sentinel
		{
		};

		Iterator()
			: m_query(nullptr)
		{
		}

		explicit Iterator(IQuery* query)
			: m_query(query)
		{
			m_query->Execute();
		}

		reference operator*() const
		{
			return *m_query;
		}

		IQuery* operator->() const
		{
			return m_query;
		}

		Iterator& operator++()
		{
			if (m_query)
				m_query->Next();
			return *this;
		}

		void operator++(int)
		{
			++(*this);
		}

		bool operator==(Sentinel) const
		{
			return m_query == nullptr || m_query->Eof();
		}

	private:
		IQuery* m_query;
	};

public:
	virtual bool Eof()   = 0;
	virtual void Next()  = 0;
	virtual void Reset() = 0;

	virtual size_t      ColumnCount() const            = 0;
	virtual std::string ColumnName(size_t index) const = 0;

	virtual bool          IsNull(size_t index) const       = 0;
	virtual int           GetInt(size_t index) const       = 0;
	virtual long long int GetLong(size_t index) const      = 0;
	virtual double        GetDouble(size_t index) const    = 0;
	virtual std::string   GetString(size_t index) const    = 0;
	virtual QString       GetQString(size_t index) const   = 0;
	virtual const char*   GetRawString(size_t index) const = 0;

	template <typename T>
	T Get(const size_t index) const
	{
		return GetImpl<T>(*this, index);
	}

	template <typename First, typename Second>
	auto Get(const size_t indexFirst, const size_t indexSecond) const
	{
		return std::make_pair(Get<First>(indexFirst), Get<Second>(indexSecond));
	}

	template <typename... ARGS>
	std::tuple<ARGS...> Get() const
	{
		return GetTupleImpl<ARGS...>(std::index_sequence_for<ARGS...> {});
	}

	template <typename Functor>
	void ForEach(Functor&& f)
	{
		std::ranges::for_each(*this, std::forward<Functor>(f));
	}

	Iterator begin()
	{
		return Iterator { this };
	}

    Iterator::Sentinel end()
	{
		return Iterator::Sentinel {};
	}

private:
	template <typename... ARGS, std::size_t... Is>
	std::tuple<ARGS...> GetTupleImpl(std::index_sequence<Is...>) const
	{
		return std::make_tuple(Get<ARGS>(Is)...);
	}
};

template <>
inline int GetImpl(const IQuery& query, const size_t index)
{
	return query.GetInt(index);
}

template <>
inline long GetImpl(const IQuery& query, const size_t index)
{
	return static_cast<long>(query.GetLong(index));
}

template <>
inline long long int GetImpl(const IQuery& query, const size_t index)
{
	return query.GetLong(index);
}

template <>
inline size_t GetImpl(const IQuery& query, const size_t index)
{
	return static_cast<size_t>(query.GetLong(index));
}

template <>
inline double GetImpl(const IQuery& query, const size_t index)
{
	return query.GetDouble(index);
}

template <>
inline std::string GetImpl(const IQuery& query, const size_t index)
{
	return query.GetString(index);
}

template <>
inline QString GetImpl(const IQuery& query, const size_t index)
{
	return query.GetQString(index);
}

template <>
inline const char* GetImpl(const IQuery& query, const size_t index)
{
	return query.GetRawString(index);
}

} // namespace HomeCompa::DB
