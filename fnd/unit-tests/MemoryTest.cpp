#include "fnd/memory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace HomeCompa;

namespace {

namespace Ctor {

struct MyClass
{
};

TEST(PropagateConstPtrTest, UniqueCtor1)
{
	PropagateConstPtr<MyClass> obj;
	EXPECT_TRUE(obj.get());
	EXPECT_TRUE(obj);
	EXPECT_FALSE(!obj);
}

TEST(PropagateConstPtrTest, UniqueCtor2)
{
	PropagateConstPtr<MyClass> obj { std::make_unique<MyClass>() };
	EXPECT_TRUE(obj.get());
	EXPECT_TRUE(obj);
	EXPECT_FALSE(!obj);
}

TEST(PropagateConstPtrTest, UniqueCtor3)
{
	PropagateConstPtr<MyClass> obj1;
	PropagateConstPtr<MyClass> obj2 = std::move(obj1);
	PropagateConstPtr<MyClass> obj3;
	obj3 = std::move(obj2);
	EXPECT_TRUE(obj3.get());
	EXPECT_TRUE(obj3);
	EXPECT_FALSE(!obj3);
}

TEST(PropagateConstPtrTest, SharedCtor1)
{
	PropagateConstPtr<MyClass, std::shared_ptr> obj;
	EXPECT_TRUE(obj.get());
	EXPECT_TRUE(obj);
	EXPECT_FALSE(!obj);
}

TEST(PropagateConstPtrTest, SharedCtor2)
{
	PropagateConstPtr<MyClass, std::shared_ptr> obj { std::make_shared<MyClass>() };
	EXPECT_TRUE(obj.get());
	EXPECT_TRUE(obj);
	EXPECT_FALSE(!obj);
}


TEST(PropagateConstPtrTest, SharedCtor3)
{
	PropagateConstPtr<MyClass, std::shared_ptr> obj1;
	PropagateConstPtr<MyClass, std::shared_ptr> obj2 = obj1;
	EXPECT_TRUE(obj2.get());
	EXPECT_TRUE(obj2);
	EXPECT_FALSE(!obj2);
}

TEST(PropagateConstPtrTest, SharedCtor4)
{
	PropagateConstPtr<MyClass, std::shared_ptr> obj1;
	PropagateConstPtr<MyClass, std::shared_ptr> obj2;
	obj2 = obj1;
	EXPECT_TRUE(obj2.get());
	EXPECT_TRUE(obj2);
	EXPECT_FALSE(!obj2);
}

TEST(PropagateConstPtrTest, SharedCtor5)
{
	PropagateConstPtr<MyClass, std::shared_ptr> obj1;
	PropagateConstPtr<MyClass, std::shared_ptr> obj2 = std::move(obj1);
	EXPECT_TRUE(obj2.get());
	EXPECT_TRUE(obj2);
	EXPECT_FALSE(!obj2);
}

TEST(PropagateConstPtrTest, SharedCtor6)
{
	PropagateConstPtr<MyClass, std::shared_ptr> obj1;
	PropagateConstPtr<MyClass, std::shared_ptr> obj2;
	obj2 = std::move(obj1);
	EXPECT_TRUE(obj2.get());
	EXPECT_TRUE(obj2);
	EXPECT_FALSE(!obj2);
}

TEST(PropagateConstPtrTest, SharedCtor7)
{
	PropagateConstPtr<MyClass, std::shared_ptr> obj1;
	auto obj2 = static_cast<std::shared_ptr<MyClass>>(obj1);
	EXPECT_TRUE(obj2.get());
	EXPECT_TRUE(obj2);
	EXPECT_FALSE(!obj2);
}

} // namespace Ctor

namespace ConstCall {

struct MyClass
{
	MOCK_METHOD(void, mutableMethod, ());
	MOCK_METHOD(void, constMethod, (), (const));

	void method()
	{
		mutableMethod();
	}

	void method() const
	{
		constMethod();
	}
};

TEST(PropagateConstPtrTest, UniqueNonConst)
{
	PropagateConstPtr<testing::StrictMock<MyClass>> obj;
	EXPECT_CALL(*obj, mutableMethod).Times(2);
	obj->method();
	obj.get()->method();
}

TEST(PropagateConstPtrTest, UniqueConst)
{
	const PropagateConstPtr<testing::StrictMock<MyClass>> obj;
	EXPECT_CALL(*obj, constMethod).Times(2);
	obj->method();
	obj.get()->method();
}

TEST(PropagateConstPtrTest, SharedNonConst)
{
	PropagateConstPtr<testing::StrictMock<MyClass>, std::shared_ptr> obj;
	EXPECT_CALL(*obj, mutableMethod).Times(2);
	obj->method();
	obj.get()->method();
}

TEST(PropagateConstPtrTest, SharedConst)
{
	const PropagateConstPtr<testing::StrictMock<MyClass>, std::shared_ptr> obj;
	EXPECT_CALL(*obj, constMethod).Times(2);
	obj->method();
	obj.get()->method();
}

} // namespace ConstCall

} // namespace
