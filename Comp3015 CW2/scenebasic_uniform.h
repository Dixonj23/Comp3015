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
    GLSLProgram blurProg;
    GLSLProgram finalProg;
    GLSLProgram pointShadowProg;

    Plane plane;
    Cube cube;

    GLuint planeTex = 0;
    GLuint planeNormal = 0;

    float tPrev;

    // camera / player
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    glm::vec3 cameraRight;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float moveSpeed;
    float mouseSensitivity;

    bool keyW, keyA, keyS, keyD;
    bool firstMouse;
    float lastMouseX;
    float lastMouseY;

    // mirrors
    struct MirrorData
    {
        glm::vec3 pos;
        float angle;
        glm::vec3 scale;
    };
    std::vector<MirrorData> mirrors;
    int selectedMirrorIndex;

    // obstacles
    struct ObstacleData
    {
        glm::vec3 pos;
        glm::vec3 scale;
    };
    std::vector<ObstacleData> obstacles;

    // simple colliders
    struct ColliderData
    {
        glm::vec3 pos;
        glm::vec3 scale;
    };
    std::vector<ColliderData> staticColliders;

    // reactor/gameplay
    bool reactorActivated;
    float reactorLightLevel;

    // corner fixtures / corner point lights
    std::vector<glm::vec3> cornerLightPositions;

    // bloom
    GLuint hdrFBO;
    GLuint colorBuffers[2];
    GLuint pingpongFBO[2];
    GLuint pingpongColorbuffers[2];
    GLuint rboDepth;
    GLuint quadVAO;
    GLuint quadVBO;
    bool bloomEnabled;
    float bloomExposure;

    // point-shadow cubemap
    GLuint pointShadowFBO;
    GLuint pointShadowCube;
    float shadowFarPlane;
    glm::vec3 shadowLightPos;
    std::array<glm::mat4, 6> shadowTransforms;

    // core helpers
    void compile();
    void setMatrices();
    void setPointShadowMatrices();
    void updateCameraVectors();

    // bloom helpers
    void setupBloomBuffers();
    void setupScreenQuad();
    void renderQuad();

    // point-shadow helpers
    void setupPointShadowMap();
    void buildPointShadowTransforms();
    void renderPointShadowPass();
    void renderShadowGeometry();

    // visible scene render
    void renderSceneGeometry();

    // draw helpers
    void drawCube(
        const glm::vec3& position,
        const glm::vec3& scale,
        const glm::vec3& color
    );

    void drawMirror(const MirrorData& mirror, bool selected);

    void drawBeam(
        const glm::vec3& start,
        const glm::vec3& end,
        const glm::vec3& color,
        float thickness = 0.12f
    );

    // gameplay / beam logic
    glm::vec3 getMirrorNormal(float degrees);
    int findLookedAtMirror() const;

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

    bool rayHitsObstacle(
        const glm::vec3& rayStart,
        const glm::vec3& rayDir,
        const ObstacleData& obstacle,
        float& hitDistance,
        glm::vec3& hitPoint
    );

    bool findClosestHitObstacle(
        const glm::vec3& rayStart,
        const glm::vec3& rayDir,
        float& hitDistance,
        glm::vec3& hitPoint
    );

    void drawBeamPath();

    // collision helpers
    bool pointInsideAABB(const glm::vec3& p, const ColliderData& collider, float radius) const;
    bool pointInsideMirrorOBB(const glm::vec3& p, const MirrorData& mirror, float radius) const;
    bool pointCollidesWithScene(const glm::vec3& p, float radius) const;
    void buildStaticColliders();

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