#include "Graphics.h"
#include "../Engine/SystemTime.h"
#include "../Engine/Game.h"
#include "CommandContext.h"

Graphics::Graphics():
    m_DisplayWidth(GlobalVariable<Window>::Get()->GetWidth()),
    m_DisplayHeight(GlobalVariable<Window>::Get()->GetHeight()),
    m_SwapChainFormat(DXGI_FORMAT_R10G10B10A2_UNORM),
    m_SwapChainSample(DXGI_SAMPLE_DESC{1,0}),
    m_DisplayIndex(0),
    m_EnableVSync(false),
    m_LimitTo30Hz(false),
    m_FrameStartTick(0),
    m_FrameTime(0),
    m_FrameIndex(0),
    m_DescriptorAllocator{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_TYPE_DSV } 
{
    GlobalVariable<Graphics>::Set(this);
	Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
#if _DEBUG //启用调试层
	Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
	THROW_HR_EXCEPT_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif

	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    THROW_HR_EXCEPT_IF_FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)));

    // Create the D3D graphics device
    Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;

    //获取显存最大的显卡
    SIZE_T MaxSize = 0;
    for (uint32_t Idx = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(Idx, &pAdapter); ++Idx)
    {
        DXGI_ADAPTER_DESC1 desc;
        pAdapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        if (desc.DedicatedVideoMemory > MaxSize && SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice))))
        {
            pAdapter->GetDesc1(&desc);
            //Utility::Printf(L"D3D12-capable hardware found:  %s (%u MB)\n", desc.Description, desc.DedicatedVideoMemory >> 20);
            MaxSize = desc.DedicatedVideoMemory;
        }
    }

    if (MaxSize > 0)
        m_pDevice = pDevice.Detach();

    if (m_pDevice == nullptr)
    {
        THROW_HR_EXCEPT_IF_FAILED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter)));
        THROW_HR_EXCEPT_IF_FAILED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice)));
        m_pDevice = pDevice.Detach();
    }
    if (m_pDevice == nullptr)
        THROW_EXCEPT("这台电脑连核显都没有？");

    //初始化当前显卡的描述符大小
    m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


    GlobalVariable<ID3D12Device>::Set(m_pDevice);
    //初始化命令管理器
    m_CommandManager.Initialize(m_pDevice);
    GlobalVariable<CommandQueueManager>::Set(&m_CommandManager);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_DisplayWidth;
    swapChainDesc.Height = m_DisplayHeight;
    swapChainDesc.Format = m_SwapChainFormat;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SampleDesc = m_SwapChainSample;
    //swapChainDesc.SampleDesc.Quality = 0;
    //swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SwapChainBufferCount;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    //创建交换链
    THROW_HR_EXCEPT_IF_FAILED(dxgiFactory->CreateSwapChainForHwnd(m_CommandManager.GetGraphicsQueue().GetCommandQueue(), GlobalVariable<Window>::Get()->GetHWND(), &swapChainDesc, nullptr, nullptr, &m_pSwapChain1));

    for (uint32_t i = 0; i < SwapChainBufferCount; ++i)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> DisplayBuffer;
        THROW_HR_EXCEPT_IF_FAILED(m_pSwapChain1->GetBuffer(i, IID_PPV_ARGS(&DisplayBuffer)));
        m_DisplayBuffer[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayBuffer.Detach());
    }
    m_DepthBuffer.Create(L"Scene Depth Buffer", m_DisplayWidth, m_DisplayHeight, DXGI_FORMAT_D32_FLOAT);
}

Graphics::~Graphics()
{
    m_CommandManager.Shutdown();
    CommandContext::DestroyAllContexts();
    DescriptorAllocator::DestroyAll();
    m_DepthBuffer.Destroy();
    for (uint32_t i = 0; i < SwapChainBufferCount; ++i)
        m_DisplayBuffer[i].Destroy();
    m_pSwapChain1->Release();

#if defined(_DEBUG)
    ID3D12DebugDevice* debugInterface;
    if (SUCCEEDED(m_pDevice->QueryInterface(&debugInterface)))
    {
        debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
        debugInterface->Release();
    }
#endif
    m_pDevice->Release();
}



void Graphics::Present()
{
    m_DisplayIndex = (m_DisplayIndex + 1) % SwapChainBufferCount;

    UINT PresentInterval = m_EnableVSync ? (std::min)(4, (int)Math::Round(m_FrameTime * 60.0f)) : 0;

    m_pSwapChain1->Present(PresentInterval, 0);

    // Test robustness to handle spikes in CPU time
    //if (s_DropRandomFrames)
    //{
    //    if (std::rand() % 25 == 0)
    //        BusyLoopSleep(0.010);
    //}
    GlobalVariable<GameTimer>::Get()->Tick();
    if (m_EnableVSync)
    {
        // With VSync enabled, the time step between frames becomes a multiple of 16.666 ms.  We need
        // to add logic to vary between 1 and 2 (or 3 fields).  This delta time also determines how
        // long the previous frame should be displayed (i.e. the present interval.)
        m_FrameTime = (m_LimitTo30Hz ? 2.0f : 1.0f) / 60.0f;
        //if (m_DropRandomFrames)
        //{
        //    if (std::rand() % 50 == 0)
        //        m_FrameTime += (1.0f / 60.0f);
        //}
    }
    else
    {
        // When running free, keep the most recent total frame time as the time step for
        // the next frame simulation.  This is not super-accurate, but assuming a frame
        // time varies smoothly, it should be close enough.
        m_FrameTime = GlobalVariable<GameTimer>::Get()->GetDeltaTime();
    }
    ++m_FrameIndex;
}

void Graphics::Terminate()
{
    m_CommandManager.IdleGPU();
}



