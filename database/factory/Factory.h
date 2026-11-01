#pragma once

#include <memory>
#include <string>

#include "export/dbfactory.h"

namespace HomeCompa::DB {

class IDatabase;
class IDatabaseWritable;

}

namespace HomeCompa::DB::Factory {

enum class Impl
{
	Sqlite,
};

namespace Details {

template <typename T, typename... AllowedTypes>
concept IsOneOf = (... || std::same_as<T, AllowedTypes>);

template <typename T>
concept ExportedDatabaseType = IsOneOf<T, IDatabase, IDatabaseWritable>;

}

template <Details::ExportedDatabaseType T>
DBFACTORY_EXPORT std::unique_ptr<T> Create(Impl impl, const std::string& connection);

extern template DBFACTORY_EXPORT std::unique_ptr<IDatabase> Create(Impl impl, const std::string& connection);
extern template DBFACTORY_EXPORT std::unique_ptr<IDatabaseWritable> Create(Impl impl, const std::string& connection);

} // namespace HomeCompa::DB::Factory
