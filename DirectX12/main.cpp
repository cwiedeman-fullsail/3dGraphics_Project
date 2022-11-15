// Simple basecode showing how to create a window and attatch a d3d12surface
#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries
#define GATEWARE_ENABLE_MATH
#define GATEWARE_ENABLE_INPUT
#define GATEWARE_ENABLE_AUDIO
// Ignore some GRAPHICS libraries we aren't going to use
#define GATEWARE_DISABLE_GDIRECTX11SURFACE // we have another template for this
#define GATEWARE_DISABLE_GOPENGLSURFACE // we have another template for this
#define GATEWARE_DISABLE_GVULKANSURFACE // we have another template for this
#define GATEWARE_DISABLE_GRASTERSURFACE // we have another template for this
// With what we want & what we don't defined we can include the API
#include "../Gateware/Gateware.h"
#include <chrono>
#include "renderer.h"



// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;
//bool changeLevel = false;
bool renderBoundries = false;

// lets pop a window and use D3D12 to clear to a jade colored screen
int main()
{
	GWindow win;
	GEventResponder msgs;
	GDirectX12Surface d3d12;
	if (+win.Create(0, 0, 800, 600, GWindowStyle::WINDOWEDBORDERED))
	{
		float clr[] = { 135.0f / 256.0f, 206.0f / 256.0f, 235.0f / 256.0f, 1.0f }; // start with a jade color
		msgs.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
		//if (+e.Read(q) && q == GWindow::Events::RESIZE)
		//	clr[0] += 0.01f; // move towards a orange as they resize
			});
		win.Register(msgs);
		if (+d3d12.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		{
			win.SetWindowName("Christopher Wiedeman - Level Renderer - D3D12");
			Renderer renderer(win, d3d12); // init
			while (+win.ProcessWindowEvents())
			{
				if (+d3d12.StartFrame())
				{
					ID3D12GraphicsCommandList* cmd;
					D3D12_CPU_DESCRIPTOR_HANDLE rtv;
					D3D12_CPU_DESCRIPTOR_HANDLE dsv;
					if (+d3d12.GetCommandList((void**)&cmd) &&
						+d3d12.GetCurrentRenderTargetView((void**)&rtv) &&
						+d3d12.GetDepthStencilView((void**)&dsv))
					{
						cmd->ClearRenderTargetView(rtv, clr, 0, nullptr);
						cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);
						renderer.UpdateCamera(&renderBoundries);
						renderer.Render();
						renderer.RenderMiniMap();// draw
						if (renderBoundries)
						{
							renderer.RenderBounds();
						}
						d3d12.EndFrame(false);
						cmd->Release();
					}
				}
			}// clean-up when renderer falls off stack
		}
	}
	return 0; // that's all folks
}