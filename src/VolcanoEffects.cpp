#include "VolcanoEffects.h"
#include "VolcanoScene.h"
#include <GL/gl.h>
#include <cmath>

namespace {
const float PI = 3.14159265358979323846f;
const int FLAME_COUNT = 34;
const int SMOKE_COUNT = 40;
const int EMBER_COUNT = 48;
const int ASH_COUNT = 32;

struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float age, life, size, phase;
};

Particle flames[FLAME_COUNT];
Particle smoke[SMOKE_COUNT];
Particle embers[EMBER_COUNT];
Particle ash[ASH_COUNT];
unsigned int randomState = 0x51F15EEDu;
float elapsed = 0.0f;
bool initialized = false;

float random01() {
    randomState = randomState * 1664525u + 1013904223u;
    return (float)((randomState >> 8) & 0x00FFFFFFu) / 16777215.0f;
}

float range(float low, float high) {
    return low + (high - low) * random01();
}

void spawnFlame(Particle &p, bool stagger) {
    const float angle = range(0.0f, 2.0f * PI);
    const float radius = std::sqrt(random01()) * 0.62f;
    p.x = VolcanoScene::craterCenterX() + radius * std::cos(angle);
    p.z = VolcanoScene::craterCenterZ() + radius * std::sin(angle);
    p.y = VolcanoScene::craterCenterY() - 0.18f;
    p.vx = range(-0.42f, 0.42f);
    p.vz = range(-0.42f, 0.42f);
    p.vy = range(2.7f, 4.7f);
    p.life = range(1.0f, 1.9f);
    p.age = stagger ? range(0.0f, p.life) : 0.0f;
    p.size = range(0.22f, 0.48f);
    p.phase = range(0.0f, 2.0f * PI);
    p.y += p.vy * p.age;
    p.x += p.vx * p.age;
    p.z += p.vz * p.age;
}

void spawnSmoke(Particle &p, bool stagger) {
    const float angle = range(0.0f, 2.0f * PI);
    const float radius = std::sqrt(random01()) * 0.52f;
    p.x = VolcanoScene::craterCenterX() + radius * std::cos(angle);
    p.z = VolcanoScene::craterCenterZ() + radius * std::sin(angle);
    p.y = VolcanoScene::craterCenterY() + 1.8f;
    const float burst = range(0.25f,0.85f);
    p.vx = burst * std::cos(angle) + range(-0.25f,0.25f);
    p.vz = burst * std::sin(angle) + range(-0.25f,0.25f);
    p.vy = range(1.25f,2.2f);
    p.life = range(4.2f,6.2f);
    p.age = stagger ? range(0.0f, p.life) : 0.0f;
    p.size = range(0.72f,1.18f);
    p.phase = range(0.0f, 2.0f * PI);
    p.y += p.vy * p.age;
    p.x += p.vx * p.age;
    p.z += p.vz * p.age;
}

void spawnAsh(Particle &p, bool stagger) {
    const float angle=range(0.0f,2.0f*PI);
    const float radius=std::sqrt(random01())*0.34f;
    const float burst=range(0.55f,2.0f);
    p.x=VolcanoScene::craterCenterX()+radius*std::cos(angle);
    p.z=VolcanoScene::craterCenterZ()+radius*std::sin(angle);
    p.y=VolcanoScene::craterCenterY()+0.08f;
    p.vx=burst*std::cos(angle)+range(-0.45f,0.45f);
    p.vz=burst*std::sin(angle)+range(-0.45f,0.45f);
    p.vy=range(3.0f,5.8f);
    p.life=range(2.1f,3.8f);
    p.age=stagger ? range(0.0f,p.life) : 0.0f;
    p.size=range(0.24f,0.58f);
    p.phase=range(0.0f,2.0f*PI);
    p.x+=p.vx*p.age;
    p.z+=p.vz*p.age;
    p.y+=p.vy*p.age-1.3f*p.age*p.age;
    p.vy-=2.6f*p.age;
}

void spawnEmber(Particle &p, bool stagger) {
    const float angle = range(0.0f,2.0f*PI);
    const float radius = std::sqrt(random01())*0.38f;
    p.x = VolcanoScene::craterCenterX()+radius*std::cos(angle);
    p.z = VolcanoScene::craterCenterZ()+radius*std::sin(angle);
    p.y = VolcanoScene::craterCenterY()+0.02f;
    p.vx = range(-1.35f,1.35f);
    p.vz = range(-1.35f,1.35f);
    p.vy = range(4.2f,8.2f);
    p.life = range(0.75f,1.55f);
    p.age = stagger ? range(0.0f,p.life) : 0.0f;
    p.size = range(0.07f,0.15f);
    p.phase = range(0.0f,2.0f*PI);
    p.x += p.vx*p.age;
    p.z += p.vz*p.age;
    p.y += p.vy*p.age-2.8f*p.age*p.age;
    p.vy -= 5.6f*p.age;
}

void billboardBasis(float &rightX, float &rightY, float &rightZ,
                    float &upX, float &upY, float &upZ) {
    GLfloat matrix[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
    // The view matrix columns map camera right/up into world space.
    rightX = matrix[0]; rightY = matrix[4]; rightZ = matrix[8];
    upX = matrix[1]; upY = matrix[5]; upZ = matrix[9];
}

void vertexBillboard(float x, float y, float z, float rightX, float rightY,
                     float rightZ, float upX, float upY, float upZ,
                     float side, float vertical) {
    glVertex3f(x + rightX * side + upX * vertical,
               y + rightY * side + upY * vertical,
               z + rightZ * side + upZ * vertical);
}

void drawLava() {
    const float floorY = VolcanoScene::lavaFloorY() + 0.08f;
    const float centerX = VolcanoScene::craterCenterX();
    const float centerZ = VolcanoScene::craterCenterZ();
    const int SEGMENTS = 96;
    const float pulse = 0.5f + 0.5f * std::sin(elapsed * 2.1f);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Soft orange glow under the brighter, animated lava surface.
    for (int layer = 0; layer < 3; ++layer) {
        const float radius = 1.52f - layer * 0.23f;
        const float alpha = 0.10f + 0.035f * pulse;
        glColor4f(1.0f, 0.16f + 0.08f * layer, 0.015f, alpha);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(centerX, floorY - 0.015f * layer, centerZ);
        for (int i = 0; i <= SEGMENTS; ++i) {
            const float angle = 2.0f * PI * i / SEGMENTS;
            glVertex3f(centerX + radius * std::cos(angle),
                       floorY - 0.015f * layer,
                       centerZ + radius * std::sin(angle));
        }
        glEnd();
    }

    glColor4f(1.0f, 0.22f + 0.12f * pulse, 0.015f, 0.92f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(centerX, floorY, centerZ);
    for (int i = 0; i <= SEGMENTS; ++i) {
        const float angle = 2.0f * PI * i / SEGMENTS;
        const float wave = 0.10f * std::sin(3.0f * angle + elapsed * 1.6f)
                         + 0.055f * std::sin(7.0f * angle - elapsed * 2.3f);
        const float radius = 1.38f + wave;
        glColor4f(1.0f, 0.16f + 0.18f * pulse
                  + 0.08f * std::sin(angle * 4.0f + elapsed),
                  0.008f, 0.94f);
        glVertex3f(centerX + radius * std::cos(angle), floorY,
                   centerZ + radius * std::sin(angle));
    }
    glEnd();

    // A few moving hot streaks break up the otherwise flat pool.
    for (int streak = 0; streak < 4; ++streak) {
        const float angle = elapsed * (0.42f + streak * 0.07f)
                          + streak * (2.0f * PI / 4.0f);
        const float radius = 0.32f + 0.13f * std::sin(elapsed * 1.8f + streak);
        const float length = 0.48f + 0.12f * std::sin(elapsed * 2.4f + streak);
        glColor4f(1.0f, 0.66f, 0.08f, 0.72f);
        glBegin(GL_QUADS);
        glVertex3f(centerX + radius * std::cos(angle) - 0.07f, floorY + 0.012f,
                   centerZ + radius * std::sin(angle));
        glVertex3f(centerX + (radius + length) * std::cos(angle) - 0.035f,
                   floorY + 0.012f,
                   centerZ + (radius + length) * std::sin(angle));
        glVertex3f(centerX + (radius + length) * std::cos(angle) + 0.035f,
                   floorY + 0.012f,
                   centerZ + (radius + length) * std::sin(angle));
        glVertex3f(centerX + radius * std::cos(angle) + 0.07f, floorY + 0.012f,
                   centerZ + radius * std::sin(angle));
        glEnd();
    }
}

void drawLavaFlowLayer(int flow, float baseAngle, float widthScale,
                       float red, float green, float blue, float alpha) {
    const int STEPS = 48;
    glBegin(GL_QUAD_STRIP);
    for (int step = 0; step <= STEPS; ++step) {
        const float t = (float)step / STEPS;
        const float radius = 3.62f + 13.05f * t;
        const float pulse = 0.78f + 0.22f * std::sin(elapsed * 2.3f
                                                   - t * 9.0f + flow * 1.7f);
        const float curve = 0.065f * std::sin(t * 5.2f + flow * 1.31f)
                          + 0.025f * std::sin(elapsed * 0.7f + t * 8.0f + flow);
        const float angle = baseAngle + curve;
        const float halfWidth = (0.12f + 0.26f * std::sin(PI * t))
                              * widthScale * pulse;
        const float leftAngle = angle - halfWidth / radius;
        const float rightAngle = angle + halfWidth / radius;
        const float yLeft = VolcanoScene::volcanoSurfaceY(radius,leftAngle) + 0.075f;
        const float yRight = VolcanoScene::volcanoSurfaceY(radius,rightAngle) + 0.075f;
        const float glow = pulse * (0.85f + 0.15f * std::sin(t * 18.0f - elapsed * 3.0f));

        glColor4f(red * glow,green * glow,blue * glow,alpha * glow);
        glVertex3f(radius * std::cos(leftAngle),yLeft,radius * std::sin(leftAngle));
        glVertex3f(radius * std::cos(rightAngle),yRight,radius * std::sin(rightAngle));
    }
    glEnd();
}

void drawLavaFlows() {
    static const float flowAngles[] = {0.28f, 1.43f, 2.61f, 3.72f, 5.02f};
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);

    for (int flow = 0; flow < 5; ++flow) {
        drawLavaFlowLayer(flow,flowAngles[flow],1.8f,0.80f,0.035f,0.002f,0.24f);
        drawLavaFlowLayer(flow,flowAngles[flow],0.92f,1.00f,0.17f,0.006f,0.60f);
        drawLavaFlowLayer(flow,flowAngles[flow],0.36f,1.00f,0.62f,0.075f,0.88f);
    }
}

void drawFlames(float rightX, float rightY, float rightZ,
                float upX, float upY, float upZ) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (int i = 0; i < FLAME_COUNT; ++i) {
        const Particle &p = flames[i];
        const float t = p.age / p.life;
        const float fade = (1.0f - t) * (0.75f + 0.25f * std::sin(elapsed * 11.0f + p.phase));
        const float width = p.size * (0.55f + 0.65f * std::sin(t * PI));
        const float height = p.size * (1.7f - 0.45f * t);
        const float sway = 0.10f * std::sin(elapsed * 4.0f + p.phase);

        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.12f, 0.005f, 0.72f * fade);
        vertexBillboard(p.x, p.y, p.z, rightX, rightY, rightZ,
                        upX, upY, upZ, -width, -height * 0.55f);
        vertexBillboard(p.x, p.y, p.z, rightX, rightY, rightZ,
                        upX, upY, upZ, width, -height * 0.55f);
        vertexBillboard(p.x, p.y, p.z, rightX, rightY, rightZ,
                        upX, upY, upZ, sway, height * 0.72f);

        glColor4f(1.0f, 0.68f, 0.055f, 0.86f * fade);
        vertexBillboard(p.x, p.y, p.z, rightX, rightY, rightZ,
                        upX, upY, upZ, -width * 0.42f, -height * 0.48f);
        vertexBillboard(p.x, p.y, p.z, rightX, rightY, rightZ,
                        upX, upY, upZ, width * 0.42f, -height * 0.48f);
        glColor4f(1.0f, 0.95f, 0.28f, 0.68f * fade);
        vertexBillboard(p.x, p.y, p.z, rightX, rightY, rightZ,
                        upX, upY, upZ, sway * 0.5f, height * 0.30f);
        glEnd();
    }
}

void drawEmbers(float rightX, float rightY, float rightZ,
                float upX, float upY, float upZ) {
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    for(int i=0;i<EMBER_COUNT;++i) {
        const Particle &p=embers[i];
        const float t=p.age/p.life;
        const float fade=(1.0f-t)*(0.72f+0.28f*std::sin(elapsed*18.0f+p.phase));
        const float speed=std::sqrt(p.vx*p.vx+p.vy*p.vy+p.vz*p.vz);
        const float trailLength=p.size*(2.0f+speed*0.35f);
        const float invSpeed=speed>0.0001f ? 1.0f/speed : 0.0f;

        glBegin(GL_LINES);
        glColor4f(1.0f,0.72f,0.12f,0.78f*fade);
        glVertex3f(p.x,p.y,p.z);
        glColor4f(1.0f,0.10f,0.005f,0.0f);
        glVertex3f(p.x-p.vx*invSpeed*trailLength,
                   p.y-p.vy*invSpeed*trailLength,
                   p.z-p.vz*invSpeed*trailLength);
        glEnd();

        const float halfWidth=p.size*0.42f;
        const float halfHeight=p.size*1.7f;
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f,0.25f,0.015f,0.6f*fade);
        vertexBillboard(p.x,p.y,p.z,rightX,rightY,rightZ,upX,upY,upZ,
                        -halfWidth,-halfHeight);
        vertexBillboard(p.x,p.y,p.z,rightX,rightY,rightZ,upX,upY,upZ,
                        halfWidth,-halfHeight);
        glColor4f(1.0f,0.94f,0.42f,0.9f*fade);
        vertexBillboard(p.x,p.y,p.z,rightX,rightY,rightZ,upX,upY,upZ,
                        0.0f,halfHeight);
        glEnd();
    }
}

void drawPuffCloud(const Particle *particles, int count,
                   float red, float green, float blue,
                   float opacity, float driftScale,
                   float rightX, float rightY, float rightZ,
                   float upX, float upY, float upZ) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLfloat modelview[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    int order[ASH_COUNT > SMOKE_COUNT ? ASH_COUNT : SMOKE_COUNT];
    float eyeDepth[ASH_COUNT > SMOKE_COUNT ? ASH_COUNT : SMOKE_COUNT];

    // Alpha blending depends on draw order: render the most distant puffs first.
    for (int i = 0; i < count; ++i) {
        const Particle &p = particles[i];
        const float centerX = p.x + driftScale * std::sin(elapsed * 0.85f + p.phase);
        const float centerZ = p.z + driftScale * 0.55f * std::cos(elapsed + p.phase);
        order[i] = i;
        eyeDepth[i] = modelview[2] * centerX + modelview[6] * p.y
                    + modelview[10] * centerZ + modelview[14];
    }
    for (int i = 1; i < count; ++i) {
        const int particleIndex = order[i];
        const float depth = eyeDepth[particleIndex];
        int j = i;
        while (j > 0 && eyeDepth[order[j - 1]] > depth) {
            order[j] = order[j - 1];
            --j;
        }
        order[j] = particleIndex;
    }

    for (int drawIndex = 0; drawIndex < count; ++drawIndex) {
        const Particle &p = particles[order[drawIndex]];
        const float t = p.age / p.life;
        const float fadeIn = t < 0.12f ? t / 0.12f : 1.0f;
        const float fadeOut = t > 0.80f ? (1.0f - t) / 0.20f : 1.0f;
        const float alpha = opacity * fadeIn * fadeOut;
        const float size = p.size * (0.62f + 1.55f * t);
        const float drift = driftScale * std::sin(elapsed * 0.85f + p.phase);
        const float centerX = p.x + drift;
        const float centerZ = p.z + driftScale * 0.55f * std::cos(elapsed + p.phase);

        glBegin(GL_TRIANGLE_FAN);
        glColor4f(red,green,blue,alpha);
        vertexBillboard(centerX,p.y,centerZ,rightX,rightY,rightZ,
                        upX,upY,upZ,0.0f,0.0f);
        glColor4f(red*0.78f,green*0.78f,blue*0.78f,0.0f);
        for (int edge = 0; edge <= 20; ++edge) {
            const float angle = 2.0f * PI * edge / 20.0f;
            vertexBillboard(centerX,p.y,centerZ,rightX,rightY,rightZ,
                            upX,upY,upZ,size*std::cos(angle),size*std::sin(angle));
        }
        glEnd();
    }
}

void drawSmoke(float rightX, float rightY, float rightZ,
               float upX, float upY, float upZ) {
    drawPuffCloud(smoke,SMOKE_COUNT,0.34f,0.33f,0.32f,0.42f,0.18f,
                  rightX,rightY,rightZ,upX,upY,upZ);
}

void drawAsh(float rightX, float rightY, float rightZ,
             float upX, float upY, float upZ) {
    drawPuffCloud(ash,ASH_COUNT,0.27f,0.23f,0.19f,0.38f,0.10f,
                  rightX,rightY,rightZ,upX,upY,upZ);
}
}

namespace VolcanoEffects {
void init() {
    randomState = 0x51F15EEDu;
    elapsed = 0.0f;
    for (int i = 0; i < FLAME_COUNT; ++i) spawnFlame(flames[i], true);
    for (int i = 0; i < SMOKE_COUNT; ++i) spawnSmoke(smoke[i], true);
    for (int i = 0; i < EMBER_COUNT; ++i) spawnEmber(embers[i], true);
    for (int i = 0; i < ASH_COUNT; ++i) spawnAsh(ash[i], true);
    initialized = true;
}

void update(float deltaSeconds) {
    if (!initialized) init();
    if (!(deltaSeconds > 0.0f)) return;
    const float dt = deltaSeconds > 0.1f ? 0.1f : deltaSeconds;
    elapsed += dt;

    for (int i = 0; i < FLAME_COUNT; ++i) {
        Particle &p = flames[i];
        p.age += dt;
        if (p.age >= p.life) spawnFlame(p, false);
        else {
            p.x += p.vx * dt;
            p.z += p.vz * dt;
            p.y += p.vy * dt;
        }
    }
    for (int i = 0; i < SMOKE_COUNT; ++i) {
        Particle &p = smoke[i];
        p.age += dt;
        if (p.age >= p.life) spawnSmoke(p, false);
        else {
            p.x += p.vx * dt;
            p.z += p.vz * dt;
            p.y += p.vy * dt;
        }
    }
    for (int i = 0; i < EMBER_COUNT; ++i) {
        Particle &p=embers[i];
        p.age+=dt;
        if(p.age>=p.life) spawnEmber(p,false);
        else {
            p.x+=p.vx*dt;
            p.z+=p.vz*dt;
            p.y+=p.vy*dt-2.8f*dt*dt;
            p.vy-=5.6f*dt;
        }
    }
    for (int i = 0; i < ASH_COUNT; ++i) {
        Particle &p=ash[i];
        p.age+=dt;
        if(p.age>=p.life) spawnAsh(p,false);
        else {
            p.x+=p.vx*dt;
            p.z+=p.vz*dt;
            p.y+=p.vy*dt-1.3f*dt*dt;
            p.vy-=2.6f*dt;
        }
    }
}

void draw() {
    if (!initialized) init();

    GLfloat rightX, rightY, rightZ, upX, upY, upZ;
    billboardBasis(rightX, rightY, rightZ, upX, upY, upZ);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT |
                 GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDepthMask(GL_FALSE);

    drawLava();
    drawLavaFlows();
    drawFlames(rightX, rightY, rightZ, upX, upY, upZ);
    drawEmbers(rightX, rightY, rightZ, upX, upY, upZ);
    drawAsh(rightX,rightY,rightZ,upX,upY,upZ);
    drawSmoke(rightX,rightY,rightZ,upX,upY,upZ);

    glPopAttrib();
}

void release() {
    initialized = false;
    elapsed = 0.0f;
}
}
