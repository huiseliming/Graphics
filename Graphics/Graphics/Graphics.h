#pragma once
#include "D3D12Header.h"
#include "CommandQueueManager.h"
#include "DescriptorHeap.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"


//D3D12图形对象
class Graphics
{
public:
	Graphics();
	virtual ~Graphics();

	void RenderScene();

	void Present();

	void Resize(uint32_t Width, uint32_t Height);

	void Terminate();

	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type){ return m_DescriptorAllocator[Type].Allocate(1); }
	void DiscardDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptor) { return m_DescriptorAllocator[Type].Discard(CpuDescriptor); }
	uint32_t GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE Type) { return m_DescriptorSize[Type]; }
	float GetFrameTime(){ return m_FrameTime; }

private:
	//const 属性
	static const uint32_t SwapChainBufferCount = 3;
	//交换链依赖属性
	uint32_t m_DisplayWidth;
	uint32_t m_DisplayHeight;
	DXGI_FORMAT m_SwapChainFormat;
	DXGI_SAMPLE_DESC m_SwapChainSample;
	ColorBuffer m_DisplayBuffer[SwapChainBufferCount];
	uint32_t m_DisplayIndex;
	DepthBuffer m_DisplayDepthBuffer;


	//视口
	D3D12_VIEWPORT m_Viewport[2];
	//裁剪框
	std::vector<D3D12_RECT> m_ScissorRect[2];


	
	//D3D12驱动实例
	ID3D12Device* m_pDevice;
	//交换链
	IDXGISwapChain1* m_pSwapChain1;
	//命令队列管理器
	CommandQueueManager m_CommandManager;

	//是否启用垂直同步
	bool m_EnableVSync;
	//是否限制到30帧
	bool m_LimitTo30Hz;
	//上一帧渲染完成的cpu周期数，也是这一帧的开始
	int64_t m_FrameStartTick;
	//上一个两帧之间的时间间隔
	float  m_FrameTime;
	//当前在渲染的帧索引
	uint64_t m_FrameIndex;


	// 仅Cpu可见的描述符堆 持久使用直到程序关闭的的堆
	DescriptorAllocator m_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	uint32_t m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};


