#pragma once
#include <exception>
#include <string>
#include <sstream>



class Exception : public std::exception
{
public:
	Exception(int line, const char* file) noexcept :
		m_line(line), m_file(file)
	{}
	const char* what() const noexcept override
	{
		std::ostringstream oss;
		oss << GetType() << std::endl
			<< GetOriginString();
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


