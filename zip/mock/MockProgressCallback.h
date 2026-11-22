#pragma once

#include <gmock/gmock.h>

#include "zip.h"

namespace HomeCompa::Test {

class MockProgressCallback : public Zip::ProgressCallback
{
public: // Zip::ProgressCallback
	MOCK_METHOD(void, OnStartWithTotal, (int64_t totalBytes), (override));
	MOCK_METHOD(void, OnSetCompleted, (int64_t bytes), (override));
	MOCK_METHOD(void, OnIncrement, (int64_t bytes), (override));
	MOCK_METHOD(void, OnDone, (), (override));
	MOCK_METHOD(void, OnFileDone, (const QString& filePath), (override));
	MOCK_METHOD(bool, OnCheckBreak, (), (override));
};

static_assert(!std::is_abstract_v<MockProgressCallback>, "MockProgressCallback is abstract!");

}
