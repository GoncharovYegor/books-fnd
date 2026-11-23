#include "fnd/Lockable.h"

#include <gtest/gtest.h>

using namespace HomeCompa;

namespace {

struct MyClass
{
};

TEST(Lockable, NonConst)
{
	auto                shared = std::make_shared<MyClass>();
	const std::weak_ptr weak   = shared;
	EXPECT_TRUE(!!Lockable<MyClass>::Lock(weak));
	shared.reset();
	EXPECT_DEBUG_DEATH(Lockable<MyClass>::Lock(weak), "");
}

TEST(Lockable, Const)
{
	std::shared_ptr<const MyClass> shared = std::make_shared<MyClass>();
	const std::weak_ptr            weak   = shared;
	EXPECT_TRUE(!!Lockable<MyClass>::Lock(weak));
	shared.reset();
	EXPECT_DEBUG_DEATH(Lockable<MyClass>::Lock(weak), "");
}

} // namespace
