#pragma once
#include "Exception.h"

#define ASSERT_EXCEPT(Expr) AssertException(__LINE__,__FILE__,#Expr)
#define THROW_ASSERT_EXCEPT(Expr) if(!(Expr)) throw ASSERT_EXCEPT(Expr)

//#define ASSERT(Expr) if(!(Expr)) THROW_ASSERT_EXCEPT(Expr)

//‘› ±Œﬁ”√
class AssertException : public Exception
{
public:
	AssertException(int line, const char* file, const char* assertstr) noexcept :
		Exception(line, file),
		m_assert(assertstr)
	{
	}
	const char* what() const noexcept
	{
		std::ostringstream oss;
		oss << GetType() << std::endl
			<< "[Assert] " << GetAssert() << std::endl
			<< GetOriginString();

		m_whatBuffer = oss.str();
		return m_whatBuffer.c_str();
	}
	const char* GetType() const noexcept
	{
		return "AssertException";
	}
	const char* GetAssert() const noexcept
	{
		return m_assert.c_str();
	}

private:
	std::string m_assert;
};