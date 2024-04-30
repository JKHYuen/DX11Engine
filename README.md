# README DRAFT (INCOMPLETE):
Custom DX11 Engine

***Windows 64-bit required** 

![ss1](/data/archive/ss2.png)

## Feature highlights
- HDR physically based rendering (PBR) with image-based lighting (IBL)
	- Using Epic Game's version of the Cook-Torrance BRDF developed for Unreal 4 (https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf p. 1 - 8)
	- Linear space calculations with gamma correction
	- Reinhard-Jodie tonemapping (https://64.github.io/tonemapping/#reinhard-jodie)
	
- IBL environment maps (Irradiance map and Pre-Filtered map (specular)) generation from equirectangular .hdr files in runtime
	- Includes skybox generation and rendering
	- Skyboxes/environment maps can be loaded/switched during run time

- Bloom
	- Hardware progressive down and up sampling with box sampling

- Parallax occlusion mapping with optional self shadowing

- Directional light with shadow mapping
	- Simple 5x5 multisample PCF

- Object and triangle level frustrum culling
        - compatible with vertex dispalcement

- Tessellation with DX11 hull and domain shaders with two modes:
	- Basic uniform tessellation
	- Distance based edge tessellation

- UI for real time scene/material editing and debugging (with options to tweak all features above)
	- Togglable bloom filter, secondary cull camera, and shadow map views for debugging

## Controls
TAB: Open UI  
Controls below are listed in app UI tooltips:     

    WASD/Mouse: main camera movement 
    Left Shift: hold to move main camera faster
    ESC: quit app
    F: toggle fullscreen
    Z: toggle directional shadow map view
    X: toggle bloom prefilter view
    C: toggle second camera view for cull debugging
    F1: toggle on-screen FPS counter
    F2: toggle wireframe view
     
## Technical Limitations:
- Some outdated code practices to keep consistent with rastertek base 
	- Raw pointers used instead of COM smart pointers
	- Constructors, destructors and copy constructors are disabled
	- Some outdated DXGI (1.1) functions are used (e.g. using IDXGISwapChain::Present rather than IDXGISwapChain1::Present1)
	
- no dynamic shader linkage (lots of repeated code)

- Object Frustum culling does not take rotation into account

- no AA

- Vertical sync on by default

- no shadow culling

- bloom blur iterations hardcoded to log2(screen or window height)
- bloom flickering (see notes in TAB menu)

- no exclusive fullscreen![ss2](https://github.com/JKHYuen/DX11Engine/assets/53157428/0375ab4e-9be6-45a1-a498-8aaa90a31758)
 mode
	- alt-enter does not work
	- app can be toggled to windowed mode (hardcoded to 1280x720 resolution) or fullscreen windowed borderless

- Only directional light source, no point lights, spotlights, etc.

- Directional Light shadow map:
	- shadow map resolution is hardcoded to 2048x2048
	- simple 5x5 multisample PCF, no hardware filtering - flickering with finer detail shadows
	- no cascades
	- shadow distance is hardcoded
	- shadow map view does not follow main world camera
	
- Environment cubemaps used for image based lighting (IBL) are generated in runtime (not cached on disk)
- IBL Cubemap generation parameters are hardcoded to the following:
	- Cube face resolution: 2048x2048
	- Irradiance map resolution: 32x32
	- Prefiltered environment map: 512x512
	- Cube map mip levels (for PBR smoothness interpolation): 9
	- Precomputed BRDF map: 512x512
	
- Cursor not constrained to window
- Cursor does not loop when adjusting values in IMGUI
