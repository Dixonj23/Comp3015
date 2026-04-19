#pragma once

#include <glm/glm.hpp>

class Scene
{
protected:
    glm::mat4 model, view, projection;

public:
    int width;
    int height;

    Scene() : m_animate(true), width(800), height(600) {}
    virtual ~Scene() {}
    virtual void keyInput(int key, int action) {}
    virtual void mouseInput(double x, double y) {}

    void setDimensions(int w, int h) {
        width = w;
        height = h;
    }

    virtual void initScene() = 0;
    virtual void update(float t) = 0;
    virtual void render() = 0;
    virtual void resize(int, int) = 0;

    void animate(bool value) { m_animate = value; }
    bool animating() { return m_animate; }

protected:
    bool m_animate;
};