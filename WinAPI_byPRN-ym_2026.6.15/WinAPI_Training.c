#include "gameplay.h"   // 벡엔드 함수 정의 파일(gameplay.c) 불러오기
#include <stdio.h>
#include <Windows.h>    // 창, 버튼, 마우스, 키보드, 타이머, GDI 그래픽, 파일 대화상자 등


#include <mmsystem.h>       // 효과음 구현
#pragma comment(lib, "winmm.lib")       // 효과음 링커
#pragma comment(lib, "Msimg32.lib")     // 직접 링커 쓰기, TransparentBit 쓰려고 부름

#define TILE_SIZE 32    // 타일 pixel
#define BOARD_X 10      // 실제 게임판 최소 x좌표 (800 중 736이 블록)
#define BOARD_Y 10      // 실제 게임판 최소 y좌표 (600 중 480이 블록, 상단에 )
#define CLIENT_WIDTH 800        // 실제 그림 그리는 영역: 클라이언트
#define CLIENT_HEIGHT 600

// 비트맵(bmp) 로드용
HBITMAP tileBitmap[13];     // 전역변수: 타일 bmp 이미지 담는 배열

// 비트맵 로드
void LoadTileImages(void);

// 리소스 로드 및 해제
void LoadResources(void);
void FreeResources(void);

// 텍스트 입력 창: 커서 처리
bool caretVisible = true;
int cursorPos = 0;      // 현재 커서 위치(텍스트 내에서)
bool isSpaceExists = false;     // 공백 입력시 띄울 텍스트 조건.
bool isNameExists = true;       // 아무 입력 없이 엔터를 할때.

// 게임오버 시 기존 점수 나오는 곳을 없애고 중앙에 나타내기 위한 중간 불
bool isGameover = false;
// 랭크 색상 객체
typedef struct {
    int r;
    int g;
    int b;
} RANKCOLOR;

RANKCOLOR rankColor[3] =
{
    { 255, 215, 0 },
    { 192, 192, 192 },
    {184, 115, 51 }
};

// 랭크 전역변수(카운트, 맥스랭크)
int count;
int maxRank;
playerInfo* ranking;

// 창 크기 설정
RECT rc =
{
    0,
    0,
    CLIENT_WIDTH,
    CLIENT_HEIGHT
};

// 보드 크기 설정
RECT edgeBoard =
{
    BOARD_X,
    BOARD_Y,
    800 - BOARD_X,
    600 - (BOARD_Y * 4)
};

// 타이머 바 설정
RECT timerBar =
{
    BOARD_X + 10,
    BOARD_Y + 10,
    BOARD_X + 10 + 360,      // 360: 타이머 길이
    BOARD_Y + 10 + 20        // 20: 타이머 두께
};

// 볼륨 슬라이더 바 설정
RECT volumeBar;
bool draggingVolume = false;
int volume = 50;

// 더블 버퍼링 위한 memDC(backDC) 만들기
HDC backDC;             // 붓 역할
HDC tileDC;             // 타일에서 쓰일 전역변수 memDC
HBITMAP backBuffer;     // 도화지 역할

// // 1. 게임 상태, 화면
// 게임 초기 화면에 들어갈 게임 상태 만들기
typedef enum {
    STATE_NAMEINPUT,
    STATE_MENU,
    STATE_GAME,
    STATE_HELP,
    STATE_RANK,
    STATE_GAMEOVER
} GameState;

GameState currentState = STATE_NAMEINPUT;

// 전역 폰트 정의
HFONT titleFont;
HFONT menuFont;
HFONT uiFont;
HFONT scoreFont;

// 텍스트 처리용 함수
void DrawText2(HDC hdc, int x, int y, LPCWSTR text, int size, COLORREF color);
void DrawText3(HDC hdc, RECT rect, LPCWSTR text, HFONT font, COLORREF color, UINT format);

// 화면별 함수 정의
void DrawNameInput(HDC hdc);
void DrawReset(HDC hdc);
void DrawMenu(HDC hdc);
void DrawTile(HDC hdc, int x, int y, int id);
void DrawBoard(HDC hdc);
void DrawGame(HDC hdc);
void DrawHelp(HDC hdc);
void DrawRank(HDC hdc);
void DrawGameOver(HDC hdc);


// // 2. 버튼
// 버튼 영역 정의
RECT confirmButton;     // 이름 작성시 확인 버튼
RECT startButton;
RECT helpButton;
RECT rankButton;
RECT exitButton;
RECT resetButton;
RECT backButton;        // 얘는 중복해서 쓰일 것.
RECT menuButton;
RECT rank2Button;       // 게임오버에 뜨는 버튼


// 버튼 크기 정의 함수
void LoadButtons(void);

// 버튼 그리기 함수
void DrawButton(
    HDC hdc,
    RECT rect,
    LPCWSTR text,
    bool isHovered);

// 현재 마우스 상태 저장용(호버링)
typedef enum {
    BUTTON_NONE,
    BUTTON_CONFIRM,
    BUTTON_START,
    BUTTON_HELP,
    BUTTON_RANK,
    BUTTON_EXIT,
    BUTTON_RANK2,
    BUTTON_RESET,
    BUTTON_BACK,
    BUTTON_MENU
} HoverButton;

HoverButton hoverButton = BUTTON_NONE;
HoverButton oldHover = BUTTON_NONE;

// // 3. 게임에 쓰일 부자재
// 타이머 바
void DrawTimerBar(HDC hdc);

// 볼륨 슬라이더
void DrawVolumeSlider(HDC hdc);


// WndProc 함수 기본 설정
LRESULT CALLBACK WndProc(   // Window Procedure: 창에서 일어나는 사건 처리 함수
    HWND hwnd,              // Handle to WiNDow: 창의 ID
    UINT msg,               // msg: 메시지 식별자
    WPARAM wParam,          // wParam: 추가 메시지 정보
    LPARAM lParam)          // IParam: 추가 메시지 정보
{
    switch (msg)
    {
        case WM_CREATE:
        {
            // backDC, backBuffer 설정
            HDC hdc = GetDC(hwnd);      // 의미: 이 창에 그림 그릴 수 있는 HDC 하나 빌리기

            backDC = CreateCompatibleDC(hdc);
            tileDC = CreateCompatibleDC(hdc);           // **DrawTile -> WM_CREATE로 옮김.

            backBuffer = CreateCompatibleBitmap(        // 이거 하려고 HDC 미리 빌렸음.
                hdc,
                CLIENT_WIDTH,
                CLIENT_HEIGHT);

            SelectObject(
                backDC,
                backBuffer);
            
            ReleaseDC(hwnd, hdc);       // 의미: 빌린 HDC 반납하기

            LoadResources();            // **많은 리소스들 렌더링 함수로 만들어 간소화함.

            MCIERROR err = mciSendString(              // bgm 파일 불러오기: media coontrol interface
                L"open x64\\Debug\\sounds\\bgm.mp3 type mpegvideo alias bgm",       // command: bgm.mp3 파일 열기. 이 파일 별명 bgm으로 함.
                NULL,       // returnString
                0,          // returnLength
                NULL);      // callback

            if (err != 0)
            {
                wchar_t buffer[256];

                mciGetErrorString(
                    err,
                    buffer,
                    256);

                MessageBox(
                    NULL,
                    buffer,
                    L"MCI OPEN Error",
                    MB_OK);
            }

            // 깜빡이는 세로선 커서용 타이머
            SetTimer(
                hwnd,
                2,          // 커서용 타이머
                500,        // 0.5초마다.
                NULL);

            // 디렉토리 경로 확인용
            /*
            wchar_t path[MAX_PATH];

            GetCurrentDirectory(
                MAX_PATH,
                path);

            MessageBox(
                hwnd,
                path,
                L"Current Dir",
                MB_OK);
                */
            return 0;
        }
        case WM_MOUSEMOVE:  // 마우스가 움직이면 현재 위치한 곳(버튼, 볼륨슬라이드)이 어딘지 파악
        {
            POINT pt;

            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);

            oldHover = hoverButton;     // 검사용

            switch (currentState)
            {
            case STATE_MENU:        // 메뉴 화면에서의 케이스
                if (PtInRect(&startButton, pt))
                    hoverButton = BUTTON_START;
                else if (PtInRect(&helpButton, pt))
                    hoverButton = BUTTON_HELP;
                else if (PtInRect(&rankButton, pt))
                    hoverButton = BUTTON_RANK;
                else if (PtInRect(&exitButton, pt))
                    hoverButton = BUTTON_EXIT;
                else
                    hoverButton = BUTTON_NONE;
                break;

            case STATE_GAME:        // 게임 화면에서의 케이스
                if (PtInRect(&resetButton, pt))     // reset 버튼 호버링
                    hoverButton = BUTTON_RESET;
                if (draggingVolume)
                {
                    volume =
                        (pt.x - volumeBar.left)
                        * 100           // volume값은 100까지 있음.
                        / (volumeBar.right - volumeBar.left);   // 실제 슬라이더 크기로 나누기.

                    if (volume < 0)     // 최소 0으로 맞추기.
                        volume = 0;

                    if (volume > 100)   // 최대 100으로 맞추기.
                        volume = 100;

                    DWORD vol =
                        (volume * 65535) / 100;

                    waveOutSetVolume(
                        NULL,       // device: NULL=기본장치
                        MAKELONG(vol, vol));        // 왼쪽, 오른쪽 음향 크기

                    
                    InvalidateRect(
                        hwnd,
                        NULL,
                        FALSE);

                    return 0;
                }
                else
                    hoverButton = BUTTON_NONE;
                break;

            case STATE_HELP:        // 도움말 화면에서의 케이스
                if (PtInRect(&backButton, pt))      // back 버튼 호버링
                    hoverButton = BUTTON_BACK;
                else
                    hoverButton = BUTTON_NONE;
                break;

            case STATE_RANK:        // 랭크 화면에서의 케이스
                if (PtInRect(&backButton, pt))      // back 버튼 호버링
                    hoverButton = BUTTON_BACK;
                else
                    hoverButton = BUTTON_NONE;
                break;

            case STATE_GAMEOVER:    // 게임오버 화면에서의 케이스
                if (PtInRect(&menuButton, pt))      // menu 버튼 호버링
                    hoverButton = BUTTON_MENU;
                else if (PtInRect(&rank2Button, pt))
                    hoverButton = BUTTON_RANK2;
                else
                    hoverButton = BUTTON_NONE;
                break;
            }

            if (oldHover != hoverButton)        // **hoverButton이 갱신되었을 때만 다시 렌더
            {
                InvalidateRect(
                    hwnd,
                    NULL,
                    FALSE);
            }

            return 0;
        }
        case WM_ERASEBKGND:     // **깜빡임 한 번 더 줄어듦 (더블 버퍼링)
            return 1;
        case WM_PAINT:      // 창 그리기
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);        // 화면에 그림 그리는 도구

            if (currentState != STATE_GAMEOVER)
                DrawReset(backDC);         // 게임오버가 아닌 경우에만 백버퍼 초기화

            switch (currentState)
            {
            case STATE_NAMEINPUT:   // 닉네임 기입 창 그리기
                DrawNameInput(backDC);
                if (isSpaceExists)      // 공백 입력시 경고문고 띄우기
                {
                    RECT alertRect =
                    {
                        100,
                        200,
                        700,
                        250
                    };

                    DrawText3(
                        backDC,
                        alertRect,
                        L"공백을 제외한 문자를 기입하세요",
                        uiFont,
                        RGB(200, 20, 20),
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }
                if (!isNameExists)      // 이름 입력 x, 엔터 시 경고문고 띄우기
                {
                    RECT alertRect =
                    {
                        100,
                        180,
                        700,
                        200
                    };

                    DrawText3(
                        backDC,
                        alertRect,
                        L"이름을 입력하세요!",
                        uiFont,
                        RGB(200, 20, 20),
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }
                break;

            case STATE_MENU:        // 메뉴 창 그리기
                DrawMenu(backDC);
                break;

            case STATE_GAME:        // 게임 창 그리기
                DrawGame(backDC);
                break;

            case STATE_HELP:        // 도움말 창 그리기
                DrawHelp(backDC);
                break;

            case STATE_RANK:        // 랭크 창 그리기
                DrawRank(backDC);
                break;

            case STATE_GAMEOVER:    // 게임오버 창 그리기
                DrawGameOver(backDC);
                break;
            }

            BitBlt(                 // 실제로 창에 그리는 부분(딱 한 번만, 더블 버퍼링의 핵심)
                hdc,
                0,
                0,
                800,
                600,

                backDC,
                0,
                0,

                SRCCOPY);

            EndPaint(hwnd, &ps);    // 그림 끝

            if (isGameover)     // 게임오버 중간변수 활용
            {
                currentState = STATE_GAMEOVER;
                InvalidateRect(hwnd, NULL, FALSE);
                isGameover = false;
            }

            return 0;
        }
        case WM_LBUTTONDOWN:    // 왼쪽 마우스 클릭 이벤트 (버튼, 볼륨 슬라이드, 블록)
        {
            int mouseX = LOWORD(lParam);    // 마우스 x
            int mouseY = HIWORD(lParam);    // 마우스 y

            POINT pt;

            pt.x = mouseX;
            pt.y = mouseY;

            // 마우스 좌표 -> 실제 보드 인덱스로 변환
            int boardX =
                (mouseX - BOARD_X - 22)
                / TILE_SIZE;

            int boardY =
                (mouseY - BOARD_Y - 22 - TILE_SIZE)
                / TILE_SIZE;

            if ((mouseX - BOARD_X - 22) < 0)        // **정수 나눗셈에서는 실제 -1~0값이 0으로 나오기에 그 경우 제외함.
                boardX = -1;

            if ((mouseY - BOARD_Y - 22 - TILE_SIZE) < 0)
                boardY = -1;

            switch (currentState)
            {
            case STATE_MENU:        // 메뉴 화면: 버튼 4개
                if (PtInRect(&startButton, pt)) {
                    currentState = STATE_GAME;
                    hoverButton = BUTTON_NONE;      // **버튼 호버링 초기화(버그 픽스)
                    
                    gameTime = 1200;    // 게임 시간 초기화.
                    score = 0;          // 점수 초기화.

                    SetTimer(           // 게임이 시작되면 바로 타이머 시작.
                        hwnd,
                        1,          // 타이머 ID
                        100,       // 0.1초 단위로 줄어듦
                        NULL);

                    PlaySound(      // 버튼 클릭시 효과음.
                        L"x64\\Debug\\sounds\\click.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);

                    mciSendString(      // 처음으로 돌리기.
                        L"seek bgm to start",
                        NULL,
                        0,
                        NULL);

                    mciSendString(      // 게임 시작 시 반복재생 시작
                        L"play bgm repeat",
                        NULL,
                        0,
                        NULL);

                    // 게임 시작 타일 초기화
                    srand((unsigned int)time(NULL));
                    initBoard();

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                if (PtInRect(&exitButton, pt)) {
                    PlaySound(      // 버튼 클릭시 효과음.
                        L"x64\\Debug\\sounds\\click.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);

                    PostMessage(
                        hwnd,
                        WM_CLOSE,
                        0,
                        0);
                }
                if (PtInRect(&helpButton, pt)) {
                    currentState = STATE_HELP;
                    hoverButton = BUTTON_NONE;

                    PlaySound(      // 버튼 클릭시 효과음.
                        L"x64\\Debug\\sounds\\click.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                if (PtInRect(&rankButton, pt)) {
                    currentState = STATE_RANK;
                    hoverButton = BUTTON_NONE;

                    ranking =           // 전역변수 사용해서 랭크 진입시에만 파일 불러오기.
                        showRanking(&count);

                    maxRank =
                        count < 3 ? count : 3;  // 3명 이하만 출력


                    PlaySound(      // 버튼 클릭시 효과음.
                        L"x64\\Debug\\sounds\\click.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                break;

            case STATE_GAME:        // 게임 화면: 버튼 1개, 슬라이드 1개, 블록 제외 공간
                if (PtInRect(&resetButton, pt)) {
                    currentState = STATE_MENU;
                    hoverButton = BUTTON_NONE;

                    PlaySound(      // 버튼 클릭시 효과음.
                        L"x64\\Debug\\sounds\\click.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);

                    mciSendString(      // 리셋버튼 눌렀을때 재생 멈춤
                        L"stop bgm",
                        NULL,
                        0,
                        NULL);

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                // 볼륨 슬라이드 기입해야 함.
                if (PtInRect(&volumeBar, pt)) {     // 볼륨 슬라이드를 클릭한 상태.
                    draggingVolume = true;          // 이걸 WM_LBUTTONUP에서 쓰일거임.

                    PlaySound(      // 버튼 클릭시 효과음.
                        L"x64\\Debug\\sounds\\click.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);
                }
                // 게임 기능: 타일 클릭 기능
                if (            // 보드 23x15 내에서 클릭했을때.
                    boardX >= 0 &&
                    boardX < MAP_WIDTH &&
                    boardY >= 0 &&
                    boardY < MAP_HEIGHT
                    )
                {
                    if (!board[boardY][boardX].isExists)        // 블록이 없는 곳 클릭했을 때.
                    {
                        int isColorMatched = searchAndMatch(    // 판별해서 효과음 재생
                            boardX,
                            boardY);

                        if (isColorMatched)     // 0 아님 1 이상
                        {
                            PlaySound(
                                L"x64\\Debug\\sounds\\match.wav",
                                NULL,       // 리소스에 포함된 사운드 재생시 사용하는 모듈 핸들(일반적 사용 x)
                                SND_FILENAME | SND_ASYNC);      // 사운드 재생 방식, 옵션 Flag: SND_FILENAME=파일이름 인식, SND_ASYNC: 다음줄 코드 곧바로 실행(비동기)
                        }
                        else
                        {
                            PlaySound(
                                L"x64\\Debug\\sounds\\miss.wav",
                                NULL,
                                SND_FILENAME | SND_ASYNC);
                        }

                        InvalidateRect(
                            hwnd,
                            NULL,
                            FALSE);
                    }
                }

                break;

            case STATE_HELP:        // 도움말 화면: 버튼 1개
                if (PtInRect(&backButton, pt)) {
                    currentState = STATE_MENU;
                    hoverButton = BUTTON_NONE;

                    PlaySound(      // 버튼 클릭시 효과음.
                        L"x64\\Debug\\sounds\\click.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                break;

            case STATE_RANK:        // 랭크 화면: 버튼 1개
                if (PtInRect(&backButton, pt)) {
                    currentState = STATE_MENU;
                    hoverButton = BUTTON_NONE;

                    PlaySound(      // 버튼 클릭시 효과음.
                        L"x64\\Debug\\sounds\\click.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                break;

            case STATE_GAMEOVER:    // 게임오버 화면: 버튼 2개
                if (PtInRect(&menuButton, pt)) {
                    currentState = STATE_MENU;
                    hoverButton = BUTTON_NONE;

                    PlaySound(      // 버튼 클릭시 효과음.
                        L"x64\\Debug\\sounds\\click.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                if (PtInRect(&rank2Button, pt)) {
                    currentState = STATE_RANK;
                    hoverButton = BUTTON_NONE;

                    ranking =           // 전역변수 사용해서 랭크 진입시에만 파일 불러오기.
                        showRanking(&count);

                    maxRank =
                        count < 3 ? count : 3;  // 3명 이하만 출력


                    PlaySound(      // 버튼 클릭시 효과음.
                        L"x64\\Debug\\sounds\\click.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                break;
            }
            // 실제 그림 영역 확인용
            /*
            RECT rc;

            GetClientRect(
                hwnd,
                &rc);

            wchar_t buffer[100];

            wsprintf(
                buffer,
                L"%d x %d",
                rc.right,
                rc.bottom);

            MessageBox(
                hwnd,
                buffer,
                L"Client Size",
                MB_OK);
                */

            return 0;
        }
            // searchAndMatch();    만약 매치 성공이면 아래 메시지 전달


            /*
            InvalidateRect(
                hwnd,
                NULL,
                TRUE);

            wchar_t buffer[100];

            wsprintf(
                buffer,
                L"X = %d\nY = %d",
                mouseX,
                mouseY);

            MessageBox(             // 메시지 창 띄우기
                hwnd,
                buffer,
                L"Mouse Click",
                MB_OK);

            return 0;
        } */
        case WM_LBUTTONUP:          // 볼륨 슬라이더의 경우 드래그를 체크해야 함.
        {
            draggingVolume = false; // 만약 떼면 드래깅 거짓.

            return 0;
        }
        case WM_TIMER:              // 타이머 설정.
        {
            if (wParam == 2)        // 커서 타이머
            {
                caretVisible = !caretVisible;

                if (currentState == STATE_NAMEINPUT) {
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            if (currentState == STATE_GAME)
            {
                gameTime--;

                if (gameTime <= 0)
                {
                    gameTime = 0;       // 최소를 0으로 맞추기
                    KillTimer(hwnd, 1);

                    // 게임화면 재 표시 위한 중간 변수 이용(게임오버)
                    isGameover = true;

                    saveRanking(playerName, score);


                    mciSendString(      // 게임오버 시 재생 멈춤
                        L"stop bgm",
                        NULL,
                        0,
                        NULL);

                    PlaySound(          // 게임오버 효과음 재생
                        L"x64\\Debug\\sounds\\gameover.wav",
                        NULL,
                        SND_FILENAME | SND_ASYNC);
                }

                InvalidateRect(
                    hwnd,
                    NULL,
                    FALSE);
            }

            return 0;
        }
        case WM_CHAR:
        {
            if (currentState != STATE_NAMEINPUT)    // 닉네임 기입 상태일때만 입력받음.
                break;
            
            wchar_t ch = (wchar_t)wParam;

            if (ch == L' ')
            {
                isSpaceExists = true;       // 경고 문구 띄우러 가자~
                
                InvalidateRect(
                    hwnd,
                    NULL,
                    FALSE);

                return 0;       // 공백 입력 금지.
            }
            

            if (ch == 13)   // Enter
            {
                if (nameLength == 0)
                {
                    isNameExists = false;

                    InvalidateRect(
                        hwnd,
                        NULL,
                        FALSE);

                    return 0;
                }
                currentState = STATE_MENU;

                InvalidateRect(hwnd, NULL, TRUE);

                return 0;
            }

            else if (ch == 8)    // Backspace
            {
                if (nameLength > 0)
                {
                    nameLength--;

                    playerName[nameLength] = L'\0';
                }
            }
            else
            {
                if (nameLength < 31)
                {
                    playerName[nameLength] = ch;
                    nameLength++;

                    playerName[nameLength] = L'\0';

                    isSpaceExists = false;
                }
            }

            InvalidateRect(hwnd, NULL, FALSE);

            return 0;
        }
        case WM_DESTROY:            // 창 닫기 버튼 눌림
        {
            DeleteObject(backBuffer);       // 전역변수로 쓰인 bitmap 삭제
            DeleteDC(backDC);               // 전역변수로 쓰인 backDC 삭제
            DeleteDC(tileDC);               // 전역변수로 쓰인 tileDC 삭제

            free(ranking);

            FreeResources();

            mciSendString(                  // 배경음 닫기.
                L"close bgm",
                NULL,
                0,
                NULL);

            PostQuitMessage(0);             // 게임 종료 명령
            return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);    // 내가 처리 안 한 이벤트는 Windows 기본 처리에 맡긴다.
}


// 실제 프로그램 시작
int WINAPI wWinMain(            // 윈도우 프로그램의 main(void) 구조. 기본 설정: 콘솔 말고 창으로 바꿔야 함.
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow)
{
    // 기본 설정
    const wchar_t CLASS_NAME[] = L"ColorTilesWindow";

    WNDCLASSW wc = { 0 };       // 창 설계도, 아직 창 아님

    wc.lpfnWndProc = WndProc;   // 이 창에서 이벤트 발생 시 WndProc 함수 호출.
    wc.hInstance = hInstance;   // 이 프로그램 소속
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);        // 설계도를 Windows에 등록하는 작업.

    // 창 생성 전 클라이언트 크기 조정
    AdjustWindowRect(
        &rc,
        WS_OVERLAPPED |
        WS_CAPTION |
        WS_SYSMENU |
        WS_MINIMIZEBOX,
        FALSE);

    HWND hwnd = CreateWindowEx(     // 실제 창 만드는 것
        0,                          // 확장 스타일: 현재 없음으로 되어 있음
        CLASS_NAME,                 // 어떤 설계도로 만들지
        L"Color Tiles",             // 창 제목
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,        // 최소화, 닫기 버튼만 있는 고정 창 스타일
        CW_USEDEFAULT,  // X 위치
        CW_USEDEFAULT,  // Y 위치
        rc.right - rc.left,            // width
        rc.bottom - rc.top,            // height
        NULL,   // 부모 창 핸들
        NULL,   // 메뉴 핸들
        hInstance,  // 인스턴스 핸들
        NULL);  // 추가 매개변수

    if (hwnd == NULL)
        return 0;

    ShowWindow(hwnd, nCmdShow);     // 창 화면에 보여주는 역할

    MSG msg;            // 이벤트 저장 상자

    while (GetMessage(&msg, NULL, 0, 0))        // 이벤트가 올때까지 기다림
    {
        TranslateMessage(&msg);             // 키보드 관련 메시지 변환(관례적임)
        DispatchMessage(&msg);              // 메시지를 WndProc로 전달
    }

    return 0;
}


// 창 벡엔드

// 닉네임 입력 창
void DrawNameInput(HDC hdc)
{
    RECT titleRect =
    {
        100,
        100,
        700,
        180
    };

    DrawText3(
        hdc,
        titleRect,
        L"플레이어 이름을 입력하세요",
        menuFont,
        RGB(0, 0, 0),
        DT_CENTER | DT_VCENTER);

    RECT inputRect =
    {
        250,
        250,
        550,
        300
    };

    Rectangle(
        hdc,
        inputRect.left,
        inputRect.top,
        inputRect.right,
        inputRect.bottom);

    DrawText3(
        hdc,
        inputRect,
        playerName,
        uiFont,
        RGB(0, 0, 0),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT infoRect =
    {
        150,
        350,
        650,
        400
    };

    DrawText3(
        hdc,
        infoRect,
        L"Enter : 확인 / Backspace : 삭제\n최대 32자까지 적을 수 있습니다.",
        uiFont,
        RGB(120, 120, 120),
        DT_CENTER | DT_WORDBREAK);
}

// 창 그리기 함수
void DrawReset(HDC hdc)     // 창 초기화
{
    Rectangle(
        hdc,
        -1,
        -1,
        CLIENT_WIDTH + 1,
        CLIENT_HEIGHT + 1);
}
// 일반 텍스트 기입용
void DrawText2(HDC hdc, 
    int x, 
    int y, 
    LPCWSTR text, 
    int size, 
    COLORREF color)        
{           // 특정 위치에서 특정 색상, 특정 폰트로 생성할 text를 간단히 쓰기 위해 씀
    HFONT font =
        CreateFont(
            size,       // 글자 높이: 20=작음, 30=보통, 50=큼, 80=타이틀
            0,
            0,
            0,
            FW_BOLD,    // 굵기: FW_NORMAL, FW_BOLD, FW_HEAVY
            FALSE,      // italic 여부
            FALSE,      // 밑줄 여부
            FALSE,      // 취소선 여부
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH,
            L"맑은 고딕");      // 폰트 이름.

    HFONT oldFont =
        (HFONT)SelectObject(        // 기존 연결되어 있던 객체(폰트)를 반환하기에 oldFont 저장 가능.
            hdc,
            font);

    COLORREF oldColor =
        SetTextColor(
        hdc,
        color);

    int oldBkMode =
        SetBkMode(
        hdc,
        TRANSPARENT);

    TextOut(
        hdc,
        x,
        y,
        text,
        lstrlen(text));

    SetBkMode(
        hdc,
        oldBkMode);

    SetTextColor(
        hdc,
        oldColor);

    SelectObject(               // 기존 폰트로 귀환.
        hdc,
        oldFont);
}

// 메인 텍스트 기입용
void DrawText3(HDC hdc, 
    RECT rect, 
    LPCWSTR text, 
    HFONT font, 
    COLORREF color, 
    UINT format)
{
    HFONT oldFont =
        (HFONT)SelectObject(
            hdc,
            font);

    COLORREF oldColor =
        SetTextColor(
            hdc,
            color);

    int oldBkMode =
        SetBkMode(
            hdc,
            TRANSPARENT);

    DrawText(
        hdc,
        text,
        -1,
        &rect,
        format);

    SetBkMode(
        hdc,
        oldBkMode);

    SetTextColor(
        hdc,
        oldColor);

    SelectObject(               // 기존 폰트로 귀환.
        hdc,
        oldFont);
}

void DrawMenu(HDC hdc)
{
    RECT rect =                 // 메인 타이틀 사이즈
    {
        220,
        50,
        580,
        200
    };

    DrawText3(
        hdc,
        rect,
        L"Color Tiles",
        titleFont,
        RGB(234, 63, 86),       // Apple Music 색상
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DrawButton(hdc, startButton, L"게임 시작", hoverButton == BUTTON_START);
    DrawButton(hdc, helpButton, L"설명", hoverButton == BUTTON_HELP);
    DrawButton(hdc, rankButton, L"랭킹", hoverButton == BUTTON_RANK);
    DrawButton(hdc, exitButton, L"종료", hoverButton == BUTTON_EXIT);
}

void DrawTile(HDC hdc, int x, int y, int id)
{
    // tileDC = CreateCompatibleDC(hdc);       // **최적화를 위해 WM_CREATE에 옮김.

    SelectObject(tileDC, tileBitmap[id]);

    TransparentBlt(     // bmp 원본 그림의 (0,0)~(32,32)를 창의 (100,100)에 복사하기
        hdc,        // 목적지DC
        x,        // 목적지X
        y,        // 목적지Y
        TILE_SIZE,         // 가로
        TILE_SIZE,         // 세로
        tileDC,      // 원본DC에 저장.
        0,          // 원본에서의 X
        0,          // 원본에서의 Y
        TILE_SIZE,  // 원본 가로
        TILE_SIZE,  // 원본 세로
        RGB(255,255,255));   // 투명으로 바꿀 색

    // DeleteDC(tileDC);         // **최적화를 위해 WM_DESTROY로 옮김.
}

void DrawBoard(HDC hdc)
{
    for (int y = 0; y < MAP_HEIGHT + 2 + 1; y++)
    {
        for (int x = 0; x < MAP_WIDTH + 1 + 1; x++)
        {
            if (y % 2 == 0)
            {
                if (x % 2 == 0) 
                    DrawTile(hdc, x * TILE_SIZE, y * TILE_SIZE, 12);     // id 12: 블랙 타일 바닥(보드)
                else
                    DrawTile(hdc, x * TILE_SIZE, y * TILE_SIZE, 11);     // id 11: 화이트 타일 바닥(보드)
            }
            else
            {
                if (x % 2 != 0)
                    DrawTile(hdc, x * TILE_SIZE, y * TILE_SIZE, 12);     // id 12: 블랙 타일 바닥(보드)
                else
                    DrawTile(hdc, x * TILE_SIZE, y * TILE_SIZE, 11);     // id 11: 화이트 타일 바닥(보드)
            }
        }
    }
}

// 타이머 바
void DrawTimerBar(
    HDC hdc)
{
    Rectangle(
        hdc,
        timerBar.left,
        timerBar.top,
        timerBar.right,
        timerBar.bottom);

    int width =
        timerBar.right -
        timerBar.left;

    int fillWidth =     // 현재 남은 시간 기준 채울 박스 크기.
        width *
        gameTime /
        1200;
    if (fillWidth <= 0)         // **만약 잘못 클릭해서 -10초인 경우 마이너스 시간이 생길 수 있어 배제.
        fillWidth = 0;      
    HBRUSH brush =
        CreateSolidBrush(
            RGB(234, 63, 86));

    RECT fillRect =
    {
        timerBar.left,
        timerBar.top,
        timerBar.left + fillWidth,
        timerBar.bottom
    };

    FillRect(
        hdc,
        &fillRect,
        brush);

    DeleteObject(
        brush);
}

// 볼륨 슬라이더
void DrawVolumeSlider(HDC hdc)
{
    // 선
    HPEN pen = CreatePen(
        PS_SOLID,       // 스타일: PS_SOLID=실선, PS_DASH=점선, PS_DOT=점
        4,              // 두께
        RGB(100, 100, 100));

    HPEN oldPen =
        SelectObject(hdc, pen);

    MoveToEx(       // 현재 펜 위치만 이동
        hdc,
        volumeBar.left,
        (volumeBar.top + volumeBar.bottom) / 2,
        NULL);      // 이전 위치를 받을 포인터.

    LineTo(         // 선 그리기(목적지)
        hdc,
        volumeBar.right,
        (volumeBar.top + volumeBar.bottom) / 2);

    SelectObject(
        hdc,
        oldPen);

    DeleteObject(pen);

    // 손잡이 위치
    int knobX =
        volumeBar.left +
        volume *
        (volumeBar.right - volumeBar.left)
        / 100;

    Ellipse(        // 원 또는 타원 그리기
        hdc,
        knobX - 8,
        volumeBar.top - 2,
        knobX + 8,
        volumeBar.bottom + 2);
}

void DrawGame(HDC hdc)
{
    DrawBoard(hdc);     // 보드 생성

    // 클리핑하기
    SaveDC(hdc);        // 현 상태 저장

    ExcludeClipRect(    // 클리핑 설정
        hdc,
        edgeBoard.left,
        edgeBoard.top,
        edgeBoard.right,
        edgeBoard.bottom);

    DrawReset(hdc);     // 바깥쪽만 초기화: 가장자리 만들기

    RestoreDC(          // 다시 불러오기
        hdc,
        -1);

    FrameRect(      // 사각형 테두리만 그림.
        hdc,
        &edgeBoard,
        (HBRUSH)GetStockObject(BLACK_BRUSH));   // 윈도우 기본 제공 검은 붓

    // reset 버튼
    DrawButton(hdc, resetButton, L"reset", hoverButton == BUTTON_RESET);

    // 남은 시간 바
    DrawTimerBar(hdc);

    if (!isGameover)        // 중간 변수로 게임오버 될때 점수 칸만 지운다.
    {
        // 점수 텍스트
        RECT scoreRect =        // 텍스트용 박스
        {
            BOARD_X + 720,
            BOARD_Y + 10,
            BOARD_X + 720 + 50,
            BOARD_Y + 10 + 32
        };

        wchar_t buffer[100];    // wchar_t: 한글, 일어, 한자 등 최소 2바이트짜리도 쓸 수 있는 것.

        wsprintf(       // buffer에 문장 넣기.(조립)
            buffer,
            L"%d",
            score);

        DrawText3(
            hdc,
            scoreRect,
            buffer,
            scoreFont,
            RGB(234, 63, 86),
            DT_CENTER | DT_SINGLELINE);
    }

    // 볼륨 슬라이더
    DrawVolumeSlider(hdc);

    // 그 옆에 현재 볼륨 % 보이기
    wchar_t buffer1[100];

    wsprintf(
        buffer1,
        L"%d%%",
        volume);

    RECT volumePercentRect =
    {
        510,
        570,
        560,
        590
    };

    DrawText3(
        hdc,
        volumePercentRect,
        buffer1,
        uiFont,
        RGB(100, 100, 100),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    

    // 타일 생성
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            if (board[y][x].isExists)
            {
                /*
                DrawTile(           // 그림자 먼저 생성 -> 그림자랑 타일 겹치니까 구려서 뺐음.
                    hdc,
                    BOARD_X + 22 + x * TILE_SIZE + 3,
                    BOARD_Y + 22 + TILE_SIZE + y * TILE_SIZE + 3,
                    10);
                */
                DrawTile(           // 그 위에 타일 생성
                    hdc,
                    BOARD_X + 22 + x * TILE_SIZE,
                    BOARD_Y + 22 + TILE_SIZE + y * TILE_SIZE,
                    board[y][x].color);
            }
        }
    }
    /*
            HDC memDC = CreateCompatibleDC(hdc);

            SelectObject(memDC, tileBitmap[0]);

            BitBlt(     // bmp 원본 그림의 (0,0)~(32,32)를 창의 (100,100)에 복사하기
                hdc,        // 목적지DC
                100,        // 목적지X
                100,        // 목적지Y
                32,         // 가로
                32,         // 세로
                memDC,      // 원본DC
                0,          // 원본X
                0,          // 원본Y
                SRCCOPY);   // 카피 명령

            DeleteDC(memDC);
     */
}

void DrawHelp(HDC hdc) 
{
    RECT rect =     // 텍스트 쓸 박스
    {
        50,
        50,
        750,
        550
    };

    wchar_t text[2000] =     // reQuest: 여기에 설명 적으시면 됩니다. ㅇㅇ
        L"빈칸 누르기: \n보드판에서 비어 있는 칸을 하나 고릅니다.\n\n"
        L"똑같은 색 찾기 : \n내가 누른 칸을 기준으로 가로(왼쪽·오른쪽)나 세로(위·아래) 방향으로 가장 가까이 있는 타일들을 봅니다.\n\n"
        L"타일 가져오기 : \n이때 서로 색깔이 똑같은 타일들이 있다면, 그 타일들을 내가 가질 수 있습니다.\n\n"
        L"점수 얻기 : \n가져온 타일 1개당 1점을 받습니다.\n\n"
        L"시간제한: \n게임 시간은 총 120초(2분)입니다.\n\n"
        L"주의할 점 : \n만약 색이 맞지 않는 빈칸을 잘못 누르면, 남은 시간이 10초씩 줄어듭니다.";

    DrawText3(
        hdc,
        rect,
        text,
        uiFont,
        RGB(0, 0, 0),
        DT_LEFT | DT_WORDBREAK);

    DrawButton(hdc, backButton, L"뒤로가기", hoverButton == BUTTON_BACK);
}

void DrawRank(HDC hdc) 
{
    if (maxRank == 0)
    {
        RECT notRankRect =
        {
            100,
            200,
            700,
            400
        };

        DrawText3(
            hdc,
            notRankRect,
            L"아직 플레이 기록이 없습니다!",
            menuFont,
            RGB(100, 100, 100),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    else
    {
        RECT titleRect =
        {
            100,
            25,
            700,
            125
        };

        DrawText3(
            hdc,
            titleRect,
            L"로컬 랭킹",
            titleFont,
            RGB(234, 63, 86),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        RECT helpRect =
        {
            100,
            400,
            700,
            500
        };

        DrawText3(
            hdc,
            helpRect,
            L"랭킹은 3위까지만 표시됩니다",
            uiFont,
            RGB(100, 100, 100),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    for (int i = 0; i < maxRank; i++)
    {
        RECT rankRect =
        {
            100,
            100 + i * 100,
            700,
            200 + i * 100
        };

        wchar_t buffer[100];
        
        wsprintf(
            buffer,
            L"%d위 : %ls (%d점)",
            i + 1,
            ranking[i].name,
            ranking[i].score);

        DrawText3(
            hdc,
            rankRect,
            buffer,
            menuFont,
            RGB(rankColor[i].r, rankColor[i].g, rankColor[i].b),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    DrawButton(hdc, backButton, L"뒤로가기", hoverButton == BUTTON_BACK);
}

void DrawGameOver(HDC hdc) 
{
    wchar_t buffer[100];    // wchar_t: 한글, 일어, 한자 등 최소 2바이트짜리도 쓸 수 있는 것.

    wsprintf(       // buffer에 문장 넣기.(조립)
        buffer,
        L"%d",
        score);

    RECT textRect =
    {
        300,
        125,
        500,
        255
    };

    DrawText3(
        hdc,
        textRect,
        L"Score",
        titleFont,
        RGB(234, 63, 86),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT scoreRect =
    {
        300,
        230,
        500,
        330
    };

    DrawText3(
        hdc,
        scoreRect,
        buffer,
        titleFont,
        RGB(234, 63, 86),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DrawButton(hdc, menuButton, L"메뉴", hoverButton == BUTTON_MENU);
    DrawButton(hdc, rank2Button, L"로컬 랭킹", hoverButton == BUTTON_RANK2);
}

// 버튼 크기 로드
void LoadButtons(void) {
    startButton.left = 300;
    startButton.top = 220;
    startButton.right = 500;
    startButton.bottom = 270;

    helpButton.left = 300;
    helpButton.top = 290;
    helpButton.right = 500;
    helpButton.bottom = 340;

    rankButton.left = 300;
    rankButton.top = 360;
    rankButton.right = 500;
    rankButton.bottom = 410;

    exitButton.left = 300;
    exitButton.top = 430;
    exitButton.right = 500;
    exitButton.bottom = 480;

    resetButton.left = 400;
    resetButton.top = 570;
    resetButton.right = 500;
    resetButton.bottom = 590;

    backButton.left = 650;
    backButton.top = 500;
    backButton.right = 750;
    backButton.bottom = 550;

    menuButton.left = 350;
    menuButton.top = 370;
    menuButton.right = 450;
    menuButton.bottom = 420;

    rank2Button.left = 350;
    rank2Button.top = 450;
    rank2Button.right = 450;
    rank2Button.bottom = 500;

    volumeBar.left = 570;
    volumeBar.top = 570;
    volumeBar.right = 770;
    volumeBar.bottom = 590;
}

// 버튼 그리기용 함수
void DrawButton(
    HDC hdc,
    RECT rect,
    LPCWSTR text,
    bool isHovered)
{
    HBRUSH brush;

    if (isHovered) {
        brush =
            CreateSolidBrush(
                RGB(180, 220, 255));
    }
    else {
        brush =
            CreateSolidBrush(
                RGB(220, 220, 220));
    }

    FillRect(
        hdc,
        &rect,  // 사각형 범위
        brush); // 어떤 색으로 칠할지

    FrameRect(      // 사각형 테두리만 그림.
        hdc,
        &rect,
        (HBRUSH)GetStockObject(BLACK_BRUSH));   // 윈도우 기본 제공 검은 붓

    DrawText3(
        hdc,
        rect,
        text,
        uiFont,
        RGB(0, 0, 0),
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    /*
    DrawText(
        hdc,
        text,
        -1,     // 문자열 길이 자동 계산: lstrlen(text); 와 동일.
        &rect,  // 글자 배치 영역
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);    // 가운데 정렬, 세로 가운데, 한 줄만 출력.
        */
}

// 비트맵 로드(main 내부)
void LoadTileImages(void) {
    wchar_t filename[20];

    for (int i = 0; i < 13; i++) {
        wsprintf(
            filename,
            L"images\\%d.bmp",
            i);

        tileBitmap[i] = (HBITMAP)LoadImage(
            NULL,
            filename,
            IMAGE_BITMAP,
            0,
            0,
            LR_LOADFROMFILE);

        if (tileBitmap[i] == NULL) {
            MessageBox(
                NULL,
                L"red.bmp 로드 실패",
                L"에러",
                MB_OK);
        }
    }
}

// 리소스 관리 함수들
void LoadResources(void)
{
    LoadTileImages();
    LoadButtons();

    // 전역폰트 설정
    titleFont = CreateFont(
        80, 0, 0, 0,
        FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH,
        L"맑은 고딕");

    menuFont = CreateFont(
        28, 0, 0, 0,
        FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH,
        L"맑은 고딕");

    uiFont = CreateFont(
        20, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH,
        L"맑은 고딕");

    scoreFont = CreateFont(
        32, 0, 0, 0,
        FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH,
        L"맑은 고딕");
}

void FreeResources(void)
{
    DeleteObject(titleFont);            // 폰트들 삭제 **DeleteDC가 아니라 DeleteObject다.
    DeleteObject(menuFont);
    DeleteObject(uiFont);
    DeleteObject(scoreFont);

    for (int i = 0; i < 13; i++)
        DeleteObject(tileBitmap[i]);
}
/* ColorTiles에 필요한 Windows.h 기본 WinAPI 지식
HWND
WNDCLASS
CreateWindowEx
ShowWindow

WM_PAINT
WM_LBUTTONDOWN

GetMessage
DispatchMessage

Rectangle
FillRect
TextOut

SetTimer
WM_TIMER

다음 단계: WM_PAINT
*/