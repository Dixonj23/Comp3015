#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"

#include <glad/glad.h>
#include "helper/glslprogram.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "helper/plane.h"
#include "helper/objmesh.h"
#include "helper/torus.h"
#include "helper/teapot.h"
#include "helper/cube.h"
#include "helper/skybox.h"

class SceneBasic_Uniform : public Scene
{
private:
    GLSLProgram prog;
    Plane plane;
    Cube cube;
    std::unique_ptr<ObjMesh> mesh;
    std::unique_ptr<ObjMesh> pillar;
    std::unique_ptr<ObjMesh> ritualMesh;
    SkyBox sky;
    GLuint fboHandle;

    // Plane textures
    GLuint planeTex1, planeTex2, planeNormal;

    // Statue textures
    GLuint statueTex1, statueTex2, statueNormal;

    // Pillar textures
    GLuint pillarTex1, pillarTex2, pillarNormal;

    GLuint corruptTex;

    float rotSpeed;
    float tPrev;
    float angle;

    //game stuff
    enum GameState { PUZZLE, PRE_RITUAL, RITUAL, CHARGE, JUMPSCARE };

    GameState gameState = PUZZLE;

    float ritualTimer = 0.0f;
    float ritualDelayTimer = 0.0f;
    float ritualDelay = 2.5f; // seconds pause before ritual
    float jumpscareTimer = 0.0f;
    bool lightViewMode = false;

    bool debugLights = true;
    bool lightSolved[3] = { false, false, false };
    glm::vec3 statueTargets[3] =
    {
        glm::vec3(-0.4f, 7.0f, -1.2f),
        glm::vec3(-0.4f, 5.0f, -1.2f),
        glm::vec3(0.6f, 3.0f, -1.2f)
    };

    bool waitingForNextLight = false;
    float lightSwitchTimer = 0.0f;
    float lightSwitchDelay = 1.0f; // second pause before switch lights

    //ritual params
    float ritualStartPos = -2.0f;
    float ritualTargetPos = 1.5f;
    float ritualMoveSpeed = 1.5f;
    bool ritualInPosition = false;

    //Camera controls
    bool keyW = false, keyA = false, keyS = false, keyD = false, keyQ = false, keyE = false, keyC = false, key1 = false, key2 = false, key3 = false;

    glm::vec3 camPos = glm::vec3(0.0f, 6.0f, 9.0f);
    float camSpeed = 4.0f;
    float camAngle;
    float camRadius;
    float camHeight;


    //Light controls
    int activeLight = 0;
    glm::vec3 lightPositions[3] = {
        glm::vec3(0.0f, 4.0f,  6.0f),
        glm::vec3(5.0f, 4.0f,  -6.0f),
        glm::vec3(-5.0f, 4.0f, -6.0f)
    };

    glm::vec3 lightDirections[3] = {
        glm::normalize(glm::vec3(0.0f, 2.0f, 0.0f) - lightPositions[0]),
        glm::normalize(glm::vec3(0.0f, 2.0f, 0.0f) - lightPositions[1]),
        glm::normalize(glm::vec3(0.0f, 2.0f, 0.0f) - lightPositions[2])
    };

    float lightYaw[3] = { 0,0,0 };
    float lightPitch[3] = { 0,0,0 };

    void keyInput(int key, int action) override;
    void setLights();
    bool checkPuzzleSolved();
    bool lightHitsRitual();

    void compile();
    void setMatrices();

    void setupFBO();
    void renderScene();

public:
    SceneBasic_Uniform();

    void initScene();
    void update( float t );
    void render();
    void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
