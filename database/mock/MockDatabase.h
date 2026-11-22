#include <gmock/gmock.h>

#include "database/interface/IDatabase.h"
#include "database/interface/ITemporaryTable.h"
#include "database/interface/ITransaction.h"

namespace HomeCompa::DB::Test {

class MockDatabaseObserver : public IDatabaseObserver
{
public: // IDatabaseObserver
	MOCK_METHOD(void, OnInsert, (std::string_view dbName, std::string_view tableName, int64_t rowId), (override));
	MOCK_METHOD(void, OnUpdate, (std::string_view dbName, std::string_view tableName, int64_t rowId), (override));
	MOCK_METHOD(void, OnDelete, (std::string_view dbName, std::string_view tableName, int64_t rowId), (override));
};

class MockDatabase : public IDatabase
{
public: // IDatabase
	MOCK_METHOD(std::unique_ptr<IQuery>, CreateQuery, (std::string_view query), (override));
	MOCK_METHOD(void, RegisterObserver, (IDatabaseObserver * observer), (override));
	MOCK_METHOD(void, UnregisterObserver, (IDatabaseObserver * observer), (override));
};

class MockDatabaseWritable : public IDatabaseWritable
{
public: // IDatabase
	MOCK_METHOD(std::unique_ptr<IQuery>, CreateQuery, (std::string_view query), (override));
	MOCK_METHOD(void, RegisterObserver, (IDatabaseObserver * observer), (override));
	MOCK_METHOD(void, UnregisterObserver, (IDatabaseObserver * observer), (override));

public: // IDatabaseWritable
	MOCK_METHOD(std::unique_ptr<ITransaction>, CreateTransaction, (), (override));
	MOCK_METHOD(std::unique_ptr<ITemporaryTable>, CreateTemporaryTable, (const std::vector<std::string_view>& fields, const std::vector<std::string_view>& additional), (override));
	MOCK_METHOD(void, CreateFunction, (std::string_view name, DatabaseFunction function), (override));
};

static_assert(!std::is_abstract_v<MockDatabaseObserver>, "MockDatabaseObserver is abstract!");
static_assert(!std::is_abstract_v<MockDatabase>, "MockDatabase is abstract!");
static_assert(!std::is_abstract_v<MockDatabaseWritable>, "MockDatabaseWritable is abstract!");

} // namespace HomeCompa::DB::Test
