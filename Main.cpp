#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <random>
#include <chrono>
#include <fstream>
#include <sstream>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

//global common variables
const int boardheight = 20;
const int boardwidth = 10;
int field[boardheight][boardwidth] = {0};
int dx = 0;
bool rotate = false;
int colorNum = 1;
float timer = 0;
float delay = 0.3;
sf::Clock cl;

//a is the actual 4 points piece
//b is a auxiliary array
struct Point{
    int x,y;
} a[4], b[4];

//figures are 8 rows x 2 columns
int figures[7][4] =
{
	1,3,5,7, // I
	2,4,5,7, // Z
	3,5,4,6, // S
	3,5,4,7, // T
	2,3,5,7, // L
	3,5,7,6, // J
	2,3,4,5, // O
};

int score = -1;
enum game_states {SPLASH, MENU, GAME, END_GAME};
int state = SPLASH;
std::vector<int> vhiscores;

#include "Global.h"
#include "CSprite.h"
#include "Background.h"
#include "GameEngine.h"

//class variables
GameEngine *pGame;
CSprite* s;

//functions
void NewGame();
bool valid();

bool GameInitialize()
{
    pGame = new GameEngine("Tetris",320,480);
    if(pGame == nullptr) return false;

    pGame->SetFrameRate(30);

    return true;
}

void GameStart()
{
    pGame->loadAssets();
    pGame->playMusic("music",true);

    sf::FloatRect rcBounds(0,0,pGame->GetWidth(), pGame->GetHeight());
    s = new CSprite("tiles",rcBounds, BA_STOP);

    ReadHiScores(vhiscores);
    NewGame();
}

void GameEnd()
{
    WriteHiScores(vhiscores);
    pGame->stopMusic("music");

    pGame->CleanupAll();
    pGame->window.close();
    delete pGame;
}

void GameActivate()
{
    pGame->continueMusic("music");
}

void GameDeactivate()
{
    pGame->pauseMusic("music");
}

void GamePaint(sf::RenderWindow &window)
{
    window.clear();

    switch(state)
    {
    case SPLASH:
        pGame->showTexture("splash",0,0, window);
        break;
    case MENU:
        {
            pGame->showTexture("menu",0,0, window);

            //show hi scores
            std::string histr="HI-SCORES\n";
            for(int i=0;i<5;i++)
            {
                histr = histr + "     " + std::to_string(vhiscores[i]) + "\n";
            }
            pGame->Text(histr, 80, 240, sf::Color::Cyan, 20, "font", window);
            break;
        }
    case GAME:
        {
            pGame->showTexture("background", 0,0, window);
            //draw the field
            for(int i=0;i<boardheight;i++)
                for(int j=0;j<boardwidth; j++)
            {
                if(field[i][j]==0) continue;
                s->SetTextureRect(sf::IntRect(field[i][j]*18,0,18,18));
                s->SetPosition(j*18,i*18);
                s->OffsetPosition(28,31); //offset
                s->Draw(window);
            }

            //the actual piece
            for(int i=0;i<4;i++)
            {
                s->SetTextureRect(sf::IntRect(colorNum*18,0,18,18));
                s->SetPosition(a[i].x*18,a[i].y*18);
                s->OffsetPosition(28,31); //offset
                s->Draw(window);
            }

            pGame->showTexture("frame",0,0,window);

            //draw the score
            std::string sc = "SCORE:  " + std::to_string(score);
            pGame->Text(sc,240,20,sf::Color::Black, 20, "font", window);

            pGame->DrawSprites(window);
            break;
        }
    case END_GAME:
        {
            pGame->Text("GAME OVER", 100,30, sf::Color::Cyan, 25, "font", window);
            pGame->Text("PRESS M", 100,100, sf::Color::Cyan, 25, "font", window);
            break;
        }
    default:
        break;
    }
    window.display();
}

void GameCycle(sf::Time delta)
{
    if( state == GAME )
    {
        float time = cl.getElapsedTime().asSeconds();
        cl.restart();
        timer+= time;

        //// <- Move -> ///
        //copy a to b and update a.
        for (int i=0;i<4;i++)  { b[i] = a[i]; a[i].x += dx; }
        //if a is not valid then get b again.
        if (!valid()) for (int i=0;i<4;i++) a[i] = b[i];

        //////Rotate//////
        if (rotate)
        {
            Point p = a[1]; //center of rotation
            for (int i=0;i<4;i++)
              {
                int x = a[i].y - p.y;
                int y = a[i].x - p.x;
                a[i].x = p.x - x;
                a[i].y = p.y + y;
              }
            //if a is not valid get b again.
            if (!valid()) for (int i=0;i<4;i++) a[i] = b[i];
        }

        ///////Tick//////
        if (timer>delay)
        {
            //one down
            for (int i=0;i<4;i++) { b[i] = a[i]; a[i].y += 1; }

            //if not valid now is because it can't move down,
            //so create a new piece.
            if (!valid())
            {
                //if any of the cells is occupied by a piece then we can't put more
                //so end game.
                for(int i=0;i<4;i++) if( field[b[i].y][b[i].x] ) state = END_GAME;
                if( state == END_GAME ) UpdateHiScores(vhiscores, score);

                 for (int i=0;i<4;i++) field[b[i].y][b[i].x] = colorNum;

                 colorNum = 1 + rand()%7; //get new color
                 int n = rand()%7; //get new figure
                 for (int i=0;i<4;i++)
                 {
                        a[i].x = figures[n][i] % 2;
                        a[i].y = figures[n][i] / 2;
                 }
            }

            timer = 0;
        }

        ///////check lines//////////
        int k = boardheight - 1;
        for (int i = boardheight-1;i>0;i--)
        {
            int count=0;
            for (int j=0;j<boardwidth;j++)
            {
                //if there is part of a piece there
                if (field[i][j]) count++;
                field[k][j] = field[i][j];
            }
            if (count<boardwidth) k--;
            else
            {
                pGame->playSound("line");
                score += 40;
            }
        }

        //restore default values
        dx=0; rotate=0; delay=0.3;
    }
}

void HandleKeys()
{
    switch(state)
    {
    case SPLASH:
        {
            if( pGame->KeyPressed(sf::Keyboard::Space)) state = MENU;
            break;
        }
    case MENU:
        {
            if( pGame->KeyPressed(sf::Keyboard::S) )
            {
                state = GAME;
                NewGame();
            }
            break;
        }
    case GAME:
        {
        //space is the fire key
        if( pGame->KeyPressed(sf::Keyboard::Up) )
            rotate = true;
        else if( pGame->KeyPressed(sf::Keyboard::Left))
            dx = -1;
        else if( pGame->KeyPressed(sf::Keyboard::Right)) dx = 1;

        //if down arrow is pressed make it go faster
        if( pGame->KeyPressed(sf::Keyboard::Down) || pGame->KeyHeld(sf::Keyboard::Down)) delay = 0.05;
        break;
        }
    case END_GAME:
        {
            if( pGame->KeyPressed(sf::Keyboard::M) ) state = MENU;
            break;
        }
    default:
        break;
    }
}

void MouseButtonDown(int x, int y, bool bLeft)
{
}

void MouseButtonUp(int x, int y, bool bLeft)
{

}

void MouseMove(int x, int y)
{

}

bool SpriteCollision(CSprite* pSpriteHitter, CSprite* pSpriteHittee)
{
    return false;
}

void SpriteDying(CSprite* pSprite)
{

}

void NewGame()
{
    score = 0;
    dx = 0;
    rotate = false;
    colorNum = 1;
    delay = 0.3;
    //initialization for the first piece
	a[0].x = 0, a[0].y = 1;
    a[1].x = 1, a[1].y = 1;
    a[2].x = 1, a[2].y = 2;
    a[3].x = 1, a[3].y = 3;

    for(int i=0;i<boardheight;i++)
        for(int j=0;j<boardwidth;j++)
            field[i][j] = 0;
}

bool valid()
{
    //if out of bounds or the cells are occupied returns 0
    for (int i=0;i<4;i++)
      if (a[i].x<0 || a[i].x>=boardwidth || a[i].y>=boardheight) return false;
      else if (field[a[i].y][a[i].x]) return false;

    return true;
};

