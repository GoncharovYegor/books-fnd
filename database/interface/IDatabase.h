#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "fnd/observer.h"

#include "database/interface/IQuery.h"

namespace HomeCompa::DB {

class ITemporaryTable;
class ITransaction;

class IDatabaseObserver : public Observer
{
public:
	virtual void OnInsert(std::string_view dbName, std::string_view tableName, int64_t rowId) = 0;
	virtual void OnUpdate(std::string_view dbName, std::string_view tableName, int64_t rowId) = 0;
	virtual void OnDelete(std::string_view dbName, std::string_view tableName, int64_t rowId) = 0;
};

class DatabaseFunctionContext // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	virtual ~DatabaseFunctionContext() = default;

	virtual void SetResult(int) = 0;
};

using DatabaseFunction = std::function<void(DatabaseFunctionContext&)>;

class IDatabase // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	static constexpr auto DEFAULT_TEMPORARY_TABLE_FIELD = "id integer primary key not null";

public:
	virtual ~IDatabase()                                                                    = default;
	[[nodiscard]] virtual std::unique_ptr<ITransaction> CreateTransaction()                 = 0;
	[[nodiscard]] virtual std::unique_ptr<IQuery>       CreateQuery(std::string_view query) = 0;
	[[nodiscard]] virtual std::unique_ptr<ITemporaryTable>
	CreateTemporaryTable(const std::vector<std::string_view>& fields = { DEFAULT_TEMPORARY_TABLE_FIELD }, const std::vector<std::string_view>& additional = {}) = 0;

	virtual void CreateFunction(std::string_view name, DatabaseFunction function) = 0;

	virtual void RegisterObserver(IDatabaseObserver* observer)   = 0;
	virtual void UnregisterObserver(IDatabaseObserver* observer) = 0;

	template <typename... ARGS>
	[[nodiscard]] std::unique_ptr<IQuery> CreateQuery(const QStringView query, const ARGS&... args)
	{
		const auto str = query.toUtf8();
		return CreateQuery(std::string_view { str.data(), static_cast<size_t>(str.size()) }, args...);
	}

	template <typename... ARGS>
	[[nodiscard]] std::unique_ptr<IQuery> CreateQuery(const std::string_view text, const ARGS&... args)
	{
		auto query  = CreateQuery(text);
		auto binder = query->Binder();
		((binder << args), ...);
		return query;
	}
};

} // namespace HomeCompa::DB
