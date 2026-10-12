#include "ImageRestore.h"

#include <expected>

#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QImage>

#include "fnd/EnumBitmask.h"
#include "fnd/IsOneOf.h"
#include "fnd/try.h"

#include "settings/ISettings.h"
#include "xml/SaxParser.h"
#include "xml/XmlAttributes.h"
#include "xml/XmlWriter.h"

#include "BookUtil.h"
#include "Constant.h"
#include "EpubParser.h"
#include "ImageUtil.h"
#include "QtTypes.h"
#include "zip.h"

#include "config/version.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace {

enum class ImageProcessing
{
	None             = 0,
	RemoveCovers     = 1 << 0,
	RemoveImages     = 1 << 1,
	GrayscaleCovers  = 1 << 2,
	GrayscaleImages  = 1 << 3,
	ConvertToJpegPng = 1 << 4,
};

}

ENABLE_BITMASK_OPERATORS(ImageProcessing);

namespace {

using Covers = std::unordered_map<QString, std::pair<bool, QByteArray>>;

constexpr auto BINARY       = u"binary";
constexpr auto ID           = u"id";
constexpr auto CONTENT_TYPE = u"content-type";

void ConvertToGrayscale(QImage& image)
{
	if (image.pixelFormat().alphaUsage() == QPixelFormat::AlphaUsage::IgnoresAlpha)
		return image.convertTo(QImage::Format::Format_Grayscale8);

	QImage alpha(image.width(), image.height(), QImage::Format::Format_Grayscale8);

	for (auto h = 0, H = image.height(); h < H; ++h)
	{
		auto* alphaBytes = alpha.scanLine(h);
		for (auto w = 0, W = image.width(); w < W; ++w, ++alphaBytes)
			*alphaBytes = static_cast<uchar>(qAlpha(image.pixel(w, h)));
	}

	image.convertTo(QImage::Format::Format_Grayscale8);
	image.setAlphaChannel(alpha);
}

std::pair<QByteArray, const char*> RecodeImage(const bool isCover, const ImageProcessing imageProcessing, const QByteArray& body, const QString& format = {})
{
	if (body.isEmpty() || (isCover && !!(imageProcessing & ImageProcessing::RemoveCovers)) || (!isCover && !!(imageProcessing & ImageProcessing::RemoveImages)))
		return {};

	auto image = Decode(body);
	if (image.isNull())
		return {};

	if ((isCover && !!(imageProcessing & ImageProcessing::GrayscaleCovers)) || (!isCover && !!(imageProcessing & ImageProcessing::GrayscaleImages)))
		ConvertToGrayscale(image);

	return Encode(image, format);
}

class BinaryParser final : public SaxParser
{
	static constexpr auto COVERPAGE_IMAGE = u"FictionBook/description/title-info/coverpage/image";

public:
	BinaryParser(QIODevice& input, Covers& covers, const ImageProcessing imageProcessing)
		: SaxParser { input }
		, m_imageProcessing { imageProcessing }
		, m_covers { covers }
	{
		Parse();
		input.seek(0);
	}

private:
	bool OnStartElement(const QStringView name, const QStringView path, const XmlAttributes& attributes) override
	{
		if (name == BINARY)
			return (m_id = attributes.GetAttribute(ID).toString()), true;

		if (path == COVERPAGE_IMAGE)
		{
			for (size_t i = 0, sz = attributes.GetCount(); i < sz; ++i)
			{
				auto attributeName  = attributes.GetName(i);
				auto attributeValue = attributes.GetValue(i);
				if (attributeName.endsWith(u":href"))
				{
					if (const auto it = std::ranges::find_if(
							attributeValue,
							[](const auto ch) {
								return ch != '#';
							}
						);
					    it != attributeValue.end())
						m_coverId = Last(attributeValue, std::distance(it, attributeValue.end())).toString().trimmed();
					break;
				}
			}
			return true;
		}

		return true;
	}

	bool OnCharacters(QStringView, const QStringView value) override
	{
		if (m_id.isEmpty())
			return true;

		auto       bytes   = QByteArray::fromBase64(value.toUtf8());
		const auto isCover = m_id == m_coverId;
		if ((isCover && !(m_imageProcessing & ImageProcessing::RemoveCovers)) || (!isCover && !(m_imageProcessing & ImageProcessing::RemoveImages)))
			m_covers.emplace(std::move(m_id), std::make_pair(isCover, std::move(bytes)));
		m_id = QString {};

		return true;
	}

private:
	const ImageProcessing m_imageProcessing;

	Covers& m_covers;
	QString m_coverId;
	QString m_id;
};

class SaxPrinter final : public SaxParser
{
	static constexpr auto FICTION_BOOK       = u"FictionBook";
	static constexpr auto DESCRIPTION        = u"FictionBook/description";
	static constexpr auto DOCUMENT_INFO      = u"FictionBook/description/document-info";
	static constexpr auto PROGRAM_USED       = u"FictionBook/description/document-info/program-used";
	static constexpr auto TITLE_INFO         = u"FictionBook/description/title-info";
	static constexpr auto AUTHOR             = u"FictionBook/description/title-info/author";
	static constexpr auto AUTHOR_FIRST_NAME  = u"first-name";
	static constexpr auto AUTHOR_LAST_NAME   = u"last-name";
	static constexpr auto AUTHOR_MIDDLE_NAME = u"middle-name";
	static constexpr auto BOOK_TITLE         = u"FictionBook/description/title-info/book-title";
	static constexpr auto SEQUENCE           = u"FictionBook/description/title-info/sequence";

public:
	SaxPrinter(QIODevice& input, QIODevice& output, Covers covers, std::unique_ptr<const ExtractedBook> metadataReplacement, const ImageProcessing imageProcessing, QString encoding)
		: SaxParser { input }
		, m_outputStream { output }
		, m_metadataReplacement { std::move(metadataReplacement) }
		, m_imageProcessing { imageProcessing }
		, m_encoding { std::move(encoding) }
		, m_covers { std::move(covers) }
	{
		Parse();
		assert(m_hasProgramUsed);
	}

	bool HasError() const noexcept
	{
		return m_hasError;
	}

private: // Util::SaxParser
	bool OnXMLDecl(QStringView /*versionStr*/, const QStringView encodingStr, QStringView /*standaloneStr*/, QStringView /*actualEncodingStr*/) override
	{
		const auto encoding = m_encoding.isEmpty() ? encodingStr.toUtf8() : m_encoding.toUtf8();
		m_writer            = std::make_unique<XmlWriter>(m_outputStream, Util::details::XmlWriterOptions { .encoding = encoding.constData() });
		return true;
	}

	bool OnProcessingInstruction(const QStringView target, const QStringView data) override
	{
		return m_writer->WriteProcessingInstruction(target, data), true;
	}

	bool OnStartElement(const QStringView name, const QStringView path, const XmlAttributes& attributes) override
	{
		if (m_specialNode || (m_metadataReplacement && IsOneOf(path, AUTHOR, BOOK_TITLE, SEQUENCE)))
			return m_specialNode = true;

		if (name == BINARY && !m_covers.empty())
			return WriteImage(attributes.GetAttribute(ID)), true;

		m_writer->WriteStartElement(name, attributes);

		if (path == TITLE_INFO && m_metadataReplacement)
			for (const auto invoker : {
					 &SaxPrinter::WriteAuthor,
					 &SaxPrinter::WriteTitle,
					 &SaxPrinter::WriteSeries,
				 })
				std::invoke(invoker, this);

		return true;
	}

	bool OnEndElement(const QStringView name, const QStringView path) override
	{
		if (m_specialNode)
		{
			if (name == BINARY)
				m_specialNode = false;

			if (IsOneOf(path, AUTHOR, BOOK_TITLE, SEQUENCE))
				m_specialNode = false;
			return true;
		}

		if (path == DOCUMENT_INFO && !m_hasProgramUsed)
		{
			m_writer->WriteStartElement(u"program-used").WriteCharacters(QString("%1 %2").arg(PRODUCT_ID, PRODUCT_VERSION)).WriteEndElement();
			m_hasProgramUsed = true;
		}

		if (path == DESCRIPTION)
		{
			if (!m_hasProgramUsed)
			{
				m_writer->WriteStartElement(u"document-info").WriteStartElement(u"program-used").WriteCharacters(QString("%1 %2").arg(PRODUCT_ID, PRODUCT_VERSION)).WriteEndElement().WriteEndElement();
				m_hasProgramUsed = true;
			}
		}

		if (path == FICTION_BOOK)
			WriteImages();

		return m_writer->WriteEndElement(), true;
	}

	bool OnCharacters(const QStringView path, const QStringView value) override
	{
		if (m_specialNode)
			return true;

		if (path != PROGRAM_USED)
			return m_writer->WriteCharacters(value), true;

		m_hasProgramUsed = true;
		return m_writer->WriteCharacters(QString("%1, %2 %3").arg(value, PRODUCT_ID, PRODUCT_VERSION)), true;
	}

	bool OnWarning(const size_t line, const size_t column, const QString& text) override
	{
		PLOGW << line << ":" << column << " " << text;
		return true;
	}

	bool OnError(const size_t line, const size_t column, const QString& text) override
	{
		m_hasError = true;
		PLOGE << line << ":" << column << " " << text;
		return false;
	}

	bool OnFatalError(const size_t line, const size_t column, const QString& text) override
	{
		return OnError(line, column, text);
	}

private:
	void WriteAuthor()
	{
		assert(m_metadataReplacement);
		const auto node = m_writer->Guard(u"author");
		node->WriteStartElement(AUTHOR_FIRST_NAME).WriteCharacters(m_metadataReplacement->authorFull.firstName).WriteEndElement();
		node->WriteStartElement(AUTHOR_MIDDLE_NAME).WriteCharacters(m_metadataReplacement->authorFull.middleName).WriteEndElement();
		node->WriteStartElement(AUTHOR_LAST_NAME).WriteCharacters(m_metadataReplacement->authorFull.lastName).WriteEndElement();
	}

	void WriteTitle()
	{
		assert(m_metadataReplacement);
		m_writer->WriteStartElement(u"book-title").WriteCharacters(m_metadataReplacement->title).WriteEndElement();
	}

	void WriteSeries()
	{
		assert(m_metadataReplacement);
		if (m_metadataReplacement->series.isEmpty())
			return;

		const auto node = m_writer->Guard(u"sequence");
		node->WriteAttribute(u"name", m_metadataReplacement->series);
		if (m_metadataReplacement->seqNumber > 0)
			node->WriteAttribute(u"number", QString::number(m_metadataReplacement->seqNumber));
	}

	void WriteImage(const QStringView imageFileName)
	{
		m_specialNode = true;

		const auto [isCover, body] = [&]() -> std::pair<bool, QByteArray> {
			try
			{
				const auto it = m_covers.find(imageFileName.toString());
				if (it == m_covers.end())
					return {};

				auto result = std::move(it->second);
				m_covers.erase(it);

				return result;
			}
			catch (const std::exception& ex)
			{
				PLOGE << ex.what();
			}

			return {};
		}();

		if (body.isNull())
		{
			PLOGW << imageFileName << " not found";
			return;
		}

		WriteImage(imageFileName, isCover, body);
	}

	void WriteImages()
	{
		for (const auto& [name, body] : m_covers)
			WriteImage(name, body.first, body.second);

		m_covers.clear();
	}

	void WriteImage(const QStringView name, const bool isCover, const QByteArray& body)
	{
		if (const auto [bytes, mediaType] = RecodeImage(isCover, m_imageProcessing, body); !bytes.isEmpty())
			m_writer->WriteStartElement(BINARY).WriteAttribute(ID, name).WriteAttribute(CONTENT_TYPE, QString(mediaType)).WriteCharacters(QString::fromUtf8(bytes.toBase64())).WriteEndElement();
	}

private:
	QIODevice&                           m_outputStream;
	std::unique_ptr<const ExtractedBook> m_metadataReplacement;
	const ImageProcessing                m_imageProcessing;
	const QString                        m_encoding;
	std::unique_ptr<XmlWriter>           m_writer;
	Covers                               m_covers;
	bool                                 m_hasError { false };
	bool                                 m_hasProgramUsed { false };
	bool                                 m_specialNode { false };
};

QByteArray PrepareToExport_stub(QIODevice& stream, Covers, std::unique_ptr<const ExtractedBook>, ImageProcessing, QString)
{
	return stream.readAll();
}

QByteArray PrepareToExport_fb2(QIODevice& stream, Covers covers, std::unique_ptr<const ExtractedBook> metadataReplacement, const ImageProcessing imageProcessing, QString encoding)
{
	if (covers.empty() && imageProcessing == ImageProcessing::None && !metadataReplacement && encoding.isEmpty())
		return stream.readAll();

	QByteArray byteArray;
	QBuffer    buffer(&byteArray);
	buffer.open(QIODevice::WriteOnly);
	const SaxPrinter saxPrinter(stream, buffer, std::move(covers), std::move(metadataReplacement), imageProcessing, std::move(encoding));
	return saxPrinter.HasError() ? QByteArray {} : byteArray;
}

QByteArray PrepareToExport_epub(QIODevice& stream, Covers covers, std::unique_ptr<const ExtractedBook> /*metadataReplacement*/, const ImageProcessing imageProcessing, QString)
{
	auto parseResult = EpubParser::Parse(stream, CommonParser::Mode::Images | CommonParser::Mode::Texts);

	const auto zipFiles = Zip::CreateZipFileController();

	if (const auto mimetypeIt = std::ranges::find(
			parseResult.texts,
			"mimetype",
			[](const auto& item) {
				return item.id;
			}
		);
	    mimetypeIt != parseResult.texts.end())
	{
		const auto [id, body] = *mimetypeIt;
		parseResult.texts.erase(mimetypeIt);
		zipFiles->AddFile(id, body);
	}

	auto addImage = [&](const QString& id, const QByteArray& body, const bool isCover) {
		if (const auto [bytes, _] = RecodeImage(isCover, imageProcessing, body, QFileInfo(id).suffix()); !bytes.isEmpty())
			zipFiles->AddFile(id, bytes);
	};

	if (!parseResult.imageIndex.isEmpty())
	{
		auto imageIndex = EpubParser::GetImageIndex(parseResult.imageIndex) | std::views::as_rvalue | std::views::transform([](auto&& item) {
							  return std::make_pair(item.second, std::move(item.first));
						  })
		                | std::ranges::to<std::unordered_map>();

		for (auto&& [id, cover] : covers)
		{
			auto&& [isCover, body] = cover;
			if (const auto name =
			        [&] {
						const auto it = imageIndex.find(isCover ? -1 : id.toInt());
						return it != imageIndex.end() ? it->second : QString {};
					}();
			    !name.isEmpty())
				addImage(name, body, isCover);
		}
	}

	for (auto&& [name, body] : parseResult.texts)
		zipFiles->AddFile(name, body);
	if (parseResult.coverExists)
		addImage(parseResult.images.front().id, parseResult.images.front().body, true);
	for (const auto& [id, body] : parseResult.images | std::views::as_rvalue | std::views::drop(parseResult.coverExists ? 1 : 0))
		addImage(id, body, false);

	QByteArray result;
	{
		QBuffer buffer(&result);
		buffer.open(QIODevice::WriteOnly);
		Zip output(buffer, Zip::Format::Zip);
		output.SetProperty(Zip::PropertyId::CompressionMethod, QVariant::fromValue(Zip::CompressionMethod::Deflate));
		output.SetProperty(Zip::PropertyId::CompressionLevel, QVariant::fromValue(Zip::CompressionLevel::Ultra));
		output.Write(*zipFiles);
	}

	return result;
}

using ExportPrepares = QByteArray (*)(QIODevice&, Covers, std::unique_ptr<const ExtractedBook>, ImageProcessing, QString encoding);

constexpr std::pair<const char*, ExportPrepares> EXPORT_PREPARERS[] {
#define ITEM(NAME) { "." #NAME, &PrepareToExport_##NAME }
	ITEM(fb2),
	ITEM(epub),
#undef ITEM
};

QByteArray PrepareToExportImpl(QIODevice& stream, const QString& folder, const QString& fileName, const ISettings& settings, QString encoding, std::unique_ptr<const ExtractedBook> metadataReplacement)
{
	Covers covers;
	ExtractBookImages(folder, fileName, settings, [&covers](QString name, const bool isCover, QByteArray body) {
		covers.try_emplace(std::move(name), std::make_pair(isCover, std::move(body)));
		return false;
	});

	const auto imageProcessing = (settings.Get(Export::REMOVE_COVER_KEY, false) ? ImageProcessing::RemoveCovers : ImageProcessing::None)
	                           | (settings.Get(Export::REMOVE_IMAGES_KEY, false) ? ImageProcessing::RemoveImages : ImageProcessing::None)
	                           | (settings.Get(Export::GRAYSCALE_COVER_KEY, false) ? ImageProcessing::GrayscaleCovers : ImageProcessing::None)
	                           | (settings.Get(Export::GRAYSCALE_IMAGES_KEY, false) ? ImageProcessing::GrayscaleImages : ImageProcessing::None)
	                           | (settings.Get(Export::CONVERT_IMAGES_KEY, false) ? ImageProcessing::ConvertToJpegPng : ImageProcessing::None);

	if (imageProcessing != ImageProcessing::None || !!metadataReplacement)
		BinaryParser(stream, covers, imageProcessing);

	const auto preparer = [&] {
		const auto it = std::ranges::find_if(EXPORT_PREPARERS, [&](const auto& item) {
			return fileName.endsWith(item.first, Qt::CaseInsensitive);
		});
		return it != std::end(EXPORT_PREPARERS) ? it->second : &PrepareToExport_stub;
	}();

	if (auto byteArray = std::invoke(preparer, stream, std::move(covers), std::move(metadataReplacement), imageProcessing, std::move(encoding)); !byteArray.isEmpty())
		return byteArray;

	stream.seek(0);
	return stream.readAll();
}

bool ParseCover(const QString& folder, const QString& fileName, const ExtractBookImagesCallback& callback, bool& stop)
{
	if (!QFile::exists(folder))
		return false;

	const Zip  zip(folder);
	const auto fileList = zip.GetFileNameList();
	const auto file     = QFileInfo(fileName).completeBaseName();
	if (!fileList.contains(file))
		return false;

	const auto stream = zip.Read(file);
	auto       body   = stream->GetStream().readAll();
	stop              = callback(Global::COVER, true, std::move(body));
	return true;
}

void ParseImages(const QString& folder, const QString& fileName, const ExtractBookImagesCallback& callback)
{
	if (!QFile::exists(folder))
		return;

	const Zip  zip(folder);
	auto       fileList   = zip.GetFileNameList();
	const auto filePrefix = QString("%1/").arg(QFileInfo(fileName).completeBaseName());
	if (const auto [begin, end] = std::ranges::remove_if(
			fileList,
			[&](const auto& item) {
				return !item.startsWith(filePrefix) /*|| item == filePrefix*/;
			}
		);
	    begin != end)
		fileList.erase(begin, end);

	for (const auto& file : fileList)
	{
		auto body = zip.Read(file)->GetStream().readAll();
		if (!body.isEmpty() && callback(file.split('/').back(), false, std::move(body)))
			return;
	}
}

constexpr const char* EXTENSIONS[] { "zip", "7z" };

void ExtractBookImagesCoverImpl(const QFileInfo& fileInfo, const QString& fileName, const ExtractBookImagesCallback& callback, const ISettings& settings)
{
	if (settings.Get(Export::REMOVE_COVER_KEY, false))
		return;

	bool stop = false;
	for (const auto* ext : EXTENSIONS)
	{
		TRY("parse cover", [&] {
			ParseCover(QString("%1/%2/%3.%4").arg(fileInfo.dir().path(), Global::COVERS, fileInfo.completeBaseName(), ext), fileName, callback, stop);
			return true;
		});
	}
}

void ExtractBookImagesImagesImpl(const QFileInfo& fileInfo, const QString& fileName, const ExtractBookImagesCallback& callback, const ISettings& settings)
{
	if (settings.Get(Export::REMOVE_IMAGES_KEY, false))
		return;

	for (const auto* ext : EXTENSIONS)
	{
		TRY("parse cover", [&] {
			ParseImages(QString("%1/%2/%3.%4").arg(fileInfo.dir().path(), Global::IMAGES, fileInfo.completeBaseName(), ext), fileName, callback);
			return true;
		});
	}
}

} // namespace

namespace HomeCompa::Util {

QByteArray PrepareToExport(QIODevice& input, const QString& folder, const QString& fileName, const ISettings& settings, QString encoding, std::unique_ptr<const ExtractedBook> metadataReplacement)
{
	return PrepareToExportImpl(input, folder, fileName, settings, std::move(encoding), std::move(metadataReplacement));
}

void ExtractBookImages(const QString& folder, const QString& fileName, const ISettings& settings, const ExtractBookImagesCallback& callback)
{
	const QFileInfo fileInfo(folder);
	ExtractBookImagesCoverImpl(fileInfo, fileName, callback, settings);
	ExtractBookImagesImagesImpl(fileInfo, fileName, callback, settings);
}

} // namespace HomeCompa::Util
