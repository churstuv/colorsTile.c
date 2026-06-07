#include <stdio.h>
#include <Windows.h>    // 창, 버튼, 마우스, 키보드, 타이머, GDI 그래픽, 파일 대화상자 등


// 비트맵(bmp) 로드
HBITMAP tileBitmap[10];     // 전역변수: 타일 bmp 이미지 담는 배열

// WndProc 함수 기본 설정
LRESULT CALLBACK WndProc(   // Window Procedure: 창에서 일어나는 사건 처리 함수
    HWND hwnd,              // Handle to WiNDow: 창의 ID
    UINT msg,               // msg: 메시지 식별자
    WPARAM wParam,          // wParam: 추가 메시지 정보
    LPARAM lParam)          // IParam: 추가 메시지 정보
{
    switch (msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);        // 화면에 그림 그리는 도구

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

            EndPaint(hwnd, &ps);    // 그림 끝
            return 0;
        }
        case WM_LBUTTONDOWN:        // 왼쪽 마우스 클릭 이벤트
        {
            int mouseX = LOWORD(lParam);    // 마우스 x
            int mouseY = HIWORD(lParam);    // 마우스 y

            // searchAndMatch();    만약 매치 성공이면 아래 메시지 전달

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
        }
        case WM_DESTROY:            // 창 닫기 버튼 눌림
            PostQuitMessage(0);     // 게임 종료 명령
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);    // 내가 처리 안 한 이벤트는 Windows 기본 처리에 맡긴다.
}

// 비트맵 로드(main 내부)
void LoadTileImages(void) {
    wchar_t filename[20];

    for (int i = 0; i < 10; i++) {
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



// 실제 프로그램 시작
int WINAPI wWinMain(            // 윈도우 프로그램의 main(void) 구조. 기본 설정: 콘솔 말고 창으로 바꿔야 함.
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"ColorTilesWindow";

    LoadTileImages();       // 타일 불러오기

    WNDCLASSW wc = { 0 };       // 창 설계도, 아직 창 아님

    wc.lpfnWndProc = WndProc;   // 이 창에서 이벤트 발생 시 WndProc 함수 호출.
    wc.hInstance = hInstance;   // 이 프로그램 소속
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);        // 설계도를 Windows에 등록하는 작업.

    HWND hwnd = CreateWindowEx(     // 실제 창 만드는 것
        0,                          // 확장 스타일: 현재 없음으로 되어 있음
        CLASS_NAME,                 // 어떤 설계도로 만들지
        L"Color Tiles",             // 창 제목
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,        // 최소화, 닫기 버튼만 있는 고정 창 스타일
        CW_USEDEFAULT,  // X
        CW_USEDEFAULT,  // Y
        800,            // width
        600,            // height
        NULL,
        NULL,
        hInstance,
        NULL);

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