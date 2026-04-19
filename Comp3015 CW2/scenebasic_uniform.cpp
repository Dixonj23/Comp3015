#include "scenebasic_uniform.h"

#include <cstdlib>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "helper/glutils.h"
#include "helper/texture.h"
#include <GLFW/glfw3.h>

using std::cerr;
using std::endl;

using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;

SceneBasic_Uniform::SceneBasic_Uniform()
    : plane(30.0f, 30.0f, 1, 1),
    cube(),
    tPrev(0.0f),
    cameraPos(0.0f, 1.8f, 12.0f),
    cameraFront(0.0f, 0.0f, -1.0f),
    cameraUp(0.0f, 1.0f, 0.0f),
    cameraRight(1.0f, 0.0f, 0.0f),
    worldUp(0.0f, 1.0f, 0.0f),
    yaw(-90.0f),
    pitch(0.0f),
    moveSpeed(5.0f),
    mouseSensitivity(0.1f),
    keyW(false), keyA(false), keyS(false), keyD(false), keyQ(false), keyE(false),
    firstMouse(true),
    lastMouseX(0.0f),
    lastMouseY(0.0f),
    selectedMirrorIndex(-1),
    reactorActivated(false),
    hdrFBO(0),
    rboDepth(0),
    quadVAO(0),
    quadVBO(0),
    bloomEnabled(true),
    bloomExposure(0.6f)
{
}

void SceneBasic_Uniform::initScene()
{
    compile();
    glEnable(GL_DEPTH_TEST);

    model = mat4(1.0f);
    projection = mat4(1.0f);

    planeTex = Texture::loadTexture("media/texture/brick1.jpg");
    planeNormal = Texture::loadTexture("media/texture/ogre_normalmap.png");

    updateCameraVectors();
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    prog.use();
    prog.setUniform("baseTexColor1", 0);
    prog.setUniform("NormalMapTex", 1);
    prog.setUniform("UseTexture", 1);
    prog.setUniform("SolidColor", glm::vec3(1.0f));

    prog.setUniform("Light.Position", vec4(0.0f, 8.0f, 6.0f, 1.0f));
    prog.setUniform("Light.La", vec3(0.2f, 0.2f, 0.2f));
    prog.setUniform("Light.Ld", vec3(0.9f, 0.9f, 0.9f));
    prog.setUniform("Light.Ls", vec3(1.0f, 1.0f, 1.0f));

    prog.setUniform("Material.Ka", vec3(0.2f, 0.2f, 0.2f));
    prog.setUniform("Material.Kd", vec3(0.8f, 0.8f, 0.8f));
    prog.setUniform("Material.Ks", vec3(0.4f, 0.4f, 0.4f));
    prog.setUniform("Material.Shininess", 32.0f);

    //animated beam
    prog.use();
    prog.setUniform("Time", 0.0f);
    prog.setUniform("IsBeam", 0);
    prog.setUniform("EmissiveStrength", 0.0f);

    //Initialise mirrors
    mirrors.push_back({ glm::vec3(-6.0f, 1.5f, 0.0f),  35.0f, glm::vec3(1.5f, 3.0f, 0.2f) });
    mirrors.push_back({ glm::vec3(5.0f, 1.5f, -2.0f), -20.0f, glm::vec3(1.5f, 3.0f, 0.2f) });
    mirrors.push_back({ glm::vec3(2.0f, 1.5f,  6.0f),  60.0f, glm::vec3(1.5f, 3.0f, 0.2f) });

    //bloom
    setupBloomBuffers();
    setupScreenQuad();

    blurProg.use();
    blurProg.setUniform("image", 0);

    finalProg.use();
    finalProg.setUniform("scene", 0);
    finalProg.setUniform("bloomBlur", 1);
}

void SceneBasic_Uniform::drawCube(
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& color)
{
    prog.setUniform("UseTexture", 0);
    prog.setUniform("SolidColor", color);

    model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    setMatrices();
    cube.render();
}

void SceneBasic_Uniform::drawBeam(
    const glm::vec3& start,
    const glm::vec3& end,
    const glm::vec3& color,
    float thickness)
{
    glm::vec3 dir = end - start;
    float length = glm::length(dir);

    if (length < 0.0001f) return;

    glm::vec3 center = (start + end) * 0.5f;
    glm::vec3 forward = glm::normalize(dir);

    glm::vec3 up(0.0f, 1.0f, 0.0f);

    float dotVal = glm::dot(up, forward);
    dotVal = glm::clamp(dotVal, -1.0f, 1.0f);

    glm::mat4 rot(1.0f);

    if (fabs(dotVal - 1.0f) < 0.0001f)
    {
        rot = glm::mat4(1.0f);
    }
    else if (fabs(dotVal + 1.0f) < 0.0001f)
    {
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1, 0, 0));
    }
    else
    {
        glm::vec3 axis = glm::normalize(glm::cross(up, forward));
        float angle = acos(dotVal);
        rot = glm::rotate(glm::mat4(1.0f), angle, axis);
    }

    prog.setUniform("UseTexture", 0);
    prog.setUniform("SolidColor", color);
    prog.setUniform("IsBeam", 1);
    prog.setUniform("EmissiveStrength", 2.5f);

    model = glm::mat4(1.0f);
    model = glm::translate(model, center);
    model *= rot;
    model = glm::scale(model, glm::vec3(thickness, length, thickness));

    setMatrices();
    cube.render();

    prog.setUniform("IsBeam", 0);
    prog.setUniform("EmissiveStrength", 0.0f);
}

void SceneBasic_Uniform::drawMirror(const MirrorData& mirror, bool selected)
{
    prog.setUniform("UseTexture", 0);
    prog.setUniform("SolidColor",
        selected ? glm::vec3(1.0f, 0.9f, 0.3f) : glm::vec3(0.8f, 0.8f, 0.85f));

    model = glm::mat4(1.0f);
    model = glm::translate(model, mirror.pos);
    model = glm::rotate(model, glm::radians(mirror.angle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, mirror.scale);

    setMatrices();
    cube.render();
}

void SceneBasic_Uniform::drawBeamPath()
{
    reactorActivated = false;

    glm::vec3 reactorPos(0.0f, 1.5f, 0.0f);
    float reactorRadius = 1.5f;

    float pulse = 0.75f + 0.25f * sin(tPrev * 3.0f);
    glm::vec3 beamColor = pulse * glm::vec3(0.2f, 0.9f, 1.0f);
    glm::vec3 hotBeamColor = pulse * glm::vec3(1.0f, 0.5f, 0.2f);

    // Visual start = where beam should appear from
    glm::vec3 visualStart(-12.0f, 1.6f, 0.0f);

    // Logic start = where raycast should start from
    glm::vec3 logicStart = visualStart;

    glm::vec3 rayDir(1.0f, 0.0f, 0.0f);

    const int maxBounces = 8;
    const float maxSegmentLength = 50.0f;

    for (int bounce = 0; bounce < maxBounces; ++bounce)
    {
        float mirrorHitDistance;
        glm::vec3 mirrorHitPoint;
        int mirrorIndex = findClosestHitMirror(logicStart, rayDir, mirrorHitDistance, mirrorHitPoint);

        glm::vec3 reactorHitPoint;
        bool hitReactor = rayHitsReactorBeforeDistance(
            logicStart,
            rayDir,
            reactorPos,
            reactorRadius,
            (mirrorIndex >= 0 ? mirrorHitDistance : maxSegmentLength),
            reactorHitPoint
        );

        if (hitReactor)
        {
            drawBeam(visualStart, reactorHitPoint, (bounce == 0 ? hotBeamColor : beamColor), 0.10f);
            reactorActivated = true;
            return;
        }

        if (mirrorIndex < 0)
        {
            drawBeam(visualStart, visualStart + rayDir * maxSegmentLength, (bounce == 0 ? hotBeamColor : beamColor), 0.10f);
            return;
        }

        // Draw beam right up to the mirror surface
        drawBeam(visualStart, mirrorHitPoint, (bounce == 0 ? hotBeamColor : beamColor), 0.10f);

        glm::vec3 mirrorNormal = getMirrorNormal(mirrors[mirrorIndex].angle);

        if (glm::dot(rayDir, mirrorNormal) > 0.0f)
            mirrorNormal = -mirrorNormal;

        glm::vec3 reflectedDir = glm::normalize(glm::reflect(rayDir, mirrorNormal));

        // NEXT SEGMENT:
        // draw from exact hit point
        visualStart = mirrorHitPoint;

        // but raycast from slightly ahead to avoid self-hit
        logicStart = mirrorHitPoint + reflectedDir * 0.02f;

        rayDir = reflectedDir;
    }

    drawBeam(visualStart, visualStart + rayDir * 10.0f, beamColor, 0.10f);
}

glm::vec3 SceneBasic_Uniform::getMirrorNormal(float degrees)
{
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(degrees), glm::vec3(0, 1, 0));
    glm::vec3 localNormal(0.0f, 0.0f, 1.0f);
    return glm::normalize(glm::vec3(rot * glm::vec4(localNormal, 0.0f)));
}

int SceneBasic_Uniform::findLookedAtMirror() const
{
    int bestIndex = -1;
    float bestScore = 0.95f;

    for (int i = 0; i < (int)mirrors.size(); ++i)
    {
        glm::vec3 toMirror = glm::normalize(mirrors[i].pos - cameraPos);
        float score = glm::dot(cameraFront, toMirror);

        float dist = glm::length(mirrors[i].pos - cameraPos);

        if (score > bestScore && dist < 12.0f)
        {
            bestScore = score;
            bestIndex = i;
        }
    }

    return bestIndex;
}

bool SceneBasic_Uniform::rayHitsReactorBeforeDistance(
    const glm::vec3& rayStart,
    const glm::vec3& rayDir,
    const glm::vec3& reactorPos,
    float reactorRadius,
    float maxDistance,
    glm::vec3& hitPoint)
{
    glm::vec3 toCenter = reactorPos - rayStart;
    float t = glm::dot(toCenter, rayDir);

    if (t < 0.0f || t > maxDistance)
        return false;

    glm::vec3 closestPoint = rayStart + rayDir * t;
    float distToCenter = glm::length(closestPoint - reactorPos);

    if (distToCenter <= reactorRadius)
    {
        hitPoint = closestPoint;
        return true;
    }

    return false;
}

bool SceneBasic_Uniform::rayHitsMirror(
    const glm::vec3& rayStart,
    const glm::vec3& rayDir,
    const MirrorData& mirror,
    float& hitDistance,
    glm::vec3& hitPoint)
{
    // Treat mirror as a vertical rectangular plane
    glm::vec3 planePoint = mirror.pos;
    glm::vec3 planeNormal = getMirrorNormal(mirror.angle);

    float denom = glm::dot(rayDir, planeNormal);

    // Parallel or nearly parallel
    if (fabs(denom) < 0.0001f)
        return false;

    float t = glm::dot(planePoint - rayStart, planeNormal) / denom;

    // Behind the ray start
    if (t <= 0.001f)
        return false;

    glm::vec3 p = rayStart + rayDir * t;

    // Convert hit point into mirror local space
    glm::mat4 modelMat(1.0f);
    modelMat = glm::translate(modelMat, mirror.pos);
    modelMat = glm::rotate(modelMat, glm::radians(mirror.angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::scale(modelMat, mirror.scale);

    glm::mat4 invModel = glm::inverse(modelMat);
    glm::vec3 local = glm::vec3(invModel * glm::vec4(p, 1.0f));

    // Cube is centered at origin; mirror face spans its local bounds
    // Thickness is along local Z, height along local Y, width along local X
    float halfWidth = 0.5f;
    float halfHeight = 0.5f;
    float halfDepth = 0.5f;

    // Check hit lies within front face rectangle
    // Since this is the plane of the mirror, local.z should be near 0
    if (fabs(local.x) <= halfWidth + 0.01f &&
        fabs(local.y) <= halfHeight + 0.01f &&
        fabs(local.z) <= halfDepth + 0.2f)
    {
        hitDistance = t;
        hitPoint = p;
        return true;
    }

    return false;
}

int SceneBasic_Uniform::findClosestHitMirror(
    const glm::vec3& rayStart,
    const glm::vec3& rayDir,
    float& hitDistance,
    glm::vec3& hitPoint)
{
    int bestIndex = -1;
    float bestDistance = 1e9f;
    glm::vec3 bestPoint(0.0f);

    for (int i = 0; i < (int)mirrors.size(); ++i)
    {
        float t;
        glm::vec3 p;
        if (rayHitsMirror(rayStart, rayDir, mirrors[i], t, p))
        {
            if (t < bestDistance)
            {
                bestDistance = t;
                bestPoint = p;
                bestIndex = i;
            }
        }
    }

    if (bestIndex >= 0)
    {
        hitDistance = bestDistance;
        hitPoint = bestPoint;
    }

    return bestIndex;
}

void SceneBasic_Uniform::compile()
{
    try {
        prog.compileShader("shader/basic_uniform.vert");
        prog.compileShader("shader/basic_uniform.frag");
        prog.link();

        blurProg.compileShader("shader/bloom_blur.vert");
        blurProg.compileShader("shader/bloom_blur.frag");
        blurProg.link();

        finalProg.compileShader("shader/bloom_final.vert");
        finalProg.compileShader("shader/bloom_final.frag");
        finalProg.link();
    }
    catch (GLSLProgramException& e) {
        cerr << e.what() << endl;
        exit(EXIT_FAILURE);
    }
}

void SceneBasic_Uniform::setupBloomBuffers()
{
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "HDR framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Ping-pong framebuffer not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneBasic_Uniform::setupScreenQuad()
{
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void SceneBasic_Uniform::renderQuad()
{
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void SceneBasic_Uniform::updateCameraVectors()
{
    vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
    cameraRight = glm::normalize(glm::cross(cameraFront, worldUp));
    cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
}

void SceneBasic_Uniform::update(float t)
{
    float deltaT = t - tPrev;
    if (tPrev == 0.0f) deltaT = 0.0f;
    tPrev = t;

    float velocity = moveSpeed * deltaT;

    glm::vec3 flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

    if (keyW) cameraPos += flatFront * velocity;
    if (keyS) cameraPos -= flatFront * velocity;
    if (keyA) cameraPos -= cameraRight * velocity;
    if (keyD) cameraPos += cameraRight * velocity;

    if (cameraPos.y < 1.0f) cameraPos.y = 1.0f;

    selectedMirrorIndex = findLookedAtMirror();

    if (selectedMirrorIndex >= 0)
    {
        float rotateSpeed = 60.0f * deltaT;

        if (keyQ) mirrors[selectedMirrorIndex].angle -= rotateSpeed;
        if (keyE) mirrors[selectedMirrorIndex].angle += rotateSpeed;
    }

    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}

void SceneBasic_Uniform::render()
{
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderSceneGeometry();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Blur bright fragments
    bool horizontal = true;
    bool firstIteration = true;
    const unsigned int blurAmount = 10;

    blurProg.use();

    for (unsigned int i = 0; i < blurAmount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);

        blurProg.setUniform("horizontal", horizontal ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            firstIteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]
        );

        renderQuad();

        horizontal = !horizontal;
        if (firstIteration)
            firstIteration = false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    finalProg.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);

    finalProg.setUniform("bloom", bloomEnabled ? 1 : 0);
    finalProg.setUniform("exposure", bloomExposure);

    renderQuad();

    glFlush();
}

void SceneBasic_Uniform::renderSceneGeometry()
{
    prog.use();
    prog.setUniform("Time", tPrev);

    float pulse = 0.75f + 0.25f * sin(tPrev * 3.0f);
    glm::vec3 beamColor = pulse * glm::vec3(0.2f, 0.9f, 1.0f);
    glm::vec3 hotBeamColor = pulse * glm::vec3(1.0f, 0.5f, 0.2f);

    //Floor
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, planeTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, planeNormal);

    prog.setUniform("UseTexture", 1);
    prog.setUniform("IsBeam", 0);
    prog.setUniform("EmissiveStrength", 0.0f);
    model = glm::mat4(1.0f);
    setMatrices();
    plane.render();

    //Room
    // Back wall
    drawCube(glm::vec3(0.0f, 2.5f, -15.0f), glm::vec3(30.0f, 5.0f, 0.5f), glm::vec3(0.45f, 0.45f, 0.5f));

    // Front wall
    drawCube(glm::vec3(0.0f, 2.5f, 15.0f), glm::vec3(30.0f, 5.0f, 0.5f), glm::vec3(0.45f, 0.45f, 0.5f));

    // Left wall
    drawCube(glm::vec3(-15.0f, 2.5f, 0.0f), glm::vec3(0.5f, 5.0f, 30.0f), glm::vec3(0.4f, 0.4f, 0.45f));

    // Right wall
    drawCube(glm::vec3(15.0f, 2.5f, 0.0f), glm::vec3(0.5f, 5.0f, 30.0f), glm::vec3(0.4f, 0.4f, 0.45f));

    // Ceiling
    drawCube(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(30.0f, 0.5f, 30.0f), glm::vec3(0.35f, 0.35f, 0.4f));

    //Mirrors
    for (int i = 0; i < (int)mirrors.size(); ++i)
    {
        drawMirror(mirrors[i], i == selectedMirrorIndex);

        drawCube(
            glm::vec3(mirrors[i].pos.x, 0.5f, mirrors[i].pos.z),
            glm::vec3(0.4f, 1.0f, 0.4f),
            glm::vec3(0.3f, 0.3f, 0.35f)
        );
    }

    //Emitter
    drawCube(glm::vec3(-12.0f, 1.0f, 0.0f), glm::vec3(1.5f, 2.0f, 1.5f), glm::vec3(1.0f, 0.4f, 0.2f));

    //Beams
    prog.setUniform("EmissiveStrength", 3.0f);
    drawBeamPath();

    //Reactor
    prog.setUniform("IsBeam", 0);
    prog.setUniform("EmissiveStrength", reactorActivated ? 2.0f : 1.0f);
    glm::vec3 reactorColor = reactorActivated
        ? glm::vec3(0.3f, 1.0f, 1.0f)
        : glm::vec3(0.1f, 0.1f, 0.1f);

    drawCube(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(2.0f, 2.0f, 2.0f), reactorColor);

    // Reactor base
    prog.setUniform("EmissiveStrength", 0.0f);
    drawCube(glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(4.0f, 0.5f, 4.0f), glm::vec3(0.25f, 0.25f, 0.3f));
}

void SceneBasic_Uniform::resize(int w, int h)
{
    glViewport(0, 0, w, h);
    width = w;
    height = h;
    projection = glm::perspective(glm::radians(60.0f), (float)w / (float)h, 0.3f, 100.0f);

    if (hdrFBO != 0)
    {
        glDeleteFramebuffers(1, &hdrFBO);
        glDeleteRenderbuffers(1, &rboDepth);
        glDeleteTextures(2, colorBuffers);
        glDeleteFramebuffers(2, pingpongFBO);
        glDeleteTextures(2, pingpongColorbuffers);
    }

    setupBloomBuffers();
}

void SceneBasic_Uniform::setMatrices()
{
    mat4 mv = view * model;
    prog.setUniform("ModelMatrix", model);
    prog.setUniform("ModelViewMatrix", mv);
    prog.setUniform("NormalMatrix", mat3(glm::transpose(glm::inverse(mv))));
    prog.setUniform("MVP", projection * mv);
}

void SceneBasic_Uniform::keyInput(int key, int action)
{
    bool pressed = (action == GLFW_PRESS);

    switch (key)
    {
    case GLFW_KEY_W: keyW = pressed; break;
    case GLFW_KEY_A: keyA = pressed; break;
    case GLFW_KEY_S: keyS = pressed; break;
    case GLFW_KEY_D: keyD = pressed; break;
    case GLFW_KEY_Q: keyQ = pressed; break;
    case GLFW_KEY_E: keyE = pressed; break;

    default:
        break;
    }
}

void SceneBasic_Uniform::mouseInput(double x, double y)
{
    if (firstMouse)
    {
        lastMouseX = (float)x;
        lastMouseY = (float)y;
        firstMouse = false;
    }

    float xoffset = (float)x - lastMouseX;
    float yoffset = lastMouseY - (float)y;

    lastMouseX = (float)x;
    lastMouseY = (float)y;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateCameraVectors();
}