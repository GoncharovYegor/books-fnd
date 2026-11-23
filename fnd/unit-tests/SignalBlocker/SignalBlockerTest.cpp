#include "fnd/SignalBlocker.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "emitter.h"

namespace {

struct Receiver : QObject
{
	MOCK_METHOD(void, doSlotImpl, ());

public slots:

	void doSlot()
	{
		doSlotImpl();
	}
};

TEST(SignalBlockerTest, NoBlockNoConnect)
{
	Emitter  emitter;
	Receiver receiver;
	emit     emitter.doSignal();
}

TEST(SignalBlockerTest, NoBlockWithConnect)
{
	Emitter                       emitter;
	testing::StrictMock<Receiver> receiver;
	QObject::connect(&emitter, &Emitter::doSignal, &receiver, &Receiver::doSlot);
	EXPECT_CALL(receiver, doSlotImpl);
	emit emitter.doSignal();
}

TEST(SignalBlockerTest, WithBlockNoConnect)
{
	Emitter  emitter;
	Receiver receiver;
	SignalBlocker(&emitter)->doSignal();
}

TEST(SignalBlockerTest, WithBlockWithConnect)
{
	Emitter                       emitter;
	testing::StrictMock<Receiver> receiver;
	QObject::connect(&emitter, &Emitter::doSignal, &receiver, &Receiver::doSlot);
	SignalBlocker(&emitter)->doSignal();
}

} // namespace
