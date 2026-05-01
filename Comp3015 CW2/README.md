# COMP3015 CW2 � Reactor Beam Puzzle

## Project Overview

This project is an OpenGL/GLSL real-time graphics prototype built for COMP3015 Coursework 2.
The scene is a procedural reactor-room puzzle where the player must rotate mirrors to redirect two
energy beams into a central reactor. The reactor only activates when both emitter beams reach the
target at the same time.

The project combines gameplay interaction with several graphics programming techniques, including
point-light shadow mapping, bloom post-processing, normal/metalness/emissive texture use, animated beam effects,
and a CPU-driven particle system.

## How to Run

Open the Visual Studio solution:

Project_Template.sln

Or run the EXE found in:

## Controls

| Input         | Action                             |
| ------------- | ---------------------------------- |
| W / A / S / D | Move player                        |
| Mouse         | Look around                        |
| Q / E         | Rotate selected mirror             |
| R             | Reset / generate new puzzle layout |

The player selects a mirror by looking at it. The HUD displays the current target and reactor status.

## Gameplay Description

The player starts near the reactor in a procedurally generated room.
Two emitters fire energy beams into the scene. Mirrors can be rotated to redirect the beams.
Both beams must reach the reactor for it to activate.

The layout is generated with guaranteed solvable beam paths. The system first creates
valid emitter-to-reactor paths with two mirrors, then places additional mirrors and obstacles
while avoiding the protected solution corridors. This keeps the puzzle playable while still allowing variation
between resets.

# Main Features

## Procedural Puzzle Generation

The scene uses a procedural layout generation to place emitters, mirrors, and obstacles. The generator
creates at least two valid beam paths before placing these obstacles. This prevents obstacles from blocking
the required solution routes.

Relevant functions:

generateSolvableLayout()
tryCreateSolutionRoute()
generateObstacles()
generateMirrors()
isNearBeamPath()

## Beam Reflection System

Each beam is ray-traced through the scene. When a beam hits a mirror, the reflection direction is calculated
using the mirror normal and glm::reflect. The beam can bounce between mirrors before either reaching the reactor,
hitting an obstacle, or leaving the valid area.

Relevant functions:

drawAllBeamPaths()
drawBeamPathFromEmitter()
rayHitsMirror()
findClosestHitMirror()
rayHitsReactorBeforeDistance()

## Point-Light Cubemap Shadows

The reactor light uses omnidirectional shadow mapping. A depth cubemap is rendered from the reactor light position,
allowing objects to cast shadows in multiple directions.

This replaced an earlier single 2D shadow-map approach, which could only represent shadows from one direction.
The cubemap method better matches the idea of a central reactor light.

Relevant shaders:

point_shadow_depth.vert
point_shadow_depth.geom
point_shadow_depth.frag

Relevant functions:

setupPointShadowMap()
buildPointShadowTransforms()
renderPointShadowPass()
renderShadowGeometry()

## Bloom / HDR Post-Processing

Bright emissive elements such as the beams, reactor core, particles, and light fixtures are extracted into a
brightness texture. This texture is blurred and composited back over the scene to create a bloom effect.

Relevant shaders:

bloom_blur.vert
bloom_blur.frag
bloom_final.vert
bloom_final.frag

## Textures and Material Maps

The scene uses texture maps for the floor, walls, ceiling, emitters, mirrors, reactor, and obstacles.
The fragment shader supports:

- diffuse/albedo textures
- normal maps
- metalness maps
- emissive maps

Because the provided OBJ loader does not support .mtl material groups, each model uses one manually assigned texture set.

## Reactor Particle System

The reactor has a CPU-driven particle system. Particles rise from the reactor and change colour/intensity based on whether
the reactor is active. When powered, the reactor becomes brighter and more visually energetic.

Relevant functions:

initReactorParticles()
updateReactorParticles()
respawnReactorParticle()
drawReactorParticles()
spawnReactorBurst()

## HUD and Interaction Feedback

The HUD displays reactor progress, the selected mirror target, and control hints.
A reset prompt appears after the reactor has been active for a few seconds.

## Shader Summary

The main fragment shader combines:

- textured diffuse colour
- normal mapping
- metalness-influenced specular response
- emissive texture contribution
- point-light cubemap shadows
- bloom extraction

The beam visual effect uses a simple animated pattern to create energy movement and flicker.

## What i started with

The project began from the COMP3015 OpenGL template and elements from my first coursework project.
The original scene was stripped down to a simple plane and then rebuilt into this new interactive puzzle prototype.

The final project adds substantial new work, including:

- first-person movement and collision
- procedural puzzle generation
- beam reflection gameplay
- two-emitter reactor activation logic
- point-light cubemap shadows
- bloom
- particles
- HUD
- better imported models and texture maps

## Problems solved during development

Several technical issues were solved during development:

- aligning beam reflection correctly with mirror normals
- supporting mirror-to-mirror reflection
- ensuring all beams could activate the reactor
- changing the reactor activation rule so both beams are required
- replacing single-direction shadows with point-light cubemap shadows
- preventing generated obstacles from blocking guaranteed solution paths
- fixing texture state warnings by using fallback textures
- adapting model orientation and scale to match gameplay collision proxies

## Evaluation

I think the strongest part of my project is the combination of graphics techniques with a small gameplay loop.
The shaders are not only visual effects; they support the puzzle feedback and make the reactor feel like the centre
of the scene, with the procedural generation making the prototype more replayable than a fixed room layout.

If I had more time, I would improve the OBJ material system so that models could use multiple materials from .mtl files.
I would also add more robust puzzle validation, more room themes, and smoother particle rendering using billboards instead
of small cube particles. I think i'd make each room more difficult as the player progresses, adding more elements such as
more emitters and new features like a beam splitter or requiring the beam to bounce a certain number of times before hitting
the reactor.

# External Assets / Sources

- COMP3015 OpenGL template code
- stb_easy_font for simple HUD text
- Reactor model: https://sketchfab.com/3d-models/scifi-reactor-core-b3fe00d6b73841a0b6b3a288efc03668
- Emitter model: https://sketchfab.com/3d-models/emitter-72e729e10d2e427ab1fd49703da454a5
- Barrel model: https://sketchfab.com/3d-models/scifi-trianglecrate-a5457e58eec14317a8f58cace8218570
- Crate model: https://www.fab.com/listings/60eb5b77-32b2-4af2-805f-a30565b4fc68
- Mirror model: https://sketchfab.com/3d-models/mirror-9-mb-755d7092bb1049efa92d5aaa47e06ffd
- Scifi textures: https://www.fab.com/listings/4a8e3702-559a-45db-be22-16ef27a4ff5a

## Video Link

https://youtu.be/VlQ7exHud84

## Github Link

https://github.com/Dixonj23/Comp3015
