#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"
#include "helper/glslprogram.h"
#include "helper/plane.h"
#include "helper/cube.h"

#include <vector>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class SceneBasic_Uniform : public Scene
{
private:
    GLSLProgram prog;

    Plane plane;
    Cube cube;

    GLuint planeTex = 0;
    GLuint planeNormal = 0;

    float tPrev;

    //camera/player
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    glm::vec3 cameraRight;
    glm::vec3 worldUp;

    //mirrors
    struct MirrorData
    {
        glm::vec3 pos;
        float angle;
        glm::vec3 scale;
    };

    std::vector<MirrorData> mirrors;
    int selectedMirrorIndex;

    float yaw;
    float pitch;
    float moveSpeed;
    float mouseSensitivity;

    bool keyW, keyA, keyS, keyD, keyQ, keyE;
    bool firstMouse;
    float lastMouseX;
    float lastMouseY;

    bool reactorActivated;

    //bloom
    GLSLProgram blurProg;
    GLSLProgram finalProg;

    GLuint hdrFBO;
    GLuint colorBuffers[2];
    GLuint pingpongFBO[2];
    GLuint pingpongColorbuffers[2];
    GLuint rboDepth;

    GLuint quadVAO;
    GLuint quadVBO;

    bool bloomEnabled;
    float bloomExposure;


    void compile();
    void setMatrices();
    void updateCameraVectors();

    void drawCube(
        const glm::vec3& position,
        const glm::vec3& scale,
        const glm::vec3& color
    );

    void drawBeam(
        const glm::vec3& start,
        const glm::vec3& end,
        const glm::vec3& color,
        float thickness = 0.12f
    );

    glm::vec3 getMirrorNormal(float degrees);
    int findLookedAtMirror() const;
    void drawMirror(const MirrorData& mirror, bool selected);

    void drawBeamPath();
    bool rayHitsMirror(
        const glm::vec3& rayStart,
        const glm::vec3& rayDir,
        const MirrorData& mirror,
        float& hitDistance,
        glm::vec3& hitPoint
    );

    int findClosestHitMirror(
        const glm::vec3& rayStart,
        const glm::vec3& rayDir,
        float& hitDistance,
        glm::vec3& hitPoint
    );

    bool rayHitsReactorBeforeDistance(
        const glm::vec3& rayStart,
        const glm::vec3& rayDir,
        const glm::vec3& reactorPos,
        float reactorRadius,
        float maxDistance,
        glm::vec3& hitPoint
    );

    void setupBloomBuffers();
    void setupScreenQuad();
    void renderQuad();
    void renderSceneGeometry();

public:
    SceneBasic_Uniform();

    void initScene() override;
    void update(float t) override;
    void render() override;
    void resize(int w, int h) override;

    void keyInput(int key, int action) override;
    void mouseInput(double x, double y) override;
};

#endif