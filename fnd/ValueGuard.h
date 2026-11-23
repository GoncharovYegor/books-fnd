#pragma once

namespace HomeCompa {

template <typename T>
class ValueGuard
{
	const T m_oldVal;
	T&      m_val;

public:
	ValueGuard(T& oldVal, const T& newVal = T())
		: m_oldVal(oldVal)
		, m_val(oldVal)
	{
		oldVal = newVal;
	}

	~ValueGuard()
	{
		m_val = m_oldVal;
	}
};

} // namespace HomeCompa
