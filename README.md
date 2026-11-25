# DO AN DO HOA MAY TINH - NUI LUA, DIA HINH, TEXTURE

## Pham vi hoan thanh
- Dia hinh 3D dang heightmap tao tu value noise; mat dat co doi nhe bao quanh nui.
- Dia hinh co doi tro va song nui nho, phu texture tro den am va da bazan low-poly quanh chan nui.
- Duong chan troi co cac day nui lua xa, them chieu sau cho vung dat xung quanh.
- Suong khi quyen trong demo giup tach lop nui gan va xa, co the bat/tat bang phim H.
- Nui lua 3D mesh vong tron voi suon doc, mieng rong va long chao ben trong.
- Mat da va mat dat su dung procedural texture OpenGL, khong phai tai anh.
- Mesh tinh duoc cache bang OpenGL display list sau lan ve dau tien, tranh tinh lai noise va phap tuyen moi khung hinh.
- Hieu ung dung nham chuyen dong, lua va khoi billboards duoc cap nhat theo delta time.
- Cac dong dung nham phat sang bam theo mesh suon nui, co loi nong va vien glow.
- Tan lua bay khoi mieng nui theo quy dao co trong luc, tao cam giac phun trao.
- Cac puff khoi trong suot duoc ve tu xa den gan; module nui khoi phuc cac trang thai OpenGL sau khi ve.
- Phap tuyen cho anh sang; toa do giao tiep de nguoi 2 ve dung nham/lua/khoi.
- `demo_main.cpp` chi la chuong trinh demo de xem model; camera/anh sang chinh do Nguoi 3 dam nhan.

## Cau truc
```
NuiLua_OpenGL/
  src/VolcanoScene.h       // API dia hinh va nui lua
  src/VolcanoScene.cpp     // dia hinh, mesh, texture
  src/VolcanoEffects.h     // API hieu ung
  src/VolcanoEffects.cpp   // dung nham, lua, khoi
  src/demo_main.cpp        // cua so xem rieng
  Makefile                 // build Linux
```

## Chay Ubuntu (VM hoac may that)
```
sudo apt update
sudo apt install -y build-essential freeglut3-dev libglu1-mesa-dev
cd NuiLua_OpenGL
make demo
./volcano_demo
```
Phai dang trong phien desktop Ubuntu co ho tro hien thi OpenGL. May ao VirtualBox can cau hinh do hoa phu hop.
Neu chi can module scene/effects de tich hop vao chuong trinh khac, chay `make` de tao `libvolcano.a`; target nay khong can GLUT. Chuong trinh chinh van can cac thu vien OpenGL tuong ung khi link.

## Chay Windows / Code::Blocks / MinGW
Cai dat FreeGLUT vao compiler neu muon chay demo doc lap; demo can `GL/glut.h`.
Neu tich hop module vao chuong trinh da co cua so/context OpenGL, chi them `src/VolcanoScene.cpp` va `src/VolcanoEffects.cpp`, khong them `demo_main.cpp`; khi link chuong trinh, them `opengl32` va cac thu vien OpenGL ma ung dung dang dung.
De build demo doc lap, them ca ba file `.cpp` va link `opengl32`, `glu32`, `freeglut` (hoac `glut32`, tuy bo thu vien). Neu dung DLL, chep `freeglut.dll` vao canh exe.
Compiler su dung C++98.

## Dieu khien demo
- A / D: Xoay quanh nui
- W / S: Tang/giam goc nhin
- + / -: Thu/phong
- F: Bat/tat wireframe
- L: Bat/tat anh sang
- H: Bat/tat suong khi quyen
- ESC: Thoat

## Tich hop vao project chung
Trong chuong trinh chinh cua Nguoi 3:
```
#include "VolcanoScene.h"
#include "VolcanoEffects.h"
// Sau khi tao OpenGL context:
VolcanoScene::initTextures();
VolcanoEffects::init();
// Moi frame, cap nhat bang deltaSeconds (giay), vi du thoi gian giua hai frame:
VolcanoEffects::update(deltaSeconds);
// Trong render loop, SAU KHI camera va lighting da duoc cau hinh:
VolcanoScene::drawSceneGeometry();
VolcanoEffects::draw();
// Khi thoat (neu con context):
VolcanoEffects::release();
VolcanoScene::releaseTextures();
```
Khong them `demo_main.cpp` vao chuong trinh chinh da co `main()`.
`drawTerrain()` va `drawVolcano()` tu tao texture neu chua goi `initTextures()`. Mesh se duoc cache o lan ve dau tien; goi `releaseTextures()` truoc khi huy OpenGL context de giai phong ca cache va texture.

## Thong so de Nguoi 2 tich hop
- Tam mieng nui: `(0, 16, 0)`; huong truc Y len.
- Ban kinh mep mieng: `3.4` (world units).
- San day long chao / dung nham: `Y = 12.0`.
- Tam nui: `(0,0,0)`; chan nui ban kinh khoang `19.5`.
- Goi `VolcanoScene::craterCenterX/Y/Z()`, `craterRadius()`, `lavaFloorY()` thay vi viet so cung.
- Khi ve particle, ve tren mieng (Y >= 16); dung nham trong mieng co the ve o Y >= 12.05.

## Thuat toan
1. **Heightmap**: moi diem `(x,z)` duoc tinh do cao `y` tu noise da tan; vung gan nui duoc lam phang de tranh giao nhau.
2. **Radial profile**: cac moc ban kinh/do cao tinh hinh chop cong, mieng loe va long chao.
3. **Mesh**: lay mau theo ban kinh va goc, noi cac dinh bang quad strips; ap nhiễu nho tao vach da tu nhien.
4. **Texture**: tao anh RGB 128x128 bang noise, dat GL_REPEAT va mipmap de tranh nhoe/hien tuong rang cua.
5. **Phong sang**: tinh vector phap tuyen tu dao ham do cao, cho anh sang bam theo mat da.

## Phan cong ro rang
- Nguoi 1: model nui + mat dat + texture (`VolcanoScene.*`)
- Nguoi 2: dung nham, lua, khoi va particles (`VolcanoEffects.*`)
- Nguoi 3: camera, lighting, UI, keyboard, tich hop (chi co camera demo trong `demo_main.cpp`)
