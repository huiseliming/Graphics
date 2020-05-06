//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "D3D12Header.h"
#include "DescriptorHeap.h"
#include "Graphics.h"

//
// DescriptorHeap implementation
//
std::mutex DescriptorAllocator::sm_AllocationMutex;
std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DescriptorAllocator::sm_DescriptorHeapPool;

void DescriptorAllocator::DestroyAll(void)
{
    sm_DescriptorHeapPool.clear();
}

ID3D12DescriptorHeap* DescriptorAllocator::RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
    std::lock_guard<std::mutex> LockGuard(sm_AllocationMutex);

    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    Desc.Type = Type;
    Desc.NumDescriptors = sm_NumDescriptorsPerHeap;
    Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Desc.NodeMask = 1;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pHeap;
    THROW_HR_EXCEPT_IF_FAILED(GlobalVariable<ID3D12Device>::Get()->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pHeap)));
    sm_DescriptorHeapPool.emplace_back(pHeap);
    return pHeap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::Allocate( uint32_t Count )
{
    ASSERT(Count > 0);
    std::lock_guard<std::mutex> LockGuard(m_AllocationMutex);
    if (m_CurrentHeap == nullptr || m_RemainingFreeHandles < Count )
    {
        if(!m_AvailableDescriptor.empty() && Count == 1 )
        {
            D3D12_CPU_DESCRIPTOR_HANDLE ret = m_AvailableDescriptor.front();
            m_AvailableDescriptor.pop();
            return ret;
        }
        m_CurrentHeap = RequestNewHeap(m_Type);
        m_CurrentHandle = m_CurrentHeap->GetCPUDescriptorHandleForHeapStart();
        m_RemainingFreeHandles = sm_NumDescriptorsPerHeap;

        if (m_DescriptorSize == 0)
            m_DescriptorSize = GlobalVariable<Graphics>::Get()->GetDescriptorSize(m_Type);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE ret = m_CurrentHandle;
    m_CurrentHandle.ptr += Count * m_DescriptorSize;
    m_RemainingFreeHandles -= Count;
    return ret;
}

void DescriptorAllocator::Discard(D3D12_CPU_DESCRIPTOR_HANDLE Descriptor)
{
    std::lock_guard<std::mutex> LockGuard(m_AllocationMutex);
    m_AvailableDescriptor.push(Descriptor);
}

//
// UserDescriptorHeap implementation
//

void UserDescriptorHeap::Create( const std::wstring& DebugHeapName )
{
    THROW_HR_EXCEPT_IF_FAILED(GlobalVariable<ID3D12Device>::Get()->CreateDescriptorHeap(&m_HeapDesc, IID_PPV_ARGS(m_Heap.ReleaseAndGetAddressOf())));
#ifdef RELEASE
    (void)DebugHeapName;
#else
    m_Heap->SetName(DebugHeapName.c_str());
#endif

    m_DescriptorSize = GlobalVariable<Graphics>::Get()->GetDescriptorSize(m_HeapDesc.Type);
    m_NumFreeDescriptors = m_HeapDesc.NumDescriptors;
    m_FirstHandle = DescriptorHandle( m_Heap->GetCPUDescriptorHandleForHeapStart(),  m_Heap->GetGPUDescriptorHandleForHeapStart() );
    m_NextFreeHandle = m_FirstHandle;
}

DescriptorHandle UserDescriptorHeap::Alloc( uint32_t Count )
{
    ASSERT(HasAvailableSpace(Count) && "Descriptor Heap out of space.  Increase heap size.");
    DescriptorHandle ret = m_NextFreeHandle;
    m_NextFreeHandle += Count * m_DescriptorSize;
    return ret;
}

bool UserDescriptorHeap::ValidateHandle( const DescriptorHandle& DHandle ) const
{
    if (DHandle.GetCpuHandle().ptr < m_FirstHandle.GetCpuHandle().ptr ||
        DHandle.GetCpuHandle().ptr >= m_FirstHandle.GetCpuHandle().ptr + m_HeapDesc.NumDescriptors * m_DescriptorSize)
        return false;

    if (DHandle.GetGpuHandle().ptr - m_FirstHandle.GetGpuHandle().ptr !=
        DHandle.GetCpuHandle().ptr - m_FirstHandle.GetCpuHandle().ptr)
        return false;

    return true;
}
