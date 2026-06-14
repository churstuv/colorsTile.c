#pragma once

#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <Windows.h>

#define MAP_WIDTH 23
#define MAP_HEIGHT 15
#define TOTAL_TILES 200
#define COLORS_NUM 10 

// 1. 타일 구조체 정의 (첫 번째 방식: struct 키워드를 그대로 사용!)
struct Tile {
    int color;       // 0 ~ 9 (10가지 색상)
    int x, y;        // 그리드 좌표
    bool isExists;   // 현재 칸에 타일이 존재하는가?
}; // <-- 구조체 정의 끝에는 세미콜론(;) 필수!

typedef struct {        // **많이 쓰일 것 같아서 typedef로 만듦.
    wchar_t name[32];   // 유니코드 가능.
    int score;
} playerInfo;


// 전역 변수로 게임판 선언 (자료형 이름 앞에 struct를 꼭 붙여줌!)
extern struct Tile board[MAP_HEIGHT][MAP_WIDTH];    // 이미 다른 파일에 전역변수 선언 있음.

extern int score;
extern int gameTime; // 제한시간 120초
extern wchar_t playerName[32];
extern int nameLength;

// 함수 선언 (프로토타입)
void initBoard();
void printBoard();
int searchAndMatch(int startX, int startY);     // **반환함수로 변경
void shuffleBoard();
bool checkDeadlock();
void saveRanking(wchar_t* playerName, int score);
playerInfo* showRanking();      // 포인터변수 반환하기.

#endif