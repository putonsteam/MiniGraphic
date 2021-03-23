# XFramework
DirectX 12,Tile-based deferred shading,Dynamic resolution,Order-independent transparency,PBR,SSR,HDR,Nvidia:HBAO,SMAA

OIT Grass Rendering:
1 PixelShader Link Compute AlphaBlend,Compute Shader total
![image](https://github.com/sevecol/XFramework/blob/master/grass.png)
2 Weighted Blended
![image](https://github.com/sevecol/XFramework/blob/master/grass_oitwb.png)

HDR:
1 ComputeShader Compute Luminance
2 PixelShader ToneMapping
![image](https://github.com/sevecol/XFramework/blob/master/hdr.png)

TiledBase DS:
Use ComputeShader cull light,shading
![image](https://github.com/sevecol/XFramework/blob/master/ds.png)
POM:
![image](https://github.com/sevecol/XFramework/blob/master/pom.png)

PBR:
![image](https://github.com/sevecol/XFramework/blob/master/PBR.png)

SMAA:
![image](https://github.com/sevecol/XFramework/blob/master/smaa.png)

SSR:
![image](https://github.com/sevecol/XFramework/blob/master/SSR.png)

RealTime GI,VoxelConeTracing:
![image](https://github.com/sevecol/XFramework/blob/master/VCT.png)
![image](https://github.com/sevecol/XFramework/blob/master/VCT1.png)
1:normal render
![image](https://github.com/sevecol/XFramework/blob/master/geometry.png)
2:voxel render
![image](https://github.com/sevecol/XFramework/blob/master/voxel.png)

AreaLighting:
![image](https://github.com/sevecol/XFramework/blob/master/arealighting1.png)
![image](https://github.com/sevecol/XFramework/blob/master/arealighting2.png)
with shadow,using voxel cone tracing compute shadow.
![image](https://github.com/sevecol/XFramework/blob/master/arealighting3.png)
http://jsfiddle.net/hh74z2ft/69/

Nvidia:HBAO,use nvidia's lib
![image](https://github.com/sevecol/XFramework/blob/master/HBAO.png)

Dof:
Blur:ComputeShader linear time GaussianBlur use box filter
![image](https://github.com/sevecol/XFramework/blob/master/dof.png)

MotionBlur:
CameraMotionBlur:
![image](https://github.com/sevecol/XFramework/blob/master/cameramotionblur.png)
ObjectMotionBlur:
![image](https://github.com/sevecol/XFramework/blob/master/entitymotionblur.png)

Sky:A Practical Analytic Model for Daylight
![image](https://github.com/sevecol/XFramework/blob/master/sky.png)

Scattering(volumetricLighting):whit shadowmap
![image](https://github.com/sevecol/XFramework/blob/master/scattering.png)

Ocean:
![image](https://github.com/sevecol/XFramework/blob/master/water.png)
