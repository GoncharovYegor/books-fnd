#include "Database.h"

#include <filesystem>
#include <numeric>
#include <sstream>
#include <string>

#include "fnd/FindPair.h"
#include "fnd/NonCopyMovable.h"
#include "fnd/observable.h"

#include "IDatabase.h"
#include "IQuery.h"
#include "ITemporaryTable.h"
#include "ITransaction.h"
#include "log.h"
#include "sqlite3ppext.h"

namespace HomeCompa::DB::Impl::Sqlite {

std::unique_ptr<ITransaction>    CreateTransactionImpl(sqlite3pp::database& db);
std::unique_ptr<IQuery>          CreateQueryImpl(sqlite3pp::database& db, std::string_view query);
std::unique_ptr<ITemporaryTable> CreateTemporaryTableImpl(IDatabaseWritable& db, const std::vector<std::string_view>& fields, const std::vector<std::string_view>& additional);

namespace {

constexpr auto PATH      = "path";
constexpr auto EXTENSION = "extension";

using ConnectionParameters = std::multimap<std::string, std::string, std::less<>>;

using ObserverMethod = void (IDatabaseObserver::*)(std::string_view dbName, std::string_view tableName, int64_t rowId);
// ocCodes: sqlite3.cpp
constexpr std::pair<int, ObserverMethod> g_opCodeToObserverMethod[] {
	{ 18, &IDatabaseObserver::OnInsert },
	{  9, &IDatabaseObserver::OnDelete },
	{ 23, &IDatabaseObserver::OnUpdate },
};

std::vector<std::string> Split(const std::string& src, const char separator)
{
	std::vector<std::string> result;
	std::istringstream       stream(src);
	std::string              s;
	while (std::getline(stream, s, separator))
		result.push_back(s);

	return result;
}

ConnectionParameters ParseConnectionString(const std::string& connection)
{
	ConnectionParameters result;
	for (const auto& parameter : Split(connection, ';'))
	{
		auto pair = Split(parameter, '=');
		if (std::size(pair) == 1)
			result.emplace(PATH, std::move(pair.front()));
		else
			result.emplace(std::move(pair.front()), std::move(pair.back()));
	}

	return result;
}

template <Details::ExportedDatabaseType T>
consteval int GetReadWriteFlag() noexcept
{
	if constexpr (std::same_as<T, IDatabaseWritable>)
		return SQLITE_OPEN_READWRITE;
	else
		return SQLITE_OPEN_READONLY;
}

int GetOpenFlags(const std::string& db, int flag)
{
	if ((flag & SQLITE_OPEN_READWRITE) && !exists(std::filesystem::path(db)))
		flag |= SQLITE_OPEN_CREATE;

	return flag | SQLITE_OPEN_URI;
}

const std::string& GetValue(const ConnectionParameters& parameters, const std::string& key)
{
	auto [begin, end] = parameters.equal_range(key);
	if (begin == end)
		throw std::invalid_argument(std::format("must pass {}", key));

	return begin->second;
}

class DatabaseFunctionContext final : public DB::DatabaseFunctionContext
{
public:
	explicit DatabaseFunctionContext(sqlite3pp::ext::context& ctx)
		: m_ctx(ctx)
	{
	}

	void SetResult(const int value) override
	{
		m_ctx.result(value);
	}

private:
	sqlite3pp::ext::context& m_ctx;
};

class Database final
    : public IDatabaseWritable
    , public Observable<IDatabaseObserver>
{
	NON_COPY_MOVABLE(Database)

private:
	struct ObserverImpl
	{
		explicit ObserverImpl(Database& self)
			: m_self(self)
		{
		}

		void OnUpdate(const int opCode, std::string_view dbName, std::string_view tableName, const int64_t rowId) const
		{
			const auto method = FindSecond(g_opCodeToObserverMethod, opCode);
			m_self.Perform(method, dbName, tableName, rowId);
		}

	private:
		Database& m_self;
	};

public:
	Database(const std::string& connection, const int flag)
		: m_connectionParameters(ParseConnectionString(connection))
		, m_db(GetValue(m_connectionParameters, PATH).data(), GetOpenFlags(GetValue(m_connectionParameters, PATH), flag))
		, m_observer(*this)
	{
		for (auto [begin, end] = m_connectionParameters.equal_range(EXTENSION); begin != end; ++begin)
			m_db.load_extension(begin->second.data());

		m_db.set_update_handler([&](const int opCode, const char* dbName, const char* tableName, const int64_t rowId) {
			m_observer.OnUpdate(opCode, dbName, tableName, rowId);
		});

		[[maybe_unused]] const auto ok = Database::CreateQuery("ATTACH DATABASE 'file:shared_attached_mem?mode=memory&cache=shared' AS tmp;")->Execute();
		assert(ok);

		PLOGV << "database connection established";
	}

	~Database() override
	{
		m_db.disconnect();
		PLOGV << "database connection closed";
	}

private: // Database
	[[nodiscard]] std::unique_ptr<ITransaction> CreateTransaction() override
	{
		return CreateTransactionImpl(m_db);
	}

	[[nodiscard]] std::unique_ptr<IQuery> CreateQuery(const std::string_view query) override
	{
		return CreateQueryImpl(m_db, query);
	}

	[[nodiscard]] std::unique_ptr<ITemporaryTable> CreateTemporaryTable(const std::vector<std::string_view>& fields, const std::vector<std::string_view>& additional) override
	{
		return CreateTemporaryTableImpl(*this, fields, additional);
	}

	void CreateFunction(const std::string_view name, DatabaseFunction function) override
	{
		m_functions.try_emplace(std::string(name), std::make_unique<sqlite3pp::ext::function>(m_db)).first->second->create(name.data(), [function = std::move(function)](sqlite3pp::ext::context& ctx) {
			DatabaseFunctionContext context(ctx);
			function(context);
		});
	}

	void RegisterObserver(IDatabaseObserver* observer) override
	{
		Register(observer);
	}

	void UnregisterObserver(IDatabaseObserver* observer) override
	{
		Unregister(observer);
	}

private:
	ConnectionParameters                                                          m_connectionParameters;
	sqlite3pp::database                                                           m_db;
	ObserverImpl                                                                  m_observer;
	std::map<std::string, std::unique_ptr<sqlite3pp::ext::function>, std::less<>> m_functions;
};

} // namespace

template <Details::ExportedDatabaseType T>
std::unique_ptr<T> Create(const std::string& connection)
{
	return std::make_unique<Database>(connection, GetReadWriteFlag<T>());
}

template DBSQLITE_EXPORT std::unique_ptr<IDatabase> Create(const std::string& connection);
template DBSQLITE_EXPORT std::unique_ptr<IDatabaseWritable> Create(const std::string& connection);

} // namespace HomeCompa::DB::Impl::Sqlite
