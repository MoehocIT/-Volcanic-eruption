#include "VolcanoScene.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdlib>

namespace {
const float PI = 3.14159265358979323846f;
const float TERRAIN_HALF = 48.0f;
const float CRATER_Y = 16.0f;
const float CRATER_R = 3.4f;
const float LAVA_FLOOR_Y = 12.0f;
GLuint rockTexture = 0;
GLuint groundTexture = 0;
GLuint terrainList = 0;
GLuint volcanoList = 0;

float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
float smooth(float t) { t = clamp01(t); return t * t * (3.0f - 2.0f * t); }
float hash2(int x, int z) {
    unsigned int h = (unsigned int)x * 374761393u + (unsigned int)z * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)(h & 65535u) / 65535.0f;
}
float noise(float x, float z) {
    int ix = (int)std::floor(x), iz = (int)std::floor(z);
    float fx = smooth(x - (float)ix), fz = smooth(z - (float)iz);
    float a = hash2(ix, iz) * (1.0f-fx) + hash2(ix+1, iz) * fx;
    float b = hash2(ix, iz+1) * (1.0f-fx) + hash2(ix+1, iz+1) * fx;
    return a * (1.0f-fz) + b * fz;
}
float terrainHeight(float x, float z) {
    const float r = std::sqrt(x*x + z*z);
    // Chan nui bang phang de khop mesh nui lua voi mat dat.
    const float fade = smooth((r - 17.0f) / 9.0f);
    const float ridge = 1.0f - std::fabs(2.0f * noise(x*0.105f+13.7f,
                                                       z*0.105f-8.2f)-1.0f);
    const float hills = 3.0f * (noise(x*0.055f,z*0.055f)-0.5f)
                      + 1.05f * (noise(x*0.16f,z*0.16f)-0.5f)
                      + 0.8f * (ridge-0.48f);
    return hills * fade;
}
float profile(float r) {
    // Ban kinh tu trong ra ngoai; mieng nui rong, thanh trong doc.
    static const float rr[] = {0.0f, 1.6f, 2.3f, 3.4f, 4.3f, 5.5f, 8.0f, 11.0f, 14.0f, 17.0f, 19.5f};
    static const float yy[] = {12.0f,12.0f,13.0f,16.0f,15.4f,14.2f,11.0f,7.1f,3.9f,1.4f,0.0f};
    for (int i = 1; i < 11; ++i) {
        if (r <= rr[i]) {
            const float t = (r-rr[i-1])/(rr[i]-rr[i-1]);
            return yy[i-1]*(1.0f-t) + yy[i]*t;
        }
    }
    return 0.0f;
}
float mountainHeight(float r, float a) {
    const float y = profile(r);
    // Gam mat nui loi lom; giam bien do o mieng va chan.
    float weight = smooth((r-3.4f)/3.0f) * (1.0f-smooth((r-15.0f)/4.5f));
    const float x = r*std::cos(a), z = r*std::sin(a);
    float irregular = 0.65f*(noise(x*0.27f,z*0.27f)-0.5f)
                    + 0.32f*std::sin(a*9.0f + r*0.63f)
                    + 0.16f*std::cos(a*17.0f-r*1.25f);
    return y + weight * irregular;
}
void vertexMountain(float r, float a) {
    const float x=r*std::cos(a), z=r*std::sin(a);
    glTexCoord2f(a*3.0f/PI, r*0.19f);
    glVertex3f(x, mountainHeight(r,a), z);
}
void makeTexture(GLuint &id, bool rock) {
    const int W=128, H=128;
    unsigned char pixels[W*H*3];
    for (int j=0; j<H; ++j) for (int i=0; i<W; ++i) {
        const float n = 0.6f*noise(i*0.15f,j*0.15f) + 0.3f*noise(i*0.46f,j*0.46f)
                      + 0.1f*hash2(i,j);
        const float grain = (n-0.5f)*85.0f;
        const int k = (j*W+i)*3;
        if (rock) {
            pixels[k]   = (unsigned char)(75 + grain);
            pixels[k+1] = (unsigned char)(69 + grain);
            pixels[k+2] = (unsigned char)(67 + grain);
        } else {
            // Dark warm ash instead of the previous green-tinted ground.
            const float ash = 50.0f + grain*0.48f;
            pixels[k]   = (unsigned char)(ash + 7.0f);
            pixels[k+1] = (unsigned char)(ash + 2.0f);
            pixels[k+2] = (unsigned char)(ash - 3.0f);
        }
    }
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D,id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Build mipmap chain manually (khong can GLU trong module).
    int w=W,h=H;
    unsigned char level[W*H*3];
    for(int i=0;i<W*H*3;++i) level[i]=pixels[i];
    int mip=0;
    while(true) {
        glTexImage2D(GL_TEXTURE_2D,mip,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,level);
        if(w==1 && h==1) break;
        const int nw=w>1?w/2:1, nh=h>1?h/2:1;
        unsigned char next[W*H*3];
        for(int y=0;y<nh;++y) for(int x=0;x<nw;++x) for(int c=0;c<3;++c) {
            int sum=0;
            for(int dy=0;dy<2;++dy) for(int dx=0;dx<2;++dx)
                sum += level[((y*2+dy < h ? y*2+dy : h-1)*w + (x*2+dx < w ? x*2+dx : w-1))*3+c];
            next[(y*nw+x)*3+c]=(unsigned char)(sum/4);
        }
        for(int i=0;i<nw*nh*3;++i) level[i]=next[i];
        w=nw;h=nh;++mip;
    }
}

void renderTerrain() {
    const int COUNT=100;
    const float step=2.0f*TERRAIN_HALF/COUNT;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,groundTexture);
    glColor3f(1,1,1);
    for(int z=0;z<COUNT;++z) {
        const float z0=-TERRAIN_HALF+z*step;
        const float z1=z0+step;
        glBegin(GL_TRIANGLE_STRIP);
        for(int x=0;x<=COUNT;++x) {
            const float xx=-TERRAIN_HALF+x*step;
            const float h0=terrainHeight(xx,z0),h1=terrainHeight(xx,z1);
            const float eps=0.3f;
            const float dx=(terrainHeight(xx+eps,z0)-terrainHeight(xx-eps,z0))/(2*eps);
            const float dz=(terrainHeight(xx,z0+eps)-terrainHeight(xx,z0-eps))/(2*eps);
            glNormal3f(-dx,1.0f,-dz);
            glTexCoord2f(xx*0.15f,z0*0.15f);glVertex3f(xx,h0,z0);
            const float dx1=(terrainHeight(xx+eps,z1)-terrainHeight(xx-eps,z1))/(2*eps);
            const float dz1=(terrainHeight(xx,z1+eps)-terrainHeight(xx,z1-eps))/(2*eps);
            glNormal3f(-dx1,1.0f,-dz1);
            glTexCoord2f(xx*0.15f,z1*0.15f);glVertex3f(xx,h1,z1);
        }
        glEnd();
    }

    // Scatter low-poly basalt around the mountain, outside its footprint.
    struct RockPoint { float x, y, z; };
    const int ROCK_SEGMENTS=6;
    for(int rock=0;rock<180;++rock) {
        const float x=-42.0f+84.0f*hash2(rock,31);
        const float z=-42.0f+84.0f*hash2(rock,97);
        const float distance=std::sqrt(x*x+z*z);
        if(distance<21.0f || distance>45.5f) continue;

        const float size=0.30f+0.78f*hash2(rock,57);
        const float height=size*(1.05f+0.55f*hash2(rock,73));
        const float baseY=terrainHeight(x,z)-size*0.08f;
        const float rotation=2.0f*PI*hash2(rock,121);
        RockPoint bottom[ROCK_SEGMENTS+1];
        RockPoint middle[ROCK_SEGMENTS+1];
        RockPoint upper[ROCK_SEGMENTS+1];
        for(int i=0;i<=ROCK_SEGMENTS;++i) {
            const int index=i%ROCK_SEGMENTS;
            const float angle=rotation+2.0f*PI*index/ROCK_SEGMENTS;
            const float wobble=0.86f+0.24f*hash2(rock*7+index,149);
            const float c=std::cos(angle),s=std::sin(angle);
            bottom[i].x=x+size*0.66f*wobble*c;
            bottom[i].y=baseY+size*0.07f;
            bottom[i].z=z+size*0.66f*wobble*s;
            middle[i].x=x+size*wobble*c;
            middle[i].y=baseY+height*0.32f;
            middle[i].z=z+size*wobble*s;
            upper[i].x=x+size*0.62f*wobble*std::cos(angle+0.11f);
            upper[i].y=baseY+height*0.76f;
            upper[i].z=z+size*0.62f*wobble*std::sin(angle+0.11f);
        }

        const RockPoint tip={x+size*0.12f*(hash2(rock,167)-0.5f),
                             baseY+height,
                             z+size*0.12f*(hash2(rock,179)-0.5f)};
        const float shade=0.48f+0.18f*hash2(rock,191);
        glColor3f(shade,shade*0.91f,shade*0.82f);
        glBegin(GL_TRIANGLES);
        for(int i=0;i<ROCK_SEGMENTS;++i) {
            const RockPoint &b0=bottom[i],&b1=bottom[i+1];
            const RockPoint &m0=middle[i],&m1=middle[i+1];
            const RockPoint &u0=upper[i],&u1=upper[i+1];
            const RockPoint *triangles[5][3] = {
                {&b0,&m0,&m1}, {&b0,&m1,&b1},
                {&m0,&u0,&u1}, {&m0,&u1,&m1},
                {&u0,&tip,&u1}
            };
            for(int t=0;t<5;++t) {
                const RockPoint &a=*triangles[t][0];
                const RockPoint &b=*triangles[t][1];
                const RockPoint &c=*triangles[t][2];
                const float abx=b.x-a.x,aby=b.y-a.y,abz=b.z-a.z;
                const float acx=c.x-a.x,acy=c.y-a.y,acz=c.z-a.z;
                float nx=aby*acz-abz*acy;
                float ny=abz*acx-abx*acz;
                float nz=abx*acy-aby*acx;
                const float length=std::sqrt(nx*nx+ny*ny+nz*nz);
                if(length>0.0001f) glNormal3f(nx/length,ny/length,nz/length);
                const RockPoint *points[3]={&a,&b,&c};
                for(int v=0;v<3;++v) {
                    glTexCoord2f(points[v]->x*0.15f,points[v]->z*0.15f);
                    glVertex3f(points[v]->x,points[v]->y,points[v]->z);
                }
            }
        }
        glEnd();
    }
    glColor3f(1,1,1);
    glDisable(GL_TEXTURE_2D);

    // Sparse pioneer vegetation grows away from the hottest volcanic slopes.
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    for(int plant=0;plant<78;++plant) {
        const float angle=2.0f*PI*hash2(plant,223);
        const float radius=23.0f+17.0f*hash2(plant,227);
        const float x=radius*std::cos(angle),z=radius*std::sin(angle);
        const float y=terrainHeight(x,z)+0.015f;
        const float scale=0.55f+0.75f*hash2(plant,229);
        const bool sapling=(plant%7)==0;

        if(sapling) {
            const float trunkHeight=0.75f*scale;
            const float crownHeight=0.55f*scale;
            glColor3f(0.25f,0.19f,0.14f);
            glLineWidth(2.0f);
            glBegin(GL_LINES);
            glVertex3f(x,y,z);
            glVertex3f(x,y+trunkHeight,z);
            glEnd();
            glBegin(GL_TRIANGLES);
            for(int branch=0;branch<4;++branch) {
                const float a=angle+branch*PI*0.5f;
                const float dx=std::cos(a),dz=std::sin(a);
                const float bx=x+dx*0.10f*scale,bz=z+dz*0.10f*scale;
                glColor3f(0.22f+0.04f*(branch%2),0.30f+0.05f*(branch%2),0.15f);
                glNormal3f(dx*0.25f,0.85f,dz*0.25f);
                glVertex3f(bx,y+trunkHeight-0.08f*scale,bz);
                glVertex3f(x-dz*0.20f*scale,y+trunkHeight+0.02f*scale,z+dx*0.20f*scale);
                glVertex3f(x+dx*0.03f*scale,y+trunkHeight+crownHeight,z+dz*0.03f*scale);
            }
            glEnd();
        } else {
            glBegin(GL_TRIANGLES);
            for(int blade=0;blade<5;++blade) {
                const float a=angle+blade*(2.0f*PI/5.0f);
                const float dx=std::cos(a),dz=std::sin(a);
                const float sideX=-dz*0.11f*scale,sideZ=dx*0.11f*scale;
                const float height=(0.34f+0.30f*hash2(plant,blade+239))*scale;
                const float lean=0.20f*scale;
                const float shade=0.19f+0.10f*hash2(plant,blade+251);
                glColor3f(shade*0.72f,shade,shade*0.55f);
                glNormal3f(dx*0.35f,0.88f,dz*0.35f);
                glVertex3f(x,y,z);
                glVertex3f(x+sideX,y+0.08f*scale,z+sideZ);
                glVertex3f(x+dx*lean,y+height,z+dz*lean);
            }
            glEnd();
        }
    }
    glPopAttrib();
}

void renderVolcano() {
    const int SEG=128, RINGS=90;
    const float maxR=19.5f;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,rockTexture);
    glColor3f(1,1,1);
    // Dao winding cua dai vong de mat ngoai huong len tren.
    glFrontFace(GL_CW);
    for(int j=0;j<RINGS;++j) {
        const float r0=1.6f+(maxR-1.6f)*j/RINGS;
        const float r1=1.6f+(maxR-1.6f)*(j+1)/RINGS;
        glBegin(GL_QUAD_STRIP);
        for(int i=0;i<=SEG;++i) {
            float a=2.0f*PI*i/SEG;
            const float dr=0.05f,da=0.005f;
            const float hR=(mountainHeight(r0+dr,a)-mountainHeight(r0-dr,a))/(2*dr);
            const float hA=(mountainHeight(r0,a+da)-mountainHeight(r0,a-da))/(2*da);
            const float slopeX=hR*std::cos(a)-hA*std::sin(a)/r0;
            const float slopeZ=hR*std::sin(a)+hA*std::cos(a)/r0;
            glNormal3f(-slopeX,1.0f,-slopeZ);vertexMountain(r0,a);
            const float hR1=(mountainHeight(r1+dr,a)-mountainHeight(r1-dr,a))/(2*dr);
            const float hA1=(mountainHeight(r1,a+da)-mountainHeight(r1,a-da))/(2*da);
            glNormal3f(-(hR1*std::cos(a)-hA1*std::sin(a)/r1),1.0f,
                       -(hR1*std::sin(a)+hA1*std::cos(a)/r1));
            vertexMountain(r1,a);
        }
        glEnd();
    }
    glFrontFace(GL_CCW);
    glDisable(GL_TEXTURE_2D);
    // Nen toi ben trong mieng (nguoi 2 se ve dung nham de len o day).
    glColor3f(0.19f,0.10f,0.08f);
    glNormal3f(0,1,0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0,LAVA_FLOOR_Y-0.03f,0);
    for(int i=0;i<=SEG;++i) {
        const float a=2.0f*PI*i/SEG;
        glVertex3f(1.62f*std::cos(a),LAVA_FLOOR_Y-0.03f,1.62f*std::sin(a));
    }
    glEnd();
    glColor3f(1,1,1);
}

GLuint buildList(void (*render)()) {
    const GLuint list=glGenLists(1);
    if(!list) return 0;
    glNewList(list,GL_COMPILE);
    render();
    glEndList();
    return list;
}

}
namespace VolcanoScene {
void initTextures() {
    glPushAttrib(GL_TEXTURE_BIT);
    if(!rockTexture) makeTexture(rockTexture,true);
    if(!groundTexture) makeTexture(groundTexture,false);
    glPopAttrib();
}
void drawTerrain() {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT);
    if(!groundTexture) initTextures();
    if(!terrainList) terrainList=buildList(renderTerrain);
    // glBindTexture is not guaranteed to be stored in a display list.
    glBindTexture(GL_TEXTURE_2D,groundTexture);
    if(terrainList) glCallList(terrainList);
    else renderTerrain();
    glPopAttrib();
}
void drawVolcano() {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT);
    if(!rockTexture) initTextures();
    if(!volcanoList) volcanoList=buildList(renderVolcano);
    // Bind on every draw so cached geometry uses its own texture in host scenes.
    glBindTexture(GL_TEXTURE_2D,rockTexture);
    if(volcanoList) glCallList(volcanoList);
    else renderVolcano();
    glPopAttrib();
}
void drawSceneGeometry() { drawTerrain();drawVolcano(); }
void releaseTextures() {
    if(terrainList) {glDeleteLists(terrainList,1);terrainList=0;}
    if(volcanoList) {glDeleteLists(volcanoList,1);volcanoList=0;}
    if(rockTexture) {glDeleteTextures(1,&rockTexture);rockTexture=0;}
    if(groundTexture) {glDeleteTextures(1,&groundTexture);groundTexture=0;}
}
float craterCenterX() {return 0.0f;}
float craterCenterY() {return CRATER_Y;}
float craterCenterZ() {return 0.0f;}
float craterRadius() {return CRATER_R;}
float lavaFloorY() {return LAVA_FLOOR_Y;}
float volcanoSurfaceY(float radius,float angleRadians) {
    return mountainHeight(radius,angleRadians);
}
}
