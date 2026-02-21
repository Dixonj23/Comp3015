#include "scenebasic_uniform.h"

/*
    Lab 1: Done
    Lab 2: Done
    Lab 3: Done
    Lab 4: Done
    CW1 time

*/

#include <cstdio>
#include <cstdlib>

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
    tPrev(0), angle(0.0f), rotSpeed(glm::pi<float>() / 8.0f),
    //sky(100.0f),
    plane(100.0f, 100.0f, 1, 1)
    //teapot(14, glm::mat4(1.0f))
    //torus(1.75f * 0.75, 0.75f * 0.75, 50, 50)
{
}


void SceneBasic_Uniform::initScene()
{
    compile();

    glEnable(GL_DEPTH_TEST);
    //model = mat4(1.0f);
    projection = mat4(1.0f);
    angle = 0.0f;

    //light
    prog.setUniform("Light.L", vec3(1.0f));
    prog.setUniform("Light.La", vec3(0.05f));
    

    GLuint spotTexture = Texture::loadTexture("media/spot/spot_texture.png");
    spot = ObjMesh::load("../Labs/media/spot/spot_triangulated.obj");
   
    setupFBO();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, spotTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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
    if (tPrev == 0.0f) 
    {
        deltaT = 0.0f;
    }
    tPrev = t;
    angle += rotSpeed * deltaT;
    
    if (this->m_animate) {
        angle += rotSpeed * deltaT;
        if (angle > glm::two_pi<float>()) angle -= glm::two_pi<float>();
    }
}

void SceneBasic_Uniform::render() {
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
    
    renderToTexture();
    glFlush();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    renderScene();
    glFlush();
}

void SceneBasic_Uniform::renderToTexture()
{

    prog.setUniform("RenderTex", 1);
    glViewport(0, 0, 512, 512);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    view = glm::lookAt(vec3(0.0f, 0.0f, 2.5f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    //vec4 lightPos = vec4(10.0f * cos(angle), 10.0f, 10.0f * sin(angle), 1.0f);
    prog.setUniform("Light.Position", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));


    //Render Spot
    
    //prog.setUniform("Material.Kd", vec3(0.2f, 0.55f, 0.9f));
    prog.setUniform("Material.Ks", vec3(0.95f, 0.95f, 0.95f));
    //prog.setUniform("Material.Ka", vec3(0.2f * 0.3f, 0.55f * 0.3f, 0.9f * 0.3f));
    prog.setUniform("Material.Shininess", 100.0f);

    model = mat4(1.0f);
    model = glm::rotate(model, angle, vec3(0.0f, 1.0f, 0.0f));
    setMatrices();
    spot->render();
    
}

void SceneBasic_Uniform::renderScene() {
    prog.setUniform("RenderTex", 0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vec3 cameraPos = vec3(2.0f * cos(angle), 1.5f, 2.0f * sin(angle));
    view = glm::lookAt(cameraPos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    
    projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.3f, 100.0f);
   
    //vec4 lightPos = vec4(10.0f * cos(angle), 10.0f, 10.0f * sin(angle), 1.0f);
    prog.setUniform("Light.Position", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));


    //Render Cube

    //prog.setUniform("Material.Kd", vec3(0.2f, 0.55f, 0.9f));
    prog.setUniform("Material.Ks", vec3(0.0f, 0.0f, 0.0f));
    //prog.setUniform("Material.Ka", vec3(0.2f * 0.3f, 0.55f * 0.3f, 0.9f * 0.3f));
    prog.setUniform("Material.Shininess", 1.0f);

    model = mat4(1.0f);
    setMatrices();
    cube.render();
}

void SceneBasic_Uniform::resize(int w, int h)
{
    glViewport(0, 0, w, h);
    width = w;
    height = h;


    projection = glm::perspective(glm::radians(70.0f), (float)w / h, 0.3f, 100.0f);
}

void SceneBasic_Uniform::setMatrices() {
    glm::mat4 mv = view * model;
    prog.setUniform("ModelMatrix", model);
    prog.setUniform("ModelViewMatrix", mv);
    prog.setUniform("NormalMatrix", glm::mat3(vec3(mv[0]), vec3(mv[1]), vec3(mv[2])));

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