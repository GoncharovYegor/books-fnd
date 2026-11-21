#include "zip.h"

#include <ranges>

#include <QBuffer>
#include <QFileInfo>
#include <QVariant>

#include "fnd/FindPair.h"
#include "fnd/linear.h"
#include "fnd/memory.h"

#include "impl/archive.h"
#include "zip/interface/file.h"
#include "zip/interface/zip.h"

#include "log.h"

using namespace HomeCompa;
using namespace ZipDetails;

namespace {

class ProgressCallbackStub final : public ProgressCallback
{
public:
	void OnStartWithTotal([[maybe_unused]] const int64_t totalBytes) override
	{
#ifdef ADDITIONAL_LOG_ENABLED
		m_currentPct = 0;
		m_l          = Linear<int64_t, int> { 0, 0, std::max(totalBytes, int64_t { 1 }), 100 };
#endif
	}

	void OnIncrement(int64_t) override
	{
	}

	void OnSetCompleted([[maybe_unused]] const int64_t bytes) override
	{
#ifdef ADDITIONAL_LOG_ENABLED
		if (!m_needLog && bytes && bytes * 10 < m_l.x().second)
			m_needLog = true;

		if (const auto pct = m_l(bytes); pct != m_currentPct)
		{
			m_currentPct = pct;
			PLOGV_IF(m_needLog) << "extracting progress: " << m_currentPct << "%";
		}
#endif
	}

	void OnDone() override
	{
#ifdef ADDITIONAL_LOG_ENABLED
		PLOGV_IF(m_needLog) << "extracting done";
#endif
	}

	void OnFileDone([[maybe_unused]] const QString& filePath) override
	{
#ifdef ADDITIONAL_LOG_ENABLED
		PLOGV_IF(m_needLog) << "extracting done: " << filePath;
#endif
	}

	bool OnCheckBreak() override
	{
		return false;
	}

private:
#ifdef ADDITIONAL_LOG_ENABLED
	Linear<int64_t, int> m_l { 0, 0, 100, 100 };
	int                  m_currentPct { 0 };
	bool                 m_needLog { false };
#endif
};

std::shared_ptr<ProgressCallback> GetProgress(std::shared_ptr<ProgressCallback> progress)
{
	if (progress)
		return progress;

	return std::make_shared<ProgressCallbackStub>();
}

constexpr std::pair<const char*, Zip::Format> ZIP_FORMATS[] {
	{ "zip",      Zip::Format::Zip },
	{  "7z", Zip::Format::SevenZip },
};

class ZipFileController final : public IZipFileController
{
private:
	struct Item
	{
		QString                name;
		std::vector<std::byte> body;
		QDateTime              time;
	};

private: // IZipFileProvider
	size_t GetCount() const noexcept override
	{
		return m_items.size();
	}

	size_t GetFileSize(const size_t index) const noexcept override
	{
		assert(index < GetCount());
		return m_items[index].body.size();
	}

	QString GetFileName(const size_t index) const override
	{
		assert(index < GetCount());
		const auto& item = m_items[index];
		assert(!item.name.isEmpty());
		return item.name;
	}

	QDateTime GetFileTime(const size_t index) const override
	{
		assert(index < GetCount());
		return m_items[index].time;
	}

	const std::vector<std::byte>& GetFileData(const size_t index) const noexcept override
	{
		assert(index < GetCount());
		return m_items[index].body;
	}

private: // IZipFileController
	void AddFile(QString name, const QByteArray& body, QDateTime time) override
	{
		m_items.emplace_back(std::move(name), std::vector(reinterpret_cast<const std::byte*>(body.data()), reinterpret_cast<const std::byte*>(body.data()) + body.size()), std::move(time));
	}

	void AddFile(const QString& path) override
	{
		const QFileInfo fileInfo(path);
		assert(fileInfo.exists());

		QFile stream(path);

		[[maybe_unused]] const auto ok = stream.open(QIODevice::ReadOnly);
		assert(ok);
		AddFile(fileInfo.fileName(), stream.readAll(), fileInfo.fileTime(QFile::FileBirthTime));
	}

private:
	std::vector<Item> m_items;
};

} // namespace

class Zip::Impl
{
public:
	Impl(const QString& filename, std::shared_ptr<ProgressCallback> progress)
		: m_zip(SevenZip::Archive::CreateReader(filename, GetProgress(std::move(progress))))
		, m_file(std::unique_ptr<IFile> {})
	{
	}

	Impl(QIODevice& stream, std::shared_ptr<ProgressCallback> progress)
		: m_zip(SevenZip::Archive::CreateReaderStream(stream, GetProgress(std::move(progress))))
		, m_file(std::unique_ptr<IFile> {})
	{
	}

	Impl(const QString& filename, const Format format, const bool appendMode, std::shared_ptr<ProgressCallback> progress)
		: m_zip(SevenZip::Archive::CreateWriter(filename, format, GetProgress(std::move(progress)), appendMode))
		, m_file(std::unique_ptr<IFile> {})
	{
	}

	Impl(QIODevice& stream, const Format format, std::shared_ptr<ProgressCallback> progress)
		: m_zip(SevenZip::Archive::CreateWriterStream(stream, format, GetProgress(std::move(progress))))
		, m_file(std::unique_ptr<IFile> {})
	{
	}

	void SetProperty(const PropertyId id, QVariant value)
	{
		m_zip->SetProperty(id, std::move(value));
	}

	QStringList GetFileNameList() const
	{
		return m_zip->GetFileNameList();
	}

	std::unique_ptr<Stream> Read(const QString& filename)
	{
		m_file.reset();
		m_file.reset(m_zip->Read(filename));
		return m_file->Read();
	}

	std::unordered_map<QString, QByteArray> ReadAll() const
	{
		return m_zip->ReadAll();
	}

	std::expected<void, QString> Test() const noexcept
	{
		return m_zip->Test();
	}

	bool Write(const IZipFileProvider& zipFileProvider)
	{
		return m_zip->Write(zipFileProvider);
	}

	bool Remove(const std::vector<QString>& fileNames)
	{
		return m_zip->Remove(fileNames);
	}

	size_t GetFileSize(const QString& filename) const
	{
		return m_zip->GetFileSize(filename);
	}

	const QDateTime& GetFileTime(const QString& filename) const
	{
		return m_zip->GetFileTime(filename);
	}

	size_t GetFileIndex(const QString& filename) const
	{
		return m_zip->GetFileIndex(filename);
	}

private:
	PropagateConstPtr<IZip>  m_zip;
	PropagateConstPtr<IFile> m_file;
};

bool Zip::IsArchive(const QString& filename)
{
	return SevenZip::Archive::IsArchive(filename);
}

QStringList Zip::GetTypes()
{
	return SevenZip::Archive::GetTypes();
}

Zip::Zip(const QString& filename, std::shared_ptr<ProgressCallback> progress)
	: m_impl(std::make_unique<Impl>(filename, std::move(progress)))
{
}

Zip::Zip(QIODevice& stream, std::shared_ptr<ProgressCallback> progress)
	: m_impl(std::make_unique<Impl>(stream, std::move(progress)))
{
}

Zip::Zip(const QString& filename, const Format format, bool appendMode, std::shared_ptr<ProgressCallback> progress)
	: m_impl(std::make_unique<Impl>(filename, format, appendMode, std::move(progress)))
{
}

Zip::Zip(QIODevice& stream, Format format, std::shared_ptr<ProgressCallback> progress)
	: m_impl(std::make_unique<Impl>(stream, format, std::move(progress)))
{
}

Zip::~Zip()                = default;
Zip::Zip(Zip&&)            = default;
Zip& Zip::operator=(Zip&&) = default;

void Zip::SetProperty(const PropertyId id, QVariant value)
{
	m_impl->SetProperty(id, std::move(value));
}

std::unique_ptr<Stream> Zip::Read(const QString& filename) const
{
	return m_impl->Read(filename);
}

std::unordered_map<QString, QByteArray> Zip::ReadAll() const
{
	return m_impl->ReadAll();
}

std::expected<void, QString> Zip::Test() const noexcept
{
	return m_impl->Test();
}

QStringList Zip::GetFileNameList() const
{
	return m_impl->GetFileNameList();
}

bool Zip::Write(const IZipFileProvider& zipFileProvider)
{
	return m_impl->Write(zipFileProvider);
}

bool Zip::Remove(const std::vector<QString>& fileNames)
{
	return m_impl->Remove(fileNames);
}

size_t Zip::GetFileSize(const QString& filename) const
{
	return m_impl->GetFileSize(filename);
}

const QDateTime& Zip::GetFileTime(const QString& filename) const
{
	return m_impl->GetFileTime(filename);
}

size_t Zip::GetFileIndex(const QString& filename) const
{
	return m_impl->GetFileIndex(filename);
}

Zip::Format Zip::FormatFromString(const QString& str)
{
	return FindSecond(ZIP_FORMATS, str.toStdString().data(), PszComparerCaseInsensitive {});
}

QString Zip::FormatToString(const Format format)
{
	return FindFirst(ZIP_FORMATS, format);
}

std::shared_ptr<IZipFileController> Zip::CreateZipFileController()
{
	return std::make_shared<ZipFileController>();
}

std::ostream& operator<<(std::ostream& stream, const Zip::Format format)
{
	return stream << FindFirst(ZIP_FORMATS, format);
}
