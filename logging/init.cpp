#include "init.h"

#include <iostream>

#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/TxtFormatter.h>

#include "LogAppender.h"
#include "QtLoHandler.h"
#include "log.h"

using namespace HomeCompa::Log;

namespace {

QString DEFAULT_LOG_PATH;

std::string CheckForAppend(QString path)
{
	const QFileInfo fileInfo(path);
	for (int i = 0;; path = fileInfo.dir().filePath(QString("%1_%2.%3").arg(fileInfo.completeBaseName()).arg(++i).arg(fileInfo.suffix())))
	{
		QFile file(path);
		if (file.open(QIODevice::WriteOnly | QIODevice::Append))
			break;
	}

	return path.toStdString();
}

template <class Formatter>
class ConsoleAppender : public plog::IAppender
{
private:
	void write(const plog::Record& record) override
	{
		(record.getSeverity() < plog::Severity::warning ? m_cerr : m_cout) << Formatter::format(record);
	}

private:
#if PLOG_CHAR_IS_UTF8
	std::ostream& m_cout = std::cout;
	std::ostream& m_cerr = std::cerr;
#else
	std::wostream& m_cout = std::wcout;
	std::wostream& m_cerr = std::wcerr;
#endif
};

std::unique_ptr<plog::IAppender> CreateAppender(QString path)
{
	if (path == LoggingInitializer::CONSOLE)
		return std::make_unique<ConsoleAppender<plog::TxtFormatter>>();

	if (path.isEmpty())
		path = DEFAULT_LOG_PATH;

	return std::make_unique<plog::RollingFileAppender<plog::TxtFormatter>>(CheckForAppend(path).data(), 1024ULL * 1024 * 1024, 10);
}

} // namespace

struct LoggingInitializer::Impl
{
	PropagateConstPtr<plog::IAppender> logAppenderImpl;
	LogAppender                        logAppender;
	QtLogHandler                       qtLogHandler;

	explicit Impl(const QString& path)
		: logAppenderImpl { CreateAppender(path) }
		, logAppender { logAppenderImpl.get() }
	{
	}
};

LoggingInitializer::LoggingInitializer(const QString& path)
	: m_impl(path)
{
}

LoggingInitializer::~LoggingInitializer() = default;

QString LoggingInitializer::AddLogFileOption(QCommandLineParser& parser, QString defaultPath)
{
	static constexpr auto LOG = "log";
	parser.addOption(
		{
			{ QString(LOG[0]), QString(LOG) },
			QString("Log file path or %1 for log to stdout/stderr").arg(CONSOLE),
			defaultPath
    }
	);
	DEFAULT_LOG_PATH = std::move(defaultPath);
	return LOG;
}
