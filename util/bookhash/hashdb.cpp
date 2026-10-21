#include "hashdb.h"

#include "database/interface/IDatabase.h"
#include "database/interface/IQuery.h"

#include "Constant.h"
#include "hashparser.h"

using namespace HomeCompa::Util;

namespace HomeCompa::Util {

BookHashItem ParseDbHash(DB::IDatabase& db, const QString& folder, const QString& file)
{
	const auto fileId = [&] {
		const auto query = db.CreateQuery("select f.FileId from File f join Folder d on d.FolderId = f.FolderId and d.Name = ? where f.Name = ?", folder, file);
		query->Execute();
		return query->Eof() ? -1LL : query->Get<long long>(0);
	}();
	if (fileId < 0)
		return {};

	auto bookHashItem = [&]() -> BookHashItem {
		const auto query = db.CreateQuery("select f.Md5, f.Title, f.Hash, f.Annotation, f.WordCount, f.SymbolCount, f.SimHash from File f where f.FileId = ?", fileId);
		query->Execute();
		if (query->Eof())
			return {};

		return BookHashItem {
			.folder = folder,
			.file   = file,
			.parseResult =
				{
							  .id         = query->Get<const char*>(0),
							  .title      = query->Get<const char*>(1),
							  .hashText   = query->Get<const char*>(2),
							  .annotation = query->Get<const char*>(3),
							  .count      = query->Get<size_t>(4),
							  .size       = query->Get<size_t>(5),
							  .simHash    = query->Get<QString>(6).toULongLong(nullptr, 16),
							  },
		};
	}();

	if (bookHashItem.folder.isEmpty())
		return {};

	const auto process = [&](const std::string_view queryText, const auto& f) {
		const auto query = db.CreateQuery(queryText, fileId);
		std::ranges::for_each(*query, f);
	};

	process("select Name, Md5, PHash, EncodedSize, DecodedSize, Width, Height, Linked, HasAlpha from Image where FileId = ? order by ImageId", [&](const DB::IQuery& query) {
		ImageHashItem item {
			.file        = query.Get<const char*>(0),
			.hash        = query.Get<const char*>(1),
			.pHash       = query.Get<QString>(2).toULongLong(nullptr, 16),
			.encodedSize = query.Get<size_t>(3),
			.decodedSize = query.Get<size_t>(4),
			.size        = { query.Get<int>(5), query.Get<int>(6) },
			.linked      = query.Get<int>(7) != 0,
			.hasAlpha    = query.Get<int>(8) != 0,
		};
		if (item.file == Global::COVER)
			bookHashItem.cover = std::move(item);
		else
			bookHashItem.images.emplace_back(item);
	});

	bookHashItem.parseResult.hashValues.reserve(10);
	process("select WordCount, Word from Histogram where FileId = ? order by HistogramId", [&](const DB::IQuery& query) {
		bookHashItem.parseResult.hashValues.emplace_back(query.Get<long long>(0), query.Get<const char*>(1));
	});

	return bookHashItem;
}

} // namespace HomeCompa::Util
