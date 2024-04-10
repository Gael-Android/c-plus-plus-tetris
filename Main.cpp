#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <termios.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

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

int main(int argc, char *argv[]) {
    char key;
    int top = 0, left = 8;
    bool newBlockNeeded = false;
    srand((unsigned int) time(NULL));

    Matrix *iScreen = new Matrix((int *) arrayScreen, ARRAY_DY, ARRAY_DX);
    Matrix *setOfBlockObjects[7][4];

    for (int i = 0; i < MAX_BLK_TYPES; i++) {
        for (int j = 0; j < MAX_BLK_DEGREES; j++) {
            setOfBlockObjects[i][j] = new Matrix(setOfBlockArrays[i + j], 3, 3);
        }
    }
    Matrix *currBlk = setOfBlockObjects[rand() % MAX_BLK_TYPES][0];
    Matrix *tempBackground = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
    Matrix *addedBlk = tempBackground->add(currBlk);
    delete tempBackground;

    Matrix *oScreen = new Matrix(iScreen);
    oScreen->paste(addedBlock, top, left);
    delete addedBlock;
    drawScreen(oScreen, SCREEN_DW);
    delete oScreen;

//    cout << "(nAlloc, nFree)" << Matrix::get_nAlloc() << " " << Matrix::get_nFree() << endl;

    while ((key = getch()) != 'q') {
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
            case ' ':
                break;
            default:
                cout << "wrong key input" << endl;
        }

        tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
        tempBlk2 = tempBlk->add(currBlk);
        delete tempBlk;
        if (tempBlk2->anyGreaterThan(1)) {
            switch (key) {
                case 'a':
                    left++;
                    break;
                case 'd':
                    left--;
                    break;
                case 's':
                    top--;
                    newBlockNeeded = true;
                    break;
                case 'w':
                    break;
                case ' ':
                    break;
            }
            delete tempBlk2;
            tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
            tempBlk2 = tempBlk->add(currBlk);
            delete tempBlk;
        }

        oScreen = new Matrix(iScreen);
        oScreen->paste(tempBlk2, top, left);
        delete tempBlk2;
        drawScreen(oScreen, SCREEN_DW);
//        delete oScreen;

        if (newBlockNeeded) {
            iScreen = new Matrix(oScreen);
            newBlockNeeded = false;
            top = 0;
            left = 8;
            currBlk = setOfBlockObjects[rand() % MAX_BLK_TYPES][0];

            tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
            tempBlk2 = tempBlk->add(currBlk);
            oScreen = new Matrix(iScreen);
            oScreen->paste(tempBlk2, top, left);
            drawScreen(oScreen, SCREEN_DW);
        }
    }

    delete iScreen;
    delete currBlk;
    delete tempBlk;
    delete tempBlk2;
    delete oScreen;

    cout << "(nAlloc, nFree) = (" << Matrix::get_nAlloc() << ',' << Matrix::get_nFree() << ")" << endl;
    cout << "Program terminated!" << endl;

    return 0;
}

