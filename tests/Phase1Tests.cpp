#include "../Gameplay.h"
#include <cstdio>
#include <cstdlib>

void require(bool condition, const char* name) {
    if (!condition) { std::printf("FAIL: %s\n", name); std::exit(1); }
}
bool near(float a, float b) { return std::fabs(a-b) < 0.02f; }

int main() {
    const Vector2 world = {1344,1344}, viewport = {1024,768};
    Player straight, diagonal;
    straight.position = diagonal.position = {400,400};
    straight.update({1,0}, 1, world);
    diagonal.update({1,1}, 1, world);
    const float dx=diagonal.position.x-400, dy=diagonal.position.y-400;
    require(near(straight.position.x,580) && near(std::sqrt(dx*dx+dy*dy),180), "180 px/s in straight and diagonal directions");
    Player fine, coarse;
    fine.position = coarse.position = {400,400};
    for(int i=0;i<120;++i) fine.update({1,0},1.0f/120,world);
    for(int i=0;i<30;++i) coarse.update({1,0},1.0f/30,world);
    require(near(fine.position.x,coarse.position.x), "same displacement at 30 and 120 updates/s");
    fine.update({-1,-1},100,world);
    require(near(fine.position.x,14) && near(fine.position.y,14), "top left player bounds");
    fine.update({1,1},100,world);
    require(near(fine.position.x,1330) && near(fine.position.y,1330), "bottom right player bounds");
    Camera camera;
    camera.follow({14,14},world,viewport);
    require(near(camera.position.x,0) && near(camera.position.y,0), "top left camera clamp");
    camera.follow({1330,1330},world,viewport);
    require(near(camera.position.x,320) && near(camera.position.y,576), "bottom right camera clamp");
    camera.mode=CameraMode::Infinite;
    camera.follow({-500,9000},world,viewport);
    Vector2 screen=camera.worldToScreen({-500,9000});
    require(near(screen.x,512) && near(screen.y,384), "infinite camera interface centres target");
    PlaySession session;
    session.update({1,0},10,world,viewport);
    require(session.elapsed==0, "menu does not simulate");
    session.start(world,viewport);
    session.update({1,0},1,world,viewport);
    const Vector2 saved=session.player.position, savedCamera=session.camera.position;
    session.togglePause();
    for(int i=0;i<600;++i) session.update({-1,1},1.0f/60,world,viewport);
    require(session.player.position.x==saved.x && session.player.position.y==saved.y
        && session.camera.position.x==savedCamera.x && session.camera.position.y==savedCamera.y
        && session.elapsed==1 && session.player.velocity.x==0, "pause freezes position, camera, timer and velocity");
    session.togglePause();
    session.update({-1,0},0.5f,world,viewport);
    require(near(session.player.position.x,saved.x-90) && near(session.elapsed,1.5f), "resume without paused time catch-up");
    std::puts("PASS: phase 1 movement, frame independence, bounds, camera and pause");
}
