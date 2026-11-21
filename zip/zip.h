#pragma once

#include <expected>
#include <memory>

#include "zip/interface/ProgressCallback.h"
#include "zip/interface/format.h"
#include "zip/interface/stream.h"
#include "zip/interface/types.h"

#include "export/zip.h"

class QVariant;
class QDateTime;

namespace HomeCompa {

class ZIP_EXPORT Zip
{
public:
	class ProgressCallback : public ZipDetails::ProgressCallback
	{
	};

	using Format            = ZipDetails::Format;
	using PropertyId        = ZipDetails::PropertyId;
	using CompressionLevel  = ZipDetails::CompressionLevel;
	using CompressionMethod = ZipDetails::CompressionMethod;

	static constexpr auto INVALID_INDEX = ZipDetails::INVALID_INDEX;

	static Format  FormatFromString(const QString& str);
	static QString FormatToString(Format format);

	static std::shared_ptr<IZipFileController> CreateZipFileController();

public:
	static bool        IsArchive(const QString& filename);
	static QStringList GetTypes();

public:
	explicit Zip(const QString& filename, std::shared_ptr<ProgressCallback> progress = {});
	explicit Zip(QIODevice& stream, std::shared_ptr<ProgressCallback> progress = {});
	Zip(const QString& filename, Format format, bool appendMode = false, std::shared_ptr<ProgressCallback> progress = {});
	Zip(QIODevice& stream, Format format, std::shared_ptr<ProgressCallback> progress = {});
	~Zip();

	[[nodiscard]] QStringList      GetFileNameList() const;
	[[nodiscard]] size_t           GetFileSize(const QString& filename) const;
	[[nodiscard]] const QDateTime& GetFileTime(const QString& filename) const;
	[[nodiscard]] size_t           GetFileIndex(const QString& filename) const;

	[[nodiscard]] std::unique_ptr<Stream>                 Read(const QString& filename) const;
	[[nodiscard]] std::unordered_map<QString, QByteArray> ReadAll() const;

	[[nodiscard]] std::expected<void, QString> Test() const noexcept;

	void SetProperty(PropertyId id, QVariant value);
	bool Write(const IZipFileProvider& zipFileProvider);

	bool Remove(const std::vector<QString>& fileNames);

public:
	Zip(const Zip&) = delete;
	Zip(Zip&&);
	Zip& operator=(const Zip&) = delete;
	Zip& operator=(Zip&&);

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace HomeCompa

ZIP_EXPORT std::ostream& operator<<(std::ostream& stream, HomeCompa::Zip::Format format);
