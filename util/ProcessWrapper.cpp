#include "ProcessWrapper.h"

#include <QDir>
#include <QEventLoop>
#include <QProcess>
#include <QRegularExpression>

#include "log.h"

namespace HomeCompa::Util {

QStringList SplitStringWithQuotes(const QString& str)
{
	QRegularExpression              regex("\"([^\"]*)\"|([^\\s,]+)");
	QRegularExpressionMatchIterator it = regex.globalMatch(str);
	QStringList                     result;
	while (it.hasNext())
	{
		QRegularExpressionMatch match = it.next();
		if (match.captured(1).length() > 0)
			result.append(match.captured(1));
		else if (match.captured(2).length() > 0)
			result.append(match.captured(2));
	}
	return result;
}

bool RunProcess(const QString& command, const QString& parameters, const QString& cwd, const bool wait)
{
	const auto args = SplitStringWithQuotes(parameters);
	if (!wait)
		return QProcess::startDetached(command, args, QDir::toNativeSeparators(cwd));

	QEventLoop eventLoop;
	QProcess   process;
	if (!cwd.isEmpty())
		process.setWorkingDirectory(QDir::toNativeSeparators(cwd));

	QByteArray fixed;
	int        errorCode = 0;
	QObject::connect(&process, &QProcess::started, [&] {
		PLOGV << QString("%1 %2 launched").arg(command, args.join(" "));
	});
	QObject::connect(&process, &QProcess::errorOccurred, [&](const auto error) {
		errorCode = static_cast<int>(error) + 1;
		PLOGE << QString("%1 %2 error: %3").arg(command, args.join(" ")).arg(errorCode);
		eventLoop.exit(errorCode);
	});
	QObject::connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), [&](const int code, const QProcess::ExitStatus) {
		eventLoop.exit(code);
	});
	QObject::connect(&process, &QProcess::readyReadStandardError, [&] {
		auto message = process.readAllStandardError();
		while (message.endsWith('\n'))
			message.chop(1);
		PLOGE << message;
	});
	QObject::connect(&process, &QProcess::readyReadStandardOutput, [&] {
		auto message = process.readAllStandardOutput();
		while (message.endsWith('\n'))
			message.chop(1);
		PLOGV << message;
	});

	process.start(QDir::toNativeSeparators(command), args, QIODevice::ReadWrite);
	if (errorCode)
		return errorCode;

	process.closeWriteChannel();

	return eventLoop.exec() == 0;
}

} // namespace HomeCompa::Util
