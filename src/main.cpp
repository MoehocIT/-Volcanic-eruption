// Person 3: camera, lighting, HUD and integration (C++98 / FreeGLUT)
#include "VolcanoScene.h"
#include "VolcanoEffects.h"
#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

static const float PI = 3.14159265358979323846f;
static float yaw=38.f, pitch=27.f, dist=76.f, targetY=8.f;
static bool lighting=true, lavaLight=true, fog=false, wire=false;
static bool help=true, paused=false, dragging=false, released=false;
static bool quakeEnabled=true;
static int timeMode=0; // 0 = automatic cycle, 1 = day, 2 = night
static float simulationTime=0.f, quakeIntensity=0.f;
static int width=1100, height=700, lastX=0, lastY=0;
static int previousTime=0, fpsStart=0, frames=0;
static float fps=0.f;
static bool fpsReady=false;
static const char* cameraName="Overview";

static float clamp(float x,float lo,float hi) {
    return x<lo?lo:(x>hi?hi:x);
}
static float daylightAmount() {
    if(timeMode==1) return 1.f;
    if(timeMode==2) return 0.f;
    const float phase=std::fmod(simulationTime/180.f+0.25f,1.f);
    const float elevation=std::sin(phase*2.f*PI);
    return clamp((elevation+0.12f)/0.48f,0.f,1.f);
}
static const char* timeName() {
    if(timeMode==1) return "DAY";
    if(timeMode==2) return "NIGHT";
    return daylightAmount()>0.55f?"DAY (AUTO)":"NIGHT (AUTO)";
}
static const char* weatherName() {
    VolcanoEffects::WeatherMode mode=VolcanoEffects::getWeatherMode();
    if(mode==VolcanoEffects::WEATHER_RAIN) return "RAIN";
    if(mode==VolcanoEffects::WEATHER_ASHFALL) return "ASHFALL";
    return "CLEAR";
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
    std::sprintf(line,"Time: %s | Weather: %s | Quake: %s",timeName(),weatherName(),quakeIntensity>0.005f?"ACTIVE":"QUIET");
    text(12,height-79,line);
    glColor3f(paused?1.f:.5f,paused?.75f:1.f,.4f);
    text(12,height-99,paused?"Eruption: PAUSED (Space to resume)":"Eruption: RUNNING (Space to pause)");
    glColor3f(1.f,1.f,1.f);
    text(12,height-119,"A/D rotate | W/S tilt | +/- zoom | Q/E target | Mouse orbit/wheel");
    text(12,height-137,"1/2/3 camera | T time | M weather | B quake | L/K lights | G fog | F wire | H HUD | ESC");
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopAttrib();
}
static void setupLights() {
    if(!lighting) { glDisable(GL_LIGHTING); glDisable(GL_LIGHT0); glDisable(GL_LIGHT1); return; }
    glEnable(GL_LIGHTING);
    const float day=daylightAmount(), night=1.f-day;
    GLfloat ambient[]={.055f+.20f*day,.075f+.20f*day,.12f+.20f*day,1.f};
    GLfloat sunDiffuse[]={.10f+.83f*day,.16f+.70f*day,.30f+.46f*day,1.f};
    const float phase=std::fmod(simulationTime/180.f+0.25f,1.f)*2.f*PI;
    GLfloat sunPos[]={-48.f*std::cos(phase),8.f+62.f*std::sin(phase),22.f,1.f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,sunDiffuse);
    glLightfv(GL_LIGHT0,GL_POSITION,sunPos); glEnable(GL_LIGHT0);
    if(lavaLight) {
        GLfloat color[]={1.f,.24f+.20f*night,.035f+.025f*night,1.f};
        GLfloat pos[]={VolcanoScene::craterCenterX(),VolcanoScene::craterCenterY()+1.5f,VolcanoScene::craterCenterZ(),1.f};
        glLightfv(GL_LIGHT1,GL_DIFFUSE,color); glLightfv(GL_LIGHT1,GL_SPECULAR,color);
        glLightfv(GL_LIGHT1,GL_POSITION,pos);
        glLightf(GL_LIGHT1,GL_CONSTANT_ATTENUATION,1.f);
        glLightf(GL_LIGHT1,GL_LINEAR_ATTENUATION,.008f);
        glLightf(GL_LIGHT1,GL_QUADRATIC_ATTENUATION,.00045f); glEnable(GL_LIGHT1);
    } else glDisable(GL_LIGHT1);
}
static void setupFog() {
    if(!fog) { glDisable(GL_FOG); return; }
    const float day=daylightAmount();
    GLfloat color[]={.025f+.085f*day,.045f+.115f*day,.095f+.14f*day,1.f};
    glFogi(GL_FOG_MODE,GL_LINEAR); glFogfv(GL_FOG_COLOR,color);
    glFogf(GL_FOG_START,36.f); glFogf(GL_FOG_END,125.f);
    glHint(GL_FOG_HINT,GL_NICEST); glEnable(GL_FOG);
}
static void drawSky() {
    const float day=daylightAmount(), night=1.f-day;
    const float phase=std::fmod(simulationTime/180.f+0.25f,1.f)*2.f*PI;
    const float skyX=0.5f+0.39f*std::cos(phase);
    const float skyY=0.18f+0.70f*std::max(0.f,std::sin(phase));
    glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_COLOR_BUFFER_BIT|GL_POINT_BIT|GL_LINE_BIT|GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST); glDisable(GL_FOG);
    glDisable(GL_BLEND); glDepthMask(GL_FALSE);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0,width,0,height);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glBegin(GL_QUADS);
    glColor3f(.012f+.24f*day,.022f+.48f*day,.075f+.72f*day); glVertex2i(0,height);
    glVertex2i(width,height);
    glColor3f(.035f+.52f*day,.045f+.66f*day,.12f+.73f*day); glVertex2i(width,0); glVertex2i(0,0);
    glEnd();
    if(night>0.05f) {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glPointSize(2.f); glBegin(GL_POINTS);
        for(int i=0;i<90;++i) {
            const int sx=(i*193+47)%1009, sy=(i*317+83)%997;
            glColor4f(.78f,.86f,1.f,night*(.35f+.65f*(.5f+.5f*std::sin(simulationTime*1.8f+i))));
            glVertex2f((sx/1009.f)*width,(0.18f+(sy/997.f)*.78f)*height);
        }
        glEnd();
    }
    const float cx=skyX*width, cy=skyY*height;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(int ring=5;ring>0;--ring) {
        const float radius=(ring*7.f+12.f);
        glColor4f(1.f,.76f,.35f,day*(0.025f+0.008f*(6-ring)));
        glBegin(GL_TRIANGLE_FAN); glVertex2f(cx,cy);
        for(int j=0;j<=40;++j) { const float a=2.f*PI*j/40.f; glVertex2f(cx+radius*std::cos(a),cy+radius*std::sin(a)); }
        glEnd();
    }
    glColor4f(1.f,.84f,.48f,day);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(cx,cy);
    for(int j=0;j<=40;++j) { const float a=2.f*PI*j/40.f; glVertex2f(cx+12.f*std::cos(a),cy+12.f*std::sin(a)); }
    glEnd();
    const float moonX=(1.f-skyX)*width, moonY=(0.25f+0.55f*(1.f-skyY))*height;
    glColor4f(.78f,.86f,1.f,night);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(moonX,moonY);
    for(int j=0;j<=32;++j) { const float a=2.f*PI*j/32.f; glVertex2f(moonX+8.f*std::cos(a),moonY+8.f*std::sin(a)); }
    glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopAttrib();
}
static void countFrame() {
    ++frames;
    int now=glutGet(GLUT_ELAPSED_TIME), elapsed=now-fpsStart;
    if(elapsed>=1000) { fps=1000.f*frames/elapsed; fpsReady=true; frames=0; fpsStart=now; }
}
static void display() {
    const float day=daylightAmount();
    glClearColor(.012f+.24f*day,.022f+.48f*day,.075f+.72f*day,1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    drawSky();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    const float a=yaw*PI/180.f,b=pitch*PI/180.f;
    gluLookAt(dist*std::cos(b)*std::sin(a),targetY+dist*std::sin(b),
              dist*std::cos(b)*std::cos(a),0,targetY,0,0,1,0);
    setupLights(); setupFog();
    glPolygonMode(GL_FRONT_AND_BACK,wire?GL_LINE:GL_FILL);
    if(quakeIntensity>0.f) {
        const float shake=quakeIntensity;
        glPushMatrix();
        glTranslatef(shake*std::sin(simulationTime*37.f),shake*0.22f*std::sin(simulationTime*51.f),shake*std::cos(simulationTime*43.f));
    }
    VolcanoScene::drawSceneGeometry();
    VolcanoEffects::draw();
    if(quakeIntensity>0.f) glPopMatrix();
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    // Keep additive lava/fire colors intact; fog applies to solid geometry.
    glDisable(GL_FOG);
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
    if(!paused) {
        VolcanoEffects::update(dt);
        simulationTime+=dt;
        const float quakePhase=std::fmod(simulationTime,24.f);
        quakeIntensity=(quakeEnabled && quakePhase<2.3f)?0.28f*(1.f-quakePhase/2.3f):0.f;
    }
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
    case 't': case 'T': timeMode=(timeMode+1)%3; break;
    case 'm': case 'M': VolcanoEffects::setWeatherMode((VolcanoEffects::WeatherMode)((VolcanoEffects::getWeatherMode()+1)%3)); break;
    case 'b': case 'B': quakeEnabled=!quakeEnabled; if(!quakeEnabled) quakeIntensity=0.f; break;
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
    std::puts("1/2/3 camera | T time | M weather | B quake | G fog | Space pause | H HUD | ESC exit");
    glutMainLoop();
    return 0;
}
