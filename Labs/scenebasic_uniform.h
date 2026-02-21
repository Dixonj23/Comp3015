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
    //Torus torus;
    //Teapot teapot;
    //GLuint brick, moss;
    Cube cube;
    //std::unique_ptr<ObjMesh> mesh;
    //SkyBox sky;
    std::unique_ptr<ObjMesh> spot;
    GLuint fboHandle;

    float rotSpeed;
    float tPrev;
    float angle;
    

    void compile();
    void setMatrices();

    void setupFBO();
    void renderToTexture();
    void renderScene();

public:
    SceneBasic_Uniform();

    void initScene();
    void update( float t );
    void render();
    void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
