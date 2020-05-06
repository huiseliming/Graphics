#pragma once
#include "D3D12Header.h"

class GpuResource
{
    friend class CommandContext;
    friend class GraphicsContext;
    friend class ComputeContext;
public:
    GpuResource() :
        m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
        m_CpuVirtualAddress(nullptr),
        m_UsageState(D3D12_RESOURCE_STATE_COMMON),
        m_TransitioningState((D3D12_RESOURCE_STATES)-1)
    {}
    GpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState,void * CpuVirtualAddress = nullptr) :
        m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
        m_CpuVirtualAddress(CpuVirtualAddress),
        m_pResource(pResource),
        m_UsageState(CurrentState),
        m_TransitioningState((D3D12_RESOURCE_STATES)-1)
    {}

    virtual void Destroy()
    {
        m_pResource = nullptr;
        m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
    }
    ID3D12Resource* operator->() { return m_pResource.Get(); }
    const ID3D12Resource* operator->() const { return m_pResource.Get(); }

    ID3D12Resource* GetResource() { return m_pResource.Get(); }
    const ID3D12Resource* GetResource() const { return m_pResource.Get(); }

    D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_GpuVirtualAddress; }
    void* GetCpuVirtualAddress() const { return m_CpuVirtualAddress; }
protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
	D3D12_RESOURCE_STATES m_UsageState;
	D3D12_RESOURCE_STATES m_TransitioningState;
	D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
	void* m_CpuVirtualAddress;
};


