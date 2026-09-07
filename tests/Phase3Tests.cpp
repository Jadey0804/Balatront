#include "../Enemies.h"
#include <cstdio>
#include <cstdlib>

void require(bool ok, const char* label) {
    if (!ok) { std::printf("FAIL: %s\n", label); std::exit(1); }
}
bool near(float a, float b) { return std::fabs(a-b) < 0.02f; }

int main() {
    const Vector2 world={1344,1344}, viewport={1024,768};
    PlaySession session;
    session.start(world,viewport);
    EnemyManager enemies;
    enemies.reset(42);
    for(unsigned int i=0;i<EnemyConfig::TypeCount;++i) {
        enemies.spawn(static_cast<EnemyType>(i), {session.player.position.x-200,session.player.position.y},true);
    }
    enemies.update(session,1,world,viewport);
    for(unsigned int i=0;i<EnemyConfig::TypeCount;++i) {
        const EnemyDefinition& stats=EnemyManager::definition(static_cast<EnemyType>(i));
        require(near(enemies.at(i).position.x,session.player.position.x-200+stats.speed),"configured pursuit speed / stationary turret");
        require(enemies.at(i).health==stats.health,"per-type health");
    }
    const Vector2 paused=enemies.at(0).position;
    const unsigned int count=enemies.activeCount();
    session.togglePause();
    enemies.update(session,120,world,viewport);
    require(enemies.activeCount()==count && enemies.at(0).position.x==paused.x,"pause freezes movement and spawning");
    session.togglePause();
    require(session.player.health==100,"no contact damage in this phase");

    // Run real update calls over two minutes, inspecting each newly created slot.
    enemies.reset(42);
    session.start(world,viewport);
    unsigned int previous=0, quarter[4]={};
    for(int frame=0;frame<7200;++frame) {
        session.update({},1.0f/60,world,viewport);
        enemies.update(session,1.0f/60,world,viewport);
        const unsigned int active=enemies.activeCount();
        quarter[frame/1800] += active-previous;
        for(unsigned int i=previous;i<active;++i) {
            const Enemy& enemy=enemies.at(i);
            const float radius=EnemyManager::definition(enemy.type).radius;
            require(enemy.position.x>=radius && enemy.position.x<=world.x-radius
                && enemy.position.y>=radius && enemy.position.y<=world.y-radius,"spawn inside world");
            Vector2 p=session.camera.worldToScreen(enemy.position);
            if(enemy.spawnedInside) {
                require(p.x>=radius && p.x<=viewport.x-radius && p.y>=radius && p.y<=viewport.y-radius,"inside spawn fits viewport");
            } else {
                require(p.x+radius<0 || p.x-radius>viewport.x || p.y+radius<0 || p.y-radius>viewport.y,"outside spawn completely invisible");
            }
            const float dx=enemy.position.x-session.player.position.x,dy=enemy.position.y-session.player.position.y;
            require(dx*dx+dy*dy>=160*160,"safe spawn distance");
        }
        previous=active;
    }
    require(enemies.insideCount>0 && enemies.outsideCount>0,"both spawn strategies appear");
    for(unsigned int n:enemies.typeCounts) require(n>0,"all four types within two minutes");
    require(quarter[0]<quarter[1] && quarter[1]<quarter[2] && quarter[2]<quarter[3],"spawn counts increase each quarter");
    std::printf("Two minute counts: %u %u %u %u; types: %u %u %u %u; inside %u outside %u\n",
        quarter[0],quarter[1],quarter[2],quarter[3],enemies.typeCounts[0],enemies.typeCounts[1],enemies.typeCounts[2],enemies.typeCounts[3],enemies.insideCount,enemies.outsideCount);
    require(near(EnemyManager::spawnInterval(0),1.6f) && near(EnemyManager::spawnInterval(30),1.2f)
        && near(EnemyManager::spawnInterval(60),.85f) && near(EnemyManager::spawnInterval(90),.55f),"four exact interval boundaries");
    enemies.reset();
    for(unsigned int i=0;i<GameConfig::MaxEnemies;++i) require(enemies.spawn(EnemyType::Grunt,{400,400},true)>=0,"pool slot available");
    require(enemies.spawn(EnemyType::Grunt,{400,400},true)==-1 && enemies.capacitySkips==1,"full pool safely refuses spawn");
    require(enemies.spawn(EnemyType::Count,{},true)==-1,"invalid type rejected");
    enemies.reset();
    require(enemies.activeCount()==0 && enemies.insideCount==0 && enemies.capacitySkips==0,"restart clears pool and counters");
    // When no offscreen world exists, bounded retries must skip rather than hang.
    session.start(viewport,viewport);
    session.elapsed=120;
    enemies.update(session,10,viewport,viewport);
    require(enemies.locationSkips>0,"impossible offscreen spawn is bounded");
    std::puts("PASS: phase 3 enemies, spawning, pause, capacity and bounds");
}
