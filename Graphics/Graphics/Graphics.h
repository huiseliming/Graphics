#pragma once
#include "D3D12Header.h"
#include "CommandQueueManager.h"
#include "DescriptorHeap.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"


//D3D12ͼ�ζ���
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
	//const ����
	static const uint32_t SwapChainBufferCount = 3;
	//��������������
	uint32_t m_DisplayWidth;
	uint32_t m_DisplayHeight;
	DXGI_FORMAT m_SwapChainFormat;
	DXGI_SAMPLE_DESC m_SwapChainSample;
	ColorBuffer m_DisplayBuffer[SwapChainBufferCount];
	uint32_t m_DisplayIndex;
	DepthBuffer m_DisplayDepthBuffer;


	//�ӿ�
	D3D12_VIEWPORT m_Viewport[2];
	//�ü���
	std::vector<D3D12_RECT> m_ScissorRect[2];


	
	//D3D12����ʵ��
	ID3D12Device* m_pDevice;
	//������
	IDXGISwapChain1* m_pSwapChain1;
	//������й�����
	CommandQueueManager m_CommandManager;

	//�Ƿ����ô�ֱͬ��
	bool m_EnableVSync;
	//�Ƿ����Ƶ�30֡
	bool m_LimitTo30Hz;
	//��һ֡��Ⱦ��ɵ�cpu��������Ҳ����һ֡�Ŀ�ʼ
	int64_t m_FrameStartTick;
	//��һ����֮֡���ʱ����
	float  m_FrameTime;
	//��ǰ����Ⱦ��֡����
	uint64_t m_FrameIndex;


	// ��Cpu�ɼ����������� �־�ʹ��ֱ������رյĵĶ�
	DescriptorAllocator m_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	uint32_t m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};


