#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"
#include "helper/glslprogram.h"
#include "helper/plane.h"
#include "helper/cube.h"
#include "helper/objmesh.h"

#include <array>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class SceneBasic_Uniform : public Scene
{
private:
    // ============================================================
    // Shader programs
    // ============================================================

    GLSLProgram prog;
    GLSLProgram blurProg;
    GLSLProgram finalProg;
    GLSLProgram pointShadowProg;
    GLSLProgram hudProg;

    // ============================================================
    // Basic geometry
    // ============================================================

    Plane plane;
    Cube cube;

    // ============================================================
    // Imported models
    // ============================================================

    std::unique_ptr<ObjMesh> reactorModel;
    std::unique_ptr<ObjMesh> emitterModel;
    std::unique_ptr<ObjMesh> mirrorModel;
    std::vector<std::unique_ptr<ObjMesh>> obstacleModels;

    // ============================================================
    // Texture sets
    // ============================================================

    struct ModelTextureSet
    {
        GLuint diffuse = 0;
        GLuint normal = 0;
        GLuint metalness = 0;
        GLuint emissive = 0;

        bool hasMetalness = false;
        bool hasEmissive = false;
    };

    ModelTextureSet emitterTextures;
    ModelTextureSet mirrorTextures;
    ModelTextureSet reactorTextures;
    std::vector<ModelTextureSet> obstacleTextureSets;

    GLuint planeTex = 0;
    GLuint planeNormal = 0;

    GLuint wallTex = 0;
    GLuint wallNormal = 0;

    GLuint ceilingTex = 0;
    GLuint ceilingNormal = 0;

    GLuint defaultBlackTex = 0;
    GLuint defaultWhiteTex = 0;

    GLuint createSolidTexture(
        unsigned char r,
        unsigned char g,
        unsigned char b,
        unsigned char a = 255
    );

    // ============================================================
    // Room settings
    // ============================================================

    float roomSize;
    float roomHalfSize;

    // ============================================================
    // Camera / player movement
    // ============================================================

    float tPrev;

    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    glm::vec3 cameraRight;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float moveSpeed;
    float mouseSensitivity;

    bool keyW;
    bool keyA;
    bool keyS;
    bool keyD;
    bool keyQ;
    bool keyE;

    bool firstMouse;
    float lastMouseX;
    float lastMouseY;

    void updateCameraVectors();

    // ============================================================
    // Gameplay objects
    // ============================================================

    struct MirrorData
    {
        glm::vec3 pos;
        float angle;
        glm::vec3 scale;
    };

    struct EmitterData
    {
        glm::vec3 pos;
        float angle;
    };

    struct ObstacleData
    {
        glm::vec3 pos;
        glm::vec3 scale;
        int modelIndex;
        float angle;
    };

    std::vector<MirrorData> mirrors;
    std::vector<EmitterData> emitters;
    std::vector<ObstacleData> obstacles;

    std::vector<glm::vec3> obstacleBaseScales;

    int selectedMirrorIndex;
    int reactorBeamHits;

    glm::vec3 getEmitterDirection(float degrees);
    glm::vec3 getMirrorNormal(float degrees);

    int findLookedAtMirror() const;

    // ============================================================
    // Procedural puzzle generation
    // ============================================================

    struct BeamSegment
    {
        glm::vec3 start;
        glm::vec3 end;
    };

    struct SolutionRoute
    {
        glm::vec3 emitterPos;
        float emitterAngle;

        glm::vec3 mirror1Pos;
        float mirror1Angle;

        glm::vec3 mirror2Pos;
        float mirror2Angle;
    };

    std::vector<BeamSegment> guaranteedPath;
    std::vector<SolutionRoute> solutionRoutes;

    glm::vec3 randomRoomPoint(float y, float margin);

    bool tryCreateSolutionRoute(SolutionRoute& route);
    void generateSolvableLayout(int routeCount);
    void generateMirrors(int count);
    void generateObstacles(int count);

    bool isPositionValid(const glm::vec3& pos, float radius);
    bool isNearBeamPath(const glm::vec3& pos, float radius) const;

    float distancePointToSegmentXZ(
        const glm::vec3& p,
        const glm::vec3& a,
        const glm::vec3& b
    ) const;

    float mirrorAngleForReflection(
        const glm::vec3& incomingDir,
        const glm::vec3& outgoingDir
    );

    void placePlayerNearReactor();

    // ============================================================
    // Collision
    // ============================================================

    struct ColliderData
    {
        glm::vec3 pos;
        glm::vec3 scale;
    };

    std::vector<ColliderData> staticColliders;

    bool pointInsideAABB(
        const glm::vec3& p,
        const ColliderData& collider,
        float radius
    ) const;

    bool pointInsideMirrorOBB(
        const glm::vec3& p,
        const MirrorData& mirror,
        float radius
    ) const;

    bool pointCollidesWithScene(
        const glm::vec3& p,
        float radius
    ) const;

    void buildStaticColliders();

    // ============================================================
    // Reactor state
    // ============================================================

    bool reactorActivated;
    float reactorLightLevel;

    bool reactorWasActiveLastFrame;
    float reactorPulseTimer;

    float reactorActiveTimer;
    bool showResetPrompt;
    bool resetKeyHeld;

    // ============================================================
    // Reactor particles
    // ============================================================

    struct ReactorParticle
    {
        glm::vec3 pos;
        glm::vec3 vel;
        glm::vec3 color;

        float life;
        float maxLife;
        float size;
    };

    std::vector<ReactorParticle> reactorParticles;

    void initReactorParticles();
    void updateReactorParticles(float dt);
    void respawnReactorParticle(ReactorParticle& p);
    void drawReactorParticles();
    void spawnReactorBurst(int count);

    // ============================================================
    // Lighting
    // ============================================================

    std::vector<glm::vec3> cornerLightPositions;

    // ============================================================
    // Point light shadow cubemap
    // ============================================================

    GLuint pointShadowFBO;
    GLuint pointShadowCube;

    float shadowFarPlane;
    glm::vec3 shadowLightPos;
    std::array<glm::mat4, 6> shadowTransforms;

    void setupPointShadowMap();
    void buildPointShadowTransforms();
    void renderPointShadowPass();
    void renderShadowGeometry();
    void setPointShadowMatrices();

    // ============================================================
    // Bloom / post-processing
    // ============================================================

    GLuint hdrFBO;
    GLuint colorBuffers[2];
    GLuint pingpongFBO[2];
    GLuint pingpongColorbuffers[2];
    GLuint rboDepth;

    GLuint quadVAO;
    GLuint quadVBO;

    bool bloomEnabled;
    float bloomExposure;

    void setupBloomBuffers();
    void setupScreenQuad();
    void renderQuad();

    // ============================================================
    // HUD
    // ============================================================

    GLuint hudVAO = 0;
    GLuint hudVBO = 0;

    void setupHUD();
    void renderHUD();

    void drawHUDLines(
        const std::vector<float>& verts,
        const glm::vec3& color
    );

    // ============================================================
    // Draw helpers
    // ============================================================

    void setMatrices();

    void drawCube(
        const glm::vec3& position,
        const glm::vec3& scale,
        const glm::vec3& color
    );

    void drawTexturedCube(
        GLuint diffuseTex,
        GLuint normalTex,
        const glm::vec3& position,
        const glm::vec3& scale
    );

    void drawTexturedModel(
        ObjMesh* mesh,
        const ModelTextureSet& textures,
        const glm::vec3& position,
        const glm::vec3& scale,
        float angleDegrees,
        float emissiveStrength = 0.0f
    );

    void drawBeam(
        const glm::vec3& start,
        const glm::vec3& end,
        const glm::vec3& color,
        float thickness = 0.12f
    );

    // ============================================================
    // Beam / mirror puzzle logic
    // ============================================================

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

    bool drawBeamPathFromEmitter(const EmitterData& emitter);
    void drawAllBeamPaths();

    // ============================================================
    // Main render/setup helpers
    // ============================================================

    void compile();
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