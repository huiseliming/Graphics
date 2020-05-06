#pragma once
#include "D3D12Header.h"
#include "CommandQueueManager.h"
#include "DynamicDescriptorHeap.h"
#include "LinearAllocator.h"
#include "PipelineState.h"
#include "GpuBuffer.h"
#include "DepthBuffer.h"
#include "ColorBuffer.h"


class CommandContext;

struct DWParam
{
    DWParam(FLOAT f) : Float(f) {}
    DWParam(UINT u) : Uint(u) {}
    DWParam(INT i) : Int(i) {}

    void operator= (FLOAT f) { Float = f; }
    void operator= (UINT u) { Uint = u; }
    void operator= (INT i) { Int = i; }

    union
    {
        FLOAT Float;
        UINT Uint;
        INT Int;
    };
};

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

class ContextManager
{
public:
    ContextManager(void) {}

    CommandContext* AllocateContext(D3D12_COMMAND_LIST_TYPE Type);

    void FreeContext(CommandContext* UsedContext);

    void DestroyAllContexts();

private:
    std::vector<std::unique_ptr<CommandContext> > sm_ContextPool[4];
    std::queue<CommandContext*> sm_AvailableContexts[4];
    std::mutex sm_ContextAllocationMutex;
};

struct NonCopyable
{
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

class CommandContext : NonCopyable
{
    friend ContextManager;
private:
    CommandContext(D3D12_COMMAND_LIST_TYPE Type);

    // 初始化Context
    void Initialize(void);

    // 重用Context
    void Reset(void);

public:
    ~CommandContext(void);

    static CommandContext& Begin(const std::wstring ID = L"")
    {
        CommandContext* NewContext = sm_ContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
        //NewContext->SetID(ID);
        //if (ID.length() > 0)
            //EngineProfiling::BeginBlock(ID, NewContext);
        return *NewContext;
    }

    static void DestroyAllContexts(void)
    {
        LinearAllocator::DestroyAll();
        DynamicDescriptorHeap::DestroyAll();
        sm_ContextManager.DestroyAllContexts();
    }
    // 将当前已记录的命令发送给GPU执行，但不释放Context
    uint64_t Flush(bool WaitForCompletion = false);

    // 将当前已记录的命令发送给GPU执行，释放Context
    uint64_t Finish(bool WaitForCompletion = false);

    ID3D12GraphicsCommandList* GetCommandList() { return m_CommandList; }

    //void CopyBuffer(GpuResource& Dest, GpuResource& Src);
    //void CopyBufferRegion(GpuResource& Dest, size_t DestOffset, GpuResource& Src, size_t SrcOffset, size_t NumBytes);
    //void CopySubresource(GpuResource& Dest, UINT DestSubIndex, GpuResource& Src, UINT SrcSubIndex);
    //void CopyCounter(GpuResource& Dest, size_t DestOffset, StructuredBuffer& Src);
    //void ResetCounter(StructuredBuffer& Buf, uint32_t Value = 0);

    DynAlloc ReserveUploadMemory(size_t SizeInBytes) { return m_CpuLinearAllocator.Allocate(SizeInBytes); }

    //static void InitializeTexture(GpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
    static void InitializeBuffer(GpuResource& Dest, const void* BufferData, size_t NumBytes, size_t Offset = 0)
    {
        CommandContext& InitContext = CommandContext::Begin();

        DynAlloc mem = InitContext.ReserveUploadMemory(NumBytes);
        SIMDMemCopy(mem.DataPtr, BufferData, Math::DivideByMultiple(NumBytes, 16));

        // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
        InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
        //这里有一处待测试错误,DynAlloc应该要传入偏移
        //InitContext.m_CommandList->CopyBufferRegion(Dest.GetResource(), Offset, mem.Buffer.GetResource(), mem.Offset, NumBytes);
        InitContext.m_CommandList->CopyBufferRegion(Dest.GetResource(), Offset, mem.Buffer.GetResource(), 0, NumBytes);
        InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

        // Execute the command list and wait for it to finish so we can release the upload buffer
        InitContext.Finish(true);
    }
    //static void InitializeTextureArraySlice(GpuResource& Dest, UINT SliceIndex, GpuResource& Src);
    //static void ReadbackTexture2D(GpuResource& ReadbackBuffer, PixelBuffer& SrcBuffer);

    //void WriteBuffer(GpuResource& Dest, size_t DestOffset, const void* Data, size_t NumBytes);
    //void FillBuffer(GpuResource& Dest, size_t DestOffset, DWParam Value, size_t NumBytes);

    void TransitionResource(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false)
    {
        D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

        if (m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
        {
            ASSERT((OldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == OldState);
            ASSERT((NewState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == NewState);
        }

        if (OldState != NewState)
        {
            ASSERT(m_NumBarriersToFlush < 16 && "Exceeded arbitrary limit on buffered barriers");
            D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

            BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            BarrierDesc.Transition.pResource = Resource.GetResource();
            BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            BarrierDesc.Transition.StateBefore = OldState;
            BarrierDesc.Transition.StateAfter = NewState;

            // Check to see if we already started the transition
            if (NewState == Resource.m_TransitioningState)
            {
                BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
                Resource.m_TransitioningState = (D3D12_RESOURCE_STATES)-1;
            }
            else
                BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

            Resource.m_UsageState = NewState;
        }
        else if (NewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            InsertUAVBarrier(Resource, FlushImmediate);

        if (FlushImmediate || m_NumBarriersToFlush == 16)
            FlushResourceBarriers();
    }
    void BeginResourceTransition(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false)
    {
        // If it's already transitioning, finish that transition
        if (Resource.m_TransitioningState != (D3D12_RESOURCE_STATES)-1)
            TransitionResource(Resource, Resource.m_TransitioningState);

        D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

        if (OldState != NewState)
        {
            ASSERT(m_NumBarriersToFlush < 16 && "Exceeded arbitrary limit on buffered barriers");
            D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

            BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            BarrierDesc.Transition.pResource = Resource.GetResource();
            BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            BarrierDesc.Transition.StateBefore = OldState;
            BarrierDesc.Transition.StateAfter = NewState;

            BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

            Resource.m_TransitioningState = NewState;
        }

        if (FlushImmediate || m_NumBarriersToFlush == 16)
            FlushResourceBarriers();
    }
    void InsertUAVBarrier(GpuResource& Resource, bool FlushImmediate = false)
    {
        ASSERT(m_NumBarriersToFlush < 16 && "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        BarrierDesc.UAV.pResource = Resource.GetResource();

        if (FlushImmediate)
            FlushResourceBarriers();
    }
    void InsertAliasBarrier(GpuResource& Before, GpuResource& After, bool FlushImmediate = false)
    {
        ASSERT(m_NumBarriersToFlush < 16 && "Exceeded arbitrary limit on buffered barriers");
        D3D12_RESOURCE_BARRIER& BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

        BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        BarrierDesc.Aliasing.pResourceBefore = Before.GetResource();
        BarrierDesc.Aliasing.pResourceAfter = After.GetResource();

        if (FlushImmediate)
            FlushResourceBarriers();
    }
    void FlushResourceBarriers(void)
    {
        if (m_NumBarriersToFlush > 0)
        {
            m_CommandList->ResourceBarrier(m_NumBarriersToFlush, m_ResourceBarrierBuffer);
            m_NumBarriersToFlush = 0;
        }
    }



    void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr);
    void SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[]);
    void SetPipelineState(const PSO& PSO) 
    { 
        ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
        if (PipelineState == m_PipelineState)
            return;

        m_CommandList->SetPipelineState(PipelineState);
        m_PipelineState = PipelineState;
    }    
    
    //void SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);


protected:
    void BindDescriptorHeaps(void);

    static ContextManager sm_ContextManager;
    //本命令列表的类型
    D3D12_COMMAND_LIST_TYPE m_Type;
    // 命令列表
    ID3D12GraphicsCommandList* m_CommandList;
    ID3D12CommandAllocator* m_CommandAllocator;
    // 根描述符
    ID3D12RootSignature* m_RootSignature;
    // PSO
    ID3D12PipelineState* m_PipelineState;
    // 缓存资源屏障
    D3D12_RESOURCE_BARRIER m_ResourceBarrierBuffer[16];
    UINT m_NumBarriersToFlush;
    // 描述符堆 引用
    ID3D12DescriptorHeap* m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    // CBV,SRV,UAV 描述符堆 达到围栏回收
    DynamicDescriptorHeap m_DynamicViewDescriptorHeap;        // HEAP_TYPE_CBV_SRV_UAV
    DynamicDescriptorHeap m_DynamicSamplerDescriptorHeap;    // HEAP_TYPE_SAMPLER
    // 线性内存申请器 达到围栏回收
    LinearAllocator m_CpuLinearAllocator;
    LinearAllocator m_GpuLinearAllocator;
};







class GraphicsContext : public CommandContext
{
public:
    static GraphicsContext& Begin(const std::wstring& ID = L"")
    {
        return reinterpret_cast<GraphicsContext&>(CommandContext::Begin(ID));
    }

    void ClearUAV(GpuBuffer& Target)
    {
        // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
        // a shader to set all of the values).
        D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
        const UINT ClearColor[4] = {};
        m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 0, nullptr);
    }
    void ClearUAV(ColorBuffer& Target)
    {
        // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
        // a shader to set all of the values).
        D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
        CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

        //TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
        const float* ClearColor = Target.GetClearColor().GetPtr();
        m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor, 1, &ClearRect);

    }

    void ClearColor(ColorBuffer& Target)
    {
        m_CommandList->ClearRenderTargetView(Target.GetRTV(), Target.GetClearColor().GetPtr(), 0, nullptr);
    }

    void ClearDepth(DepthBuffer& Target)
    {
        m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
    }

    void ClearStencil(DepthBuffer& Target)
    {
        m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
    }

    void ClearDepthAndStencil(DepthBuffer& Target)
    {
        m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
    }

    void BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
    {
        m_CommandList->BeginQuery(QueryHeap, Type, HeapIndex);
    }

    void EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
    {
        m_CommandList->EndQuery(QueryHeap, Type, HeapIndex);
    }

    void ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset)
    {
        m_CommandList->ResolveQueryData(QueryHeap, Type, StartIndex, NumQueries, DestinationBuffer, DestinationBufferOffset);
    }

    void SetRootSignature(const RootSignature& RootSig)
    {
        if (RootSig.GetSignature() == m_RootSignature)
            return;

        m_CommandList->SetGraphicsRootSignature(m_RootSignature = RootSig.GetSignature());

        m_DynamicViewDescriptorHeap.ParseGraphicsRootSignature(RootSig);
        m_DynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(RootSig);
    }

    void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]) { m_CommandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, nullptr); }
    void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV) { m_CommandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, &DSV); }
    void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV) { SetRenderTargets(1, &RTV); }
    void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(1, &RTV, DSV); }
    void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(0, nullptr, DSV); }

    void SetViewport(const D3D12_VIEWPORT& vp)
    { 
        m_CommandList->RSSetViewports(1, &vp); 
    }

    void SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f)
    {
        D3D12_VIEWPORT vp;
        vp.Width = w;
        vp.Height = h;
        vp.MinDepth = minDepth;
        vp.MaxDepth = maxDepth;
        vp.TopLeftX = x;
        vp.TopLeftY = y;
        m_CommandList->RSSetViewports(1, &vp);
    }

    void SetScissor(const D3D12_RECT& rect)
    {
        ASSERT(rect.left < rect.right&& rect.top < rect.bottom);
        m_CommandList->RSSetScissorRects(1, &rect);
    }

    void SetScissor(UINT left, UINT top, UINT right, UINT bottom) 
    { 
        SetScissor(CD3DX12_RECT(left, top, right, bottom));
    }

    void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
    {
        ASSERT(rect.left < rect.right&& rect.top < rect.bottom);
        m_CommandList->RSSetViewports(1, &vp);
        m_CommandList->RSSetScissorRects(1, &rect);
    }

    void SetViewportAndScissor(UINT x, UINT y, UINT w, UINT h)
    {
        SetViewport((float)x, (float)y, (float)w, (float)h);
        SetScissor(x, y, x + w, y + h);
    }

    void SetStencilRef(UINT StencilRef) 
    { 
        m_CommandList->OMSetStencilRef(StencilRef); 
    }

    //void SetBlendFactor(Color BlendFactor);

    void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology) 
    { 
        m_CommandList->IASetPrimitiveTopology(Topology); 
    }

    void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants) 
    {
        m_CommandList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
    }

    void SetConstant(UINT RootIndex, DWParam Val, UINT Offset = 0) 
    { 
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Val.Uint, Offset); 
    }

    void SetConstants(UINT RootIndex, DWParam X)
    {
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
    }

    void SetConstants(UINT RootIndex, DWParam X, DWParam Y)
    {
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
    }

    void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
    {
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
    }

    void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
    {
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
        m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, W.Uint, 3);
    }

    void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
    {
        m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, CBV);
    }

    void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
        DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
        SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
        memcpy(cb.DataPtr, BufferData, BufferSize);
        m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress);
    }

    //void SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset = 0);
    //void SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset = 0);

    void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
    {
        m_CommandList->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
    }

    void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle) { SetDynamicDescriptors(RootIndex, Offset, 1, &Handle); }
    void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]) { m_DynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles); }
    void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle) { SetDynamicSamplers(RootIndex, Offset, 1, &Handle); }
    void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]) { m_DynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles); }

    void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView) { m_CommandList->IASetIndexBuffer(&IBView); }
    void SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView) { SetVertexBuffers(Slot, 1, &VBView); }
    void SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[]) { m_CommandList->IASetVertexBuffers(StartSlot, Count, VBViews); }
    void SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VertexData)
    {
        ASSERT(VertexData != nullptr && Math::IsAligned(VertexData, 16));

        size_t BufferSize = Math::AlignUp(NumVertices * VertexStride, 16);
        DynAlloc vb = m_CpuLinearAllocator.Allocate(BufferSize);

        SIMDMemCopy(vb.DataPtr, VertexData, BufferSize >> 4);

        D3D12_VERTEX_BUFFER_VIEW VBView;
        VBView.BufferLocation = vb.GpuAddress;
        VBView.SizeInBytes = (UINT)BufferSize;
        VBView.StrideInBytes = (UINT)VertexStride;

        m_CommandList->IASetVertexBuffers(Slot, 1, &VBView);
    }

    void SetDynamicIB(size_t IndexCount, const uint16_t* IndexData)
    {
        ASSERT(IndexData != nullptr && Math::IsAligned(IndexData, 16));

        size_t BufferSize = Math::AlignUp(IndexCount * sizeof(uint16_t), 16);
        DynAlloc ib = m_CpuLinearAllocator.Allocate(BufferSize);

        SIMDMemCopy(ib.DataPtr, IndexData, BufferSize >> 4);

        D3D12_INDEX_BUFFER_VIEW IBView;
        IBView.BufferLocation = ib.GpuAddress;
        IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
        IBView.Format = DXGI_FORMAT_R16_UINT;

        m_CommandList->IASetIndexBuffer(&IBView);
    }

    void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
    {
        ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
        DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
        SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
        m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, cb.GpuAddress);
    }

    void Draw(UINT VertexCount, UINT VertexStartOffset = 0)
    {
        DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
    }

    void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0)
    {
        DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
    }

    void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation = 0, UINT StartInstanceLocation = 0)
    {
        FlushResourceBarriers();
        m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
        m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
        m_CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    }

    void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
    {
        FlushResourceBarriers();
        m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
        m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
        m_CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    }

    //void DrawIndirect(GpuBuffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0);
    //void ExecuteIndirect(CommandSignature& CommandSig, GpuBuffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0, uint32_t MaxCommands = 1, GpuBuffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);

private:


};














