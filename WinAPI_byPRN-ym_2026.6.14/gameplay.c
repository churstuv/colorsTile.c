#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <Windows.h>

#include "gameplay.h"   // 벡엔드 함수 정의 파일(gameplay.c) 불러오기
/*
#define MAP_WIDTH 23
#define MAP_HEIGHT 15
#define TOTAL_TILES 200
#define COLORS_NUM 10   // 요청사항 반영: NUMS_COLOR -> COLORS_NUM
*/

// 1. 타일 구조체 정의 (첫 번째 방식: struct 키워드를 그대로 사용!)
/*
struct Tile {
    int color;       // 0 ~ 9 (10가지 색상)
    int x, y;        // 그리드 좌표
    bool isExists;   // 현재 칸에 타일이 존재하는가?
}; // <-- 구조체 정의 끝에는 세미콜론(;) 필수!
*/

// 전역 변수로 게임판 선언 (자료형 이름 앞에 struct를 꼭 붙여줌!)
struct Tile board[MAP_HEIGHT][MAP_WIDTH];

int score = 0;
int gameTime = 1200; // 제한시간 120초(0.1초 단위)

// 함수 선언 (프로토타입)
void initBoard();
void printBoard();
void searchAndMatch(int startX, int startY);
void shuffleBoard();
bool checkDeadlock();

/* // main파일은 WinAPI로 대체되었다.
int main(void) {
    system("cls");
    // 1. 난수 생성기 초기화 (가장 먼저 실행되어야 함!)
    srand((unsigned int)time(NULL));

    // 2. 게임판 초기화 (무작위 200개 배치)
    initBoard();

    // 3. 테스트용 메인 게임 루프 (터미널 좌표 입력 방식)
    while (gameTime > 0) {
        system("cls"); // 화면 지우기 (Windows 전용)

        printf("=== Color Tiles 백엔드 테스트 ===\n");
        printf("현재 점수: %d 점 | 남은 시간: %d 초\n\n", score, gameTime);

        // 맵 출력
        printBoard();

        // 교착 상태 검사 예시
        if (checkDeadlock() == true) {
            printf("\n[교착 상태 감지!] 타일을 자동으로 재배치합니다.\n");
            Sleep(2000); // 2초 대기
            shuffleBoard();
            continue;
        }

        // 터미널 입력 테스트용 (나중에 프론트엔드가 마우스 클릭 좌표로 대체할 부분)
        int inputX, inputY;
        printf("\n클릭할 빈 칸의 좌표를 입력하세요 (X Y 입력, 종료는 -1 -1): ");
        scanf_s("%d %d", &inputX, &inputY);

        if (inputX == -1 && inputY == -1) {
            printf("게임을 종료합니다.\n");
            break;
        }

        // 입력 좌표 예외 처리
        if (inputX < 0 || inputX >= MAP_WIDTH || inputY < 0 || inputY >= MAP_HEIGHT) {
            printf("잘못된 범위의 좌표입니다!\n");
            Sleep(1000);
            continue;
        }

        // 규칙 1: 블록이 존재하는 칸을 클릭했을 때는 아무런 일도 발생하지 않음
        if (board[inputY][inputX].isExists) {
            printf("타일이 이미 존재하는 칸은 클릭할 수 없습니다!\n");
            Sleep(1000);
            continue;
        }

        // 탐색 및 매칭 로직 실행
        searchAndMatch(inputX, inputY);

        // 시간 감소 시뮬레이션 (루프 돌 때마다 1초씩 감소한다고 가정)
        gameTime--;
    }

    printf("\n=== GAME OVER ===\n");
    printf("최종 점수: %d 점\n", score);

    return 0;
}
*/


// 비어있는 백엔드 핵심 함수 구역

// [미션 1] 23x15 공간 중 '중복 없이 무작위 200칸'에만 10가지 색상 타일 배치하기
void initBoard() {
    // 1단계: 우선 모든 345칸을 깨끗하게 빈칸 상태(-1)로 밀어버리기
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            board[y][x].color = -1;
            board[y][x].x = x;
            board[y][x].y = y;
            board[y][x].isExists = false;
        }
    }

    // 2단계: 중복 없이 무작위 좌표를 사방으로 찔러가며 딱 200개 채우기
    int count = 0;
    while (count < TOTAL_TILES) {
        int randx = rand() % MAP_WIDTH;
        int randy = rand() % MAP_HEIGHT;

        if (board[randy][randx].isExists == false) {
            board[randy][randx].isExists = true;
            board[randy][randx].color = rand() % COLORS_NUM; // 수정된 상수 적용
            count++;
        }
        else {
            continue;
        }
    }
}

// 터미널에 맵 데이터를 예쁘게 찍어주는 함수
void printBoard() {
    // 상단 X축 인덱스 가이드
    printf("   ");
    for (int x = 0; x < MAP_WIDTH; x++) printf("%2d", x);
    printf("\n   ");
    for (int x = 0; x < MAP_WIDTH; x++) printf("--");
    printf("\n");

    for (int y = 0; y < MAP_HEIGHT; y++) {
        printf("%2d|", y); // 좌측 Y축 인덱스 가이드
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (board[y][x].isExists) {
                printf("%2d", board[y][x].color); // 타일이 있으면 색상 숫자 출력
            }
            else {
                printf(" ."); // 빈 칸은 점(.)으로 표시
            }
        }
        printf("\n");
    }
}

// [미션 2] 클릭 위치 기준 상하좌우 최단거리 타일 매칭 및 점수/페널티 연산
void searchAndMatch(int startX, int startY) {
    // 여기에 사방 탐색 로직 작성 예정
    struct Tile* leftTile = NULL;
    struct Tile* rightTile = NULL;
    struct Tile* upTile = NULL;
    struct Tile* downTile = NULL;
    // printf("\n(%d, %d) 좌표를 기준으로 4방향 탐색을 수행해야 합니다.\n", startX, startY);       // **필요없음
    for (int x = startX - 1; x >= 0; x--) {
        if (board[startY][x].isExists == true) {
            leftTile = &board[startY][x];
            break;
        }
    }
    for (int x = startX + 1; x < MAP_WIDTH; x++) {
        if (board[startY][x].isExists == true) {
            rightTile = &board[startY][x];
            break;
        }
    }
    for (int y = startY - 1; y >= 0; y--) {
        if (board[y][startX].isExists == true) {
            upTile = &board[y][startX];
            break;
        }
    }
    for (int y = startY + 1; y < MAP_HEIGHT; y++) {
        if (board[y][startX].isExists == true) {
            downTile = &board[y][startX];
            break;
        }
    }
    int colorCount[COLORS_NUM] = { 0 }; // 각 색상별로 발견된 타일 수를 세는 배열
    if (leftTile != NULL) {
        colorCount[(*leftTile).color] += 1;
    }
    if (rightTile != NULL) {
        colorCount[(*rightTile).color] += 1;
    }
    if (upTile != NULL) {
        colorCount[(*upTile).color] += 1;
    }
    if (downTile != NULL) {
        colorCount[(*downTile).color] += 1;
    }

    int colorMatched = 0;
    for (int i = 0; i < COLORS_NUM; i++) {
        if (colorCount[i] > 1) {
            score += colorCount[i];
            colorMatched = 1;
            if (upTile != NULL && (*upTile).color == i) {
                (*upTile).isExists = false;
            }
            if (downTile != NULL && (*downTile).color == i) {
                (*downTile).isExists = false;
            }
            if (leftTile != NULL && (*leftTile).color == i) {
                (*leftTile).isExists = false;
            }
            if (rightTile != NULL && (*rightTile).color == i) {
                (*rightTile).isExists = false;
            }
        }
    }
    if (colorMatched == 0) {
        gameTime -= 100;        // **0.1초 단위라 10 -> 100으로 변경
    }

    // Sleep(1500);         // **창에서는 필요없음.
}

// [미션 3] 교착 상태 시 현재 화면에 남은 타일들의 위치를 무작위로 다시 섞기
void shuffleBoard() {
    int remainTile[TOTAL_TILES] = { 0 };
    int count = 0;

    for (int startY = 0; startY < MAP_HEIGHT; startY++) {
        for (int startX = 0; startX < MAP_WIDTH; startX++) {
            if (board[startY][startX].isExists == true) {
                remainTile[count] = board[startY][startX].color;
                count++;
            }
        }
    }

    for (int i = 0; i < count; i++) {
        int newIndex = rand() % count;
        int tmp = remainTile[i];
        remainTile[i] = remainTile[newIndex];
        remainTile[newIndex] = tmp;
    }

    int idx = 0;
    for (int startY = 0; startY < MAP_HEIGHT; startY++) {
        for (int startX = 0; startX < MAP_WIDTH; startX++) {
            if (board[startY][startX].isExists == true) {
                board[startY][startX].color = remainTile[idx];
                idx++;
            }
        }
    }
}

// [미션 4] 더 이상 맞출 수 있는 타일 쌍이 없는 교착 상태(Deadlock) 검사
bool checkDeadlock() {
    for (int startY = 0; startY < MAP_HEIGHT; startY++) {
        for (int startX = 0; startX < MAP_WIDTH; startX++) {
            if (board[startY][startX].isExists == false) {
                struct Tile* leftTile = NULL;
                struct Tile* rightTile = NULL;
                struct Tile* upTile = NULL;
                struct Tile* downTile = NULL;
                for (int x = startX - 1; x >= 0; x--) {
                    if (board[startY][x].isExists == true) {
                        leftTile = &board[startY][x];
                        break;
                    }
                }
                for (int x = startX + 1; x < MAP_WIDTH; x++) {
                    if (board[startY][x].isExists == true) {
                        rightTile = &board[startY][x];
                        break;
                    }
                }
                for (int y = startY - 1; y >= 0; y--) {
                    if (board[y][startX].isExists == true) {
                        upTile = &board[y][startX];
                        break;
                    }
                }
                for (int y = startY + 1; y < MAP_HEIGHT; y++) {
                    if (board[y][startX].isExists == true) {
                        downTile = &board[y][startX];
                        break;
                    }
                }
                int colorCount[COLORS_NUM] = { 0 }; // 각 색상별로 발견된 타일 수를 세는 배열
                if (leftTile != NULL) {
                    colorCount[(*leftTile).color] += 1;
                }
                if (rightTile != NULL) {
                    colorCount[(*rightTile).color] += 1;
                }
                if (upTile != NULL) {
                    colorCount[(*upTile).color] += 1;
                }
                if (downTile != NULL) {
                    colorCount[(*downTile).color] += 1;
                }

                for (int i = 0; i < COLORS_NUM; i++) {
                    if (colorCount[i] >= 2) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}