#include "zip.h"

#include <ranges>

#include <QBuffer>
#include <QFile>
#include <QTemporaryDir>
#include <QVariant>

#include <gtest/gtest.h>

#include "mock/MockProgressCallback.h"

using namespace HomeCompa::Test;
using namespace HomeCompa;

namespace {

namespace Create {

TEST(ZipTest, Types)
{
	EXPECT_FALSE(Zip::GetTypes().isEmpty());
	EXPECT_TRUE(Zip::GetTypes().contains("zip"));
	EXPECT_TRUE(Zip::GetTypes().contains("7z"));
}

TEST(ZipTest, Ctors)
{
	auto create = [] {
		QFile stream(":/dst/archive.7z");
		std::ignore = stream.open(QIODevice::ReadOnly);

		auto progress = std::make_shared<testing::StrictMock<MockProgressCallback>>();
		EXPECT_CALL(*progress, OnCheckBreak).WillRepeatedly(testing::Return(false));
		EXPECT_CALL(*progress, OnStartWithTotal(24));
		EXPECT_CALL(*progress, OnIncrement(0));
		EXPECT_CALL(*progress, OnIncrement(24));
		return Zip(stream, std::move(progress));
	};

	auto zip1 = create();
	auto zip2(std::move(zip1));
	auto zip3 = create();
	EXPECT_TRUE(zip3.Test());
	zip3 = std::move(zip2);
	EXPECT_TRUE(zip3.Test());
}

} // namespace Create

namespace Fail {

TEST(ZipTest, CannotOpen)
{
	static constexpr auto badName = "bad:file\nname";
	EXPECT_FALSE(Zip::IsArchive(badName));
	EXPECT_THROW(Zip(QString(badName)), std::runtime_error);
}

TEST(ZipTest, CannotFindFileInArchive)
{
	QFile stream(":/dst/archive.7z");
	std::ignore = stream.open(QIODevice::ReadOnly);
	const Zip zip(stream);
	EXPECT_EQ(zip.GetFileIndex("42.txt"), Zip::INVALID_INDEX);
	const auto test = [&] {
		std::ignore = zip.Read("random_file_name.dat");
	};
	EXPECT_THROW(test(), std::runtime_error);
}

TEST(ZipTest, WriteReadable)
{
	QFile stream(":/dst/archive.7z");
	std::ignore = stream.open(QIODevice::ReadOnly);
	Zip        zip(stream);
	const auto test = [&] {
		const auto files = Zip::CreateZipFileController();
		std::ignore      = zip.Write(*files);
	};
#ifdef NDEBUG
	EXPECT_THROW(test(), std::runtime_error);
#else
	EXPECT_DEATH(test(), "");
#endif
}

TEST(ZipTest, RemoveReadable)
{
	QFile stream(":/dst/archive.7z");
	std::ignore = stream.open(QIODevice::ReadOnly);
	Zip        zip(stream);
	const auto test = [&] {
		std::ignore = zip.Remove({ "42.txt" });
	};
#ifdef NDEBUG
	EXPECT_THROW(test(), std::runtime_error);
#else
	EXPECT_DEATH(test(), "");
#endif
}

TEST(ZipTest, SetPropertyReadable)
{
	QFile stream(":/dst/archive.7z");
	std::ignore = stream.open(QIODevice::ReadOnly);
	Zip        zip(stream);
	const auto test = [&] {
		zip.SetProperty(Zip::PropertyId::CompressionLevel, {});
	};
#ifdef NDEBUG
	EXPECT_THROW(test(), std::runtime_error);
#else
	EXPECT_DEATH(test(), "");
#endif
}

TEST(ZipTest, WriteStreamReadable)
{
	QFile stream(":/dst/archive.7z");
	std::ignore = stream.open(QIODevice::ReadOnly);
	const Zip  zip(stream);
	const auto streamWrapper = zip.Read(zip.GetFileNameList().front());
	EXPECT_EQ(streamWrapper->GetStream().write("125"), -1);
}

TEST(ZipTest, ReadEmptyStream)
{
	QBuffer stream;
	stream.open(QIODevice::ReadOnly);
	const auto test = [&] {
		[[maybe_unused]] const Zip zip(stream);
	};
	EXPECT_THROW(test(), std::exception);
}

TEST(ZipTest, TestWritable)
{
	QBuffer stream;
	stream.open(QIODevice::WriteOnly);
	const auto test = [&] {
		const Zip zip(stream, Zip::Format::Zip);
		std::ignore = zip.Test();
	};
	EXPECT_DEBUG_DEATH(test(), "");
}

TEST(ZipTest, ReadWritable)
{
	QBuffer stream;
	stream.open(QIODevice::WriteOnly);
	const auto test = [&] {
		const Zip zip(stream, Zip::Format::Zip);
		std::ignore = zip.Read("42.txt");
	};
#ifdef NDEBUG
	EXPECT_THROW(test(), std::runtime_error);
#else
	EXPECT_DEATH(test(), "");
#endif
}

TEST(ZipTest, ReadAllWritable)
{
	QBuffer stream;
	stream.open(QIODevice::WriteOnly);
	const auto test = [&] {
		const Zip zip(stream, Zip::Format::Zip);
		std::ignore = zip.ReadAll();
	};
#ifdef NDEBUG
	EXPECT_THROW(test(), std::runtime_error);
#else
	EXPECT_DEATH(test(), "");
#endif
}

} // namespace Fail

namespace ZipFormat {

struct ZipFormatTest : testing::TestWithParam<std::pair<Zip::Format, const char*>>
{
};

INSTANTIATE_TEST_SUITE_P(ZipFormatTests, ZipFormatTest, testing::Values(std::make_pair(Zip::Format::SevenZip, "7z"), std::make_pair(Zip::Format::Zip, "zip")));

TEST_P(ZipFormatTest, FormatToStream)
{
	const auto& [format, name] = GetParam();
	std::ostringstream stream;
	stream << format;
	EXPECT_STREQ(stream.str().data(), name);
}

TEST_P(ZipFormatTest, FormatToString)
{
	const auto& [format, name] = GetParam();
	const auto result          = Zip::FormatToString(format).toStdString();
	EXPECT_STREQ(result.data(), name);
}

TEST_P(ZipFormatTest, FormatFromString)
{
	const auto& [format, name] = GetParam();
	const auto result          = Zip::FormatFromString(name);
	EXPECT_EQ(result, format);
}

} // namespace ZipFormat

namespace Decode {

void TestDecode(const Zip& zip, const QByteArray& input)
{
	ASSERT_TRUE(zip.Test());
	const auto files = zip.GetFileNameList();
	ASSERT_EQ(files.size(), 1);
	const auto file1   = files.front().toStdString();
	const auto fileStr = file1.data();
	ASSERT_STREQ(fileStr, "input.txt");
	EXPECT_EQ(zip.Read(files.front())->GetStream().readAll(), input);
	EXPECT_EQ(zip.GetFileSize(files.front()), input.size());
	EXPECT_TRUE(zip.GetFileTime(files.front()).isValid());
	EXPECT_EQ(zip.GetFileIndex(files.front()), 0);

	const auto all = zip.ReadAll();
	ASSERT_FALSE(all.empty());
	const auto& [name, bytes] = *all.cbegin();
	ASSERT_STREQ(name.toStdString().data(), "input.txt");
	EXPECT_EQ(bytes, input);
}

namespace DecodeMemory {

struct DecodeMemoryTest : testing::TestWithParam<QString>
{
};

INSTANTIATE_TEST_SUITE_P(DecodeMemoryTests, DecodeMemoryTest, testing::Values("archive.7z", "archive.gz", "archive.tar", "archive.wim", "archive.zip"));

TEST_P(DecodeMemoryTest, Read)
{
	const auto input = [] {
		QFile src(":/src/input.txt");
		return src.open(QIODevice::ReadOnly) ? src.readAll() : QByteArray {};
	}();
	ASSERT_FALSE(input.isEmpty());

	const auto test = [&] {
		const auto file = GetParam();
		QFile      stream(QString(":/dst/%1").arg(file));
		std::ignore = stream.open(QIODevice::ReadOnly);
		const Zip zip(stream);
		TestDecode(zip, input);
	};
	EXPECT_NO_THROW(test());
}

} // namespace DecodeMemory

namespace DecodeFile {

struct DecodeFileTest : testing::TestWithParam<QString>
{
};

INSTANTIATE_TEST_SUITE_P(DecodeFileTestTests, DecodeFileTest, testing::Values("archive.7z", "archive.gz", "archive.tar", "archive.wim", "archive.zip"));

TEST_P(DecodeFileTest, Read)
{
	const auto input = [] {
		QFile src(":/src/input.txt");
		return src.open(QIODevice::ReadOnly) ? src.readAll() : QByteArray {};
	}();
	ASSERT_FALSE(input.isEmpty());

	const QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	auto test = [&] {
		const auto file     = GetParam();
		const auto filePath = dir.filePath(file);
		{
			QFile streamIn(QString(":/dst/%1").arg(file));
			std::ignore = streamIn.open(QIODevice::ReadOnly);
			QFile streamOut(filePath);
			ASSERT_TRUE(streamOut.open(QIODevice::WriteOnly));
			streamOut.write(streamIn.readAll());
		}

		EXPECT_TRUE(Zip::IsArchive(filePath));
		const Zip zip(filePath);
		TestDecode(zip, input);
	};
	EXPECT_NO_THROW(test());
}

} // namespace DecodeFile

} // namespace Decode

namespace Encode {

struct ZipTest : testing::TestWithParam<Zip::Format>
{
};

struct ZipCreateTest : ZipTest
{
};

INSTANTIATE_TEST_SUITE_P(ZipCreateTests, ZipCreateTest, testing::Values(Zip::Format::Zip, Zip::Format::SevenZip, Zip::Format::BZip2, Zip::Format::GZip, Zip::Format::Tar, Zip::Format::Xz));

TEST_P(ZipCreateTest, JustCreateMemory)
{
	const auto format = GetParam();
	QBuffer    buffer;
	buffer.open(QIODevice::WriteOnly);
	const auto test = [&] {
		[[maybe_unused]] const Zip zip(buffer, format);
	};
	EXPECT_NO_THROW(test());
}

struct ZipMainFormatTest : ZipTest
{
};

INSTANTIATE_TEST_SUITE_P(ZipMainFormatTests, ZipMainFormatTest, testing::Values(Zip::Format::Zip, Zip::Format::SevenZip));

void Process(const Zip::Format format, const std::vector<std::pair<QString, QByteArray>>& files = {})
{
	QByteArray data;
	{
		QBuffer buffer(&data);
		buffer.open(QIODevice::WriteOnly);
		Zip        zip(buffer, format);
		const auto zipFiles = Zip::CreateZipFileController();
		for (const auto& [name, body] : files)
			zipFiles->AddFile(name, body);
		ASSERT_NO_THROW(zip.Write(*zipFiles));
	}

	QBuffer buffer(&data);
	buffer.open(QIODevice::ReadOnly);
	Zip        zip(buffer);
	const auto unzippedFileNames = zip.GetFileNameList();
	EXPECT_EQ(unzippedFileNames.size(), files.size());
	for (const auto& [file, unzippedFileName] : std::views::zip(files, unzippedFileNames))
	{
		EXPECT_EQ(file.first, unzippedFileName);
		EXPECT_EQ(file.second, zip.Read(unzippedFileName)->GetStream().readAll());
	}
}

TEST_P(ZipMainFormatTest, ReadWrite)
{
	const auto format = GetParam();
	Process(format);
}

TEST_P(ZipMainFormatTest, ReadWriteFiles)
{
	const auto format = GetParam();

	const QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const auto filePath = dir.filePath(QString::number(static_cast<int>(format)));
	std::ignore         = QFile(filePath).open(QIODevice::WriteOnly);
	const auto encode   = [&] {
		Zip        zip(filePath, format);
		const auto zipFiles = Zip::CreateZipFileController();
		for (int i = 0; i < 42; ++i)
			zipFiles->AddFile(QString::number(i), QByteArray(i, i + '0'));
		const auto fileToAdd = dir.filePath("42");
		{
			QFile file(fileToAdd);
			ASSERT_TRUE(file.open(QIODevice::WriteOnly));
			file.write("12345");
		}
		zipFiles->AddFile(fileToAdd);
		EXPECT_NO_THROW(zip.Write(*zipFiles));
	};
	EXPECT_NO_THROW(encode());

#ifdef NDEBUG
	{
		Zip zip(filePath);
		EXPECT_THROW(zip.Remove({ "17", "31" }), std::exception);
	}
#endif
	{
		Zip zip(filePath, format, true);
		EXPECT_FALSE(zip.Remove({ "-1" }));
	}
	{
		Zip zip(filePath, format, true);
		EXPECT_FALSE(zip.Remove({ "100500" }));
	}
	{
		Zip zip(filePath, format, true);
		EXPECT_FALSE(zip.Remove({ "abc.txt" }));
	}
	{
		Zip zip(filePath, format, true);
		EXPECT_FALSE(zip.Remove({ "" }));
	}
	{
		Zip zip(filePath, format, true);
		EXPECT_TRUE(zip.Remove({ "17", "31" }));
	}
	{
		Zip zip(filePath);
		EXPECT_EQ(zip.GetFileNameList().size(), 41);
	}
}

TEST_P(ZipMainFormatTest, ReadWriteBigFile)
{
	const auto format = GetParam();

	QByteArray data;
	{
		QBuffer buffer(&data);
		buffer.open(QIODevice::WriteOnly);

		auto progress = std::make_shared<testing::StrictMock<MockProgressCallback>>();
		EXPECT_CALL(*progress, OnStartWithTotal);
		EXPECT_CALL(*progress, OnCheckBreak).WillRepeatedly(testing::Return(false));
		EXPECT_CALL(*progress, OnIncrement).Times(testing::AnyNumber());
		EXPECT_CALL(*progress, OnFileDone(QString { "42.dat" }));
		EXPECT_CALL(*progress, OnDone);

		Zip        zip(buffer, format, std::move(progress));
		const auto zipFiles = Zip::CreateZipFileController();
		QByteArray bytes(17 * 1024 * 1024, 42);
		std::ranges::fill_n(bytes.begin(), 42000, 13);
		zipFiles->AddFile("42.dat", bytes);
		ASSERT_NO_THROW(zip.Write(*zipFiles));
	}

	QBuffer buffer(&data);
	buffer.open(QIODevice::ReadOnly);
	Zip        zip(buffer);
	const auto streamWrapper = zip.Read("42.dat");
	auto&      stream        = streamWrapper->GetStream();
	{
		const auto received = stream.read(42000);
		EXPECT_EQ(received.size(), 42000);
		EXPECT_TRUE(std::ranges::all_of(received, [](const auto item) {
			return item == 13;
		}));
		EXPECT_EQ(stream.pos(), 42000);
	}
	{
		const auto received = stream.read(42000);
		EXPECT_EQ(received.size(), 42000);
		EXPECT_TRUE(std::ranges::all_of(received, [](const auto item) {
			return item == 42;
		}));
		EXPECT_EQ(stream.pos(), 84000);
	}
	{
		EXPECT_TRUE(stream.seek(21000));
		EXPECT_EQ(stream.pos(), 21000);
		const auto received = stream.read(21000);
		EXPECT_EQ(received.size(), 21000);
		EXPECT_TRUE(std::ranges::all_of(received, [](const auto item) {
			return item == 13;
		}));
		EXPECT_EQ(stream.pos(), 42000);
	}
	{
		const auto received = stream.read(84000);
		EXPECT_EQ(received.size(), 84000);
		EXPECT_TRUE(std::ranges::all_of(received, [](const auto item) {
			return item == 42;
		}));
		EXPECT_EQ(stream.pos(), 126000);
	}

	const auto tail = stream.readAll();
	EXPECT_EQ(tail.size(), 17 * 1024 * 1024 - 126000);
	const auto position = stream.pos();
	EXPECT_EQ(stream.size(), position);
	EXPECT_FALSE(stream.seek(17 * 1024 * 1024 + 1));
	EXPECT_EQ(stream.pos(), position);
	EXPECT_TRUE(stream.seek(stream.size()));
	EXPECT_EQ(stream.pos(), 17 * 1024 * 1024);
}

struct ZipSingleFileTest : ZipTest
{
};

INSTANTIATE_TEST_SUITE_P(ZipSingleFileTests, ZipSingleFileTest, testing::Values(Zip::Format::Zip, Zip::Format::SevenZip, Zip::Format::GZip));

TEST_P(ZipSingleFileTest, ReadWriteSingle)
{
	const auto format = GetParam();
	Process(
		format,
		{
			{ "12345qwert6", "data.dat" }
    }
	);
}

} // namespace Encode

} // namespace
