#include <stdio.h>
#include <Windows.h>    // 창, 버튼, 마우스, 키보드, 타이머, GDI 그래픽, 파일 대화상자 등

#include "gameplay.h"   // 벡엔드 함수 정의 파일(gameplay.c) 불러오기

#pragma comment(lib, "Msimg32.lib")     // 직접 링커 쓰기, TransparentBit 쓰려고 부름

#define TILE_SIZE 32    // 타일 pixel
#define BOARD_X 10      // 실제 게임판 최소 x좌표 (800 중 736이 블록)
#define BOARD_Y 10      // 실제 게임판 최소 y좌표 (600 중 480이 블록, 상단에 )
#define CLIENT_WIDTH 800        // 실제 그림 그리는 영역: 클라이언트
#define CLIENT_HEIGHT 600

// 비트맵(bmp) 로드
HBITMAP tileBitmap[13];     // 전역변수: 타일 bmp 이미지 담는 배열

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

// // 1. 게임 상태, 화면
// 게임 초기 화면에 들어갈 게임 상태 만들기
typedef enum {
    STATE_MENU,
    STATE_GAME,
    STATE_HELP,
    STATE_RANK,
    STATE_GAMEOVER
} GameState;

GameState currentState = STATE_MENU;

// 화면별 함수 정의
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
RECT startButton;
RECT helpButton;
RECT rankButton;
RECT exitButton;
RECT resetButton;
RECT backButton;        // 얘는 중복해서 쓰일 것.
RECT menuButton;
RECT exit2Button;       // 게임오버에 뜨는 버튼


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
    BUTTON_START,
    BUTTON_HELP,
    BUTTON_RANK,
    BUTTON_EXIT,
    BUTTON_EXIT2,
    BUTTON_RESET,
    BUTTON_BACK,
    BUTTON_MENU
} HoverButton;

HoverButton hoverButton = BUTTON_NONE;
HoverButton oldHover = BUTTON_NONE;

// 비트맵 로드(main 내부)
void LoadTileImages(void);


// WndProc 함수 기본 설정
LRESULT CALLBACK WndProc(   // Window Procedure: 창에서 일어나는 사건 처리 함수
    HWND hwnd,              // Handle to WiNDow: 창의 ID
    UINT msg,               // msg: 메시지 식별자
    WPARAM wParam,          // wParam: 추가 메시지 정보
    LPARAM lParam)          // IParam: 추가 메시지 정보
{
    switch (msg)
    {
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
                else if (PtInRect(&exit2Button, pt))
                    hoverButton = BUTTON_EXIT2;
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
        case WM_PAINT:      // 창 그리기
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);        // 화면에 그림 그리는 도구

            switch (currentState)
            {
            case STATE_MENU:        // 메뉴 창 그리기
                DrawMenu(hdc);
                break;

            case STATE_GAME:        // 게임 창 그리기
                DrawGame(hdc);
                break;

            case STATE_HELP:        // 도움말 창 그리기
                DrawHelp(hdc);
                break;

            case STATE_RANK:        // 랭크 창 그리기
                DrawRank(hdc);
                break;

            case STATE_GAMEOVER:    // 게임오버 창 그리기
                DrawGameOver(hdc);
                break;
            }

            EndPaint(hwnd, &ps);    // 그림 끝
            return 0;
        }
        case WM_LBUTTONDOWN:    // 왼쪽 마우스 클릭 이벤트 (버튼, 볼륨 슬라이드, 블록)
        {
            int mouseX = LOWORD(lParam);    // 마우스 x
            int mouseY = HIWORD(lParam);    // 마우스 y

            POINT pt;

            pt.x = mouseX;
            pt.y = mouseY;

            switch (currentState)
            {
            case STATE_MENU:        // 메뉴 화면: 버튼 4개
                if (PtInRect(&startButton, pt)) {
                    currentState = STATE_GAME;
                    hoverButton = BUTTON_NONE;      // **버튼 호버링 초기화(버그 픽스)
                    
                    // 게임 시작 타일 초기화
                    srand((unsigned int)time(NULL));
                    initBoard();

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                if (PtInRect(&exitButton, pt)) {
                    PostMessage(
                        hwnd,
                        WM_CLOSE,
                        0,
                        0);
                }
                if (PtInRect(&helpButton, pt)) {
                    currentState = STATE_HELP;
                    hoverButton = BUTTON_NONE;

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                if (PtInRect(&rankButton, pt)) {
                    currentState = STATE_RANK;
                    hoverButton = BUTTON_NONE;

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

                   InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                // 볼륨 슬라이드 기입해야 함.
                break;

            case STATE_HELP:        // 도움말 화면: 버튼 1개
                if (PtInRect(&backButton, pt)) {
                    currentState = STATE_MENU;
                    hoverButton = BUTTON_NONE;

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

                    InvalidateRect(
                        hwnd,
                        NULL,
                        TRUE);
                }
                if (PtInRect(&exit2Button, pt)) {
                    PostMessage(
                        hwnd,
                        WM_CLOSE,
                        0,
                        0);
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
        case WM_DESTROY:            // 창 닫기 버튼 눌림
            PostQuitMessage(0);     // 게임 종료 명령
            return 0;
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

    LoadButtons();          // 버튼 크기 불러오기 함수

    LoadTileImages();       // 타일 불러오기 함수

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

void DrawMenu(HDC hdc)
{   
    DrawReset(hdc);
    DrawButton(hdc, startButton, L"게임 시작", hoverButton == BUTTON_START);
    DrawButton(hdc, helpButton, L"설명", hoverButton == BUTTON_HELP);
    DrawButton(hdc, rankButton, L"랭킹", hoverButton == BUTTON_RANK);
    DrawButton(hdc, exitButton, L"종료", hoverButton == BUTTON_EXIT);
}

void DrawTile(HDC hdc, int x, int y, int id)
{
    HDC memDC = CreateCompatibleDC(hdc);

    SelectObject(memDC, tileBitmap[id]);

    TransparentBlt(     // bmp 원본 그림의 (0,0)~(32,32)를 창의 (100,100)에 복사하기
        hdc,        // 목적지DC
        x,        // 목적지X
        y,        // 목적지Y
        TILE_SIZE,         // 가로
        TILE_SIZE,         // 세로
        memDC,      // 원본DC
        0,          // 원본에서의 X
        0,          // 원본에서의 Y
        TILE_SIZE,  // 원본 가로
        TILE_SIZE,  // 원본 세로
        RGB(255,255,255));   // 투명으로 바꿀 색

    DeleteDC(memDC);
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

    // 점수 텍스트

    // 볼륨 슬라이더

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
    DrawReset(hdc);

    RECT rect =
    {
        50,
        50,
        750,
        550
    };

    DrawText(       
        hdc,
        L"빈 칸을 클릭해서 같은 색 타일을 제거하세요.",
        -1,
        &rect,
        DT_CENTER);

    DrawButton(hdc, backButton, L"뒤로가기", hoverButton == BUTTON_BACK);
}

void DrawRank(HDC hdc) 
{
    DrawReset(hdc);

    TextOut(        // 한 줄만 출력하는 함수
        hdc,
        300,
        100,
        L"랭킹 준비중",
        6);

    DrawButton(hdc, backButton, L"뒤로가기", hoverButton == BUTTON_BACK);
}

void DrawGameOver(HDC hdc) 
{
    wchar_t buffer[100];    // wchar_t: 한글, 일어, 한자 등 최소 2바이트짜리도 쓸 수 있는 것.

    wsprintf(       // buffer에 문장 넣기.(조립)
        buffer,
        L"최종 점수 : %d",
        score);

    TextOut(        // 한 줄만 출력하는 함수(출력)
        hdc,        // dc의 핸들(device context)
        300,        // 문자열 x 좌표
        200,        // 문자열 y 좌표
        buffer,     // 출력 문자열
        lstrlen(buffer));   // 출력할 문자열 길이

    DrawButton(hdc, menuButton, L"메뉴", hoverButton == BUTTON_MENU);
    DrawButton(hdc, exitButton, L"종료", hoverButton == BUTTON_EXIT);
}

// 버튼 크기 로드
void LoadButtons(void) {
    startButton.left = 300;
    startButton.top = 150;
    startButton.right = 500;
    startButton.bottom = 200;

    helpButton.left = 300;
    helpButton.top = 220;
    helpButton.right = 500;
    helpButton.bottom = 270;

    rankButton.left = 300;
    rankButton.top = 290;
    rankButton.right = 500;
    rankButton.bottom = 340;

    exitButton.left = 300;
    exitButton.top = 360;
    exitButton.right = 500;
    exitButton.bottom = 410;

    resetButton.left = 500;
    resetButton.top = 570;
    resetButton.right = 600;
    resetButton.bottom = 590;

    backButton.left = 650;
    backButton.top = 500;
    backButton.right = 750;
    backButton.bottom = 550;

    menuButton.left = 350;
    menuButton.top = 370;
    menuButton.right = 450;
    menuButton.bottom = 420;

    exit2Button.left = 350;
    exit2Button.top = 450;
    exit2Button.right = 450;
    exit2Button.bottom = 500;
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

    DrawText(
        hdc,
        text,
        -1,     // 문자열 길이 자동 계산: lstrlen(text); 와 동일.
        &rect,  // 글자 배치 영역
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);    // 가운데 정렬, 세로 가운데, 한 줄만 출력.
}

// 비트맵 로드(main 내부)
void LoadTileImages(void) {
    wchar_t filename[20];

    for (int i = 0; i < 13; i++) {
        wsprintf(
            filename,
            L"%d.bmp",
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