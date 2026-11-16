#pragma once

#include <gmock/gmock.h>

#include "settings/Settings.h"

namespace HomeCompa::Test {

class MockSettings : public ISettings
{
public: // ISettings
	MOCK_METHOD(QVariant, Get, (const QString& key, const QVariant& defaultValue), (const, override));
	MOCK_METHOD(QVariant, Get, (const QString& key, const std::function<QVariant()>& defaultValueGetter), (const, override));
	MOCK_METHOD(bool, Set, (const QString& key, const QVariant& value, bool sync), (override));
	MOCK_METHOD(bool, HasKey, (const QString& key), (const, override));
	MOCK_METHOD(bool, HasGroup, (const QString& group), (const, override));
	MOCK_METHOD(QStringList, GetKeys, (const QString& prefix), (const, override));
	MOCK_METHOD(QStringList, GetGroups, (const QString& prefix), (const, override));
	MOCK_METHOD(std::unique_ptr<ISettings>, CreateGroupSettings, (QString prefix), (override));
	MOCK_METHOD(void, Remove, (const QString& key), (override));
	MOCK_METHOD(void, Save, (const QString& path, const QString& prefix), (const, override));
	MOCK_METHOD(void, Load, (const QString& path, const QString& prefix), (override));
	MOCK_METHOD(void, RegisterObserver, (ISettingsObserver * observer), (override));
	MOCK_METHOD(void, UnregisterObserver, (ISettingsObserver * observer), (override));
};

static_assert(!std::is_abstract_v<MockSettings>, "MockSettings is abstract!");

} // namespace HomeCompa::Test
