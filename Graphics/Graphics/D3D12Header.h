#pragma once
#include <sdkddkver.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include "d3dx12.h"


#if defined(_DEBUG)
#include <dxgidebug.h>
#else

#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dxguid.lib")


/* Define By DaiMingze BEGIN */
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)


#include "../Utility/Utility.h"
#include <mutex>
#include <vector>
#include <queue>

/* Define By DaiMingze END   */

