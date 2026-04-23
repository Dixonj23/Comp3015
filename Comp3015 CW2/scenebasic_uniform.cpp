#include "scenebasic_uniform.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>

#include <sstream>
#include "helper/stb/stb_easy_font.h"

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
    keyW(false), keyA(false), keyS(false), keyD(false),
    firstMouse(true),
    lastMouseX(0.0f),
    lastMouseY(0.0f),
    selectedMirrorIndex(-1),
    reactorActivated(false),
    reactorLightLevel(0.0f),
    hdrFBO(0),
    rboDepth(0),
    quadVAO(0),
    quadVBO(0),
    bloomEnabled(true),
    bloomExposure(0.6f),
    pointShadowFBO(0),
    pointShadowCube(0),
    shadowFarPlane(25.0f),
    shadowLightPos(0.0f, 1.5f, 0.0f),
    reactorWasActiveLastFrame(false),
    reactorPulseTimer(0.0f),
    emitterPos(-12.0f, 1.0f, 0.0f),
    emitterAngle(0.0f)
{
}

void SceneBasic_Uniform::compile()
{
    try
    {
        prog.compileShader("shader/basic_uniform.vert");
        prog.compileShader("shader/basic_uniform.frag");
        prog.link();

        blurProg.compileShader("shader/bloom_blur.vert");
        blurProg.compileShader("shader/bloom_blur.frag");
        blurProg.link();

        hudProg.compileShader("shader/hud.vert");
        hudProg.compileShader("shader/hud.frag");
        hudProg.link();

        finalProg.compileShader("shader/bloom_final.vert");
        finalProg.compileShader("shader/bloom_final.frag");
        finalProg.link();

        pointShadowProg.compileShader("shader/point_shadow_depth.vert");
        pointShadowProg.compileShader("shader/point_shadow_depth.geom");
        pointShadowProg.compileShader("shader/point_shadow_depth.frag");
        pointShadowProg.link();
    }
    catch (GLSLProgramException& e)
    {
        cerr << e.what() << endl;
        exit(EXIT_FAILURE);
    }
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
    prog.setUniform("PointShadowMap", 2);

    prog.setUniform("UseTexture", 1);
    prog.setUniform("SolidColor", glm::vec3(1.0f));
    prog.setUniform("Time", 0.0f);
    prog.setUniform("IsBeam", 0);
    prog.setUniform("EmissiveStrength", 0.0f);

    prog.setUniform("Material.Ka", vec3(0.2f, 0.2f, 0.2f));
    prog.setUniform("Material.Kd", vec3(0.8f, 0.8f, 0.8f));
    prog.setUniform("Material.Ks", vec3(0.4f, 0.4f, 0.4f));
    prog.setUniform("Material.Shininess", 32.0f);

    // Corner light fixture positions
    cornerLightPositions.push_back(glm::vec3(-13.5f, 4.5f, -13.5f));
    cornerLightPositions.push_back(glm::vec3(13.5f, 4.5f, -13.5f));
    cornerLightPositions.push_back(glm::vec3(-13.5f, 4.5f, 13.5f));
    cornerLightPositions.push_back(glm::vec3(13.5f, 4.5f, 13.5f));

    // Mirrors
    mirrors.push_back({ glm::vec3(-6.0f, 1.5f,  0.0f),  35.0f, glm::vec3(1.5f, 3.0f, 0.2f) });
    mirrors.push_back({ glm::vec3(5.0f, 1.5f, -2.0f), -20.0f, glm::vec3(1.5f, 3.0f, 0.2f) });
    mirrors.push_back({ glm::vec3(2.0f, 1.5f,  6.0f),  60.0f, glm::vec3(1.5f, 3.0f, 0.2f) });

    // Obstacles
    obstacles.push_back({ glm::vec3(-2.0f, 1.0f, -6.0f), glm::vec3(2.0f, 2.0f, 2.0f) });
    obstacles.push_back({ glm::vec3(6.0f, 1.5f,  3.0f), glm::vec3(1.5f, 3.0f, 1.5f) });
    obstacles.push_back({ glm::vec3(-8.0f, 0.75f, 5.0f), glm::vec3(3.0f, 1.5f, 1.0f) });
    obstacles.push_back({ glm::vec3(3.0f, 1.0f, -9.0f), glm::vec3(2.5f, 2.0f, 2.5f) });

    buildStaticColliders();

    setupBloomBuffers();
    setupScreenQuad();
    setupPointShadowMap();
    setupHUD();

    initReactorParticles();

    blurProg.use();
    blurProg.setUniform("image", 0);

    finalProg.use();
    finalProg.setUniform("scene", 0);
    finalProg.setUniform("bloomBlur", 1);
}

void SceneBasic_Uniform::initReactorParticles()
{
    reactorParticles.clear();
    reactorParticles.resize(80);

    for (auto& p : reactorParticles)
    {
        respawnReactorParticle(p);
        p.life = static_cast<float>(rand()) / RAND_MAX * p.maxLife;
    }
}

void SceneBasic_Uniform::respawnReactorParticle(ReactorParticle& p)
{
    glm::vec3 reactorCenter(0.0f, 1.2f, 0.0f);

    float rx = ((float)rand() / RAND_MAX - 0.5f) * 1.2f;
    float rz = ((float)rand() / RAND_MAX - 0.5f) * 1.2f;
    float ry = ((float)rand() / RAND_MAX) * 0.4f;

    p.pos = reactorCenter + glm::vec3(rx, ry, rz);

    float vx = ((float)rand() / RAND_MAX - 0.5f) * 0.25f;
    float vz = ((float)rand() / RAND_MAX - 0.5f) * 0.25f;
    float vy = 0.6f + ((float)rand() / RAND_MAX) * 0.8f;

    p.vel = glm::vec3(vx, vy, vz);

    float activeMix = reactorActivated ? 1.0f : 0.0f;
    p.color = glm::mix(
        glm::vec3(1.0f, 0.35f, 0.15f),
        glm::vec3(0.35f, 0.9f, 1.0f),
        activeMix
    );

    p.maxLife = reactorActivated ? 1.8f : 1.2f;
    p.life = p.maxLife;

    p.size = reactorActivated ? 0.08f : 0.05f;
}

void SceneBasic_Uniform::spawnReactorBurst(int count)
{
    glm::vec3 reactorCenter(0.0f, 1.2f, 0.0f);

    for (int i = 0; i < count; ++i)
    {
        ReactorParticle p;

        float angle = ((float)i / (float)count) * glm::two_pi<float>();
        float radiusJitter = 0.2f + ((float)rand() / RAND_MAX) * 0.4f;
        float upward = 1.2f + ((float)rand() / RAND_MAX) * 1.2f;

        glm::vec3 outward = glm::normalize(glm::vec3(cos(angle), 0.15f, sin(angle)));

        p.pos = reactorCenter + outward * radiusJitter;
        p.vel = outward * (1.8f + ((float)rand() / RAND_MAX) * 1.5f);
        p.vel.y += upward;

        p.color = glm::vec3(0.5f, 1.0f, 1.2f);
        p.maxLife = 1.2f;
        p.life = p.maxLife;
        p.size = 0.12f + ((float)rand() / RAND_MAX) * 0.06f;

        reactorParticles.push_back(p);
    }
}

void SceneBasic_Uniform::setupHUD()
{
    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);

    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, 1024 * 1024, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void SceneBasic_Uniform::setMatrices()
{
    mat4 mv = view * model;
    prog.setUniform("ModelMatrix", model);
    prog.setUniform("ModelViewMatrix", mv);
    prog.setUniform("NormalMatrix", mat3(glm::transpose(glm::inverse(mv))));
    prog.setUniform("MVP", projection * mv);
}

void SceneBasic_Uniform::setPointShadowMatrices()
{
    pointShadowProg.setUniform("ModelMatrix", model);
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

void SceneBasic_Uniform::setupPointShadowMap()
{
    const unsigned int SHADOW_SIZE = 1024;

    glGenFramebuffers(1, &pointShadowFBO);

    glGenTextures(1, &pointShadowCube);
    glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowCube);

    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,
            GL_DEPTH_COMPONENT,
            SHADOW_SIZE,
            SHADOW_SIZE,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointShadowCube, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Point shadow framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneBasic_Uniform::buildPointShadowTransforms()
{
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 1.0f, shadowFarPlane);

    shadowTransforms[0] = shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0));
    shadowTransforms[1] = shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0));
    shadowTransforms[2] = shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
    shadowTransforms[3] = shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));
    shadowTransforms[4] = shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0));
    shadowTransforms[5] = shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0));
}

void SceneBasic_Uniform::renderPointShadowPass()
{
    const unsigned int SHADOW_SIZE = 1024;

    shadowLightPos = glm::vec3(0.0f, 2.5f, 0.0f); // reactor center
    shadowFarPlane = 25.0f;
    buildPointShadowTransforms();

    glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    pointShadowProg.use();

    for (int i = 0; i < 6; ++i)
    {
        pointShadowProg.setUniform(("shadowMatrices[" + std::to_string(i) + "]").c_str(), shadowTransforms[i]);
    }

    pointShadowProg.setUniform("lightPos", shadowLightPos);
    pointShadowProg.setUniform("far_plane", shadowFarPlane);

    renderShadowGeometry();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneBasic_Uniform::drawCube(
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& color
)
{
    prog.setUniform("UseTexture", 0);
    prog.setUniform("SolidColor", color);

    model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    setMatrices();
    cube.render();
}

void SceneBasic_Uniform::drawMirror(const MirrorData& mirror, bool selected)
{
    prog.setUniform(
        "SolidColor",
        selected ? glm::vec3(1.0f, 0.9f, 0.3f) : glm::vec3(0.8f, 0.8f, 0.85f)
    );
    prog.setUniform("UseTexture", 0);

    model = glm::mat4(1.0f);
    model = glm::translate(model, mirror.pos);
    model = glm::rotate(model, glm::radians(mirror.angle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, mirror.scale);

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

    float dotVal = glm::clamp(glm::dot(up, forward), -1.0f, 1.0f);
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

void SceneBasic_Uniform::drawHUDLines(const std::vector<float>& verts, const glm::vec3& color)
{
    hudProg.use();
    hudProg.setUniform("uColor", color);

    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(float), verts.data());

    glDrawArrays(GL_LINES, 0, (GLsizei)(verts.size() / 2));

    glBindVertexArray(0);
}

void drawText(
    GLuint vao,
    GLuint vbo,
    GLSLProgram& prog,
    const std::string& text,
    float x,
    float y,
    float scale,
    const glm::vec3& color,
    int screenW,
    int screenH)
{
    char buffer[99999];
    int num_quads = stb_easy_font_print(0, 0, (char*)text.c_str(), nullptr, buffer, sizeof(buffer));

    std::vector<float> verts;
    verts.reserve(num_quads * 6 * 2);

    float invW = 2.0f / (float)screenW;
    float invH = 2.0f / (float)screenH;

    auto pushVertex = [&](float px, float py)
        {
            float ndcX = px * invW - 1.0f;
            float ndcY = 1.0f - py * invH;

            verts.push_back(ndcX);
            verts.push_back(ndcY);
        };

    unsigned char* ptr = (unsigned char*)buffer;

    for (int i = 0; i < num_quads; ++i)
    {
        float* quad = (float*)(ptr + i * 64);

        float x0 = quad[0] * scale + x;
        float y0 = quad[1] * scale + y;

        float x1 = quad[4] * scale + x;
        float y1 = quad[5] * scale + y;

        float x2 = quad[8] * scale + x;
        float y2 = quad[9] * scale + y;

        float x3 = quad[12] * scale + x;
        float y3 = quad[13] * scale + y;

        pushVertex(x0, y0);
        pushVertex(x1, y1);
        pushVertex(x2, y2);

        pushVertex(x0, y0);
        pushVertex(x2, y2);
        pushVertex(x3, y3);
    }

    prog.use();
    prog.setUniform("uColor", color);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(float), verts.data());

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verts.size() / 2));

    glBindVertexArray(0);
}

void SceneBasic_Uniform::renderShadowGeometry()
{
    pointShadowProg.use();

    auto drawShadowCube = [&](const glm::vec3& pos, const glm::vec3& scale)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::scale(model, scale);
            setPointShadowMatrices();
            cube.render();
        };

    // floor
    model = glm::mat4(1.0f);
    setPointShadowMatrices();
    plane.render();

    // walls + ceiling
    drawShadowCube(glm::vec3(0.0f, 2.5f, -15.0f), glm::vec3(30.0f, 5.0f, 0.5f));
    drawShadowCube(glm::vec3(0.0f, 2.5f, 15.0f), glm::vec3(30.0f, 5.0f, 0.5f));
    drawShadowCube(glm::vec3(-15.0f, 2.5f, 0.0f), glm::vec3(0.5f, 5.0f, 30.0f));
    drawShadowCube(glm::vec3(15.0f, 2.5f, 0.0f), glm::vec3(0.5f, 5.0f, 30.0f));
    drawShadowCube(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(30.0f, 0.5f, 30.0f));

    // obstacles
    for (const auto& obstacle : obstacles)
    {
        drawShadowCube(obstacle.pos, obstacle.scale);
    }

    // mirror stands + panels
    for (const auto& mirror : mirrors)
    {
        drawShadowCube(glm::vec3(mirror.pos.x, 0.5f, mirror.pos.z), glm::vec3(0.4f, 1.0f, 0.4f));

        model = glm::mat4(1.0f);
        model = glm::translate(model, mirror.pos);
        model = glm::rotate(model, glm::radians(mirror.angle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, mirror.scale);
        setPointShadowMatrices();
        cube.render();
    }

    // emitter
    drawShadowCube(glm::vec3(-12.0f, 1.0f, 0.0f), glm::vec3(1.5f, 2.0f, 1.5f));

    // reactor
    drawShadowCube(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(2.0f, 2.0f, 2.0f));
    drawShadowCube(glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(4.0f, 0.5f, 4.0f));
}

void SceneBasic_Uniform::renderSceneGeometry()
{
    prog.use();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowCube);

    prog.setUniform("Time", tPrev);

    // corner fixture emissive cubes
    glm::vec3 redLight = glm::vec3(1.2f, 0.05f, 0.05f);
    glm::vec3 whiteLight = glm::vec3(1.3f, 1.3f, 1.3f);

    glm::vec3 cornerLightColor = glm::mix(redLight, whiteLight, reactorLightLevel);
    float cornerLightEmissive = glm::mix(1.6f, 1.1f, reactorLightLevel);

    float alarmPulse = 0.5f + 0.5f * sin(tPrev * 6.0f);
    float pulseAmount = (reactorLightLevel < 0.95f) ? (1.0f - reactorLightLevel) : 0.0f;
    cornerLightEmissive *= 1.0f + pulseAmount * (0.4f * alarmPulse);

    for (const auto& pos : cornerLightPositions)
    {
        prog.setUniform("IsBeam", 0);

        // housing
        drawCube(
            pos + glm::vec3(0.0f, 0.3f, 0.0f),
            glm::vec3(0.5f, 0.5f, 0.5f),
            glm::vec3(0.15f, 0.15f, 0.18f)
        );

        // emissive bulb
        prog.setUniform("EmissiveStrength", cornerLightEmissive);
        drawCube(
            pos,
            glm::vec3(0.35f, 0.35f, 0.35f),
            cornerLightColor
        );
    }
    prog.setUniform("EmissiveStrength", 0.0f);

    // floor
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, planeTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, planeNormal);

    model = glm::mat4(1.0f);
    prog.setUniform("UseTexture", 1);
    prog.setUniform("IsBeam", 0);
    prog.setUniform("EmissiveStrength", 0.0f);
    setMatrices();
    plane.render();

    // room
    drawCube(glm::vec3(0.0f, 2.5f, -15.0f), glm::vec3(30.0f, 5.0f, 0.5f), glm::vec3(0.45f, 0.45f, 0.5f));
    drawCube(glm::vec3(0.0f, 2.5f, 15.0f), glm::vec3(30.0f, 5.0f, 0.5f), glm::vec3(0.45f, 0.45f, 0.5f));
    drawCube(glm::vec3(-15.0f, 2.5f, 0.0f), glm::vec3(0.5f, 5.0f, 30.0f), glm::vec3(0.4f, 0.4f, 0.45f));
    drawCube(glm::vec3(15.0f, 2.5f, 0.0f), glm::vec3(0.5f, 5.0f, 30.0f), glm::vec3(0.4f, 0.4f, 0.45f));
    drawCube(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(30.0f, 0.5f, 30.0f), glm::vec3(0.35f, 0.35f, 0.4f));

    // obstacles
    prog.setUniform("IsBeam", 0);
    prog.setUniform("EmissiveStrength", 0.0f);
    for (const auto& obstacle : obstacles)
    {
        drawCube(obstacle.pos, obstacle.scale, glm::vec3(0.25f, 0.25f, 0.30f));
    }

    // mirrors
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
    bool emitterSelected = isLookingAtEmitter();

    glm::vec3 emitterBaseColor = emitterSelected
        ? glm::vec3(1.0f, 0.8f, 0.25f)
        : glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec3 emitterNozzleColor = emitterSelected
        ? glm::vec3(1.0f, 0.8f, 0.25f)
        : glm::vec3(1.0f, 1.0f, 1.0f);

    // emitter base
    prog.setUniform("IsBeam", 0);

    drawCube(
        emitterPos,
        glm::vec3(1.5f, 2.0f, 1.5f),
        emitterBaseColor
    );


    // emitter nozzle
    prog.setUniform("UseTexture", 0);
    prog.setUniform("SolidColor", emitterNozzleColor);
    prog.setUniform("IsBeam", 0);


    model = glm::mat4(1.0f);
    model = glm::translate(model, emitterPos + glm::vec3(0.0f, 0.6f, 0.0f));
    model = glm::rotate(model, glm::radians(-emitterAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.7f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f, 0.25f, 0.25f));
    setMatrices();
    cube.render();


    // reactor
    prog.setUniform("IsBeam", 0);
    prog.setUniform("EmissiveStrength", glm::mix(1.2f, 2.5f, reactorLightLevel));

    glm::vec3 reactorColor = reactorActivated
        ? glm::vec3(0.3f, 1.0f, 1.0f)
        : glm::vec3(0.1f, 0.1f, 0.1f);

    drawCube(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(2.0f, 2.0f, 2.0f), reactorColor);

    prog.setUniform("EmissiveStrength", 0.0f);
    drawCube(glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(4.0f, 0.5f, 4.0f), glm::vec3(0.25f, 0.25f, 0.3f));

    // beams
    drawBeamPath();

    //particles
    drawReactorParticles();
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

void SceneBasic_Uniform::renderHUD()
{
    glDisable(GL_DEPTH_TEST);

    // crosshair
    float cx = 0.0f;
    float cy = 0.0f;
    float s = 0.015f;

    std::vector<float> crosshair = {
        cx - s, cy,     cx + s, cy,
        cx, cy - s,     cx, cy + s
    };
    drawHUDLines(crosshair, glm::vec3(1.0f, 1.0f, 1.0f));

    std::string reactorText =
        reactorActivated ? "REACTOR: ONLINE" : "REACTOR: OFFLINE";

    bool emitterSelected = isLookingAtEmitter();

    std::string targetText;
    if (emitterSelected)
    {
        targetText = "TARGET: EMITTER";
    }
    else if (selectedMirrorIndex >= 0)
    {
        targetText = "TARGET: MIRROR " + std::to_string(selectedMirrorIndex + 1);
    }
    else
    {
        targetText = "TARGET: NONE";
    }

    std::string controlsText = "Q / E - ROTATE TARGET";

    glm::vec3 reactorColor =
        reactorActivated ? glm::vec3(0.4f, 1.0f, 1.0f)
        : glm::vec3(1.0f, 0.3f, 0.3f);

    glm::vec3 targetColor =
        emitterSelected ? glm::vec3(1.0f, 0.85f, 0.3f) :
        (selectedMirrorIndex >= 0 ? glm::vec3(1.0f, 1.0f, 1.0f)
            : glm::vec3(0.7f, 0.7f, 0.7f));

    drawText(hudVAO, hudVBO, hudProg, targetText, 20, 90, 1.6f, targetColor, width, height);

    drawText(hudVAO, hudVBO, hudProg, reactorText, 20, 40, 2.0f, reactorColor, width, height);
    drawText(hudVAO, hudVBO, hudProg, targetText, 20, 520, 1.6f, targetColor, width, height);
    drawText(hudVAO, hudVBO, hudProg, controlsText, 20, 550, 1.4f, glm::vec3(0.8f, 0.8f, 0.8f), width, height);

    glEnable(GL_DEPTH_TEST);
}

void SceneBasic_Uniform::updateReactorParticles(float dt)
{
    for (auto& p : reactorParticles)
    {
        p.life -= dt;

        if (p.life <= 0.0f)
        {
            respawnReactorParticle(p);
            continue;
        }

        p.pos += p.vel * dt;

        //outward drift
        p.vel.x *= 0.995f;
        p.vel.z *= 0.995f;

        //lift variation
        p.vel.y += reactorActivated ? 0.15f * dt : 0.05f * dt;
    }
}

void SceneBasic_Uniform::drawReactorParticles()
{
    prog.setUniform("UseTexture", 0);
    prog.setUniform("IsBeam", 0);

    for (const auto& p : reactorParticles)
    {
        float alpha = glm::clamp(p.life / p.maxLife, 0.0f, 1.0f);

        glm::vec3 color = p.color * (0.4f + 0.6f * alpha);
        float emissive = reactorActivated
            ? (1.8f * alpha + 0.6f)
            : (1.0f * alpha + 0.2f);

        prog.setUniform("EmissiveStrength", emissive);

        drawCube(
            p.pos,
            glm::vec3(p.size),
            color
        );
    }

    prog.setUniform("EmissiveStrength", 0.0f);
}

void SceneBasic_Uniform::update(float t)
{
    float deltaT = t - tPrev;
    if (tPrev == 0.0f) deltaT = 0.0f;
    tPrev = t;

    updateReactorParticles(deltaT);

    buildStaticColliders();

    float target = reactorActivated ? 1.0f : 0.0f;
    float speed = 2.0f;
    reactorLightLevel += (target - reactorLightLevel) * speed * deltaT;

    glm::vec3 proposedPos = cameraPos;
    glm::vec3 flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    float velocity = moveSpeed * deltaT;

    if (keyW) proposedPos += flatFront * velocity;
    if (keyS) proposedPos -= flatFront * velocity;
    if (keyA) proposedPos -= cameraRight * velocity;
    if (keyD) proposedPos += cameraRight * velocity;

    proposedPos.x = glm::clamp(proposedPos.x, -14.0f, 14.0f);
    proposedPos.z = glm::clamp(proposedPos.z, -14.0f, 14.0f);

    if (proposedPos.y < 1.0f) proposedPos.y = 1.0f;
    if (proposedPos.y > 2.5f) proposedPos.y = 2.5f;

    float playerRadius = 0.4f;

    glm::vec3 testPosX = cameraPos;
    testPosX.x = proposedPos.x;
    if (!pointCollidesWithScene(testPosX, playerRadius))
        cameraPos.x = testPosX.x;

    glm::vec3 testPosZ = cameraPos;
    testPosZ.z = proposedPos.z;
    if (!pointCollidesWithScene(testPosZ, playerRadius))
        cameraPos.z = testPosZ.z;

    glm::vec3 testPosY = cameraPos;
    testPosY.y = proposedPos.y;
    if (!pointCollidesWithScene(testPosY, playerRadius))
        cameraPos.y = testPosY.y;

    selectedMirrorIndex = findLookedAtMirror();

    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);


    bool emitterSelected = isLookingAtEmitter();

    if (emitterSelected)
    {
        float rotateSpeed = 60.0f * deltaT;
        if (keyQ) emitterAngle -= rotateSpeed;
        if (keyE) emitterAngle += rotateSpeed;
    }
    else if (selectedMirrorIndex >= 0)
    {
        float rotateSpeed = 60.0f * deltaT;
        if (keyQ) mirrors[selectedMirrorIndex].angle -= rotateSpeed;
        if (keyE) mirrors[selectedMirrorIndex].angle += rotateSpeed;
    }
}

void SceneBasic_Uniform::render()
{
    // 1) point-shadow pass
    renderPointShadowPass();

    // 2) visible HDR scene pass
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    prog.use();

    // main reactor point light
    shadowLightPos = glm::vec3(0.0f, 1.5f, 0.0f);

    glm::vec3 ambientOff = glm::vec3(0.13f, 0.13f, 0.14f);
    glm::vec3 ambientOn = glm::vec3(0.20f, 0.22f, 0.26f);

    glm::vec3 diffuseOff = glm::vec3(0.18f, 0.26f, 0.32f);
    glm::vec3 diffuseOn = glm::vec3(0.60f, 0.85f, 1.05f);

    glm::vec3 specOff = glm::vec3(0.18f, 0.25f, 0.30f);
    glm::vec3 specOn = glm::vec3(0.85f, 0.95f, 1.10f);

    glm::vec3 lightAmbient = glm::mix(ambientOff, ambientOn, reactorLightLevel);
    glm::vec3 lightDiffuse = glm::mix(diffuseOff, diffuseOn, reactorLightLevel);
    glm::vec3 lightSpecular = glm::mix(specOff, specOn, reactorLightLevel);

    prog.setUniform("Light.Position", view * glm::vec4(shadowLightPos, 1.0f));
    prog.setUniform("Light.La", lightAmbient);
    prog.setUniform("Light.Ld", lightDiffuse);
    prog.setUniform("Light.Ls", lightSpecular);

    float linear = glm::mix(0.14f, 0.10f, reactorLightLevel);
    float quadratic = glm::mix(0.05f, 0.035f, reactorLightLevel);

    prog.setUniform("Light.Constant", 1.0f);
    prog.setUniform("Light.Linear", linear);
    prog.setUniform("Light.Quadratic", quadratic);

    // point-shadow uniforms
    prog.setUniform("PointShadowLightPos", shadowLightPos);
    prog.setUniform("FarPlane", shadowFarPlane);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowCube);

    // corner point lights
    glm::vec3 cornerAmbientOff = glm::vec3(0.08f, 0.01f, 0.01f);
    glm::vec3 cornerDiffuseOff = glm::vec3(2.2f, 0.15f, 0.15f);
    glm::vec3 cornerSpecOff = glm::vec3(2.5f, 0.25f, 0.25f);

    glm::vec3 cornerAmbientOn = glm::vec3(0.10f, 0.10f, 0.10f);
    glm::vec3 cornerDiffuseOn = glm::vec3(1.3f, 1.3f, 1.3f);
    glm::vec3 cornerSpecOn = glm::vec3(1.6f, 1.6f, 1.6f);

    float alarmPulse = 0.5f + 0.5f * sin(tPrev * 6.0f);
    float pulseAmount = 1.0f - reactorLightLevel;

    glm::vec3 cornerLa = glm::mix(cornerAmbientOff, cornerAmbientOn, reactorLightLevel);
    glm::vec3 cornerLd = glm::mix(cornerDiffuseOff, cornerDiffuseOn, reactorLightLevel);
    glm::vec3 cornerLs = glm::mix(cornerSpecOff, cornerSpecOn, reactorLightLevel);

    for (int i = 0; i < 4; ++i)
    {
        std::string base = "CornerLights[" + std::to_string(i) + "]";
        prog.setUniform((base + ".Position").c_str(), view * glm::vec4(cornerLightPositions[i], 1.0f));
        prog.setUniform((base + ".La").c_str(), cornerLa);

        glm::vec3 pulseLd = cornerLd * (1.0f + pulseAmount * 0.8f * alarmPulse);
        glm::vec3 pulseLs = cornerLs * (1.0f + pulseAmount * 0.8f * alarmPulse);

        prog.setUniform((base + ".Ld").c_str(), pulseLd);
        prog.setUniform((base + ".Ls").c_str(), pulseLs);

        prog.setUniform((base + ".Constant").c_str(), 1.0f);
        prog.setUniform((base + ".Linear").c_str(), 0.22f);
        prog.setUniform((base + ".Quadratic").c_str(), 0.20f);
    }

    renderSceneGeometry();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 3) blur bloom texture
    bool horizontal = true;
    bool firstIteration = true;
    const unsigned int blurAmount = 10;

    blurProg.use();

    for (unsigned int i = 0; i < blurAmount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        blurProg.setUniform("horizontal", horizontal ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,
            firstIteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);

        renderQuad();

        horizontal = !horizontal;
        if (firstIteration)
            firstIteration = false;
    }

    // 4) final composite
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

    renderHUD();

    glFlush();
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

bool SceneBasic_Uniform::pointInsideAABB(const glm::vec3& p, const ColliderData& collider, float radius) const
{
    glm::vec3 half = collider.scale * 0.5f;
    glm::vec3 minBounds = collider.pos - half;
    glm::vec3 maxBounds = collider.pos + half;

    return (
        p.x + radius > minBounds.x && p.x - radius < maxBounds.x &&
        p.y + radius > minBounds.y && p.y - radius < maxBounds.y &&
        p.z + radius > minBounds.z && p.z - radius < maxBounds.z
        );
}

bool SceneBasic_Uniform::pointInsideMirrorOBB(const glm::vec3& p, const MirrorData& mirror, float radius) const
{
    glm::mat4 modelMat(1.0f);
    modelMat = glm::translate(modelMat, mirror.pos);
    modelMat = glm::rotate(modelMat, glm::radians(mirror.angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::scale(modelMat, mirror.scale);

    glm::mat4 invModel = glm::inverse(modelMat);
    glm::vec3 local = glm::vec3(invModel * glm::vec4(p, 1.0f));

    return (
        local.x + radius > -0.5f && local.x - radius < 0.5f &&
        local.y + radius > -0.5f && local.y - radius < 0.5f &&
        local.z + radius > -0.5f && local.z - radius < 0.5f
        );
}

bool SceneBasic_Uniform::pointCollidesWithScene(const glm::vec3& p, float radius) const
{
    for (const auto& mirror : mirrors)
    {
        if (pointInsideMirrorOBB(p, mirror, radius))
            return true;

        ColliderData stand;
        stand.pos = glm::vec3(mirror.pos.x, 0.5f, mirror.pos.z);
        stand.scale = glm::vec3(0.4f, 1.0f, 0.4f);

        if (pointInsideAABB(p, stand, radius))
            return true;
    }

    for (const auto& collider : staticColliders)
    {
        if (pointInsideAABB(p, collider, radius))
            return true;
    }

    return false;
}

void SceneBasic_Uniform::buildStaticColliders()
{
    staticColliders.clear();

    staticColliders.push_back({ glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(2.0f, 2.0f, 2.0f) });
    staticColliders.push_back({ glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(4.0f, 0.5f, 4.0f) });

    staticColliders.push_back({ glm::vec3(-12.0f, 1.0f, 0.0f), glm::vec3(1.5f, 2.0f, 1.5f) });

    for (const auto& mirror : mirrors)
    {
        staticColliders.push_back({
            glm::vec3(mirror.pos.x, 0.5f, mirror.pos.z),
            glm::vec3(0.4f, 1.0f, 0.4f)
            });
    }

    for (const auto& obstacle : obstacles)
    {
        staticColliders.push_back({ obstacle.pos, obstacle.scale });
    }
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

bool SceneBasic_Uniform::rayHitsMirror(
    const glm::vec3& rayStart,
    const glm::vec3& rayDir,
    const MirrorData& mirror,
    float& hitDistance,
    glm::vec3& hitPoint)
{
    glm::vec3 planePoint = mirror.pos;
    glm::vec3 planeNormal = getMirrorNormal(mirror.angle);

    float denom = glm::dot(rayDir, planeNormal);
    if (fabs(denom) < 0.0001f)
        return false;

    float t = glm::dot(planePoint - rayStart, planeNormal) / denom;
    if (t <= 0.001f)
        return false;

    glm::vec3 p = rayStart + rayDir * t;

    glm::mat4 modelMat(1.0f);
    modelMat = glm::translate(modelMat, mirror.pos);
    modelMat = glm::rotate(modelMat, glm::radians(mirror.angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::scale(modelMat, mirror.scale);

    glm::mat4 invModel = glm::inverse(modelMat);
    glm::vec3 local = glm::vec3(invModel * glm::vec4(p, 1.0f));

    if (fabs(local.x) <= 0.5f + 0.01f &&
        fabs(local.y) <= 0.5f + 0.01f &&
        fabs(local.z) <= 0.7f)
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

bool SceneBasic_Uniform::rayHitsObstacle(
    const glm::vec3& rayStart,
    const glm::vec3& rayDir,
    const ObstacleData& obstacle,
    float& hitDistance,
    glm::vec3& hitPoint)
{
    glm::vec3 half = obstacle.scale * 0.5f;
    glm::vec3 minB = obstacle.pos - half;
    glm::vec3 maxB = obstacle.pos + half;

    float tMin = 0.0f;
    float tMax = 1e9f;

    for (int i = 0; i < 3; ++i)
    {
        if (fabs(rayDir[i]) < 0.0001f)
        {
            if (rayStart[i] < minB[i] || rayStart[i] > maxB[i])
                return false;
        }
        else
        {
            float invD = 1.0f / rayDir[i];
            float t1 = (minB[i] - rayStart[i]) * invD;
            float t2 = (maxB[i] - rayStart[i]) * invD;

            if (t1 > t2) std::swap(t1, t2);

            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            if (tMin > tMax)
                return false;
        }
    }

    if (tMin <= 0.001f)
        return false;

    hitDistance = tMin;
    hitPoint = rayStart + rayDir * tMin;
    return true;
}

bool SceneBasic_Uniform::findClosestHitObstacle(
    const glm::vec3& rayStart,
    const glm::vec3& rayDir,
    float& hitDistance,
    glm::vec3& hitPoint)
{
    bool found = false;
    float bestDistance = 1e9f;
    glm::vec3 bestPoint(0.0f);

    for (const auto& obstacle : obstacles)
    {
        float t;
        glm::vec3 p;
        if (rayHitsObstacle(rayStart, rayDir, obstacle, t, p))
        {
            if (t < bestDistance)
            {
                bestDistance = t;
                bestPoint = p;
                found = true;
            }
        }
    }

    if (found)
    {
        hitDistance = bestDistance;
        hitPoint = bestPoint;
    }

    return found;
}

void SceneBasic_Uniform::drawBeamPath()
{
    reactorActivated = false;

    glm::vec3 reactorPos(0.0f, 1.5f, 0.0f);
    float reactorRadius = 1.5f;

    float pulse = 0.75f + 0.25f * sin(tPrev * 3.0f);
    glm::vec3 beamColor = pulse * glm::vec3(0.2f, 0.9f, 1.0f);
    glm::vec3 hotBeamColor = pulse * glm::vec3(1.0f, 0.5f, 0.2f);

    glm::vec3 emitterMuzzle = emitterPos + glm::vec3(0.0f, 0.6f, 0.0f) + getEmitterDirection(emitterAngle) * 1.0f;

    glm::vec3 visualStart = emitterMuzzle;
    glm::vec3 logicStart = emitterMuzzle;
    glm::vec3 rayDir = getEmitterDirection(emitterAngle);

    const int maxBounces = 8;
    const float maxSegmentLength = 50.0f;

    for (int bounce = 0; bounce < maxBounces; ++bounce)
    {
        float mirrorHitDistance = 1e9f;
        glm::vec3 mirrorHitPoint(0.0f);
        int mirrorIndex = findClosestHitMirror(logicStart, rayDir, mirrorHitDistance, mirrorHitPoint);

        float obstacleHitDistance = 1e9f;
        glm::vec3 obstacleHitPoint(0.0f);
        bool hitObstacle = findClosestHitObstacle(logicStart, rayDir, obstacleHitDistance, obstacleHitPoint);

        float maxDistance = maxSegmentLength;
        if (mirrorIndex >= 0) maxDistance = std::min(maxDistance, mirrorHitDistance);
        if (hitObstacle) maxDistance = std::min(maxDistance, obstacleHitDistance);

        glm::vec3 reactorHitPoint;
        bool hitReactor = rayHitsReactorBeforeDistance(
            logicStart, rayDir, reactorPos, reactorRadius, maxDistance, reactorHitPoint
        );

        if (hitReactor)
        {
            drawBeam(visualStart, reactorHitPoint, (bounce == 0 ? hotBeamColor : beamColor), 0.10f);
            reactorActivated = true;
            return;
        }

        bool obstacleIsFirst = hitObstacle && obstacleHitDistance <= mirrorHitDistance;
        if (obstacleIsFirst)
        {
            drawBeam(visualStart, obstacleHitPoint, (bounce == 0 ? hotBeamColor : beamColor), 0.10f);
            return;
        }

        if (mirrorIndex >= 0)
        {
            drawBeam(visualStart, mirrorHitPoint, (bounce == 0 ? hotBeamColor : beamColor), 0.10f);

            glm::vec3 mirrorNormal = getMirrorNormal(mirrors[mirrorIndex].angle);
            if (glm::dot(rayDir, mirrorNormal) > 0.0f)
                mirrorNormal = -mirrorNormal;

            glm::vec3 reflectedDir = glm::normalize(glm::reflect(rayDir, mirrorNormal));

            visualStart = mirrorHitPoint;
            logicStart = mirrorHitPoint + reflectedDir * 0.02f;
            rayDir = reflectedDir;
            continue;
        }

        drawBeam(visualStart, visualStart + rayDir * maxSegmentLength, (bounce == 0 ? hotBeamColor : beamColor), 0.10f);
        return;
    }

    drawBeam(visualStart, visualStart + rayDir * 10.0f, beamColor, 0.10f);
}

glm::vec3 SceneBasic_Uniform::getEmitterDirection(float degrees)
{
    float r = glm::radians(degrees);
    return glm::normalize(glm::vec3(cos(r), 0.0f, sin(r)));
}

bool SceneBasic_Uniform::isLookingAtEmitter() const
{
    glm::vec3 toEmitter = glm::normalize(emitterPos - cameraPos);
    float score = glm::dot(cameraFront, toEmitter);
    float dist = glm::length(emitterPos - cameraPos);


    return (score > 0.95f && dist < 12.0f);
}