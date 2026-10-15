#pragma once

#include "fnd/StrUtil.h"

#include "log.h"

namespace HomeCompa::Util {

template <typename R, typename S, typename T>
R Try(const S& name, const T& functor, const std::string_view file, const int line)
{
	try
	{
#ifndef NDEBUG
		PLOGV << name << " started";
#endif
		auto result = functor();
#ifndef NDEBUG
		PLOGV << name << " finished";
#endif
		return std::forward<R>(result);
	}
	catch (const std::exception& ex)
	{
		std::filesystem::path thisPath(__FILE__);
		thisPath = thisPath.parent_path().parent_path().parent_path().parent_path();
		PLOGE << std::format("{} failed. src{}, {}: {}", name, file.substr(thisPath.string().length()), line, ex.what());
	}
	catch (...)
	{
		std::filesystem::path thisPath(__FILE__);
		thisPath = thisPath.parent_path().parent_path().parent_path().parent_path();
		PLOGE << std::format("{} failed. src{}, {}: unknown error", name, file.substr(thisPath.string().length()), line);
	}

	return R {};
}

} // namespace HomeCompa::Util

#define TRY(NAME, FUNCTOR) HomeCompa::Util::Try<std::invoke_result_t<decltype(FUNCTOR)>>(NAME, FUNCTOR, __FILE__, __LINE__)
