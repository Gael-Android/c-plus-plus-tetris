﻿#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <termios.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <math.h>

#include "colors.h"
#include "Matrix.h"

using namespace std;


/**************************************************************/
/**************** Linux System Functions **********************/
/**************************************************************/

char saved_key = 0;

int tty_raw(int fd);    /* put terminal into a raw mode */
int tty_reset(int fd);    /* restore terminal's mode */

/* Read 1 character - echo defines echo mode */
char getch() {
    char ch;
    int n;
    while (1) {
        tty_raw(0);
        n = read(0, &ch, 1);
        tty_reset(0);
        if (n > 0)
            break;
        else if (n < 0) {
            if (errno == EINTR) {
                if (saved_key != 0) {
                    ch = saved_key;
                    saved_key = 0;
                    break;
                }
            }
        }
    }
    return ch;
}

void sigint_handler(int signo) {
    // cout << "SIGINT received!" << endl;
    // do nothing;
}

void sigalrm_handler(int signo) {
    alarm(1);
    saved_key = 's';
}

void registerInterrupt() {
    struct sigaction act, oact;
    act.sa_handler = sigint_handler;
    sigemptyset(&act.sa_mask);
#ifdef SA_INTERRUPT
    act.sa_flags = SA_INTERRUPT;
#else
    act.sa_flags = 0;
#endif
    if (sigaction(SIGINT, &act, &oact) < 0) {
        cerr << "sigaction error" << endl;
        exit(1);
    }
}

void registerAlarm() {
    struct sigaction act, oact;
    act.sa_handler = sigalrm_handler;
    sigemptyset(&act.sa_mask);
#ifdef SA_INTERRUPT
    act.sa_flags = SA_INTERRUPT;
#else
    act.sa_flags = 0;
#endif
    if (sigaction(SIGALRM, &act, &oact) < 0) {
        cerr << "sigaction error" << endl;
        exit(1);
    }
    alarm(1);
}

/**************************************************************/
/**************** Tetris Blocks Definitions *******************/
/**************************************************************/
#define MAX_BLK_TYPES 7
#define MAX_BLK_DEGREES 4

int T0D0[] = {1, 1, 1, 1, -1};
int T0D1[] = {1, 1, 1, 1, -1};
int T0D2[] = {1, 1, 1, 1, -1};
int T0D3[] = {1, 1, 1, 1, -1};

int T1D0[] = {0, 1, 0, 1, 1, 1, 0, 0, 0, -1};
int T1D1[] = {0, 1, 0, 0, 1, 1, 0, 1, 0, -1};
int T1D2[] = {0, 0, 0, 1, 1, 1, 0, 1, 0, -1};
int T1D3[] = {0, 1, 0, 1, 1, 0, 0, 1, 0, -1};

int T2D0[] = {1, 0, 0, 1, 1, 1, 0, 0, 0, -1};
int T2D1[] = {0, 1, 1, 0, 1, 0, 0, 1, 0, -1};
int T2D2[] = {0, 0, 0, 1, 1, 1, 0, 0, 1, -1};
int T2D3[] = {0, 1, 0, 0, 1, 0, 1, 1, 0, -1};

int T3D0[] = {0, 0, 1, 1, 1, 1, 0, 0, 0, -1};
int T3D1[] = {0, 1, 0, 0, 1, 0, 0, 1, 1, -1};
int T3D2[] = {0, 0, 0, 1, 1, 1, 1, 0, 0, -1};
int T3D3[] = {1, 1, 0, 0, 1, 0, 0, 1, 0, -1};

int T4D0[] = {0, 1, 0, 1, 1, 0, 1, 0, 0, -1};
int T4D1[] = {1, 1, 0, 0, 1, 1, 0, 0, 0, -1};
int T4D2[] = {0, 1, 0, 1, 1, 0, 1, 0, 0, -1};
int T4D3[] = {1, 1, 0, 0, 1, 1, 0, 0, 0, -1};

int T5D0[] = {0, 1, 0, 0, 1, 1, 0, 0, 1, -1};
int T5D1[] = {0, 0, 0, 0, 1, 1, 1, 1, 0, -1};
int T5D2[] = {0, 1, 0, 0, 1, 1, 0, 0, 1, -1};
int T5D3[] = {0, 0, 0, 0, 1, 1, 1, 1, 0, -1};

int T6D0[] = {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1};
int T6D1[] = {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -1};
int T6D2[] = {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1};
int T6D3[] = {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -1};

int *setOfBlockArrays[] = {
        T0D0, T0D1, T0D2, T0D3,
        T1D0, T1D1, T1D2, T1D3,
        T2D0, T2D1, T2D2, T2D3,
        T3D0, T3D1, T3D2, T3D3,
        T4D0, T4D1, T4D2, T4D3,
        T5D0, T5D1, T5D2, T5D3,
        T6D0, T6D1, T6D2, T6D3,
};

void drawScreen(Matrix *screen, int wall_depth) {
    int dy = screen->get_dy();
    int dx = screen->get_dx();
    int dw = wall_depth;
    int **array = screen->get_array();

    for (int y = 0; y < dy - dw + 1; y++) {
        for (int x = dw - 1; x < dx - dw + 1; x++) {
            if (array[y][x] == 0)
                cout << "□ ";
            else if (array[y][x] == 1)
                cout << "■ ";
            else if (array[y][x] == 10)
                cout << "◈ ";
            else if (array[y][x] == 20)
                cout << "★ ";
            else if (array[y][x] == 30)
                cout << "● ";
            else if (array[y][x] == 40)
                cout << "◆ ";
            else if (array[y][x] == 50)
                cout << "▲ ";
            else if (array[y][x] == 60)
                cout << "♣ ";
            else if (array[y][x] == 70)
                cout << "♥ ";
            else
                cout << "X ";
        }
        cout << endl;
    }
}

/**************************************************************/
/******************** Tetris Main Loop ************************/
/**************************************************************/

#define SCREEN_DY  10
#define SCREEN_DX  10
#define SCREEN_DW  4

#define ARRAY_DY (SCREEN_DY + SCREEN_DW)
#define ARRAY_DX (SCREEN_DX + 2*SCREEN_DW)

int arrayScreen[ARRAY_DY][ARRAY_DX] = {
        {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

void deleteFullLines(Matrix *gameMap, int top, int left, int blockHeight) {
//    gameMap->print();
//    cout << "top : " << top << endl;
//    cout << "left : " << left << endl;
//    cout << "blockHeight : " << blockHeight << endl;

    int **arrayGameMap = gameMap->get_array();

//    cout << "탐색해야 하는 위로부터의 거리" << endl;
    for (int i = top; i < top + blockHeight; ++i) {
        if (i > SCREEN_DY - 1) {
            break;
        }
//        cout << i << endl;

        bool isFull = true;
        for (int j = 0; j < ARRAY_DX; ++j) {
//            cout << arrayGameMap[i][j] << " ";
            if (arrayGameMap[i][j] == 0) {
                isFull = false;
            }
        }
//        cout << endl << "isFull? " << isFull << endl;
        if (isFull) {
            Matrix *tempBackground = gameMap->clip(0, 0, i, ARRAY_DX);
//            tempBackground->print();
            gameMap->paste(tempBackground, 1, 0);
            delete tempBackground;
//            gameMap->print();
        }
    }
}

bool checkIsTouchedTop(Matrix *gameMap) {
    int **arrayGameMap = gameMap->get_array();

    bool isTopOccupied = false;
    for (int i = SCREEN_DW; i < SCREEN_DX + SCREEN_DW; ++i) {
//        cout << arrayGameMap[0][i] << " ";
        if (arrayGameMap[0][i] == 1) {
            isTopOccupied = true;
            break;
        }
    }
//    cout << endl;
    if (isTopOccupied) {
        return true;
    }
    else {
        return false;
    }
}

int getSideLength(int arr[]) {
    int i = 0;
    int counter = 0;
    while (arr[i] != -1) {
        counter++;
        i++;
    }
    return sqrt(counter);
}

#define INIT_TOP 0
#define INIT_LEFT 8

int main(int argc, char *argv[]) {
//    Matrix* list[7][4];
//    for (int i = 0; i < 7; ++i) {
//        for (int j = 0; j < 4; ++j) {
//            list[i][j] = new Matrix();
//        }
//    }
//
//    for (int i = 0; i < 7; ++i) {
//        for (int j = 0; j < 4; ++j) {
//            delete list[i][j];
//        }
//    }
//
//    cout << "(nAlloc, nFree, alloc-free) = (" << Matrix::get_nAlloc() << ',' << Matrix::get_nFree() << ","
//         << Matrix::get_nAlloc() - Matrix::get_nFree() << ")" << endl;
//    cout << "Program terminated!" << endl;
//
//    return 0;


    char key;
    int top = INIT_TOP, left = INIT_LEFT;
    int blockType;
    int idxBlockDegree = 0;
    bool newBlockNeeded = false;

    srand((unsigned int) time(NULL));

    Matrix *iScreen = new Matrix((int *) arrayScreen, ARRAY_DY, ARRAY_DX);
    Matrix *setOfBlockObjects[MAX_BLK_TYPES][MAX_BLK_DEGREES];

    for (int i = 0; i < MAX_BLK_TYPES; i++) {
        for (int j = 0; j < MAX_BLK_DEGREES; j++) {
            int sideLength = getSideLength(setOfBlockArrays[i * MAX_BLK_DEGREES + j]);
            setOfBlockObjects[i][j] = new Matrix(setOfBlockArrays[i * MAX_BLK_DEGREES + j], sideLength, sideLength);
        }
    }

//    for (int i = 0; i < MAX_BLK_TYPES; ++i) {
//        for (int j = 0; j < MAX_BLK_DEGREES; ++j) {
//            delete setOfBlockObjects[i][j];
//        }
//    }
//    delete iScreen;
//
//    cout << "(nAlloc, nFree, alloc-free) = (" << Matrix::get_nAlloc() << ',' << Matrix::get_nFree() << ","
//         << Matrix::get_nAlloc() - Matrix::get_nFree() << ")" << endl;
//    cout << "Program terminated!" << endl;
//
//    return 0;


    blockType = rand() % MAX_BLK_TYPES;
    Matrix *currBlk = setOfBlockObjects[blockType][idxBlockDegree];
    Matrix *tempBackground = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
    Matrix *addedBlk = tempBackground->add(currBlk);

    Matrix *oScreen = new Matrix(iScreen);
    oScreen->paste(addedBlk, top, left);

    cout << "(nAlloc, nFree, diff)" << Matrix::get_nAlloc() << " " << Matrix::get_nFree() << " "
         << Matrix::get_nAlloc() - Matrix::get_nFree() << endl;

    drawScreen(oScreen, SCREEN_DW);

    // (게임 루프)
    while ((key = getch()) != 'q') { // 종료 키 q
        // 새로운 블록이 필요하다면,
        if (newBlockNeeded) {
//            cout << "NEW BLOCK NEEDED!!!" << endl;

            delete iScreen;
            iScreen = new Matrix(oScreen);

            deleteFullLines(iScreen, top, left, currBlk->get_dy());
            if(checkIsTouchedTop(iScreen)) {
                cout << "GAME OVER" << endl;
                return 0;
            }

            top = INIT_TOP;
            left = INIT_LEFT;

            blockType = rand() % MAX_BLK_TYPES;
            currBlk = setOfBlockObjects[blockType][idxBlockDegree];

            newBlockNeeded = false;
        }

        // 디버깅용 메모리 추적
//        cout << "(nAlloc, nFree, diff)" << Matrix::get_nAlloc() << " " << Matrix::get_nFree() << " "
//             << Matrix::get_nAlloc() - Matrix::get_nFree() << endl;

        // 입력 처리
        switch (key) {
            case 'a':
                left--;
                break;
            case 'd':
                left++;
                break;
            case 's':
                top++;
                break;
            case 'w':
                break;
            case 'p':
                idxBlockDegree++;
                if (idxBlockDegree > MAX_BLK_DEGREES - 1) {
                    idxBlockDegree = 0;
                }
                currBlk = setOfBlockObjects[blockType][idxBlockDegree];
                break;
            case 'l':
                idxBlockDegree--;
                if (idxBlockDegree < 0) {
                    idxBlockDegree = MAX_BLK_DEGREES - 1;
                }
                currBlk = setOfBlockObjects[blockType][idxBlockDegree];
                break;
            case ' ':
                do {
                    // 내려감
                    top++;

                    // 임시 백그라운드에 현재 블럭을 더해서 addedBlk를 만듬, 더하고 생각하기
                    delete tempBackground;
                    tempBackground = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
                    delete addedBlk;
                    addedBlk = tempBackground->add(currBlk);

                } while (!addedBlk->anyGreaterThan(1)); // 충돌체크
                break;
            default:
                cout << "wrong key input" << endl;
        }

        // 임시 백그라운드에 현재 블럭을 더해서 addedBlk를 만듬, 허락보다 용서가 쉽다
        delete tempBackground;
        tempBackground = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
        delete addedBlk;
        addedBlk = tempBackground->add(currBlk);

        // 충돌처리, 이전으로 돌리고, 사후처리
        if (addedBlk->anyGreaterThan(1)) {
            cout << "충돌발생!!!" << endl;
            cout << "top : " << top << endl;
            cout << "left : " << left << endl;

            switch (key) {
                case 'a':
                    left++;
                    break;
                case 'd':
                    left--;
                    break;
                case 's':
                    top--;
                    newBlockNeeded = true; // 새로운 블록 필요
                    break;
                case 'p':
                    idxBlockDegree--;
                    if (idxBlockDegree < 0) {
                        idxBlockDegree = MAX_BLK_DEGREES - 1;
                    }
                    currBlk = setOfBlockObjects[blockType][idxBlockDegree];
                    break;
                case 'l':
                    idxBlockDegree++;
                    if (idxBlockDegree > MAX_BLK_DEGREES - 1) {
                        idxBlockDegree = 0;
                    }
                    currBlk = setOfBlockObjects[blockType][idxBlockDegree];
                    break;
                case 'w':
                    break;
                case ' ':
                    top--;
                    newBlockNeeded = true; // 새로운 블록 필요
                    break;
            }

            // 사후처리 : 임시 백그라운드에 현재 블럭을 더해서 addedBlk를 만듬
            delete tempBackground;
            tempBackground = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
            delete addedBlk;
            addedBlk = tempBackground->add(currBlk);
        }

        // 화면 그려주기
        delete oScreen;
        oScreen = new Matrix(iScreen);
        oScreen->paste(addedBlk, top, left);
        drawScreen(oScreen, SCREEN_DW);
    }

    delete iScreen;
//    delete currBlk; // 이거 없애면 됨. 왜 그냥 참조를 delete하려 한거야
    delete tempBackground;
    delete addedBlk;
    delete oScreen;


//    for (int i = 0; i < MAX_BLK_TYPES; ++i) {
//        for (int j = 0; j < MAX_BLK_DEGREES; ++j) {
//            cout << "i : " << i << ", j : " << j << endl;
//            setOfBlockObjects[i][j]->print();
//            cout << endl;
//        }
//    }

    for (int i = 0; i < MAX_BLK_TYPES; i++) {
        for (int j = 0; j < MAX_BLK_DEGREES; j++) {
            delete setOfBlockObjects[i][j];
        }
    }


    cout << "(nAlloc, nFree, alloc-free) = (" << Matrix::get_nAlloc() << ',' << Matrix::get_nFree() << ","
         << Matrix::get_nAlloc() - Matrix::get_nFree() << ")" << endl;
    cout << "Program terminated!" << endl;

    return 0;
}

