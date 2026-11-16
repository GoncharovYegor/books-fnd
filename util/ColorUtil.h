#pragma once

#include <QPalette>

#include "export/util.h"

class QColor;
class QString;

namespace HomeCompa {

class ISettings;

}

namespace HomeCompa::Util {

UTIL_EXPORT QString ToString(const QColor& color);
UTIL_EXPORT QString ToString(const QPalette& palette, QPalette::ColorRole role);
UTIL_EXPORT std::optional<QColor> GetColor(const ISettings& settings, const QString& key);

} // namespace HomeCompa::Util
