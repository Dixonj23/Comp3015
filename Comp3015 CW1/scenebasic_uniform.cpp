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

using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;


SceneBasic_Uniform::SceneBasic_Uniform() :
    tPrev(0), angle(0.0f), rotSpeed(glm::pi<float>() / 8.0f), camAngle(4.7f), camRadius(-5.0f), camHeight(3.5f),
    //sky(100.0f),
    plane(100.0f, 100.0f, 1, 1)
    {
        mesh = ObjMesh::load("media/portal.obj", false, true);
        pillar = ObjMesh::load("media/pillar/pillar.obj", false);
    }


bool cPressedLastFrame = false;
float baseYaw[3];

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
    statueTex2 = Texture::loadTexture("media/textures/EntradaBossSP_EntradaTexture4_BaseColor.png");
    //statueTex2 = Texture::loadTexture("media/texture/cement.jpg");
    statueNormal = Texture::loadTexture("media/textures/EntradaBossSP_EntradaTexture3_Normal.png");

    // Pillar Textures
    pillarTex1 = Texture::loadTexture("media/pillar/pillar_large_MAT_BaseColor.jpg");
    pillarTex2 = Texture::loadTexture("media/pillar/pillar_large_MAT_BaseColor.jpg");
    pillarNormal = Texture::loadTexture("media/pillar/pillar_large_MAT_Normal.jpg");

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

void SceneBasic_Uniform::update(float t)
{
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
    if (lightViewMode) {
        //spotlight control
        float rotSpeed = 1.5f;

        if (keyW) lightPitch[activeLight] += rotSpeed * deltaT;
        if (keyS) lightPitch[activeLight] -= rotSpeed * deltaT;
        if (keyA) lightYaw[activeLight] += rotSpeed * deltaT;
        if (keyD) lightYaw[activeLight] -= rotSpeed * deltaT;

        if (key1) activeLight = 0;
        if (key2) activeLight = 1;
        if (key3) activeLight = 2;


        for (int i = 0; i < 3; i++)
        {
            glm::vec3 dir;
            dir.x = cos(lightPitch[i]) * sin(lightYaw[i]);
            dir.y = sin(lightPitch[i]);
            dir.z = cos(lightPitch[i]) * cos(lightYaw[i]);

            lightPitch[activeLight] = glm::clamp(lightPitch[activeLight],
                -glm::radians(60.0f),
                glm::radians(60.0f));

            float yawLimit = glm::radians(60.0f);

            lightYaw[activeLight] = glm::clamp(
                lightYaw[activeLight],
                baseYaw[activeLight] - yawLimit,
                baseYaw[activeLight] + yawLimit
            );

            lightDirections[i] = glm::normalize(dir);
        }
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

        //light colour
        name << "Lights[" << i << "].L";
        //prog.setUniform(name.str().c_str(), glm::vec3(1.0f));
        prog.setUniform(name.str().c_str(), colours[i]);
        name.str(""); name.clear();

        // spotlight params
        name << "Lights[" << i << "].Cutoff";
        prog.setUniform(name.str().c_str(), cos(glm::radians(5.0f)));
        name.str(""); name.clear();

        name << "Lights[" << i << "].Exponent";
        prog.setUniform(name.str().c_str(), 45.0f);

        //selected light brighter
        if (i == activeLight)
            prog.setUniform(("Lights[" + std::to_string(i) + "].L").c_str(),
                glm::vec3(1.5f));
        else
            prog.setUniform(("Lights[" + std::to_string(i) + "].L").c_str(),
                colours[i]);
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
    glClearColor(fogCol.r, fogCol.g, fogCol.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



    if (lightViewMode)
    {
        // Camera sits at active light
        glm::vec3 eye = lightPositions[activeLight] + glm::vec3(0.0f, 0.2f, 0.0f);

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
            glm::vec3(0.0f, 2.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
   
    //vec4 lightPos = vec4(10.0f * cos(angle), 10.0f, 10.0f * sin(angle), 1.0f);
    setLights();

    //bind statue textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, statueTex1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, statueTex2);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, statueNormal);

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


    // bind pillar textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pillarTex1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pillarTex2);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, pillarNormal);

    //Render Pillar
    prog.setUniform("Material.Kd", vec3(0.4f, 0.4f, 0.4f));
    prog.setUniform("Material.Ks", vec3(0.5f, 0.5f, 0.5f));
    prog.setUniform("Material.Ka", vec3(0.9f, 0.9f, 0.9f));
    prog.setUniform("Material.Shininess", 100.0f);

    model = mat4(1.0f);
    model = glm::scale(glm::translate(model, vec3(0.0f, -0.5f, 0.0f)), vec3(0.0008f, 0.0005f, 0.0008f));
    setMatrices();
    //pillar->render();


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


