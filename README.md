# C++ DX11 Game Engine

DX11 engine made from scratch as practice to familiarize with the Windows/Direct3D api and as preparation for a custom DX12 game engine (currently in development). This project is considered finished, features will be ported and expanded on in the DX12 engine. Implemented features mainly focus on graphics rendering techniques (forward rendering), but the intention is to eventually create a specialized game engine for my future projects. Source code is included for reference, project was built on Visual Studio 2022. Go to *releases* to try the playable build!

***Windows 64-bit required** 

![ss2](https://github.com/JKHYuen/DX11Engine/assets/53157428/0375ab4e-9be6-45a1-a498-8aaa90a31758)

## Feature Highlights
- HDR physically based rendering (PBR) with image-based lighting (IBL)
	- Using Epic Game's version of the Cook-Torrance BRDF developed for Unreal 4 ([link](https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf) p. 1 - 8)
	- Linear space calculations with gamma correction
	- Reinhard-Jodie tonemapping ([link](https://64.github.io/tonemapping/#reinhard-jodie))
- IBL environment maps (Irradiance and Pre-Filtered (specular) cubemap) generation from equirectangular .hdr files in runtime
	- Includes skybox cubemap generation and rendering
	- Skyboxes/environment maps can be loaded/switched during run time
- Bloom
	- Hardware progressive down and up sampling with box sampling
- Parallax occlusion mapping with optional self shadowing
- Directional light with shadow mapping
	- Simple 5x5 multisample PCF
- Object and triangle frustrum culling
        - compatible with vertex dispalcement
- Tessellation with DX11 hull and domain shaders with two modes:
	- Basic uniform tessellation
	- Distance based edge tessellation
- UI for real time scene/material editing and debugging (with options to tweak all features above)
	- Togglable bloom filter, secondary cull camera, and shadow map views for debugging

## Controls
TAB: Open UI  
  
*Hover over "(?)" and "(!)" icons in the TAB menu for useful/important tooltips!*
![ss3](https://github.com/JKHYuen/DX11Engine/assets/53157428/411f27d7-08d3-499a-8b23-639f01cdc2f6)

Controls below are listed in app UI tooltips:     

    WASD/Mouse: main camera movement 
    Left Shift: hold to move main camera faster
    ESC: quit app
    F:   toggle fullscreen
    Z:   toggle directional shadow map view
    X:   toggle bloom prefilter view
    C:   toggle second camera view for cull debugging
    F1:  toggle on-screen FPS counter
    F2:  toggle wireframe view
     
## Technical Limitations
*Most of these issues and systems will be addressed/improved on in a new DX12 engine project.*
- This project started from the Rastertek tutorial series ([link](https://rastertek.com/tutdx11win10.html) Tutorial 1 - 16), kept some outdated code practices to keep style consistent
	- Raw pointers used instead of COM smart pointers
	- Constructors, destructors and copy constructors are disabled
	- Some outdated DXGI (1.1) functions are used (e.g. using IDXGISwapChain::Present rather than IDXGISwapChain1::Present1)
- No dynamic shader linkage (some repeated code in shader classes)
	- Shader macros used for tessellation mode switching only, can be used in other places for performance gain (e.g. bloom implementation)
 	- No shader cache, all shaders are compiled every time app is booted
- Object and triangle Frustum culling does not take rotation into account
- No AA
- Vertical sync on by default
- No shadow culling
- Bloom blur iterations hardcoded to log2(screen or window height)
- Bloom flickering due to HDR rendering (see notes in TAB menu for quick fixes)
	- Unreal Engine 4 uses TAA to alleviate issue (not implemented in this project)
	- Additional notes in *Bloom.h*
- No exclusive fullscreen mode
	- Alt-enter will not work
	- App can be toggled to windowed mode (hardcoded to 1280x720 resolution) or fullscreen windowed borderless
- Only directional light source, no point lights, spotlights, etc.
	- Point lights were implemented at some point, but are commented out to simplify project
- Directional light shadow map:
	- Shadow map resolution is hardcoded to 2048x2048
	- Simple 5x5 multisample PCF, no hardware filtering - flickering with finer detail shadows
	- No shadow cascades
	- Shadow distance is hardcoded
	- Shadow map view is stationary (does not follow main world camera)
- Loaded environment cubemaps used for IBL are cached during runtime, but not cached on disk
- IBL Cubemap generation parameters are hardcoded to the following:
	- Cube face resolution: 2048x2048
	- Irradiance map resolution: 32x32
	- Prefiltered environment map: 512x512
	- Cube map mip levels (for PBR smoothness interpolation): 9
	- Precomputed BRDF map: 512x512
- No asset compression
- Parallax occlusion with self shadowing not very optimized (quality is tweakable)
	- Visual artifacts if used with a non-flat surface (therefore it should not be used with displacement mapping) 
- Cursor not constrained to window
- (QOL) Cursor does not loop when adjusting values in IMGUI

## Asset Sources
- https://polyhaven.com/hdris
- https://freepbr.com/

## Additional Resources Used
 - https://learnopengl.com/PBR/IBL/Diffuse-irradiance
 - https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
 - https://chanhaeng.blogspot.com/2019/01/normalparllax-mapping-with-self.html
 - https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/
 - https://catlikecoding.com/unity/tutorials/advanced-rendering/tessellation/
