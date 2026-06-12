#pragma once

struct playerInfo {
	char name[20];
	int score;
};

void saveRanking(char* playerName, int score);
void showRanking();