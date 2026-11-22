#pragma once
#include <cstdint>
#include <functional>

#include <QDateTime>

class QIODevice;
class QString;

namespace HomeCompa {

class IZipFileProvider // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	virtual ~IZipFileProvider() = default;

	virtual size_t                        GetCount() const noexcept                = 0;
	virtual QString                       GetFileName(size_t index) const          = 0;
	virtual QDateTime                     GetFileTime(size_t index) const          = 0;
	virtual const std::vector<std::byte>& GetFileData(size_t index) const noexcept = 0;
};

class IZipFileController : public IZipFileProvider
{
public:
	virtual void AddFile(QString name, const QByteArray& body, QDateTime time = {}) = 0;
	virtual void AddFile(const QString& path)                                       = 0;
};

} // namespace HomeCompa

namespace HomeCompa::ZipDetails {

class ProgressCallback // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	virtual ~ProgressCallback() = default;

	virtual void OnStartWithTotal(int64_t totalBytes) = 0;
	virtual void OnSetCompleted(int64_t bytes)        = 0;
	virtual void OnIncrement(int64_t bytes)           = 0;
	virtual void OnDone()                             = 0;
	virtual void OnFileDone(const QString& filePath)  = 0;
	virtual bool OnCheckBreak()                       = 0;
};

} // namespace HomeCompa::ZipDetails
