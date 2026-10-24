#pragma once

#include <memory>

#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"

namespace bit7z {

class BitInputArchive;

}

namespace HomeCompa::ZipDetails {

class IFile;

}

namespace HomeCompa::ZipDetails::SevenZip {

struct FileItem;
class PacketQueue;

class Context
{
	NON_COPY_MOVABLE(Context)

public:
	explicit Context(const bit7z::BitInputArchive& zip);
	~Context();

public:
	void Extract(uint32_t index, std::weak_ptr<PacketQueue> queue);

private:
	struct Impl;
	PropagateConstPtr<Impl> m_impl;
};

namespace File {

std::unique_ptr<IFile> Read(Context& ctx, const FileItem& fileItem);

};

} // namespace HomeCompa::ZipDetails::SevenZip
