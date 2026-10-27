#pragma once

#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"

#include "export/util.h"

class QString;

namespace HomeCompa::Util {

class UTIL_EXPORT Transliterator
{
	NON_COPY_MOVABLE(Transliterator)

public:
	Transliterator();
	~Transliterator();
	[[nodiscard]] bool    IsReady() const noexcept;
	[[nodiscard]] QString Transliterate(QString fileName) const;

private:
	struct Impl;
	PropagateConstPtr<Impl> m_impl;
};

} // namespace HomeCompa::Util
