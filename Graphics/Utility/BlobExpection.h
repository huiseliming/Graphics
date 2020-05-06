#pragma once
#include "Exception.h"
#include "../Graphics/D3D12Header.h"


#define THROW_BLOB_EXCEPT(hr,ErrorMsgBlob) if( FAILED((hr)) ) throw BlobExpection(__LINE__,__FILE__,(ErrorMsgBlob))
//ID3DBlob附带异常信息的异常
class BlobExpection :public Exception
{
public:
public:
	BlobExpection(int line, const char* file, Microsoft::WRL::ComPtr<ID3DBlob>& pMsg) noexcept :
		Exception(line, file), m_ErrorMsg((char*)(pMsg->GetBufferPointer() ? pMsg->GetBufferPointer() : ""))
	{

	}
	const char* what() const noexcept
	{
		std::ostringstream oss;
		oss << GetType() << std::endl
			<< GetOriginString() << std::endl
			<< "[CompileErrorMsg]" << GetErrorMsg();
		m_whatBuffer = oss.str();
		return m_whatBuffer.c_str();
	}
	const char* GetType() const noexcept
	{
		return "BlobExpection";
	}
	std::string GetErrorMsg() const noexcept
	{
		return m_ErrorMsg.c_str();
	}
private:
	std::string m_ErrorMsg;

};




