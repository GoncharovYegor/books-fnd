#include "Database.h"

#include <gtest/gtest.h>

#include "interface/ICommand.h"
#include "interface/IDatabase.h"
#include "interface/IQuery.h"
#include "interface/ITemporaryTable.h"
#include "interface/ITransaction.h"

#include "mock/MockDatabase.h"

using namespace HomeCompa::DB::Test;
using namespace HomeCompa::DB;

namespace {

constexpr auto DATABASE_PATH = R"("file:ut_sqlite_impl_db?mode=memory&cache=shared")";

struct DatabaseTest : testing::Test
{
	std::unique_ptr<IDatabaseWritable> dw { HomeCompa::DB::Impl::Sqlite::Create<IDatabaseWritable>(DATABASE_PATH) };
	std::unique_ptr<IDatabase>         dr { HomeCompa::DB::Impl::Sqlite::Create<IDatabase>(std::format(R"(path={})", DATABASE_PATH)) };

	DatabaseTest()
	{
		const auto tr = dw->CreateTransaction();
		tr->CreateCommand(R"(
			CREATE TABLE TestTable (
				TestID		INTEGER NOT NULL PRIMARY KEY,
				TestTitle	VARCHAR (128),
				TestValue	REAL
			)
		)")
			->Execute();
		tr->Commit();
	}
};

TEST(Database, Failed)
{
	const auto test = [] {
		std::ignore = HomeCompa::DB::Impl::Sqlite::Create<IDatabase>("db=stub");
	};
	EXPECT_THROW(test(), std::invalid_argument);
}

TEST_F(DatabaseTest, CreateDatabase)
{
	EXPECT_TRUE(!!dr);
	EXPECT_TRUE(!!dw);
}

TEST_F(DatabaseTest, ColumnName)
{
	const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable");
	query->Execute();
	EXPECT_EQ(query->ColumnCount(), 3);
	EXPECT_STREQ(query->ColumnName(0).data(), "TestID");
	EXPECT_STREQ(query->ColumnName(1).data(), "TestTitle");
	EXPECT_STREQ(query->ColumnName(2).data(), "TestValue");
}

TEST_F(DatabaseTest, CreateDatabaseFunction)
{
	bool test = false;
	dw->CreateFunction("TestFunction", [&](DatabaseFunctionContext& ctx) {
		ctx.SetResult(42);
		test = true;
	});
	const auto query = dw->CreateQuery("select TestFunction()");
	query->Execute();
	EXPECT_TRUE(test);
	EXPECT_FALSE(query->Eof());
	EXPECT_EQ(query->Get<int>(0), 42);
}

TEST_F(DatabaseTest, CommandBind)
{
	const auto tr      = dw->CreateTransaction();
	const auto command = tr->CreateCommand("insert into TestTable(TestID, TestTitle, TestValue) values(:id, :txt, :value)");
	command->Bind(":id", 42);
	command->Bind(":txt", "txt");
	command->Bind(":value", 42.0);
	EXPECT_TRUE(command->Execute());
	tr->Commit();
}

TEST_F(DatabaseTest, CommandBind2)
{
	const auto tr      = dw->CreateTransaction();
	const auto command = tr->CreateCommand("insert into TestTable(TestID, TestTitle, TestValue) values(:id, :txt, :value)");
	command->Bind(":id", 0LL);
	command->Bind(":txt");
	command->Bind(":value");
	EXPECT_TRUE(command->Execute());
	tr->Rollback();
}

TEST_F(DatabaseTest, CommandBind3)
{
	const auto tr = dw->CreateTransaction();
	EXPECT_TRUE(tr->CreateCommand("insert into TestTable(TestID) values(1)")->Execute());
	EXPECT_FALSE(tr->CreateCommand("insert into TestTable(TestID) values(1)")->Execute());
	tr->Commit();
}

TEST_F(DatabaseTest, CommandBinder1)
{
	const auto tr      = dw->CreateTransaction();
	const auto command = tr->CreateCommand("insert into TestTable(TestID, TestTitle, TestValue) values(?, ?, ?)");
	command->Binder() << 42 << std::string { "42" } << 42.0;
	command->Execute();
	tr->Commit();
}

TEST_F(DatabaseTest, CommandBinder2)
{
	const auto tr      = dw->CreateTransaction();
	const auto command = tr->CreateCommand("insert into TestTable(TestID, TestTitle, TestValue) values(?, ?, ?)");
	*command << 42 << "42" << std::optional { 42.0 };
	command->Execute();
	tr->Commit();
}

TEST_F(DatabaseTest, CommandBinder3)
{
	const auto tr      = dw->CreateTransaction();
	const auto command = tr->CreateCommand("insert into TestTable(TestID, TestTitle) values(?, ?)");
	command->Execute(std::make_pair(42, "42"));
	tr->Commit();
}

TEST_F(DatabaseTest, QueryBinder)
{
	{
		const auto tr      = dw->CreateTransaction();
		const auto command = tr->CreateCommand("insert into TestTable(TestID, TestTitle, TestValue) values(?, ?, ?)");
		command->Binder() << 42 << "42" << 42.0;
		command->Execute();
		tr->Commit();
	}

	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable");
		query->ForEach([&](IQuery& query) {
			const auto [id, txt, value] = query.Get<long long, QString, double>();
			EXPECT_EQ(id, 42);
			EXPECT_STREQ(txt.toStdString().data(), "42");
			EXPECT_DOUBLE_EQ(value, 42.0);
		});
	}

	const auto check = [&](IQuery& query, const auto&... args) {
		EXPECT_TRUE(query.Execute(args...));
		ASSERT_FALSE(query.Eof());
		const auto [id, txt, value] = query.Get<long long, QString, double>();
		EXPECT_EQ(id, 42);
		EXPECT_STREQ(txt.toStdString().data(), "42");
		EXPECT_DOUBLE_EQ(value, 42.0);
		query.Reset();
	};

	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable");
		check(*query);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestID = ?");
		query->Bind(0, 42);
		check(*query);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestID = :id");
		query->Bind(":id", 42);
		check(*query);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestID = ?", 42);
		check(*query);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestID = ?");
		check(*query, 42);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestValue = ?");
		query->Bind(0, 42.0);
		check(*query);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestValue = :value");
		query->Bind(":value", 42.0);
		check(*query);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestValue = ?", 42.0);
		check(*query);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestValue = ?");
		check(*query, 42.0);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestTitle = ?");
		query->Bind(0, "42");
		check(*query);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestTitle = :txt");
		query->Bind(":txt", "42");
		check(*query);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestTitle = ?", "42");
		check(*query);
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestTitle = ?");
		check(*query, "42");
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestID = ?");
		query->Bind(0);
		query->Execute();
		EXPECT_TRUE(query->Eof());
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestTitle = :txt");
		query->Bind(":txt");
		query->Execute();
		EXPECT_TRUE(query->Eof());
	}
	{
		const auto query = dr->CreateQuery("select TestID, TestTitle, TestValue from TestTable where TestID = ? and TestTitle = ? and TestTitle < ?");
		query->Binder() << 42 << "42" << 42.0;
		check(*query);
	}
}

TEST_F(DatabaseTest, TransactionCommit)
{
	const auto tr = dw->CreateTransaction();
	EXPECT_TRUE(tr->CreateCommand("insert into TestTable(TestID, TestTitle) values(?, ?)", 1, QString { "qwerty" })->Execute());
	EXPECT_TRUE(tr->CreateCommand("insert into TestTable(TestID, TestTitle) values(?, ?)")->Execute(2, QString { "asdfgh" }));
	EXPECT_TRUE(tr->CreateCommand("insert into TestTable(TestID, TestTitle) values(3, ?)", QString { "zxcvbn" })->Execute());
	EXPECT_TRUE(tr->CreateCommand("insert into TestTable(TestID, TestTitle) values(4, ?)")->Execute(QString { "123456" }));
	tr->Commit();
}

TEST_F(DatabaseTest, TransactionRollback)
{
	const auto tr = dw->CreateTransaction();
	EXPECT_TRUE(tr->CreateCommand("insert into TestTable(TestID, TestTitle) values(?, ?)", 1, QString { "qwerty" })->Execute());
	EXPECT_TRUE(tr->CreateCommand("insert into TestTable(TestID, TestTitle) values(?, ?)")->Execute(2, QString { "asdfgh" }));
	EXPECT_TRUE(tr->CreateCommand("insert into TestTable(TestID, TestTitle) values(3, ?)", QString { "zxcvbn" })->Execute());
	EXPECT_TRUE(tr->CreateCommand("insert into TestTable(TestID, TestTitle) values(4, ?)")->Execute(QString { "123456" }));
	tr->Rollback();
}

TEST_F(DatabaseTest, DatabaseObserve)
{
	testing::StrictMock<MockDatabaseObserver> observer;
	dw->RegisterObserver(&observer);
	EXPECT_CALL(observer, OnInsert("main", "TestTable", 1));
	EXPECT_CALL(observer, OnUpdate("main", "TestTable", 1));
	EXPECT_CALL(observer, OnDelete("main", "TestTable", 1));
	const auto tr = dw->CreateTransaction();
	EXPECT_TRUE(tr->CreateCommand("insert into TestTable(TestID, TestTitle) values(1, 'qwerty')")->Execute());
	EXPECT_TRUE(tr->CreateCommand("update TestTable set TestTitle = 'asdfg' where TestID = 1")->Execute());
	EXPECT_TRUE(tr->CreateCommand("delete from TestTable where TestID = 1")->Execute());
	tr->Commit();
	dw->UnregisterObserver(&observer);
}

TEST_F(DatabaseTest, DatabaseTemporaryTable)
{
	const auto tmpTable = dw->CreateTemporaryTable({ IDatabase::DEFAULT_TEMPORARY_TABLE_FIELD }, { "CREATE INDEX IF NOT EXISTS tmp.IX_Temp ON Tab_1 (id)" });
	EXPECT_TRUE(!!tmpTable);
	EXPECT_STREQ(tmpTable->GetSchemaName().data(), "tmp");
	EXPECT_TRUE(tmpTable->GetName().starts_with("tmp"));
}

} // namespace
