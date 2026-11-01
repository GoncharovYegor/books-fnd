#include "Factory.h"

#include <cassert>

#include "database/interface/IDatabase.h"

#include "database/impl/sqlite/Database.h"

namespace HomeCompa::DB::Factory {

namespace {

template <typename T>
using FactoryCreator = std::unique_ptr<T> (*)(const std::string& connection);

constexpr std::pair<FactoryCreator<IDatabase>, FactoryCreator<IDatabaseWritable>> DATABASE_CREATIRS[] {
	{
	 &DB::Impl::Sqlite::Create<IDatabase>,
	 &DB::Impl::Sqlite::Create<IDatabaseWritable>,
	 },
};

size_t ToIndex(const Impl impl)
{
	const auto index = static_cast<size_t>(impl);
	assert(index < std::size(DATABASE_CREATIRS));
	return index;
}

template <typename T>
FactoryCreator<T> GetCreator(const Impl impl)
{
	return DATABASE_CREATIRS[ToIndex(impl)];
}

template <>
FactoryCreator<IDatabase> GetCreator(const Impl impl)
{
	return DATABASE_CREATIRS[ToIndex(impl)].first;
}

template <>
FactoryCreator<IDatabaseWritable> GetCreator(const Impl impl)
{
	return DATABASE_CREATIRS[ToIndex(impl)].second;
}

} // namespace

template <Details::ExportedDatabaseType T>
std::unique_ptr<T> Create(const Impl impl, const std::string& connection)
{
	const auto creator = GetCreator<T>(impl);
	assert(creator);
	return creator(connection);
}

template DBFACTORY_EXPORT std::unique_ptr<IDatabase> Create(Impl impl, const std::string& connection);
template DBFACTORY_EXPORT std::unique_ptr<IDatabaseWritable> Create(Impl impl, const std::string& connection);

} // namespace HomeCompa::DB::Factory
