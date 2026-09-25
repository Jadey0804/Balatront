#pragma once
#include <cstdint>

struct TerrainTile {
    unsigned int images[4] = {}; // These are the four picture parts of one tile.
};

// A map place always gives the same ground when the seed is the same.
class ProceduralMap {
public:
    static constexpr int RegionSize = 64;
    static constexpr int SmallPondCount = 6;
    static constexpr int BranchPairs = 3;
    std::uint32_t seed = 1;
    static int floorDivide(int v, int d) { return v/d - (v%d < 0 ? 1 : 0); }

    TerrainTile appearance(int x, int y) const {
        TerrainTile result;
        if (water(x,y)) {
            const bool t=!water(x,y-1), b=!water(x,y+1), l=!water(x-1,y), r=!water(x+1,y);
            result.images[0]=t ? (l?16:21) : (l?15:14);
            result.images[1]=t ? (r?22:21) : (r?19:14);
            result.images[2]=b ? (l?17:18) : (l?15:14);
            result.images[3]=b ? (r?20:18) : (r?19:14);
            return result;
        }
        if (lava(x, y)) {
            for (unsigned int& id : result.images) id = 24;
            return result;
        }
        const unsigned int mask=unsigned(roadVertex(x,y)) | (unsigned(roadVertex(x+1,y))<<1)
            | (unsigned(roadVertex(x,y+1))<<2) | (unsigned(roadVertex(x+1,y+1))<<3);
        static constexpr unsigned int ids[16]={0,23,3,8,2,4,0,5,1,0,7,9,12,6,11,13};
        if (mask==6 || mask==9) {
            // Use two corner pictures when one diagonal tile picture does not exist.
            static constexpr unsigned int corners[4]={23,3,2,1};
            for (unsigned int i=0;i<4;++i) result.images[i]=mask & (1u<<i) ? corners[i] : 0;
        } else for (unsigned int& id:result.images) id=ids[mask];
        return result;
    }

    unsigned int tile(int x,int y) const {
        return appearance(x,y).images[0];
    }

    bool lava(int x, int y) const {
        const int col = floorDivide(x, RegionSize), row = floorDivide(y, RegionSize);
        std::uint32_t random = hash(col ^ 0x6a91, row ^ 0x24d7);
        const bool hasPool = next(random) % 4 != 0;
        const float cx = float(12 + next(random) % 40), cy = float(12 + next(random) % 40);
        const float rx = float(8 + next(random) % 7), ry = float(6 + next(random) % 7);
        const float px = float(x - col * RegionSize) + 0.5f;
        const float py = float(y - row * RegionSize) + 0.5f;
        const float side = next(random) % 2 ? 1.0f : -1.0f;
        bool hot = hasPool && (ellipse(px, py, cx, cy, rx, ry)
            || ellipse(px, py, cx + side * rx * 0.55f, cy + ry * 0.3f, rx * 0.7f, ry * 0.75f)
            || ellipse(px, py, cx - side * rx * 0.45f, cy - ry * 0.45f, rx * 0.55f, ry * 0.65f));
        if (next(random) % 3 != 0
            && ellipse(px, py, cx + side * rx * 0.7f, cy - ry * 0.8f, rx * 0.5f, ry * 0.45f))
            hot = false;
        if (!hot || water(x, y)) return false;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                if (roadVertex(x + dx, y + dy)) return false;
        return true;
    }

    bool water(int x,int y) const {
        const int col=floorDivide(x,RegionSize), row=floorDivide(y,RegionSize);
        std::uint32_t random=hash(col,row);
        const bool largePond = next(random)%4 != 0;
        const float cx=float(34+next(random)%12), cy=float(34+next(random)%12);
        const float rx=float(7+next(random)%9), ry=float(6+next(random)%10);
        const float px=float(x-col*RegionSize)+0.5f, py=float(y-row*RegionSize)+0.5f;
        const float side=next(random)%2 ? 1.0f : -1.0f;
        bool wet=ellipse(px,py,cx,cy,rx,ry)
            || ellipse(px,py,cx+side*rx*0.55f,cy+ry*0.35f,rx*0.65f,ry*0.8f);
        if (next(random)%3!=0 && ellipse(px,py,cx-side*rx*0.4f,cy-ry*0.75f,rx*0.55f,ry*0.5f)) wet=false;
        wet = wet && largePond;
        // Put some small water pools 
        random = hash(col ^ 0x579b, row);
        for (int i = 0; i < SmallPondCount; ++i) {
            const float smallX = float(7 + (i % 2) * 28 + next(random) % 11);
            const float smallY = float(7 + (i / 2) * 18 + next(random) % 7);
            const float smallRX = float(2 + next(random) % 3);
            const float smallRY = float(2 + next(random) % 4);
            if (ellipse(px, py, smallX, smallY, smallRX, smallRY)) wet = true;
        }
        if (!wet || px<3 || py<3 || px>=RegionSize-3 || py>=RegionSize-3) return false;
        // leave some grass near roads so water does not cover the road.
        for (int dy=-1;dy<=1;++dy)
            for (int dx=-1;dx<=1;++dx)
                if (roadVertex(x+dx,y+dy)) return false;
        return true;
    }

    bool roadVertex(int x,int y) const {
        const int col=floorDivide(x,RegionSize), row=floorDivide(y,RegionSize);
        const int lx=x-col*RegionSize, ly=y-row*RegionSize;
        const int topX=8+hash(col,row)%12, bottomX=8+hash(col,row+1)%12;
        const int bendY=26+hash(col^0x1357,row)%14;
        const int leftY=8+hash(col,row^0x2468)%12, rightY=8+hash(col+1,row^0x2468)%12;
        const int bendX=26+hash(col,row^0x3579)%14;
        if ((ly<=bendY && withinBand(lx,topX,1)) || (ly>=bendY && withinBand(lx,bottomX,1))
            || (between(lx,topX,bottomX) && withinBand(ly,bendY,1))
            || (lx<=bendX && withinBand(ly,leftY,1)) || (lx>=bendX && withinBand(ly,rightY,1))
            || (between(ly,leftY,rightY) && withinBand(lx,bendX,1))) return true;
        std::uint32_t random=hash(col^0x468a,row);
        for (int i = 0; i < BranchPairs; ++i) {
            const int branchY = 42 + i * 7 + next(random) % 3;
            const int endX = bottomX + 6 + next(random) % 18;
            const int endY = branchY + int(next(random) % 5) - 2;
            if ((between(lx, bottomX, endX) && ly == branchY)
                || (lx == endX && between(ly, branchY, endY))) return true;
            const int branchX = 42 + i * 7 + next(random) % 3;
            const int tipY = rightY + 6 + next(random) % 18;
            if (lx == branchX && between(ly, rightY, tipY)) return true;
        }
        return false;
    }

private:
    static bool withinBand(int v,int c,int r) { return v>=c-r && v<=c+r; }
    static bool between(int v,int a,int b) { return a<b ? v>=a && v<=b : v>=b && v<=a; }
    static bool ellipse(float x,float y,float cx,float cy,float rx,float ry) {
        const float dx=(x-cx)/rx, dy=(y-cy)/ry;
        return dx*dx+dy*dy<=1;
    }
    static std::uint32_t next(std::uint32_t& state) {
        state=state*1664525u+1013904223u;
        return state>>8;
    }
    std::uint32_t hash(int x,int y) const {
        std::uint32_t value=seed^(static_cast<std::uint32_t>(x)*0x9e3779b9u)
            ^(static_cast<std::uint32_t>(y)*0x85ebca6bu);
        value^=value>>16; value*=0x7feb352du;
        value^=value>>15; value*=0x846ca68bu;
        return value^(value>>16);
    }
};