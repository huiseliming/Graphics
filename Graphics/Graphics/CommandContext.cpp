#include "CommandContext.h"


ContextManager CommandContext::sm_ContextManager;

CommandContext* ContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE Type)
{
    std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);
    auto& AvailableContexts = sm_AvailableContexts[Type];

    CommandContext* ret = nullptr;
    if (AvailableContexts.empty())
    {
        ret = new CommandContext(Type);
        sm_ContextPool->emplace_back(ret);
        ret->Initialize();
    }
    else
    {
        ret = AvailableContexts.front();
        AvailableContexts.pop();
        ret->Reset();
    }
    ASSERT(ret != nullptr);
    return ret;
}

void ContextManager::FreeContext(CommandContext* UsedContext)
{
    ASSERT(UsedContext != nullptr);
    std::lock_guard<std::mutex> LockGuard(sm_ContextAllocationMutex);
    sm_AvailableContexts[UsedContext->m_Type].push(UsedContext);
}

void ContextManager::DestroyAllContexts()
{
    for (uint32_t i = 0; i < 4; ++i)
        sm_ContextPool[i].clear();
}

CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE Type) :
    m_Type(Type),
    m_DynamicViewDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
    m_DynamicSamplerDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
    m_CpuLinearAllocator(kCpuWritable),
    m_GpuLinearAllocator(kGpuExclusive)
{
    m_CommandList = nullptr;
    m_CommandAllocator = nullptr;
    ZeroMemory(m_DescriptorHeaps, sizeof(m_DescriptorHeaps));
    m_RootSignature = nullptr;
    m_PipelineState = nullptr;
    m_NumBarriersToFlush = 0;
}

void CommandContext::Initialize(void)
{
    GlobalVariable<CommandQueueManager>::Get()->CreateNewCommandList(m_Type, &m_CommandList, &m_CommandAllocator);
}

void CommandContext::Reset(void)
{
    // We only call Reset() on previously freed contexts.  The command list persists, but we must
    // request a new allocator.
    ASSERT(m_CommandList != nullptr && m_CommandAllocator == nullptr);
    m_CommandAllocator = GlobalVariable<CommandQueueManager>::Get()->GetQueue(m_Type).RequestAllocator();
    m_CommandList->Reset(m_CommandAllocator, nullptr);

    m_RootSignature = nullptr;
    m_PipelineState = nullptr;
    m_NumBarriersToFlush = 0;
    BindDescriptorHeaps();
}

CommandContext::~CommandContext(void)
{
    if (m_CommandList != nullptr)
        m_CommandList->Release();
}

uint64_t CommandContext::Flush(bool WaitForCompletion)
{
    FlushResourceBarriers();
    uint64_t FenceValue = GlobalVariable<CommandQueueManager>::Get()->GetQueue(m_Type).ExecuteCommandList(m_CommandList);

    if (WaitForCompletion)
        GlobalVariable<CommandQueueManager>::Get()->WaitForFence(FenceValue);
    m_CommandList->Reset(m_CommandAllocator, nullptr);
    if (m_RootSignature)
        m_CommandList->SetGraphicsRootSignature(m_RootSignature);
    if (m_PipelineState)
        m_CommandList->SetPipelineState(m_PipelineState);
    BindDescriptorHeaps();
    return FenceValue;
}


uint64_t CommandContext::Finish(bool WaitForCompletion)
{
    ASSERT(m_Type == D3D12_COMMAND_LIST_TYPE_DIRECT || m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE);
    FlushResourceBarriers();

    ASSERT(m_CommandAllocator != nullptr);

    CommandQueue& Queue = GlobalVariable<CommandQueueManager>::Get()->GetQueue(m_Type);

    uint64_t FenceValue = Queue.ExecuteCommandList(m_CommandList);
    Queue.DiscardAllocator(FenceValue, m_CommandAllocator);
    m_CommandAllocator = nullptr;

    m_CpuLinearAllocator.CleanupUsedPages(FenceValue);
    m_GpuLinearAllocator.CleanupUsedPages(FenceValue);
    m_DynamicViewDescriptorHeap.CleanupUsedHeaps(FenceValue);
    m_DynamicSamplerDescriptorHeap.CleanupUsedHeaps(FenceValue);

    if (WaitForCompletion)
        GlobalVariable<CommandQueueManager>::Get()->WaitForFence(FenceValue);

    sm_ContextManager.FreeContext(this);

    return FenceValue;
}

void CommandContext::BindDescriptorHeaps(void)
{
    UINT NonNullHeaps = 0;
    ID3D12DescriptorHeap* HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        ID3D12DescriptorHeap* HeapIter = m_DescriptorHeaps[i];
        if (HeapIter != nullptr)
            HeapsToBind[NonNullHeaps++] = HeapIter;
    }

    if (NonNullHeaps > 0)
        m_CommandList->SetDescriptorHeaps(NonNullHeaps, HeapsToBind);
}

void CommandContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr)
{
    if (m_DescriptorHeaps[Type] != HeapPtr)
    {
        m_DescriptorHeaps[Type] = HeapPtr;
        BindDescriptorHeaps();
    }
}

void CommandContext::SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[])
{
    bool AnyChanged = false;
    for (UINT i = 0; i < HeapCount; ++i)
    {
        if (m_DescriptorHeaps[Type[i]] != HeapPtrs[i])
        {
            m_DescriptorHeaps[Type[i]] = HeapPtrs[i];
            AnyChanged = true;
        }
    }
    if (AnyChanged)
        BindDescriptorHeaps();
}
