#include <iostream>
#include<fstream>
#include "GamesEngineeringBase.h"
#include "GameConfig.h"
#include "Game.h"


using namespace std;

class Plane {
    int x, y;
    GamesEngineeringBase::Image image;
    int yCollide;
public:
    Plane(GamesEngineeringBase::Window& canvas, std::string filename) {
        image.load(filename);
        x = (canvas.getWidth() / 2) - (image.width / 2);
        y = canvas.getHeight() - image.height;
        yCollide = image.height / 3;
    }

    void draw(GamesEngineeringBase::Window& canvas) {
        for (unsigned int k = 0; k < image.height; k++)
            if (k + y > 0 && k + y < canvas.getHeight()) {
                for (unsigned int j = 0; j < image.width; j++)
                    if (j + x > 0 && j + x < canvas.getWidth())
                        if (image.alphaAtUnchecked(j, k) > 0)
                            canvas.draw(x + j, y + k, image.atUnchecked(j, k));
            }

        // draw green line at point of collision
      //  for (unsigned int k = 0; k < image.width; k++)
        //    canvas.draw(x + k, canvas.getHeight() - yCollide, 0, 255, 0);
    }

    void update(GamesEngineeringBase::Window& canvas, int _x) {
        x += _x;
        x = max(0, x);
        x = min(static_cast<int>(canvas.getWidth()) - static_cast<int>(image.width), x);
    }

    int getYCollide() { return yCollide; }
    int getX() { return x; }
    int getWidth() { return image.width; }
};

class tile {
    GamesEngineeringBase::Image sprite;
public:
    tile() {}
    void load(std::string filename) {
        sprite.load(filename);
    }
    void draw(GamesEngineeringBase::Window& canvas, int y) {
        for (unsigned int i = 0; i < sprite.height; i++)
            // bounds checking goes here
            if (y + i > 0 && (y + i) < (canvas.getHeight()))
                for (unsigned int n = 0; n < sprite.width; n++)
                    canvas.draw(n, y + i, sprite.atUnchecked(n, i));
    }
    unsigned int getHeight() { return sprite.height; }
    unsigned int getWidth() { return sprite.width; }
    GamesEngineeringBase::Image& getSprite() { return sprite; }

};


const unsigned int tileNum = 6;
class tileSet {
    tile t[tileNum];
    unsigned int size = tileNum;
public:
    // create and load tiles here
    tileSet(std::string pre = "") {
        for (unsigned int i = 0; i < tileNum; i++) {
            std::string filename;
            filename = "Resources/" + pre + std::to_string(i) + ".png";
            t[i].load(filename);
        }
    }

    // access individual tile here
    tile& operator[](unsigned int index) { return t[index]; }
};

const int worldSize = 1000;
class world {
    tileSet ts;
    tileSet alphas;
    unsigned int* tarray;
    unsigned int size;

public:
    world() : ts(), alphas("alpha") {
        tarray = new unsigned int[worldSize];
        size = worldSize;
        for (unsigned int i = 0; i < worldSize; i++)
            tarray[i] = rand() % 6;
    }

    //- File handling start

    world(const std::string& filename) : ts(), alphas("alpha") {
        std::ifstream fin(filename);

        fin >> size; // first number is the world size

        tarray = new unsigned int[size];


        // read order
        for (unsigned int i = 0; i < size; i++) {
            if (!(fin >> tarray[i])) {
                std::cerr << "File does not contain enough numbers.\n";
                delete[] tarray;
                tarray = nullptr;
                size = 0;
                return;
            }
        }
        fin.close();

        cout << "Loaded world from file '" << filename << "'.\n";
        cout << "worldSize = " << size << "\n";
        cout << "Tile order: ";
        for (unsigned int i = 0; i < size; i++)
            cout << tarray[i] << " ";
        cout << "\n";

    }

    ~world() {
        delete[] tarray;
    }

    //- File Handling end


    void draw(GamesEngineeringBase::Window& canvas, unsigned int y) {

        unsigned int offset = y % 384;
        unsigned int Y = y / 384;

        ts[tarray[Y % size]].draw(canvas, (canvas.getHeight() / 2) + offset);
        ts[tarray[(Y + 1) % size]].draw(canvas, offset);
        ts[tarray[(Y + 2) % size]].draw(canvas, offset - (canvas.getHeight() / 2));
    }

    void drawAlphas(GamesEngineeringBase::Window& canvas, unsigned int y) {

        unsigned int offset = y % 384;
        unsigned int Y = y / 384;

        alphas[tarray[Y % size]].draw(canvas, (canvas.getHeight() / 2) + offset);
        alphas[tarray[(Y + 1) % size]].draw(canvas, offset);
        alphas[tarray[(Y + 2) % size]].draw(canvas, offset - (canvas.getHeight() / 2));
    }


    // this a simple first collision that just draws the line of collision
    void collision(GamesEngineeringBase::Window& canvas, Plane& h, unsigned int y) {

        int Y = y / 384;
        tile& T = alphas[tarray[Y % size]];

        unsigned int yCoord = T.getHeight() - ((h.getYCollide() + y) % T.getHeight());

        for (unsigned int x = 0; x < canvas.getWidth(); x++)
            if (T.getSprite().at(x, yCoord, 0) < 1)
                canvas.draw(x, canvas.getHeight() - h.getYCollide(), 255, 0, 0);
    }

    void collision2(GamesEngineeringBase::Window& canvas, Plane& h, unsigned int y) {

        int Y = y / 384;
        tile& T = alphas[tarray[Y % size]];

        unsigned int yCoord = T.getHeight() - ((h.getYCollide() + y) % T.getHeight());

        for (unsigned int x = 0; x < h.getWidth(); x++)
            if (T.getSprite().at(x + h.getX(), yCoord, 0) < 1)
                canvas.draw(x + h.getX(), canvas.getHeight() - h.getYCollide(), 255, 0, 0);
            else
                canvas.draw(x + h.getX(), canvas.getHeight() - h.getYCollide(), 0, 255, 0);
    }
};

class Manager {
    Plane hero;
    world w;
    unsigned int y = 0;
    bool drawAlpha = false;
public:
    Manager(GamesEngineeringBase::Window& canvas) : hero(canvas, "Resources/L.png"), w() {}
    Manager(GamesEngineeringBase::Window& canvas, string filename) : hero(canvas, "Resources/L.png"), w(filename) {}
    void update(GamesEngineeringBase::Window& canvas) {
        int x = 0;
        y += 2;
        drawAlpha = false;
        if (canvas.keyPressed(VK_UP)) y += 5;
        if (canvas.keyPressed(VK_DOWN)) y -= 1;
        if (canvas.keyPressed(VK_LEFT)) x -= 1;
        if (canvas.keyPressed(VK_RIGHT)) x += 1;
        if (canvas.keyPressed('A')) drawAlpha = true;
        hero.update(canvas, x);
    }
    void draw(GamesEngineeringBase::Window& canvas) {
        if (drawAlpha) w.drawAlphas(canvas, y);
        else w.draw(canvas, y);
        hero.draw(canvas);
        w.collision2(canvas, hero, y);

    }

};


int main() {
    Game game;
    return game.run();
}