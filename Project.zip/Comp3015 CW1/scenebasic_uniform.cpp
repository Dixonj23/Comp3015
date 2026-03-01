#include "scenebasic_uniform.h"

/*
    Added Git repo
    Made first commit
*/

#include <cstdio>
#include <cstdlib>

#include <GLFW/glfw3.h>
extern GLFWwindow* window;

#include <string>
using std::string;

#include <iostream>
using std::cerr;
using std::endl;

#include <glm/gtc/matrix_transform.hpp>

#include "helper/glutils.h"
#include "helper/texture.h"

#include <sstream>
#include "helper/stb/stb_image.h"

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;


SceneBasic_Uniform::SceneBasic_Uniform() :
    tPrev(0), angle(0.0f), rotSpeed(glm::pi<float>() / 8.0f), camAngle(4.7f), camRadius(-5.0f), camHeight(6.5f),
    sky(100.0f),
    plane(100.0f, 100.0f, 1, 1)
    {
        mesh = ObjMesh::load("media/portal.obj", false, true);
        ritualMesh = ObjMesh::load("media/relic.obj", false, true);
        jumpscareMesh = ObjMesh::load("media/bs_ears.obj", false, true);
    }


bool cPressedLastFrame = false;
float baseYaw[3];

float globalTime = 0.0f;
float targetSolveTime[3] = { -1.0f, -1.0f, -1.0f };

void SceneBasic_Uniform::keyInput(int key, int action)
{
    bool down = (action == GLFW_PRESS);

    switch (key)
    {
    case GLFW_KEY_W: keyW = down; break;
    case GLFW_KEY_A: keyA = down; break;
    case GLFW_KEY_S: keyS = down; break;
    case GLFW_KEY_D: keyD = down; break;
    case GLFW_KEY_Q: keyQ = down; break;
    case GLFW_KEY_E: keyE = down; break;

    case GLFW_KEY_1: key1 = down; break;
    case GLFW_KEY_2: key2 = down; break;
    case GLFW_KEY_3: key3 = down; break;

    case GLFW_KEY_C: if (action == GLFW_PRESS && !cPressedLastFrame)
        {
            lightViewMode = !lightViewMode;
            cPressedLastFrame = true;
        }

        if (action == GLFW_RELEASE)
        {
            cPressedLastFrame = false;
        }
        break;
    default: break;
    }
}

unsigned int loadCubeMap(const std::vector<std::string>& faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

        if (data)
        {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0,
                format,
                width,
                height,
                0,
                format,
                GL_UNSIGNED_BYTE,
                data
            );

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void SceneBasic_Uniform::initScene()
{
    compile();
    glEnable(GL_DEPTH_TEST);
    model = mat4(1.0f);
    projection = mat4(1.0f);
    angle = 0.0f;
    
    // Plane Textures
    planeTex1 = Texture::loadTexture("media/texture/brick1.jpg");
    planeTex2 = Texture::loadTexture("media/texture/brick1.jpg");
    planeNormal = Texture::loadTexture("media/texture/ogre_normalmap.png");

    // Statue Textures
    statueTex1 = Texture::loadTexture("media/textures/EntradaBossSP_EntradaTexture3_BaseColor.png");
    //statueTex2 = Texture::loadTexture("media/textures/EntradaBossSP_EntradaTexture4_BaseColor.png");
    statueTex2 = Texture::loadTexture("media/texture/cement.jpg");
    statueNormal = Texture::loadTexture("media/textures/EntradaBossSP_EntradaTexture3_Normal.png");

    //corrupted statue tex
    corruptTex = Texture::loadTexture("media/textures/EntradaBossSP_EntradaTexture4_BaseColor.png");
    prog.setUniform("CorruptTex", 3);

    //skybox texture
    std::vector<std::string> faces =
    {
        "media/texture/cube/sky/px.png",
        "media/texture/cube/sky/nx.png",
        "media/texture/cube/sky/ny.png",
        "media/texture/cube/sky/py.png",
        "media/texture/cube/sky/pz.png",
        "media/texture/cube/sky/nz.png"
    };

    cubeTex = loadCubeMap(faces);
    prog.setUniform("SkyBoxTex", 4);

    //setupFBO();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    //Fog
    prog.setUniform("Fog.Color", glm::vec3(0.02f, 0.02f, 0.03f));
    prog.setUniform("Fog.MaxDist", 20.0f);
    prog.setUniform("Fog.MinDist", 1.0f);

    // Initialise light directions to point at statue
    glm::vec3 target(0.0f, 2.0f, 0.0f); // center of statue

    for (int i = 0; i < 3; i++)
    {
        glm::vec3 dir = glm::normalize(target - lightPositions[i]);

        // Calculate pitch
        lightPitch[i] = asin(dir.y);

        // Calculate yaw
        lightYaw[i] = atan2(dir.x, dir.z);
        baseYaw[i] = lightYaw[i];

        // Store direction
        lightDirections[i] = dir;
    }
}

void SceneBasic_Uniform::compile()
{
	try {
		prog.compileShader("shader/basic_uniform.vert");
		prog.compileShader("shader/basic_uniform.frag");
		prog.link();
		prog.use();
	} catch (GLSLProgramException &e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}


bool SceneBasic_Uniform::checkPuzzleSolved()
{
    return lightSolved[0] && lightSolved[1] && lightSolved[2];
}

bool SceneBasic_Uniform::lightHitsRitual()
{
    glm::vec3 ritualPos(0.0f, 2.5f, 0.0f);

    for (int i = 0;i < 3;i++)
    {
        glm::vec3 toRitual = glm::normalize(ritualPos - lightPositions[i]);

        float alignment = glm::dot(toRitual, lightDirections[i]);

        if (alignment > 0.95f)
            return true;
    }

    return false;
}

void SceneBasic_Uniform::update(float t)
{
    //global timer
    globalTime = t;

    //delta timer
    float deltaT = t - tPrev;
    if (tPrev == 0.0f) {
        deltaT = 0.0f;
    }
    tPrev = t;
    angle += rotSpeed * deltaT;

    //camera movement
    if (!lightViewMode) {
        if (keyA) camAngle -= 1.5f * deltaT;
        if (keyD) camAngle += 1.5f * deltaT;

        if (keyW) camRadius -= 3.0f * deltaT;
        if (keyS) camRadius += 3.0f * deltaT;

        if (keyQ) camHeight -= 3.0f * deltaT;
        if (keyE) camHeight += 3.0f * deltaT;


        //printf("%.1f \n", camAngle);
        //printf("%.1f \n", camRadius);
        //printf("%.1f \n", camHeight);
    }
    //spotlight control / solving
    if (lightViewMode)
    {
        int i = activeLight;

        // only allow movement if not solved and not waiting
        if (!lightSolved[i] && !waitingForNextLight)
        {
            float rotSpeed = 1.5f;

            if (keyW) lightPitch[i] += rotSpeed * deltaT;
            if (keyS) lightPitch[i] -= rotSpeed * deltaT;
            if (keyA) lightYaw[i] += rotSpeed * deltaT;
            if (keyD) lightYaw[i] -= rotSpeed * deltaT;

            // clamp and rebuild direction for active light
            lightPitch[i] = glm::clamp(lightPitch[i], -glm::radians(60.0f), glm::radians(60.0f));

            float yawLimit = glm::radians(60.0f);
            lightYaw[i] = glm::clamp(lightYaw[i], baseYaw[i] - yawLimit, baseYaw[i] + yawLimit);

            glm::vec3 dir;
            dir.x = cos(lightPitch[i]) * sin(lightYaw[i]);
            dir.y = sin(lightPitch[i]);
            dir.z = cos(lightPitch[i]) * cos(lightYaw[i]);
            lightDirections[i] = glm::normalize(dir);

            // SOLVE CHECK 
            glm::vec3 toTarget = glm::normalize(statueTargets[i] - lightPositions[i]);
            float alignment = glm::dot(toTarget, lightDirections[i]);
            float ang = acos(glm::clamp(alignment, -1.0f, 1.0f));

            if (ang < glm::radians(2.0f))   
            {
                lightSolved[i] = true;
                waitingForNextLight = true;
                lightSwitchTimer = 0.0f;

                if (targetSolveTime[i] < 0.0f)
                    targetSolveTime[i] = globalTime;   // start spreading now
            }
        }

        // switching lights should still work even while waiting
        if (gameState == PUZZLE && !waitingForNextLight)
        {
            if (key1) activeLight = 0;
            if (key2) activeLight = 1;
            if (key3) activeLight = 2;
        }
    }

    //delay before switching
    if (waitingForNextLight)
    {
        lightSwitchTimer += deltaT;

        if (lightSwitchTimer > lightSwitchDelay)
        {
            waitingForNextLight = false;

            // advance to the next UNSOLVED light
            int next = activeLight;
            for (int step = 0; step < 3; step++)
            {
                next = (next + 1) % 3;
                if (!lightSolved[next]) { activeLight = next; break; }
            }
        }
    }

    //game state
    if (gameState == PUZZLE)
    {
        if (checkPuzzleSolved())
        {
            gameState = PRE_RITUAL;
            ritualDelayTimer = 0.0f;

            lightViewMode = false;
            //activeLight = 0;   // force player to main spotlight
        }
    }

    if (gameState == PRE_RITUAL)
    {
        ritualDelayTimer += deltaT;

        if (ritualDelayTimer > ritualDelay)
        {
            gameState = RITUAL;
            ritualTimer = 0.0f;
        }
    }

    if (gameState == RITUAL)
    {
        ritualTimer += deltaT;

        if (!ritualInPosition)
        {
            ritualStartPos += ritualMoveSpeed * deltaT;

            if (ritualStartPos >= ritualTargetPos)
            {
                ritualStartPos = ritualTargetPos;
                ritualInPosition = true;
            }
        }
        else
        {
            // once in place wait before charge stage
            if (ritualTimer > 10.0f)
                gameState = CHARGE;
        }
    }

    if (gameState == CHARGE)
    {
        if (lightHitsRitual())
        {
            gameState = JUMPSCARE;
            jumpscareTimer = 0.0f;
        }
    }

    if (gameState == JUMPSCARE)
    {
        jumpscareTimer += deltaT;

        // move creature toward player
        jumpscareDistance -= jumpscareSpeed * deltaT;

        if (jumpscareDistance < 1.0f)
            jumpscareDistance = 1.0f;
    }
}

void SceneBasic_Uniform::setLights()
{
    glm::vec3 colours[3] = {
        glm::vec3(1.0f, 0.2f, 0.2f),  // red-ish
        glm::vec3(0.2f, 1.0f, 0.2f),  // green-ish
        glm::vec3(0.2f, 0.2f, 1.0f)   // blue-ish
    };

    for (int i = 0; i < 3; i++)
    {
        //target params
        prog.setUniform("ActiveLight", activeLight);

        prog.setUniform(("TargetPos[" + std::to_string(i) + "]").c_str(), statueTargets[i]);
        prog.setUniform(("TargetSolved[" + std::to_string(i) + "]").c_str(), lightSolved[i] ? 1 : 0);

        prog.setUniform("Time", globalTime);
        prog.setUniform("SpreadSpeed", 0.5f);      
        prog.setUniform("StartRadius", 0.25f);       
        prog.setUniform("MaxSpreadRadius", 3.0f);   

        for (int i = 0; i < 3; i++) {
            prog.setUniform(("TargetSolveTime[" + std::to_string(i) + "]").c_str(),
                (targetSolveTime[i] < 0.0f ? globalTime : targetSolveTime[i]));
        }


        glm::vec4 posEye = view * glm::vec4(lightPositions[i], 1.0f);
        glm::vec3 dirEye = glm::normalize(glm::mat3(view) * lightDirections[i]);

        std::stringstream name;

        // Position
        name << "Lights[" << i << "].Position";
        prog.setUniform(name.str().c_str(), posEye);
        name.str(""); name.clear();

        // Direction
        name << "Lights[" << i << "].Direction";
        prog.setUniform(name.str().c_str(), dirEye);
        name.str(""); name.clear();

        // ambient colour
        name << "Lights[" << i << "].La";
        prog.setUniform(name.str().c_str(), glm::vec3(0.05f));
        name.str(""); name.clear();

        //spotlight color
        glm::vec3 lightColor;

        if (lightSolved[i])
        {
            if (waitingForNextLight && i == activeLight)
                lightColor = glm::vec3(0.6f, 0.0f, 0.8f); // brighter purple flash
            else
                lightColor = glm::vec3(0.35f, 0.0f, 0.45f); // normal dark purple
        }
        else if (i == activeLight)
        {
            //brighter closer to target
            glm::vec3 toTarget = glm::normalize(statueTargets[i] - lightPositions[i]);
            float alignment = glm::dot(toTarget, lightDirections[i]);

            float t = glm::clamp((alignment - 0.8f) / 0.2f, 0.0f, 1.0f);

            float baseIntensity = 0.15f;
            float intensity = baseIntensity + (1.0f - baseIntensity) * t;

            lightColor = glm::vec3(intensity);
        }
        else
        {
            // dim inactive lights
            lightColor = glm::vec3(0.15f);
        }

        //ritual override
        if (gameState == RITUAL)
            lightColor = glm::vec3(1.0f, 0.0f, 0.0f);

        name << "Lights[" << i << "].L";
        prog.setUniform(name.str().c_str(), lightColor);
        name.str(""); name.clear();

        // spotlight params
        name << "Lights[" << i << "].Cutoff";
        prog.setUniform(name.str().c_str(), cos(glm::radians(5.0f)));
        name.str(""); name.clear();

        name << "Lights[" << i << "].Exponent";
        prog.setUniform(name.str().c_str(), 45.0f);
    }
}

void SceneBasic_Uniform::render() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    renderScene();
    glFlush();
}

void SceneBasic_Uniform::renderScene() {
    prog.setUniform("RenderTex", 0);
    glViewport(0, 0, width, height);

    glm::vec3 fogCol(0.005f, 0.005f, 0.01f);

    if (gameState == JUMPSCARE)
    {
        fogCol = glm::vec3(0.5f, 0.0f, 0.0f);
    }
    glClearColor(fogCol.r, fogCol.g, fogCol.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (lightViewMode)
    {
        // Camera sits at active light
        glm::vec3 eye = lightPositions[activeLight] + glm::vec3(0.0f, 0.6f, 0.0f);

        // Look in spotlight direction
        glm::vec3 center = eye + lightDirections[activeLight];

        view = glm::lookAt(
            eye,
            center,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
    else
    {
        // Normal orbit camera
        camPos.x = camRadius * cos(camAngle);
        camPos.z = camRadius * sin(camAngle);
        camPos.y = camHeight;

        view = glm::lookAt(
            camPos,
            glm::vec3(0.0f, 6.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }

    // Render Skybox
    glDepthMask(GL_FALSE);

    prog.setUniform("RenderSkybox", true);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex);

    model = mat4(1.0f);
    mat4 skyView = mat4(mat3(view)); // remove translation
    prog.setUniform("ModelViewMatrix", skyView * model);
    prog.setUniform("MVP", projection * skyView * model);
    sky.render();

    glDepthMask(GL_TRUE);

    prog.setUniform("RenderSkybox", false);
   
    //vec4 lightPos = vec4(10.0f * cos(angle), 10.0f, 10.0f * sin(angle), 1.0f);
    setLights();

    if (debugLights)
    {
        //Statue targets
        for (int i = 0;i < 3;i++)
        {
            model = mat4(1.0f);
            model = glm::translate(model, statueTargets[i]);
            model = glm::scale(model, glm::vec3(0.08f));

            prog.setUniform("Material.Kd", vec3(0.0f, 1.0f, 0.0f));

            setMatrices();
            cube.render();
        }
    }

    

    //bind statue textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, statueTex1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, statueTex2);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, statueNormal);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, corruptTex);

    //Render Mesh
    prog.setUniform("Material.Kd", vec3(0.4f, 0.4f, 0.4f));
    prog.setUniform("Material.Ks", vec3(0.5f, 0.5f, 0.5f));
    prog.setUniform("Material.Ka", vec3(0.9f ,0.9f, 0.9f));
    prog.setUniform("Material.Shininess", 100.0f);

    model = mat4(1.0f);
    model = glm::scale(glm::translate(model, vec3(0.0f, 0.0f, 0.0f)), vec3(0.05f));
    //model = glm::rotate(model, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    setMatrices();
    mesh->render();


    //bind plane textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, planeTex1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, planeTex2);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, planeNormal);

    //Render Plane
    prog.setUniform("Material.Kd", vec3(0.7f, 0.7f, 0.7f));
    prog.setUniform("Material.Ks", vec3(0.95f, 0.95f, 0.95f));
    prog.setUniform("Material.Ka", vec3(0.2f, 0.2f, 0.2f));
    prog.setUniform("Material.Shininess", 180.0f);

    model = mat4(1.0f);
    setMatrices();
    plane.render();
    
    
    //Render Ritual
    if (gameState == RITUAL || gameState == CHARGE)
    {
        model = mat4(1.0f);
        model = glm::scale(glm::translate(model, vec3(0.0f, 6.5f, ritualStartPos)), vec3(0.005f));

        if (ritualInPosition)
        {
            model = glm::rotate(model, ritualTimer * 5.0f, vec3(0, 1, 0));
        }

        setMatrices();
        ritualMesh->render();
    }

    if (gameState == JUMPSCARE)
    {
        vec3 forward = normalize(vec3(view[0][2], view[1][2], view[2][2])) * -1.0f;
        vec3 pos = camPos + forward * jumpscareDistance;

        model = mat4(1.0f);
        model = glm::translate(model, pos);
        float scale = 0.2f + jumpscareTimer ;

        model = glm::scale(model, vec3(scale));

        setMatrices();
        jumpscareMesh->render();
    }

}

void SceneBasic_Uniform::resize(int w, int h)
{
    glViewport(0, 0, w, h);
    width = w;
    height = h;

    if (lightViewMode)
        projection = glm::perspective(glm::radians(60.0f), (float)w / h, 0.3f, 100.0f);
    else
        projection = glm::perspective(glm::radians(70.0f), (float)w / h, 0.3f, 100.0f);
}

void SceneBasic_Uniform::setMatrices() {
    glm::mat4 mv = view * model;
    prog.setUniform("ModelMatrix", model);
    prog.setUniform("ModelViewMatrix", mv);

    glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(mv)));
    prog.setUniform("NormalMatrix", normalMat);

    prog.setUniform("MVP", projection * mv);
}

void SceneBasic_Uniform::setupFBO() {
    glGenFramebuffers(1, &fboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
    GLuint renderTex;
    glGenTextures(1, &renderTex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 512, 512);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTex, 0);

    GLuint depthBuf;
    glGenRenderbuffers(1, &depthBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 512, 512);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);
    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);
    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (result == GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is complete" << endl;
    }
    else {
        std::cout << "Framebuffer error: " << result << endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


