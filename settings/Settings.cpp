#include "Settings.h"

#include <ranges>

#include <QFile>
#include <QSettings>

#include "fnd/memory.h"
#include "fnd/observable.h"

#include "ISettingsObserver.h"

namespace HomeCompa::SettingsFactory {

namespace {

void Copy(ISettings& dst, const ISettings& src)
{
	const auto enumerate = [&](const auto& r) -> void {
		for (const auto& key : src.GetKeys())
		{
			const auto value = src.Get(key);
			dst.Set(key, value, false);
		}

		for (const auto& group : src.GetGroups())
		{
			const SettingsGroup dstGroup(dst, group), srcGroup(src, group);
			r(r);
		}
	};

	enumerate(enumerate);
}

class SettingsStub final : public AbstractSettings
{
private: // ISettings
	[[nodiscard]] QVariant Get(const QString& /*key*/, const QVariant& defaultValue) const override
	{
		return defaultValue;
	}

	[[nodiscard]] QVariant Get(const QString& /*key*/, const std::function<QVariant()>& defaultValueGetter) const override
	{
		return defaultValueGetter();
	}

	bool Set(const QString& /*key*/, const QVariant& /*value*/, bool /*sync*/) override
	{
		return true;
	}

	[[nodiscard]] bool HasKey(const QString& /*key*/) const override
	{
		return false;
	}

	[[nodiscard]] bool HasGroup(const QString& /*group*/) const override
	{
		return false;
	}

	[[nodiscard]] QStringList GetKeys() const override
	{
		return {};
	}

	[[nodiscard]] QStringList GetGroups() const override
	{
		return {};
	}

	void Remove(const QString& /*key*/) override
	{
	}

	void Save(const QString& /*path*/) const override
	{
	}

	void Load(const QString& /*path*/) override
	{
	}

	std::unique_lock<std::recursive_mutex> BeginGroup(const QString&) const override
	{
		return std::unique_lock(m_mutex);
	}

	void EndGroup() const override
	{
	}

	void RegisterObserver(ISettingsObserver*) override
	{
	}

	void UnregisterObserver(ISettingsObserver*) override
	{
	}

private:
	mutable std::recursive_mutex m_mutex;
};

class SettingsDecorator final : public AbstractSettings
{
public:
	SettingsDecorator(std::shared_ptr<ISettings> impl, std::unordered_map<QString, QVariant> replacement)
		: m_impl { std::move(impl) }
		, m_replacement { std::move(replacement) }
	{
	}

private: // ISettings
	[[nodiscard]] QVariant Get(const QString& key, const QVariant& defaultValue) const override
	{
		if (const auto it = m_replacement.find(key); it != m_replacement.end())
			return it->second;

		return m_impl->Get(key, defaultValue);
	}

	QVariant Get(const QString& key, const std::function<QVariant()>& defaultValueGetter) const override
	{
		if (const auto it = m_replacement.find(key); it != m_replacement.end())
			return it->second;

		return m_impl->Get(key, defaultValueGetter);
	}

	bool Set(const QString& key, const QVariant& value, const bool sync) override
	{
		if (auto it = m_replacement.find(key); it != m_replacement.end())
		{
			if (it->second == value)
				return false;

			it->second = value;
			return true;
		}

		return m_impl->Set(key, value, sync);
	}

	[[nodiscard]] bool HasKey(const QString& key) const override
	{
		return m_replacement.contains(key) || m_impl->HasKey(key);
	}

	[[nodiscard]] bool HasGroup(const QString& group) const override
	{
		return m_impl->HasGroup(group);
	}

	[[nodiscard]] QStringList GetKeys() const override
	{
		auto keys = m_impl->GetKeys() | std::ranges::to<std::unordered_set>();
		std::ranges::copy(m_replacement | std::views::keys, std::inserter(keys, keys.end()));
		return keys | std::ranges::to<QStringList>();
	}

	[[nodiscard]] QStringList GetGroups() const override
	{
		return m_impl->GetGroups();
	}

	void Remove(const QString& key) override
	{
		m_impl->Remove(key);
		m_replacement.erase(key);
	}

	void Save(const QString& path) const override
	{
		m_impl->Save(path);
	}

	void Load(const QString& path) override
	{
		m_impl->Load(path);
	}

	std::unique_lock<std::recursive_mutex> BeginGroup(const QString&) const override
	{
		return std::unique_lock(m_mutex);
	}

	void EndGroup() const override
	{
	}

	void RegisterObserver(ISettingsObserver* observer) override
	{
		m_impl->RegisterObserver(observer);
	}

	void UnregisterObserver(ISettingsObserver* observer) override
	{
		m_impl->UnregisterObserver(observer);
	}

private:
	mutable std::recursive_mutex                  m_mutex;
	PropagateConstPtr<ISettings, std::shared_ptr> m_impl;
	std::unordered_map<QString, QVariant>         m_replacement;
};

class Settings final
    : public AbstractSettings
    , Observable<ISettingsObserver>
{
public:
	explicit Settings(const QString& fileName)
		: m_settings(fileName, QSettings::Format::IniFormat)
	{
	}

	Settings(const QString& organization, const QString& application)
		: m_settings(QSettings::NativeFormat, QSettings::UserScope, organization, application)
	{
	}

private: // ISettings
	[[nodiscard]] QVariant Get(const QString& key, const QVariant& defaultValue = {}) const override
	{
		std::lock_guard lock(m_mutex);
		if (auto value = m_settings.value(key); value.isValid())
			return value;
		return defaultValue;
	}

	QVariant Get(const QString& key, const std::function<QVariant()>& defaultValueGetter) const override
	{
		std::lock_guard lock(m_mutex);
		if (auto value = m_settings.value(key); value.isValid())
			return value;
		return defaultValueGetter();
	}

	bool Set(const QString& key, const QVariant& value, const bool sync) override
	{
		if (Get(key) == value)
			return false;

		std::lock_guard lock(m_mutex);
		m_settings.setValue(key, value);
		Perform(&ISettingsObserver::HandleValueChanged, Key(key), std::cref(value));

		if (sync)
			m_settings.sync();

		return true;
	}

	[[nodiscard]] bool HasKey(const QString& key) const override
	{
		std::lock_guard lock(m_mutex);
		return m_settings.contains(key);
	}

	[[nodiscard]] bool HasGroup(const QString& group) const override
	{
		return GetGroups().contains(group);
	}

	[[nodiscard]] QStringList GetKeys() const override
	{
		std::lock_guard lock(m_mutex);
		return m_settings.childKeys();
	}

	[[nodiscard]] QStringList GetGroups() const override
	{
		std::lock_guard lock(m_mutex);
		return m_settings.childGroups();
	}

	void Remove(const QString& key) override
	{
		std::lock_guard lock(m_mutex);
		m_settings.remove(key);
		m_settings.sync();
	}

	void Save(const QString& path) const override
	{
		QFile::remove(path);
		Settings dst(path);
		Copy(dst, *this);
	}

	void Load(const QString& path) override
	{
		assert(QFile::exists(path));
		const Settings src(path);
		Copy(*this, src);
	}

	std::unique_lock<std::recursive_mutex> BeginGroup(const QString& group) const override
	{
		std::unique_lock lock(m_mutex);
		m_settings.beginGroup(group);
		m_group.push_back(Key(group));
		return lock;
	}

	void EndGroup() const override
	{
		std::lock_guard lock(m_mutex);
		m_settings.endGroup();
		m_group.pop_back();
	}

	void RegisterObserver(ISettingsObserver* observer) override
	{
		Register(observer);
	}

	void UnregisterObserver(ISettingsObserver* observer) override
	{
		Unregister(observer);
	}

private:
	QString Key(const QString& key) const
	{
		return m_group.back() + (m_group.back().isEmpty() ? "" : "/") + key;
	}

private:
	mutable QSettings            m_settings;
	mutable std::vector<QString> m_group { 1 };
	mutable std::recursive_mutex m_mutex;
};

} // namespace

std::unique_ptr<AbstractSettings> CreateStub()
{
	return std::make_unique<SettingsStub>();
}

std::unique_ptr<AbstractSettings> Create(const QString& fileName)
{
	return std::make_unique<Settings>(fileName);
}

std::unique_ptr<AbstractSettings> Create(const QString& organization, const QString& application)
{
	return std::make_unique<Settings>(organization, application);
}

std::unique_ptr<AbstractSettings> CreateDecorator(std::shared_ptr<ISettings> impl, std::unordered_map<QString, QVariant> replacement)
{
	return std::make_unique<SettingsDecorator>(std::move(impl), std::move(replacement));
}

} // namespace HomeCompa::SettingsFactory
