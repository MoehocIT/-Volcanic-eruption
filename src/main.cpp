// Person 3: camera, lighting, controls and integration (C++98 / FreeGLUT)
#include "VolcanoScene.h"
#include "VolcanoEffects.h"
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

static const float PI = 3.14159265358979323846f;
static float yaw = 38.f, pitch = 27.f, dist = 76.f, targetY = 8.f;
static bool lighting = true, lavaLight = true, wire = false, help = true, dragging = false;
static int width = 1100, height = 700, lastX = 0, lastY = 0;

static void text(int x, int y, const char *s)
{
    glRasterPos2i(x, y);
    while (*s)
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *s++);
}
static void overlay()
{
    if (!help)
        return;
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3f(1.f, 1.f, 1.f);
    text(15, height - 24, "VOLCANO | CAMERA & LIGHTING");
    text(15, height - 44, "A/D rotate  W/S tilt  +/- zoom  Mouse drag orbit  Wheel zoom");
    text(15, height - 64, "L lighting  K lava glow  F wireframe  H help  R reset  ESC exit");
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}
static void setupLights()
{
    if (!lighting)
    {
        glDisable(GL_LIGHTING);
        return;
    }
    glEnable(GL_LIGHTING);
    GLfloat ambient[] = {.24f, .25f, .30f, 1.f};
    GLfloat sunDiffuse[] = {.93f, .87f, .76f, 1.f};
    GLfloat sunPos[] = {-35.f, 65.f, 30.f, 1.f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    glEnable(GL_LIGHT0);
    if (lavaLight)
    {
        GLfloat color[] = {1.f, .28f, .06f, 1.f};
        GLfloat pos[] = {VolcanoScene::craterCenterX(), VolcanoScene::lavaFloorY() + 2.f, VolcanoScene::craterCenterZ(), 1.f};
        glLightfv(GL_LIGHT1, GL_DIFFUSE, color);
        glLightfv(GL_LIGHT1, GL_SPECULAR, color);
        glLightfv(GL_LIGHT1, GL_POSITION, pos);
        glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.f);
        glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, .015f);
        glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, .0015f);
        glEnable(GL_LIGHT1);
    }
    else
        glDisable(GL_LIGHT1);
}
static void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const float a = yaw * PI / 180.f, b = pitch * PI / 180.f;
    gluLookAt(dist * cos(b) * sin(a), targetY + dist * sin(b), dist * cos(b) * cos(a), 0, targetY, 0, 0, 1, 0);
    setupLights();
    // Ve dia hinh va mo hinh nui lua - Nguoi 1
    glPolygonMode(GL_FRONT_AND_BACK, wire ? GL_LINE : GL_FILL);
    VolcanoScene::drawSceneGeometry();

    // Khoi phuc che do ve de particle khong bi anh huong
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Ve dung nham, lua, khoi va particle - Nguoi 2
    VolcanoEffects::draw();
    overlay();
    glutSwapBuffers();
}
static void reshape(int w, int h)
{
    width = w > 0 ? w : 1;
    height = h > 0 ? h : 1;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55., (double)width / height, .2, 400.);
    glMatrixMode(GL_MODELVIEW);
}
// Cap nhat hieu ung nui lua khoang 60 FPS
static int previousTime = -1;

static void timer(int value)
{
    (void)value;

    int currentTime = glutGet(GLUT_ELAPSED_TIME);

    if (previousTime < 0)
    {
        previousTime = currentTime;
    }

    float deltaSeconds =
        (currentTime - previousTime) / 1000.0f;

    previousTime = currentTime;

    // Gioi han delta time de tranh particle nhay qua xa
    if (deltaSeconds > 0.05f)
    {
        deltaSeconds = 0.05f;
    }

    VolcanoEffects::update(deltaSeconds);

    glutPostRedisplay();

    // Lap lai sau khoang 16 ms
    glutTimerFunc(16, timer, 0);
}
static void reset()
{
    yaw = 38.f;
    pitch = 27.f;
    dist = 76.f;
    targetY = 8.f;
}
static void zoom(float delta)
{
    dist += delta;
    if (dist < 22.f)
        dist = 22.f;
    if (dist > 170.f)
        dist = 170.f;
}
static void keyboard(unsigned char k, int, int)
{
    switch (k)
    {
    case 27:
        // Giai phong hieu ung cua Nguoi 2
        VolcanoEffects::release();

        // Giai phong texture cua Nguoi 1
        VolcanoScene::releaseTextures();

        std::exit(0);
        break;
    case 'a':
    case 'A':
        yaw -= 5;
        break;
    case 'd':
    case 'D':
        yaw += 5;
        break;
    case 'w':
    case 'W':
        pitch += 4;
        break;
    case 's':
    case 'S':
        pitch -= 4;
        break;
    case '+':
    case '=':
        zoom(-3);
        break;
    case '-':
    case '_':
        zoom(3);
        break;
    case 'l':
    case 'L':
        lighting = !lighting;
        break;
    case 'k':
    case 'K':
        lavaLight = !lavaLight;
        break;
    case 'f':
    case 'F':
        wire = !wire;
        break;
    case 'h':
    case 'H':
        help = !help;
        break;
    case 'r':
    case 'R':
        reset();
        break;
    }
    if (pitch > 85)
        pitch = 85;
    if (pitch < 5)
        pitch = 5;
    glutPostRedisplay();
}
static void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        dragging = (state == GLUT_DOWN);
        lastX = x;
        lastY = y;
    }
    if (state == GLUT_DOWN)
    {
        if (button == 3)
            zoom(-3);
        if (button == 4)
            zoom(3);
    }
    glutPostRedisplay();
}
static void motion(int x, int y)
{
    if (!dragging)
        return;
    yaw += (x - lastX) * .35f;
    pitch -= (y - lastY) * .25f;
    if (pitch > 85)
        pitch = 85;
    if (pitch < 5)
        pitch = 5;
    lastX = x;
    lastY = y;
    glutPostRedisplay();
}
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutCreateWindow("Volcano 3D - Camera, Lighting and Controls");
    glClearColor(.11f, .16f, .23f, 1.f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    VolcanoScene::initTextures();
    VolcanoEffects::init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(16, timer, 0);
    std::puts("A/D rotate | W/S tilt | +/- zoom | Mouse orbit | L/K lights | F wire | H help | R reset | ESC quit");
    glutMainLoop();
    return 0;
}
