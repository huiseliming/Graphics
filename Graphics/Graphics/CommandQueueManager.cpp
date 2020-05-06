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
#include "CommandQueueManager.h"
#include "Graphics.h"

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE Type) :
    m_Type(Type),
    m_CommandQueue(nullptr),
    m_pFence(nullptr),
    m_NextFenceValue((uint64_t)Type << 56 | 1),
    m_LastCompletedFenceValue((uint64_t)Type << 56),
    m_AllocatorPool(Type)
{
}

CommandQueue::~CommandQueue()
{
    Shutdown();
}

void CommandQueue::Shutdown()
{
    if (m_CommandQueue == nullptr)
        return;

    m_AllocatorPool.Shutdown();

    CloseHandle(m_FenceEventHandle);

    m_pFence->Release();
    m_pFence = nullptr;

    m_CommandQueue->Release();
    m_CommandQueue = nullptr;
}


void CommandQueue::Initialize(ID3D12Device* pDevice)
{
    ASSERT(pDevice != nullptr);
    ASSERT(!IsReady());
    ASSERT(m_AllocatorPool.Size() == 0);

    D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
    QueueDesc.Type = m_Type;
    QueueDesc.NodeMask = 1;
    pDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&m_CommandQueue));
    m_CommandQueue->SetName(L"CommandQueueManager::m_CommandQueue");

    THROW_HR_EXCEPT_IF_FAILED(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
    m_pFence->SetName(L"CommandQueueManager::m_pFence");
    m_pFence->Signal((uint64_t)m_Type << 56);

    m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
    ASSERT(m_FenceEventHandle != NULL);

    m_AllocatorPool.Initialize(pDevice);

    ASSERT(IsReady());
}

uint64_t CommandQueue::ExecuteCommandList( ID3D12CommandList* List )
{
    std::lock_guard<std::mutex> LockGuard(m_FenceMutex);

    THROW_HR_EXCEPT_IF_FAILED(((ID3D12GraphicsCommandList*)List)->Close());

    // Kickoff the command list
    m_CommandQueue->ExecuteCommandLists(1, &List);

    // Signal the next fence value (with the GPU)
    m_CommandQueue->Signal(m_pFence, m_NextFenceValue);

    // And increment the fence value.  
    return m_NextFenceValue++;
}

uint64_t CommandQueue::IncrementFence(void)
{
    std::lock_guard<std::mutex> LockGuard(m_FenceMutex);
    m_CommandQueue->Signal(m_pFence, m_NextFenceValue);
    return m_NextFenceValue++;
}

bool CommandQueue::IsFenceComplete(uint64_t FenceValue)
{
    // Avoid querying the fence value by testing against the last one seen.
    // The max() is to protect against an unlikely race condition that could cause the last
    // completed fence value to regress.
    if (FenceValue > m_LastCompletedFenceValue)
        m_LastCompletedFenceValue = (std::max)(m_LastCompletedFenceValue, m_pFence->GetCompletedValue());

    return FenceValue <= m_LastCompletedFenceValue;
}

void CommandQueue::StallForFence(uint64_t FenceValue)
{
    CommandQueue& Producer = GlobalVariable<CommandQueueManager>::Get()->GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    m_CommandQueue->Wait(Producer.m_pFence, FenceValue);
}

void CommandQueue::StallForProducer(CommandQueue& Producer)
{
    ASSERT(Producer.m_NextFenceValue > 0);
    m_CommandQueue->Wait(Producer.m_pFence, Producer.m_NextFenceValue - 1);
}

void CommandQueue::WaitForFence(uint64_t FenceValue)
{
    if (IsFenceComplete(FenceValue))
        return;

    // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
    // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
    // the fence can only have one event set on completion, then thread B has to wait for 
    // 100 before it knows 99 is ready.  Maybe insert sequential events?
    {
        std::lock_guard<std::mutex> LockGuard(m_EventMutex);

        m_pFence->SetEventOnCompletion(FenceValue, m_FenceEventHandle);
        WaitForSingleObject(m_FenceEventHandle, INFINITE);
        m_LastCompletedFenceValue = FenceValue;
    }
}

ID3D12CommandAllocator* CommandQueue::RequestAllocator()
{
    uint64_t CompletedFence = m_pFence->GetCompletedValue();

    return m_AllocatorPool.RequestAllocator(CompletedFence);
}

void CommandQueue::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
{
    m_AllocatorPool.DiscardAllocator(FenceValue, Allocator);
}


CommandQueueManager::CommandQueueManager() :
    m_Device(nullptr),
    m_GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
    m_ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
    m_CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY)
{
}

CommandQueueManager::~CommandQueueManager()
{
    Shutdown();
}
void CommandQueueManager::Initialize(ID3D12Device* pDevice)
{
    ASSERT(pDevice != nullptr);

    m_Device = pDevice;

    m_GraphicsQueue.Initialize(pDevice);
    m_ComputeQueue.Initialize(pDevice);
    m_CopyQueue.Initialize(pDevice);
}

void CommandQueueManager::Shutdown()
{
    m_GraphicsQueue.Shutdown();
    m_ComputeQueue.Shutdown();
    m_CopyQueue.Shutdown();
}

void CommandQueueManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList** ppCommandList, ID3D12CommandAllocator** ppAllocator)
{
    ASSERT(Type != D3D12_COMMAND_LIST_TYPE_BUNDLE && "Bundles are not yet supported");
    switch (Type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT: *ppAllocator = m_GraphicsQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE: *ppAllocator = m_ComputeQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_COPY: *ppAllocator = m_CopyQueue.RequestAllocator(); break;
    }

    THROW_HR_EXCEPT_IF_FAILED(m_Device->CreateCommandList(1, Type, *ppAllocator, nullptr, IID_PPV_ARGS(ppCommandList)));
    (*ppCommandList)->SetName(L"CommandList");
}

void CommandQueueManager::WaitForFence(uint64_t FenceValue)
{
    CommandQueue& Producer = GlobalVariable<CommandQueueManager>::Get()->GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    Producer.WaitForFence(FenceValue);
}
