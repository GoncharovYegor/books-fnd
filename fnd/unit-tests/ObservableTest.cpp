#include "fnd/observable.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fnd/observer.h"

using namespace HomeCompa;

namespace {

class ITestObserver : public Observer
{
public:
	virtual void Observe() = 0;
};

class TestObserver : public ITestObserver
{
	[[maybe_unused]] static void CompileTest()
	{
		[[maybe_unused]] const TestObserver mock;
	}

public: // ITestObserver
	MOCK_METHOD(void, Observe, (), (override));
};

class TestObservable final : public Observable<ITestObserver>
{
public:
	void Go()
	{
		Perform(&ITestObserver::Observe);
	}

	size_t Count() const noexcept
	{
		return GetObserverCount();
	}
};

TEST(Util_Observable, Create)
{
	const TestObservable observable;
	EXPECT_EQ(observable.Count(), 0);
	TestObserver observer;
	EXPECT_EQ(observable.Count(), 0);
}

TEST(Util_Observable, SubscribeNoCall)
{
	TestObservable observable;
	{
		testing::StrictMock<TestObserver> observer;
		observable.Register(&observer);
		EXPECT_EQ(observable.Count(), 1);
	}
	EXPECT_EQ(observable.Count(), 0);
}

TEST(Util_Observable, SubscribeCallNoSubscribe)
{
	TestObservable observable;
	{
		testing::StrictMock<TestObserver> observer;
		observable.Go();
		EXPECT_EQ(observable.Count(), 0);
	}
	EXPECT_EQ(observable.Count(), 0);
}

TEST(Util_Observable, SubscribeCall)
{
	TestObservable observable;
	{
		testing::StrictMock<TestObserver> observer;
		EXPECT_EQ(observable.Count(), 0);
		observable.Register(&observer);
		EXPECT_EQ(observable.Count(), 1);
		EXPECT_CALL(observer, Observe()).Times(1);
		observable.Go();
	}
	EXPECT_EQ(observable.Count(), 0);
}

TEST(Util_Observable, SubscribeCall2)
{
	TestObservable observable;
	{
		testing::StrictMock<TestObserver> observer;
		EXPECT_EQ(observable.Count(), 0);
		observable.Register(&observer);
		EXPECT_EQ(observable.Count(), 1);
		EXPECT_CALL(observer, Observe()).Times(2);
		observable.Go();
		observable.Go();
	}
	EXPECT_EQ(observable.Count(), 0);
}

TEST(Util_Observable, SubscribeCallUnsubscribe)
{
	TestObservable                    observable;
	testing::StrictMock<TestObserver> observer;
	EXPECT_EQ(observable.Count(), 0);
	observable.Register(&observer);
	EXPECT_EQ(observable.Count(), 1);
	EXPECT_CALL(observer, Observe()).Times(1);
	observable.Go();
	observable.Unregister(&observer);
	EXPECT_EQ(observable.Count(), 0);
	observable.Go();
}

TEST(Util_Observable, SubscribeNotUnsubscribe)
{
	TestObservable observable;
	{
		testing::StrictMock<TestObserver> observer;
		EXPECT_EQ(observable.Count(), 0);
		observable.Register(&observer);
		EXPECT_EQ(observable.Count(), 1);
	}
	EXPECT_EQ(observable.Count(), 0);
	observable.Go();
}

TEST(Util_Observable, ObservableDead)
{
	testing::StrictMock<TestObserver> observer;
	{
		TestObservable observable;
		EXPECT_EQ(observable.Count(), 0);
		observable.Register(&observer);
		EXPECT_EQ(observable.Count(), 1);
		EXPECT_CALL(observer, Observe()).Times(1);
		observable.Go();
		EXPECT_EQ(observable.Count(), 1);
	}
}

} // namespace
