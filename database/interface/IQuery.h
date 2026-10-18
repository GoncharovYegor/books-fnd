#pragma once

#include <string>

#include <boost/iterator/iterator_facade.hpp>

#include "ICommand.h"

namespace HomeCompa::DB {

class IQuery;
template <typename T>
T GetImpl(const IQuery& query, size_t index) = delete;

class IQuery : public ICommand
{
public:
	class Iterator : public boost::iterator_facade<Iterator, IQuery, boost::forward_traversal_tag>
	{
	public:
		explicit Iterator(IQuery* query = nullptr)
			: m_query { query }
		{
			if (!m_query)
				return;

			m_query->Execute();
			m_id = 0;
		}

	private:
		friend class boost::iterator_core_access;

		IQuery& dereference() const
		{
			return *m_query;
		}

		bool equal(const Iterator& other) const
		{
			return m_id == other.m_id;
		}

		void increment()
		{
			m_query->Next();
			m_query->Eof() ? m_id = -1 : ++m_id;
		}

	private:
		IQuery*   m_query;
		long long m_id { -1 };
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

	Iterator begin()
	{
		return Iterator { this };
	}

	Iterator end()
	{
		return Iterator {};
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
