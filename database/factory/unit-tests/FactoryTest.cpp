#include "database/factory/Factory.h"

#include <gtest/gtest.h>

using namespace HomeCompa::DB;

namespace {

TEST(DBFactory, CreateSqliteDatabaseStatic)
{
	const auto db = Factory::Create<IDatabase>(IFactory::Impl::Sqlite, R"(path="file:my_shared_mem_db?mode=memory&cache=shared")");
	EXPECT_TRUE(!!db);
}

TEST(DBFactory, CreateSqliteDatabaseWritableStatic)
{
	const auto db = Factory::Create<IDatabaseWritable>(IFactory::Impl::Sqlite, R"(path="file:my_shared_mem_db?mode=memory&cache=shared")");
	EXPECT_TRUE(!!db);
}

TEST(DBFactory, CreateSqliteDatabaseFactory)
{
	std::unique_ptr<const IFactory> factory { std::make_unique<DatabaseFactory>() };
	EXPECT_TRUE(!!factory);
	const auto db = factory->Create(IFactory::Impl::Sqlite, R"(path="file:my_shared_mem_db?mode=memory&cache=shared")");
	EXPECT_TRUE(!!db);
}

TEST(DBFactory, CreateSqliteDatabaseWritableFactory)
{
	std::unique_ptr<const IFactory> factory { std::make_unique<DatabaseFactory>() };
	EXPECT_TRUE(!!factory);
	const auto db = factory->CreateWritable(IFactory::Impl::Sqlite, R"(path="file:my_shared_mem_db?mode=memory&cache=shared")");
	EXPECT_TRUE(!!db);
}

} // namespace
