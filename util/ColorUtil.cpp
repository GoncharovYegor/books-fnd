#include "ColorUtil.h"

#include "settings/ISettings.h"

namespace HomeCompa::Util {

QString ToString(const QColor& color)
{
	return QString("#%1%2%3").arg(color.red(), 2, 16, QChar { '0' }).arg(color.green(), 2, 16, QChar { '0' }).arg(color.blue(), 2, 16, QChar { '0' });
}

QString ToString(const QPalette& palette, const QPalette::ColorRole role)
{
	return ToString(palette.color(role));
}

std::optional<QColor> GetColor(const ISettings& settings, const QString& key)
{
	const auto var = settings.Get(key);
	if (!var.isValid())
		return {};

	bool       ok    = false;
	const auto value = var.toUInt(&ok);
	if (!ok)
		return {};

	return QColor { static_cast<int>(value & 0xFF), static_cast<int>((value >> 8) & 0xFF), static_cast<int>((value >> 16) & 0xFF), 0xFF - static_cast<int>((value >> 24) & 0xFF) };
}

} // namespace HomeCompa::Util
