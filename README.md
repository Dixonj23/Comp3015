# Ritual of the Three Lights
(fancy name)

A small interactive horror puzzle built using C++, OpenGL, shaders and GLFW. The player must control three spotlights and aim them at targets on a totally normal statue to complete the ritual.
Once all targets are activated, a supernatural sequence begins which culminates in something i'll spoil at the end of this document (in case you want to play it first).


# Developement Environment
This project was developed and tested using the following environment:
- IDE: Visual Studio 2022
- Operating System: Windows 11
- Graphics API: OpenGL 4.6
- Libraries Used:
    - GLFW (window/input handling)
    - GLAD (OpenGL loader)
    - stb_image (texture loading)
    - Custom helper classes provided with the coursework framework
The executable version included does not require Visual Studio and can be run directly


# Controls
| Key         | Action                 |
| ----------- | ---------------------- |
| W A S D     | Move camera/spotlight  |
| Q / E       | Adjust camera height   |
| C           | Toggle spotlight/camera|
| 1 / 2 / 3   | Switch Active Spotlight|
| ESC         | Exit Program           |


# Rendering Techniques
The prototype uses several real-time rendering techniques implemented using GLSL shaders.
- Spotlight Lighting:
    Each light uses a Blinn-Phong lighting model with a spotlight cutoff and exponent parameters.
    Utilising directional spotlight cones with adjustable intensity and specular highlights.
- Normal Mapping
    Normal maps are used to enhance surface detail on various models without increasing geometric complexity.
    This is umplemented using TBN matrices constructed in the vertex shader and used in the fragment shader.
- Fog Rendering
    Atmospheric fog is applied based on fragment depth, where the final color is blended with the fog color to create a dense atmosphere
- Skybox Rendering
    Siz images come together into a cubemap skybox which surrounds the environemnt to simulate distant lighting and a space like atmosphere.
    The skybox is rendered first with depth writes disabled, then the rest of the scen is rendered normally.
- Procedural Texture Corruption (Research paper inspired)
    When a target is solved, a corruption texture spreads outward from that target point over time.
    This effect is inspired by procedural texture blending techniques described in:
    Dong, J.; Liu, J.; Yao, K.; Chantler, M.; Qi, L.; Yu, H.; Jian, M. Survey of Procedural Methods for Two-Dimensional Texture Generation. Sensors 2020, 20, 1135. https://doi.org/10.3390/s20041135


# Code Structure
The project is organised into several key components:
- Scenebasuc_uniform.ccp
  This is the main scene controller and contains most gameplay logic. It is responsible got handling input, updating camera movement, controlling spotlight direction, managing gameplay states and rendering the overall scene.

There are two main shaders that manage everything
- basic_uniform.vert
  The vertex shader handles world and view space calculations as well as building the aformentioned TBN matrix for normal mapping.
  After which this data is passed to the fragment shader
- basic_uniform.frag
  THe fragment shader performs the lighting calculations while applying the rendering techniques such as normal mapping and fog.
  It also handles the real time rendering of the skybox and the corruption spreading texture blend.


# Assets
Assets used include:
- 3D models for statue and ritual object from sketchfab
- A 3D model for the creature, provided in the coursework template
- Texture for surfaces and normal maps, found with their accompanying models
- Skybox cubemap


# How the Prototype works
The puzzle has a certain flow:
1. The player begins on a platform with a mystery statue in an unknown realm of both light and dark
2. The player can switch between an orbiting camera and three distinct spotlights using keyboard controls
3. Switching to each spotlight causes a specific point on the statue to glow
4. When the correct alignment is achieved:
    - the spotlight turns purple
    - a corruption effect slowly spreads, revealing the details under the stone
5. Once all three targets are solved:
   - A ritual object emerges from the portal into the scene
   - The object spins while the the spotlights go red
6. A jumpscare effect triggers, with a creatue model rapidly approaching the camera
