#pragma once

#include <memory>
#include <string>

#include "log.h"

#include "export/dbsqlite.h"

namespace HomeCompa::DB {

class IDatabase;
class IDatabaseWritable;

}

namespace HomeCompa::DB::Impl::Sqlite {

namespace Details {

template <typename T, typename... AllowedTypes>
concept IsOneOf = (... || std::same_as<T, AllowedTypes>);

template <typename T>
concept ExportedDatabaseType = IsOneOf<T, IDatabase, IDatabaseWritable>;

}

template <Details::ExportedDatabaseType T>
DBSQLITE_EXPORT std::unique_ptr<T> Create(const std::string& connection);

extern template DBSQLITE_EXPORT std::unique_ptr<IDatabase> Create(const std::string& connection);
extern template DBSQLITE_EXPORT std::unique_ptr<IDatabaseWritable> Create(const std::string& connection);

inline std::string_view LogStatement(const std::string_view statement)
{
#ifndef NDEBUG
	PLOGD << statement;
#endif
	return statement;
}

} // namespace HomeCompa::DB::Impl::Sqlite
