#pragma once
#pragma once
#include "Exception.h"
#include "HrException.h"
#include "AssertException.h"
#include "BlobExpection.h"

// 指示文件位置的异常
#define EXCEPT()  Exception(__LINE__,__FILE__)
#define THROW_EXCEPT() throw EXCEPT()


// HRESULT检测失败异常
#define HRESULT_EXCEPT(hr) HrExpection(__LINE__,__FILE__,(hr))
#define THROW_HR_EXCEPT(hr) if( FAILED((hr)) ) throw HrExpection(__LINE__,__FILE__,(hr))
//LastError
#define LAST_EXCEPT() HrExpection(__LINE__,__FILE__,GetLastError())
#define THROW_LAST_EXCEPT(expr) if (!(expr)) throw HrExpection(__LINE__,__FILE__,GetLastError())

// 断言异常
#define ASSERT_EXCEPT(Expr) AssertException(__LINE__,__FILE__,#Expr)
#define THROW_ASSERT_EXCEPT(Expr) if(!(Expr)) throw ASSERT_EXCEPT(Expr)

//使用这个异常类抛出编译着色器时的错误信息
#define THROW_BLOB_EXCEPT(hr,ErrorMsgBlob) if( FAILED((hr)) ) throw BlobExpection(__LINE__,__FILE__,(ErrorMsgBlob))

