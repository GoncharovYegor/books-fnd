#pragma once

#include <QObject>

struct Emitter : QObject
{
	Q_OBJECT
signals:
	void doSignal() const;
};
