// Person 3: camera, lighting, HUD and integration (C++98 / FreeGLUT)
#include "VolcanoScene.h"
#include "VolcanoEffects.h"
#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

static const float PI = 3.14159265358979323846f;
static float yaw=38.f, pitch=27.f, dist=76.f, targetY=8.f;
static bool lighting=true, lavaLight=true, fog=false, wire=false;
static bool help=true, paused=false, dragging=false, released=false;
static int width=1100, height=700, lastX=0, lastY=0;
static int previousTime=0, fpsStart=0, frames=0;
static float fps=0.f;
static bool fpsReady=false;
static const char* cameraName="Overview";

static float clamp(float x,float lo,float hi) {
    return x<lo?lo:(x>hi?hi:x);
}
static const char* state(bool enabled) { return enabled?"ON":"OFF"; }
static void text(int x,int y,const char* s) {
    glRasterPos2i(x,y);
    while(*s) glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*s++);
}
static void overlay() {
    if(!help) return;
    glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_DEPTH_BUFFER_BIT|GL_LIGHTING_BIT|
                 GL_TEXTURE_BIT|GL_COLOR_BUFFER_BIT|GL_POLYGON_BIT);
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG); glDisable(GL_CULL_FACE); glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,width,0,height);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor3f(.055f,.075f,.10f);
    glBegin(GL_QUADS);
    glVertex2i(0,height); glVertex2i(760,height);
    glVertex2i(760,height-142); glVertex2i(0,height-142);
    glEnd();
    glColor3f(1.f,1.f,1.f);
    char line[256];
    text(12,height-19,"VOLCANO 3D | CAMERA & LIGHTING");
    if(fpsReady) std::sprintf(line,"FPS: %.1f | Camera: %s | Target Y: %.1f",fps,cameraName,targetY);
    else std::sprintf(line,"FPS: measuring... | Camera: %s | Target Y: %.1f",cameraName,targetY);
    text(12,height-39,line);
    std::sprintf(line,"Lighting: %s | Lava lamp: %s | Fog: %s | Wireframe: %s",
                 state(lighting),lighting?state(lavaLight):"OFF",state(fog),state(wire));
    text(12,height-59,line);
    glColor3f(paused?1.f:.5f,paused?.75f:1.f,.4f);
    text(12,height-79,paused?"Eruption: PAUSED (Space to resume)":"Eruption: RUNNING (Space to pause)");
    glColor3f(1.f,1.f,1.f);
    text(12,height-99,"A/D rotate | W/S tilt | +/- zoom | Q/E target up/down | Mouse orbit/wheel");
    text(12,height-119,"1 overview | 2 crater | 3 top | L/K lights | G fog | F wire");
    text(12,height-137,"H hide HUD | R reset camera | ESC exit");
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopAttrib();
}
static void setupLights() {
    if(!lighting) { glDisable(GL_LIGHTING); glDisable(GL_LIGHT0); glDisable(GL_LIGHT1); return; }
    glEnable(GL_LIGHTING);
    GLfloat ambient[]={.24f,.25f,.30f,1.f};
    GLfloat sunDiffuse[]={.93f,.87f,.76f,1.f};
    GLfloat sunPos[]={-35.f,65.f,30.f,1.f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,sunDiffuse);
    glLightfv(GL_LIGHT0,GL_POSITION,sunPos); glEnable(GL_LIGHT0);
    if(lavaLight) {
        GLfloat color[]={1.f,.28f,.06f,1.f};
        GLfloat pos[]={VolcanoScene::craterCenterX(),VolcanoScene::lavaFloorY()+2.f,VolcanoScene::craterCenterZ(),1.f};
        glLightfv(GL_LIGHT1,GL_DIFFUSE,color); glLightfv(GL_LIGHT1,GL_SPECULAR,color);
        glLightfv(GL_LIGHT1,GL_POSITION,pos);
        glLightf(GL_LIGHT1,GL_CONSTANT_ATTENUATION,1.f);
        glLightf(GL_LIGHT1,GL_LINEAR_ATTENUATION,.015f);
        glLightf(GL_LIGHT1,GL_QUADRATIC_ATTENUATION,.0015f); glEnable(GL_LIGHT1);
    } else glDisable(GL_LIGHT1);
}
static void setupFog() {
    if(!fog) { glDisable(GL_FOG); return; }
    GLfloat color[]={.11f,.16f,.23f,1.f};
    glFogi(GL_FOG_MODE,GL_LINEAR); glFogfv(GL_FOG_COLOR,color);
    glFogf(GL_FOG_START,36.f); glFogf(GL_FOG_END,125.f);
    glHint(GL_FOG_HINT,GL_NICEST); glEnable(GL_FOG);
}
static void countFrame() {
    ++frames;
    int now=glutGet(GLUT_ELAPSED_TIME), elapsed=now-fpsStart;
    if(elapsed>=1000) { fps=1000.f*frames/elapsed; fpsReady=true; frames=0; fpsStart=now; }
}
static void display() {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    const float a=yaw*PI/180.f,b=pitch*PI/180.f;
    gluLookAt(dist*std::cos(b)*std::sin(a),targetY+dist*std::sin(b),
              dist*std::cos(b)*std::cos(a),0,targetY,0,0,1,0);
    setupLights(); setupFog();
    glPolygonMode(GL_FRONT_AND_BACK,wire?GL_LINE:GL_FILL);
    VolcanoScene::drawSceneGeometry();
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    // Keep additive lava/fire colors intact; fog applies to solid geometry.
    glDisable(GL_FOG);
    VolcanoEffects::draw();
    overlay(); glutSwapBuffers(); countFrame();
}
static void reshape(int w,int h) {
    width=w>0?w:1; height=h>0?h:1;
    glViewport(0,0,width,height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(55.,(double)width/height,.2,400.);
    glMatrixMode(GL_MODELVIEW);
}
static void timer(int value) {
    (void)value;
    if(released) return;
    int now=glutGet(GLUT_ELAPSED_TIME);
    float dt=clamp((now-previousTime)/1000.f,0.f,.05f);
    previousTime=now;
    if(!paused) VolcanoEffects::update(dt);
    glutPostRedisplay(); glutTimerFunc(16,timer,0);
}
static void preset(int number) {
    if(number==1) { yaw=38.f; pitch=27.f; dist=76.f; targetY=8.f; cameraName="Overview"; }
    else if(number==2) { yaw=38.f; pitch=48.f; dist=29.f; targetY=VolcanoScene::craterCenterY(); cameraName="Crater"; }
    else { yaw=0.f; pitch=85.f; dist=90.f; targetY=8.f; cameraName="Top"; }
}
static void zoom(float delta) { dist=clamp(dist+delta,22.f,170.f); cameraName="Custom"; }
static void cleanup() {
    if(released) return;
    released=true;
    VolcanoEffects::release(); VolcanoScene::releaseTextures();
}
static void keyboard(unsigned char k,int,int) {
    switch(k) {
    case 27: cleanup(); glutLeaveMainLoop(); return;
    case 'a': case 'A': yaw-=5.f; cameraName="Custom"; break;
    case 'd': case 'D': yaw+=5.f; cameraName="Custom"; break;
    case 'w': case 'W': pitch+=4.f; cameraName="Custom"; break;
    case 's': case 'S': pitch-=4.f; cameraName="Custom"; break;
    case '+': case '=': zoom(-3.f); break;
    case '-': case '_': zoom(3.f); break;
    case 'q': case 'Q': targetY=clamp(targetY+1.f,0.f,30.f); cameraName="Custom"; break;
    case 'e': case 'E': targetY=clamp(targetY-1.f,0.f,30.f); cameraName="Custom"; break;
    case '1': preset(1); break;
    case '2': preset(2); break;
    case '3': preset(3); break;
    case 'l': case 'L': lighting=!lighting; break;
    case 'k': case 'K': lavaLight=!lavaLight; break;
    case 'g': case 'G': fog=!fog; break;
    case 'f': case 'F': wire=!wire; break;
    case 'h': case 'H': help=!help; break;
    case 'r': case 'R': preset(1); break;
    case ' ': paused=!paused; previousTime=glutGet(GLUT_ELAPSED_TIME); break;
    }
    pitch=clamp(pitch,5.f,85.f); glutPostRedisplay();
}
static void mouse(int button,int status,int x,int y) {
    if(button==GLUT_LEFT_BUTTON) { dragging=(status==GLUT_DOWN); lastX=x; lastY=y; }
}
static void wheel(int wheelNumber,int direction,int x,int y) {
    (void)wheelNumber; (void)x; (void)y;
    zoom(direction>0?-3.f:3.f); glutPostRedisplay();
}
static void motion(int x,int y) {
    if(!dragging) return;
    yaw+=(x-lastX)*.35f; pitch=clamp(pitch-(y-lastY)*.25f,5.f,85.f);
    lastX=x; lastY=y; cameraName="Custom"; glutPostRedisplay();
}
int main(int argc,char** argv) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
    glutInitWindowSize(width,height);
    glutCreateWindow("Volcano 3D - Camera, Lighting and Controls");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glClearColor(.11f,.16f,.23f,1.f);
    glEnable(GL_DEPTH_TEST); glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    VolcanoScene::initTextures(); VolcanoEffects::init();
    glutDisplayFunc(display); glutReshapeFunc(reshape); glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse); glutMouseWheelFunc(wheel); glutMotionFunc(motion);
    glutCloseFunc(cleanup);
    previousTime=fpsStart=glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16,timer,0);
    std::puts("1/2/3 camera | Q/E target | G fog | Space pause | H HUD | ESC exit");
    glutMainLoop();
    return 0;
}
