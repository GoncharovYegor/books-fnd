#include "reader.h"

#include <condition_variable>
#include <queue>

#include <QBuffer>

#include "fnd/NonCopyMovable.h"
#include "fnd/ScopedCall.h"

#include "bit7z/bitinputarchive.hpp"
#include "zip/interface/file.h"
#include "zip/interface/stream.h"

#include "FileItem.h"
#include "QtTypes.h"
#include "log.h"

namespace HomeCompa::ZipDetails::SevenZip {

namespace {

struct BytePacket
{
	std::unique_ptr<char[]> storage;
	size_t                  length { 0 };
};

class PacketQueue
{
	NON_COPY_MOVABLE(PacketQueue)

public:
	PacketQueue() = default;

	~PacketQueue()
	{
		Close();
	}

	void Push(BytePacket packet)
	{
		std::lock_guard lock(m_queueGuard);
		assert(!m_isClosed && packet.length != 0 && !!packet.storage);

		m_queue.push(std::move(packet));
		m_condition.notify_one();
	}

	std::optional<BytePacket> Pop()
	{
		std::unique_lock lock(m_queueGuard);
		m_condition.wait(lock, [this]() {
			return !m_queue.empty() || m_isClosed;
		});

		if (m_queue.empty() && m_isClosed)
			return std::nullopt;

		auto result = std::move(m_queue.front());
		m_queue.pop();
		return result;
	}

	void Close()
	{
		std::lock_guard lock(m_queueGuard);
		m_isClosed = true;
		m_condition.notify_all();
	}

private:
	std::condition_variable m_condition;
	std::queue<BytePacket>  m_queue;
	std::mutex              m_queueGuard;
	bool                    m_isClosed { false };
};

class QIODeviceStream final : public QIODevice
{
public:
	QIODeviceStream(PacketQueue& queue, const qint64 size)
		: m_queue { queue }
		, m_size { size }
	{
	}

private: // QIODevice
	qint64 readData(char* destBuffer, qint64 bytesToRead) override
	{
		qint64 result = 0;
		while (bytesToRead > 0)
		{
			if (!m_packet)
				if (m_packet = m_queue.Pop(); !m_packet)
					break;

			assert(m_packet);
			const auto& [bytes, size] = *m_packet;

			const auto bytesLeft = static_cast<qint64>(size) - m_packetPosition;
			const auto toRead    = std::min(bytesLeft, bytesToRead);
			memcpy(destBuffer + result, bytes.get() + m_packetPosition, toRead);
			if (toRead < bytesLeft)
			{
				m_packetPosition += toRead;
			}
			else
			{
				m_packet = {};
				m_packetPosition = 0;
			}

			result      += toRead;
			bytesToRead -= toRead;
		}

		m_position += result;
		return result;
	}

	qint64 writeData(const char* /*data*/, qint64 /*len*/) override
	{
		return assert(false && "no impl"), 0;
	}

	qint64 size() const override
	{
		return m_size;
	}

	qint64 pos() const override
	{
		return m_position;
	}

	bool seek(const qint64 pos) override
	{
		assert(pos == m_position);
		return true;
	}

private:
	PacketQueue& m_queue;
	const qint64 m_size;
	qint64       m_position { 0 };

	std::optional<BytePacket> m_packet;
	qint64                    m_packetPosition { 0 };
};

class QStdStreamBuf final : public std::streambuf
{
	NON_COPY_MOVABLE(QStdStreamBuf)

public:
	explicit QStdStreamBuf(PacketQueue& queue, const size_t bufferSize = size_t { 64 } * 1024u)
		: m_queue { queue }
		, m_bufferSize { bufferSize }
	{
		setg(nullptr, nullptr, nullptr);
		AllocateNewBuffer();
	}

	~QStdStreamBuf() override
	{
		sync();
	}

private: // std::streambuf
	int_type overflow(const int_type ch) override
	{
		FlushBuffer();

		if (!traits_type::eq_int_type(ch, traits_type::eof()))
		{
			*pptr() = traits_type::to_char_type(ch);
			pbump(1);
		}

		return traits_type::not_eof(ch);
	}

	int sync() override
	{
		FlushBuffer();
		return 0;
	}

	pos_type seekoff(off_type off, const std::ios_base::seekdir dir, const std::ios_base::openmode mode) override
	{
		FlushBuffer();

		switch (dir)
		{
			case std::ios_base::end:
				off = m_position - off;
				break;
			case std::ios_base::cur:
				off = m_position + off;
				break;
			default:
				break;
		}
		return seekpos(off, mode);
	}

	pos_type seekpos(const pos_type off, std::ios_base::openmode) override
	{
		FlushBuffer();
		return static_cast<pos_type>(m_position >= off ? (m_position = off) : static_cast<ptrdiff_t>(-1));
	}

private:
	void AllocateNewBuffer()
	{
		m_buffer = { std::make_unique_for_overwrite<char[]>(m_bufferSize), m_bufferSize };
		setp(m_buffer.storage.get(), m_buffer.storage.get() + m_bufferSize);
	}

	void FlushBuffer()
	{
		const auto pendingBytes = pptr() - pbase();
		if (pendingBytes == 0)
			return;

		m_buffer.length = pendingBytes;
		m_queue.Push(std::move(m_buffer));
		m_position += pendingBytes;

		AllocateNewBuffer();
	}

private:
	PacketQueue& m_queue;
	const size_t m_bufferSize;
	BytePacket   m_buffer;
	ptrdiff_t    m_position { 0 };
};

class QStdOStream final : public std::ostream
{
public:
	explicit QStdOStream(PacketQueue& queue)
		: std::ostream(&m_streamBuf)
		, m_streamBuf { queue }
	{
	}

private:
	QStdStreamBuf m_streamBuf;
};

class StreamImpl final : public Stream
{
public:
	StreamImpl(const bit7z::BitInputArchive& zip, const FileItem& fileItem)
		: m_stream(m_queue, static_cast<int64_t>(fileItem.size))
	{
		m_stream.open(QIODevice::ReadOnly);
		QStdOStream stream(m_queue);
		zip.extractTo(stream, fileItem.index);
		m_queue.Close();
	}

private: // Stream
	QIODevice& GetStream() override
	{
		return m_stream;
	}

private:
	PacketQueue     m_queue;
	QIODeviceStream m_stream;
};

class FileReader final : public IFile
{
public:
	FileReader(const bit7z::BitInputArchive& zip, const FileItem& fileItem)
		: m_zip { zip }
		, m_fileItem { fileItem }
	{
	}

private: // IFile
	std::unique_ptr<Stream> Read() override
	{
		try
		{
			return std::make_unique<StreamImpl>(m_zip, m_fileItem);
		}
		catch (const std::exception& ex)
		{
			PLOGE << m_fileItem.name << " read error: " << ex.what();
			throw;
		}
		catch (...)
		{
			PLOGE << m_fileItem.name << " read error: unknown error";
			throw;
		}
	}

	std::unique_ptr<Stream> Write() override
	{
		throw std::runtime_error("Cannot write with reader");
	}

private:
	const bit7z::BitInputArchive& m_zip;
	const FileItem&               m_fileItem;
};

} // namespace

namespace File {

std::unique_ptr<IFile> Read(const bit7z::BitInputArchive& zip, const FileItem& fileItem)
{
	return std::make_unique<FileReader>(zip, fileItem);
}

} // namespace File

} // namespace HomeCompa::ZipDetails::SevenZip
