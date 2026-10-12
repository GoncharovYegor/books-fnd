#pragma once

#include <functional>
#include <memory>

#include "BookUtil.h"

#include "export/util.h"

class QByteArray;
class QImage;
class QIODevice;
class QString;

namespace HomeCompa {

class ISettings;
class Zip;

} // namespace HomeCompa

namespace HomeCompa::Util {

UTIL_EXPORT QByteArray
PrepareToExport(QIODevice& input, const QString& folder, const QString& fileName, const ISettings& settings, QString encoding, std::unique_ptr<const ExtractedBook> metadataReplacement = {});

using ExtractBookImagesCallback = std::function<bool(QString /*name*/, bool /*isCover*/, QByteArray /*body*/)>;
UTIL_EXPORT void ExtractBookImages(const QString& folder, const QString& fileName, const ISettings& settings, const ExtractBookImagesCallback& callback);

} // namespace HomeCompa::Util
