#pragma once
#include <exception>
#include <string>
#include <sstream>

#define EXCEPT(...)  Exception(__LINE__,__FILE__,##__VA_ARGS__)
#define THROW_EXCEPT(...) throw EXCEPT(##__VA_ARGS__)

class Exception : public std::exception
{
public:
	Exception(int line, const char* file) noexcept :
		m_line(line), m_file(file)
	{}
	explicit Exception(int line, const char* file, char const* const Message) noexcept :
		m_line(line), m_file(file),std::exception(Message)
	{}
	const char* what() const noexcept override
	{
		std::ostringstream oss;
		oss << GetType() << std::endl
			<< "[Description] " <<std::exception::what() << std::endl
			<< GetOriginString() << std::endl;
		m_whatBuffer = oss.str();
		return m_whatBuffer.c_str();
	}
	virtual const char* GetType() const noexcept
	{
		return "Exception";
	}
	int GetLine() const noexcept
	{
		return m_line;
	}
	const std::string& GetFile() const noexcept
	{
		return m_file;
	}
	std::string GetOriginString() const noexcept
	{
		std::ostringstream oss;
		oss << "[File] " << m_file << std::endl
			<< "[Line] " << m_line;
		return oss.str();
	}
private:
	int m_line;
	std::string m_file;
protected:
	mutable std::string m_whatBuffer;
};


