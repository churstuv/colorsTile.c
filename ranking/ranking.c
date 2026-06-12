#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "ranking.h"

void saveRanking(char* playerName, int score) {
	FILE* fp = NULL;
	fp = fopen("ranking.txt", "a");

	if (fp != NULL) {
		fprintf(fp,"%s %d\n", playerName, score);
		fclose(fp);
	}
}

void showRanking() {
	FILE* fp = NULL;
	struct playerInfo player[100];
	int count_player = 0;

	fp = fopen("ranking.txt", "r");

	if (fp != NULL) {
		while (fscanf(fp, "%s %d", player[count_player].name, &player[count_player].score) != EOF) {
				count_player++;
		}
		fclose(fp);
	}
	for (int i = 0; i < count_player-1; i++) {
		for (int j = 0; j < count_player - 1 - i; j++){
			if(player[j].score < player[j+1].score){
				struct playerInfo tmp;
				tmp = player[j];
				player[j] = player[j+1];
				player[j+1] = tmp;
			}
		}
	}
	printf("Ranking List\n");
	for (int i = 0; i < count_player; i++)
		printf("ranking : %d\nplayer : %s\n score : %d\n", i+1, player[i].name, player[i].score);
}