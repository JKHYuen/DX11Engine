DRAFT (INCOMPLETE):

CONTROLS:
TAB - menu
ESC - quit app

Technical Limitations:
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

- no exclusive fullscreen mode
	- alt-enter does not work properly
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
- (QOL) Cursor does not loop when adjusting values in IMGUI
