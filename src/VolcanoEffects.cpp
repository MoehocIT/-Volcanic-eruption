#include "VolcanoEffects.h"
#include "VolcanoScene.h"
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>

namespace
{
    const float PI = 3.14159265358979323846f;
    const int MAX_FIRE = 180;
    const int MAX_SMOKE = 110;
    const int MAX_SPARK = 90;

    struct Particle
    {
        float x, y, z, vx, vy, vz, life, maxLife, size;
        bool active;
    };
    Particle fire[MAX_FIRE], smoke[MAX_SMOKE], sparks[MAX_SPARK];
    float elapsed = 0.0f;
    bool initialized = false;

    float random01() { return (float)std::rand() / (float)RAND_MAX; }
    float randomRange(float a, float b) { return a + (b - a) * random01(); }
    float clamp01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }

    void resetFire(Particle &p)
    {
        const float a = randomRange(0.0f, 2.0f * PI);
        const float r = randomRange(0.25f, 2.5f);
        p.x = r * std::cos(a);
        p.z = r * std::sin(a);
        p.y = VolcanoScene::craterCenterY() + randomRange(-0.1f, 0.35f);
        p.vx = randomRange(-0.65f, 0.65f);
        p.vz = randomRange(-0.65f, 0.65f);
        p.vy = randomRange(2.0f, 5.2f);
        p.maxLife = p.life = randomRange(0.45f, 1.15f);
        p.size = randomRange(0.22f, 0.55f);
        p.active = true;
    }
    void resetSmoke(Particle &p)
    {
        const float a = randomRange(0.0f, 2.0f * PI);
        const float r = randomRange(0.2f, 1.7f);
        p.x = r * std::cos(a);
        p.z = r * std::sin(a);
        p.y = VolcanoScene::craterCenterY() + randomRange(0.0f, 0.7f);
        p.vx = randomRange(-0.35f, 0.35f);
        p.vz = randomRange(-0.35f, 0.35f);
        p.vy = randomRange(1.0f, 2.0f);
        p.maxLife = p.life = randomRange(2.4f, 4.2f);
        p.size = randomRange(0.65f, 1.25f);
        p.active = true;
    }
    void resetSpark(Particle &p)
    {
        const float a = randomRange(0.0f, 2.0f * PI);
        const float r = randomRange(0.3f, 2.4f);
        p.x = r * std::cos(a);
        p.z = r * std::sin(a);
        p.y = VolcanoScene::craterCenterY() + randomRange(0.0f, 0.4f);
        p.vx = randomRange(-1.8f, 1.8f);
        p.vz = randomRange(-1.8f, 1.8f);
        p.vy = randomRange(2.5f, 6.0f);
        p.maxLife = p.life = randomRange(0.7f, 1.5f);
        p.size = randomRange(0.045f, 0.10f);
        p.active = true;
    }
    void drawBillboard(float x, float y, float z, float size)
    {
        // Hinh vuong quay mat len camera khong can billboard; particle nam tren mat phang XZ.
        glBegin(GL_QUADS);
        glVertex3f(x - size, y, z - size);
        glVertex3f(x + size, y, z - size);
        glVertex3f(x + size, y, z + size);
        glVertex3f(x - size, y, z + size);
        glEnd();
    }
    void drawLava()
    {
        const float cx = VolcanoScene::craterCenterX();
        const float cy = VolcanoScene::lavaFloorY() + 0.07f;
        const float cz = VolcanoScene::craterCenterZ();
        const float radius = 1.58f;
        const int rings = 18, seg = 96;
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        // Lop nen do cam phat sang.
        glColor4f(1.0f, 0.12f, 0.005f, 0.95f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(cx, cy, cz);
        for (int i = 0; i <= seg; ++i)
        {
            float a = 2 * PI * i / seg;
            glVertex3f(cx + radius * std::cos(a), cy, cz + radius * std::sin(a));
        }
        glEnd();
        // Cac vong song nham chuyen dong.
        for (int j = 0; j < rings; ++j)
        {
            float r0 = radius * j / rings, r1 = radius * (j + 1) / rings;
            glBegin(GL_QUAD_STRIP);
            for (int i = 0; i <= seg; ++i)
            {
                float a = 2 * PI * i / seg;
                float wave = 0.045f * std::sin(a * 7.0f + elapsed * 2.4f + r0 * 5.0f) + 0.025f * std::sin(a * 13.0f - elapsed * 3.1f);
                float y0 = cy + wave, y1 = cy + wave + 0.012f;
                float pulse = 0.5f + 0.5f * std::sin(elapsed * 2.0f + r0 * 5.0f + a * 3.0f);
                glColor4f(1.0f, 0.16f + 0.45f * pulse, 0.005f, 0.82f);
                glVertex3f(cx + r0 * std::cos(a), y0, cz + r0 * std::sin(a));
                glColor4f(1.0f, 0.55f + 0.35f * pulse, 0.025f, 0.92f);
                glVertex3f(cx + r1 * std::cos(a), y1, cz + r1 * std::sin(a));
            }
            glEnd();
        }
        // Cac vet nut vang cam tren be mat.
        glLineWidth(2.0f);
        glColor4f(1.0f, 0.78f, 0.08f, 0.9f);
        glBegin(GL_LINES);
        for (int k = 0; k < 13; ++k)
        {
            float a = k * 2.39996f + elapsed * 0.035f;
            float r0 = 0.18f + 0.25f * (k % 3), r1 = 1.0f + 0.12f * (k % 4);
            glVertex3f(cx + r0 * std::cos(a), cy + 0.035f, cz + r0 * std::sin(a));
            glVertex3f(cx + r1 * std::cos(a + 0.13f), cy + 0.035f, cz + r1 * std::sin(a + 0.13f));
        }
        glEnd();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }
    void drawParticles(Particle *items, int count, int type)
    {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, type == 1 ? GL_ONE_MINUS_SRC_ALPHA : GL_ONE);
        glDepthMask(GL_FALSE);
        for (int i = 0; i < count; ++i)
        {
            Particle &p = items[i];
            if (!p.active)
                continue;
            float t = clamp01(p.life / p.maxLife);
            float alpha;
            if (type == 1)
            { // smoke: xam dan, phong to khi bay len
                float s = p.size * (1.0f + 1.7f * (1.0f - t));
                alpha = 0.30f * t;
                glColor4f(0.24f, 0.25f, 0.27f, alpha);
                glPushMatrix();
                glTranslatef(p.x, p.y, p.z);
                glScalef(s, s * 0.72f, s);
                glutSolidSphere(1.0, 8, 7);
                glPopMatrix();
            }
            else
            {
                alpha = t;
                if (type == 0)
                {
                    glColor4f(1.0f, 0.18f + 0.55f * (1.0f - t), 0.015f, alpha);
                    drawBillboard(p.x, p.y, p.z, p.size * (0.45f + 0.55f * t));
                }
                else
                {
                    glColor4f(1.0f, 0.72f, 0.12f, alpha);
                    glBegin(GL_LINES);
                    glVertex3f(p.x, p.y, p.z);
                    glVertex3f(p.x - p.vx * 0.035f, p.y - p.vy * 0.035f, p.z - p.vz * 0.035f);
                    glEnd();
                }
            }
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }
}

namespace VolcanoEffects
{
    void init()
    {
        if (initialized)
            return;
        std::srand(7319);
        for (int i = 0; i < MAX_FIRE; ++i)
            resetFire(fire[i]);
        for (int i = 0; i < MAX_SMOKE; ++i)
        {
            resetSmoke(smoke[i]);
            smoke[i].life = randomRange(0.0f, smoke[i].maxLife);
        }
        for (int i = 0; i < MAX_SPARK; ++i)
        {
            resetSpark(sparks[i]);
            sparks[i].life = randomRange(0.0f, sparks[i].maxLife);
        }
        initialized = true;
    }
    void update(float dt)
    {
        if (!initialized)
            init();
        if (dt < 0)
            dt = 0;
        if (dt > 0.05f)
            dt = 0.05f;
        elapsed += dt;
        for (int i = 0; i < MAX_FIRE; ++i)
        {
            Particle &p = fire[i];
            p.life -= dt;
            if (p.life <= 0)
            {
                resetFire(p);
                continue;
            }
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.z += p.vz * dt;
            p.vx *= 0.99f;
            p.vz *= 0.99f;
        }
        for (int i = 0; i < MAX_SMOKE; ++i)
        {
            Particle &p = smoke[i];
            p.life -= dt;
            if (p.life <= 0)
            {
                resetSmoke(p);
                continue;
            }
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.z += p.vz * dt;
            p.vx += randomRange(-0.18f, 0.18f) * dt;
            p.vz += randomRange(-0.18f, 0.18f) * dt;
        }
        for (int i = 0; i < MAX_SPARK; ++i)
        {
            Particle &p = sparks[i];
            p.life -= dt;
            if (p.life <= 0)
            {
                resetSpark(p);
                continue;
            }
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.z += p.vz * dt;
            p.vy -= 3.0f * dt;
        }
    }
    void draw()
    {
        if (!initialized)
            init();
        drawLava();
        drawParticles(fire, MAX_FIRE, 0);
        drawParticles(smoke, MAX_SMOKE, 1);
        drawParticles(sparks, MAX_SPARK, 2);
    }
    void release() { initialized = false; }
}
