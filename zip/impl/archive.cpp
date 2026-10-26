#include "archive.h"

#include <fstream>
#include <ranges>
#include <thread>

#include <QVariant>

#include "fnd/FindPair.h"
#include "fnd/QIODeviceStreamWrapper.h"
#include "fnd/StrUtil.h"

#include "interface/types.h"

#include "bit7z/bit7zlibrary.hpp"
#include "bit7z/bitarchiveeditor.hpp"
#include "bit7z/bitarchivereader.hpp"
#include "bit7z/bitarchivewriter.hpp"
#include "bit7z/bitformat.hpp"
#include "bit7z/bitpropvariant.hpp"
#include "platform/StrUtil.h"
#include "zip/interface/file.h"
#include "zip/interface/format.h"
#include "zip/interface/zip.h"

#include "FileItem.h"
#include "QtTypes.h"
#include "log.h"
#include "reader.h"

namespace HomeCompa::ZipDetails::SevenZip {

namespace {

constexpr const char* ARCHIVE_EXTENSIONS[] {
	"7z",   "zip",   "rar",  "bzip2", "bz2",    "tbz2", "tbz",      "gz",  "gzip", "tgz", "tar",  "ova",   "wim",  "swm", "xz",  "txz",  "zipx",   "jar", "xpi",  "odt",    "ods",  "odp",
	"docx", "xlsx",  "pptx", "epub",  "001",    "ar",   "deb",      "apm", "arj",  "cab", "chm",  "chi",   "msi",  "doc", "xls", "ppt",  "msg",    "obj", "cpio", "cramfs", "dmg",  "dll",
	"exe",  "dylib", "ext",  "ext2",  "ext3",   "ext4", "fat",      "flv", "gpt",  "hfs", "hfsx", "hxs",   "ihex", "lzh", "lha", "lzma", "lzma86", "mbr", "mslz", "mub",    "nsis", "ntfs",
	"pmd",  "ppmd",  "qcow", "qcow2", "qcow2c", "rpm",  "squashfs", "swf", "te",   "udf", "scap", "uefif", "vmdk", "vdi", "vhd", "xar",  "pkg",    "z",   "taz",
};

#ifdef _WIN32
constexpr std::pair<QChar, QChar> FIX_TABLE[] {
	{ QChar { 0x22 }, QChar { 0xF022 } },
    { QChar { 0x2A }, QChar { 0xF02A } },
    { QChar { 0x2F }, QChar { 0xF02F } },
    { QChar { 0x3A }, QChar { 0xF03A } },
    { QChar { 0x3C }, QChar { 0xF03C } },
	{ QChar { 0x3E }, QChar { 0xF03E } },
    { QChar { 0x3F }, QChar { 0xF03F } },
    { QChar { 0x5C }, QChar { 0xF05C } },
    { QChar { 0x7C }, QChar { 0xF07C } },
};

QString FixFileName(QString fileName)
{
	if (std::ranges::any_of(fileName, [](const QChar& item) {
			return item.category() == QChar::Other_PrivateUse;
		}))
		for (const auto& [to, from] : FIX_TABLE)
			fileName.replace(from, to);
	return fileName;
}
#else
QString FixFileName(QString fileName)
{
	return fileName;
}
#endif

#ifdef _WIN32
	#define fromBit7zString fromStdWString
	#define toBit7zString toStdWString
#else
	#define fromBit7zString fromStdString
	#define toBit7zString toStdString
using FILETIME = bit7z::FILETIME;
using DWORD    = bit7z::DWORD;
#endif

const bit7z::BitInOutFormat& GetInOutFormat(const Format format)
{
	static constexpr std::pair<Format, const bit7z::BitInOutFormat&> formats[] {
#define ZIP_FORMAT_ITEM(NAME) { Format::NAME, bit7z::BitFormat::NAME },
		ZIP_FORMAT_ITEMS_X_MACRO
#undef ZIP_FORMAT_ITEM
	};

	return FindSecond(formats, format);
}

constexpr auto NANO100_IN_SECOND         = 10000000;
constexpr auto SECONDS_FROM_1601_TO_1970 = 11644473600LL;

std::optional<qint64> GetTime(const bit7z::BitInputArchive& archive, const uint32_t index, const bit7z::BitProperty property)
{
	if (const auto prop = archive.itemProperty(index, property); !prop.isEmpty())
		if (auto fileTime = prop.getFileTime(); fileTime.dwHighDateTime != 0 || fileTime.dwLowDateTime != 0)
			return ((static_cast<qint64>(fileTime.dwHighDateTime) << 32) | fileTime.dwLowDateTime) / NANO100_IN_SECOND - SECONDS_FROM_1601_TO_1970;

	return std::nullopt;
}

void SetTime(bit7z::BitGenericItem& item, const QDateTime& time)
{
	const auto nano100 = (time.toSecsSinceEpoch() + SECONDS_FROM_1601_TO_1970) * NANO100_IN_SECOND;
	FILETIME   fileTime { .dwLowDateTime = static_cast<DWORD>(nano100 & 0xFFFFFFFF), .dwHighDateTime = static_cast<DWORD>(nano100 >> 32) };
	item.setItemProperty(bit7z::BitProperty::CTime, bit7z::BitPropVariant { fileTime });
	item.setItemProperty(bit7z::BitProperty::ATime, bit7z::BitPropVariant { fileTime });
	item.setItemProperty(bit7z::BitProperty::MTime, bit7z::BitPropVariant { fileTime });
}

FileStorage CreateFileList(const bit7z::BitInputArchive& archive)
{
	FileStorage result;

	const auto numItems = archive.itemsCount();

	result.files.reserve(numItems);

	for (std::remove_const_t<decltype(numItems)> i = 0; i < numItems; ++i)
	{
		const auto size = archive.itemProperty(i, bit7z::BitProperty::Size).getUInt64();

		auto time = [&]() -> QDateTime {
			auto fileTime = GetTime(archive, i, bit7z::BitProperty::CTime);
			if (!fileTime)
			{
				fileTime = GetTime(archive, i, bit7z::BitProperty::MTime);
				if (!fileTime)
				{
					fileTime = GetTime(archive, i, bit7z::BitProperty::ATime);
					if (!fileTime)
						return {};
				}
			}

			return QDateTime::fromSecsSinceEpoch(*fileTime);
		}();

		auto fileName = QDir::fromNativeSeparators(FixFileName(QString::fromBit7zString(archive.itemProperty(i, bit7z::BitProperty::Path).getString())));

		if (const auto it = result.index.try_emplace(fileName, result.files.size()); it.second)
			result.files.emplace_back(i, std::move(fileName), size, std::move(time), archive.isItemFolder(i));
		else
			PLOGW << "something strange: " << fileName << " duplicates";
	}

	return result;
}

class ZipImpl : public IZip
{
protected:
	explicit ZipImpl(std::shared_ptr<ProgressCallback> progress)
		: m_progress { std::move(progress) }
	{
	}

private: // IZip
	void SetProperty(const PropertyId /*id*/, QVariant /*value*/) override
	{
		assert(false && "Cannot set properties with reader");
		throw std::runtime_error("Cannot set properties with reader");
	}

	QStringList GetFileNameList() const override
	{
		return m_files.files | std::views::filter([](const auto& item) {
				   return !item.isDir;
			   })
		     | std::views::transform([](const auto& item) {
				   return item.name;
			   })
		     | std::ranges::to<QStringList>();
	}

	size_t GetFileSize(const QString& filename) const override
	{
		return m_files.GetFile(filename).size;
	}

	const QDateTime& GetFileTime(const QString& filename) const override
	{
		return m_files.GetFile(filename).time;
	}

	size_t GetFileIndex(const QString& filename) const override
	{
		if (const auto it = m_files.index.find(filename); it != m_files.index.end())
			return it->second;

		return INVALID_INDEX;
	}

	std::unique_ptr<IFile> Read(const QString& /*filename*/) const override
	{
		assert(false && "Cannot read with writer");
		throw std::runtime_error("Cannot read with writer");
	}

	std::unordered_map<QString, QByteArray> ReadAll() const override
	{
		assert(false && "Cannot read with writer");
		throw std::runtime_error("Cannot read with writer");
	}

	bool Write(const IZipFileProvider& /*zipFileProvider*/) override
	{
		assert(false && "Cannot write with reader");
		throw std::runtime_error("Cannot write with reader");
	}

	bool Remove(const std::vector<QString>& /*fileNames*/) override
	{
		assert(false && "Cannot remove with reader");
		throw std::runtime_error("Cannot remove with reader");
	}

protected:
	bit7z::Bit7zLibrary               m_lib;
	std::shared_ptr<ProgressCallback> m_progress;
	FileStorage                       m_files;
};

class Reader : public ZipImpl
{
protected:
	explicit Reader(std::shared_ptr<ProgressCallback> progress)
		: ZipImpl(std::move(progress))
	{
	}

	void SetCallbacks() const
	{
		m_archive->setTotalCallback([this](const int64_t value) {
			m_progress->OnStartWithTotal(value);
		});

		m_archive->setProgressCallback([this, total = int64_t { 0 }](const int64_t value) mutable {
			m_progress->OnIncrement(value - total);
			total = value;
			return !m_progress->OnCheckBreak();
		});

		m_archive->setFileCallback([this](const auto& file) {
			m_progress->OnFileDone(QString::fromBit7zString(file));
		});
	}

private: // IZip
	std::unique_ptr<IFile> Read(const QString& filename) const override
	{
		return File::Read(*m_ctx, m_files.GetFile(filename));
	}

	std::unordered_map<QString, QByteArray> ReadAll() const override
	{
#ifdef ADDITIONAL_LOG_ENABLED
		PLOGV << "extracting...";
#endif
		std::map<bit7z::tstring, std::vector<bit7z::byte_t>> output;
		m_archive->extractTo(output);
		return output | std::views::transform([](auto& item) {
				   return std::make_pair(
					   QDir::fromNativeSeparators(QString::fromBit7zString(item.first)),
					   QByteArray { reinterpret_cast<const char*>(item.second.data()), static_cast<qsizetype_t>(item.second.size()) }
				   );
			   })
		     | std::ranges::to<std::unordered_map>();
	}

	std::expected<void, QString> Test() const noexcept override
	try
	{
		m_archive->test();
		return {};
	}
	catch (const bit7z::BitException& ex)
	{
		return std::unexpected(QString { ex.what() });
	}
	catch (...)
	{
		return std::unexpected(QString { "unknown archive test error" });
	}

protected:
	std::unique_ptr<bit7z::BitArchiveReader> m_archive;
	std::unique_ptr<Context>                 m_ctx;
};

class ReaderFile : public Reader
{
public:
	ReaderFile(const QString& filename, std::shared_ptr<ProgressCallback> progress)
		: Reader(std::move(progress))
		, m_stream { CreateStream(filename) }
	{
		m_archive = std::make_unique<bit7z::BitArchiveReader>(m_lib, m_stream);
		m_ctx     = std::make_unique<Context>(*m_archive);
		m_files   = CreateFileList(*m_archive);
		SetCallbacks();
	}

private:
	static std::ifstream CreateStream(const QString& filename)
	{
		if (!QFile::exists(filename))
			Error::CannotOpenArchive(filename);

		return std::ifstream { Platform::StringToPath(filename), std::ios_base::in | std::ios::binary };
	}

private:
	std::ifstream m_stream;
};

class ReaderStream : public Reader
{
public:
	ReaderStream(QIODevice& stream, std::shared_ptr<ProgressCallback> progress)
		: Reader(std::move(progress))
	{
		if (stream.isReadable())
		{
			m_bytes.resize(stream.size());
			stream.read(reinterpret_cast<char*>(m_bytes.data()), static_cast<qint64>(m_bytes.size()));
		}
		m_archive = std::make_unique<bit7z::BitArchiveReader>(m_lib, m_bytes);
		m_files   = CreateFileList(*m_archive);
		SetCallbacks();
	}

protected:
	std::vector<bit7z::byte_t> m_bytes;
};

class Writer : public ZipImpl
{
protected:
	explicit Writer(std::shared_ptr<ProgressCallback> progress)
		: ZipImpl(std::move(progress))
	{
	}

	void SetCallbacks() const
	{
		m_archive->setTotalCallback([this](const int64_t value) {
			m_progress->OnStartWithTotal(value);
		});

		m_archive->setProgressCallback([this, total = int64_t { 0 }](const int64_t value) mutable {
			m_progress->OnIncrement(value - total);
			total = value;
			return !m_progress->OnCheckBreak();
		});

		m_archive->setFileCallback([this](const auto& file) {
			m_progress->OnFileDone(QString::fromBit7zString(file));
		});
	}

private: // IZip
	void SetProperty(const PropertyId id, QVariant value) override
	{
		m_properties[id] = std::move(value);
	}

protected:
	void AddFiles(const IZipFileProvider& zipFileProvider) const
	{
		SetArchiveProperties();

		for (size_t i = 0, sz = zipFileProvider.GetCount(); i < sz; ++i)
		{
			auto& item = m_archive->addFile(zipFileProvider.GetFileData(i), zipFileProvider.GetFileName(i).toBit7zString());
			SetTime(item, zipFileProvider.GetFileTime(i));
		}
	}

	void UpdateFileList()
	{
		if (const auto* inputArchive = m_archive->toInputArchive())
			m_files = CreateFileList(*inputArchive);
	}

	std::expected<void, QString> Test() const noexcept override
	{
		assert(false && "cannot test writable archive");
		return {};
	}

	template <typename Impl, typename... ARGS>
	[[nodiscard]] auto CreateArchiveImpl(ARGS&&... args) const
	{
		return std::make_unique<Impl>(m_lib, std::forward<ARGS>(args)...);
	}

private:
	void SetArchiveProperties() const
	{
		const auto& format = m_archive->compressionFormat();

		const auto getProperty = [&](const PropertyId id) {
			const auto it = m_properties.find(id);
			return it != m_properties.end() ? it->second : QVariant {};
		};

		if (const auto value = getProperty(PropertyId::CompressionLevel); value.isValid() && format.hasFeature(bit7z::FormatFeatures::CompressionLevel))
			m_archive->setCompressionLevel(static_cast<bit7z::BitCompressionLevel>(static_cast<uint32_t>(value.value<CompressionLevel>())));

		if (const auto value = getProperty(PropertyId::CompressionMethod); value.isValid() && format.hasFeature(bit7z::FormatFeatures::MultipleMethods))
			m_archive->setCompressionMethod(static_cast<bit7z::BitCompressionMethod>(static_cast<uint32_t>(value.value<CompressionMethod>())));

		if (const auto value = getProperty(PropertyId::SolidArchive); value.isValid() && format.hasFeature(bit7z::FormatFeatures::SolidArchive))
			m_archive->setSolidMode(value.toBool());

		if (const auto value = getProperty(PropertyId::ThreadsCount); value.isValid())
			m_archive->setThreadsCount(value.toUInt());
	}

protected:
	std::unique_ptr<bit7z::BitArchiveWriter> m_archive;

private:
	std::unordered_map<PropertyId, QVariant> m_properties {
		{ PropertyId::CompressionLevel,               QVariant::fromValue(CompressionLevel::Ultra) },
		{     PropertyId::ThreadsCount, static_cast<uint32_t>(std::thread::hardware_concurrency()) },
	};
};

class WriterFile final : public Writer
{
public:
	WriterFile(QString filename, const Format format, std::shared_ptr<ProgressCallback> progress, const bool appendMode)
		: Writer(std::move(progress))
		, m_filename { std::move(filename) }
	{
		m_archive = CreateArchive(format, appendMode);
		SetCallbacks();
		UpdateFileList();
	}

private: // IZip
	bool Write(const IZipFileProvider& zipFileProvider) override
	{
		AddFiles(zipFileProvider);
		m_archive->compressTo(m_filename.toBit7zString());
		UpdateFileList();
		m_progress->OnDone();

		return true;
	}

	bool Remove(const std::vector<QString>& fileNames) override
	{
		auto* editor = m_archive->toEditor();
		if (!editor)
			throw std::runtime_error("Cannot remove with writer");

		const auto sorted = m_files.files | std::views::transform([](const auto& item) {
								auto list = item.name.split('/');
								return list.size() < 2 ? std::make_pair(item.name, item.name) : std::make_pair(list.front() + '/', item.name);
							})
		                  | std::ranges::to<std::multimap>();

		size_t count = 0;
		for (const auto& fileName : fileNames)
		{
			for (auto [it, end] = sorted.equal_range(fileName); it != end; ++it)
			{
				const auto& file = m_files.GetFile(it->second);
				editor->deleteItem(file.index, bit7z::DeletePolicy::RecurseDirs);
				++count;
			}
		}

		editor->applyChanges();
		UpdateFileList();

		PLOGI << m_filename << ", files removed: " << count;

		return true;
	}

private:
	[[nodiscard]] std::unique_ptr<bit7z::BitArchiveWriter> CreateArchive(const Format format, const bool appendMode) const
	{
		if (!appendMode && QFile::exists(m_filename))
			QFile::remove(m_filename);

		if (!QFile::exists(m_filename))
			return CreateArchiveImpl<bit7z::BitArchiveWriter>(m_filename.toBit7zString(), GetInOutFormat(format));

		static constexpr std::array outFormats {
			&bit7z::BitFormat::Zip, &bit7z::BitFormat::BZip2, &bit7z::BitFormat::SevenZip, &bit7z::BitFormat::Xz, &bit7z::BitFormat::Wim, &bit7z::BitFormat::Tar, &bit7z::BitFormat::GZip,
		};

		const bit7z::BitArchiveReader reader(m_lib, m_filename.toBit7zString());
		const auto                    it = std::ranges::find(outFormats, reader.detectedFormat().value(), [](const auto* item) {
			return item->value();
		});
		assert(it != std::end(outFormats));
		if (it == std::end(outFormats))
			throw std::runtime_error(std::format("Unsupported file format for editing: ", m_filename));

		return CreateArchiveImpl<bit7z::BitArchiveEditor>(m_filename.toBit7zString(), **it);
	}

private:
	const QString m_filename;
};

class WriterStream final : public Writer
{
public:
	WriterStream(QIODevice& stream, const Format format, std::shared_ptr<ProgressCallback> progress)
		: Writer(std::move(progress))
		, m_stream { stream }
	{
		m_archive = std::make_unique<bit7z::BitArchiveWriter>(m_lib, std::vector<std::byte> {}, GetInOutFormat(format));
		SetCallbacks();
	}

private: // IZip
	bool Write(const IZipFileProvider& zipFileProvider) override
	{
		AddFiles(zipFileProvider);
		const auto stream = QStdOStream::create(m_stream);
		m_archive->compressTo(*stream);
		UpdateFileList();
		m_progress->OnDone();

		return true;
	}

private:
	QIODevice& m_stream;
};

} // namespace

std::unique_ptr<IZip> Archive::CreateReader(const QString& filename, std::shared_ptr<ProgressCallback> progress)
{
	return std::make_unique<ReaderFile>(filename, std::move(progress));
}

std::unique_ptr<IZip> Archive::CreateReaderStream(QIODevice& stream, std::shared_ptr<ProgressCallback> progress)
{
	return std::make_unique<ReaderStream>(stream, std::move(progress));
}

std::unique_ptr<IZip> Archive::CreateWriter(const QString& filename, const Format format, std::shared_ptr<ProgressCallback> progress, bool appendMode)
{
	return std::make_unique<WriterFile>(filename, format, std::move(progress), appendMode);
}

std::unique_ptr<IZip> Archive::CreateWriterStream(QIODevice& stream, Format format, std::shared_ptr<ProgressCallback> progress)
{
	return std::make_unique<WriterStream>(stream, format, std::move(progress));
}

bool Archive::IsArchive(const QString& filename)
{
	return std::ranges::any_of(ARCHIVE_EXTENSIONS, [arg = filename.toLower()](const char* item) {
		return arg.endsWith(item);
	});
}

QStringList Archive::GetTypes()
{
	return ARCHIVE_EXTENSIONS | std::views::transform([](const char* item) {
			   return QString(item);
		   })
	     | std::ranges::to<QStringList>();
}

} // namespace HomeCompa::ZipDetails::SevenZip
