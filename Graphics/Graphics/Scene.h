#pragma once

#include "D3D12Header.h"
#include "CommandContext.h"


class Scene
{
public:


	void Render()
	{
		using namespace Graphics;
		Console::Print("Scene Render Begin\n");
		GraphicsContext& Context = GraphicsContext::Begin(L"Scene::Render");
		Context.SetViewportAndScissor(0, 0, g_DisplayWidth, g_DisplayHeight);
		Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer],D3D12_RESOURCE_STATE_RENDER_TARGET ,true);

		g_DisplayPlane[g_CurrentBuffer].SetClearColor({ 0.690196097f, 0.768627524f, 0.870588303f, 1.000000000f });
		Context.ClearColor(g_DisplayPlane[g_CurrentBuffer]);
		Context.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
		Context.ClearDepth(g_SceneDepthBuffer);
		D3D12_CPU_DESCRIPTOR_HANDLE RTVs[] =
		{
			g_DisplayPlane[g_CurrentBuffer].GetRTV()
		};
		Context.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_READ);
		Context.SetRenderTargets(_countof(RTVs), RTVs, g_SceneDepthBuffer.GetDSV_DepthReadOnly());
		Context.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);
		Context.Finish();
	}


private:
	
};

