#pragma once

#include <filesystem>
#include <format>

#include <QString>

template <>
struct std::formatter<QString> : std::formatter<std::string>
{
	auto format(const QString& obj, std::format_context& ctx) const
	{
		return std::formatter<std::string>::format(obj.toStdString(), ctx);
	}
};

template <>
struct std::formatter<std::filesystem::path> : std::formatter<std::string>
{
	auto format(const std::filesystem::path& obj, std::format_context& ctx) const
	{
		return std::formatter<std::string>::format(obj.string(), ctx);
	}
};
