#include "graphics.h"
#include "winbgim.h"
#include "backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

//configurari generale
#define W  1366
#define H 768
#define CULOARE_FUNDAL BLACK
#define FONT BOLD_FONT

//configurari butoane
#define BUTON_CULOARE BLACK
#define BUTON_CULOARE_HOVER YELLOW
#define BUTON_CULOARE_TEXT WHITE
#define BUTON_FONT BOLD_FONT
#define BUTON_FONT_SIZE 7
#define WB 320
#define HB 80

//configurari paleta de culoare de la selectie playeri
const int CULORI_DISPONIBILE[] = {1, 2, 3, 5, 9, 10, 11, 14};
const int NUMAR_CULORI = 8;

//coliziuni
#define RAZA_PUNCT 8
#define MARJA_COLIZIUNE 15 //distanta minima acceptata intre o linie si un punct
//variabile pentru sistemul de logging
FILE* fisier_log = NULL;
//starile pe care le poate avea jocul
enum stare {
	MENIU_PRINCIPAL,
	MENIU_MOD_JOC,
	INSTRUCTIUNI,
	JOC,
	SETARI,
	SETUP_JOC,
	PLASARE_PUNCTE
};

//structuri pentru interfata
struct buton {
	int x, y, w, h;
	char text[50];
};
struct caseta_nume {
	int x, y, w, h;
	char text[21];
	bool activ;
};

//structuri pentru joc
struct punct {
	int x, y;
	bool folosit;
};
struct segment {
	punct p1, p2;
	int jucator; //1 - player1 si 2 - player2
};

//butoane:)
buton buton_start, buton_setari, buton_instr, buton_iesire, buton_inapoi ;
buton buton_pvp, buton_pvb , buton_start_joc;
buton buton_continua, buton_gata ;
buton buton_undo;

//setari default
int stare_curenta = MENIU_PRINCIPAL;
int mod_joc = 0; //0 - pvp si 1 - pvb

//date despre jucatori
char nume_player1[21] = "Player 1";
char nume_player2[21] = "Player 2";
int culoare_player1 = 1; //default e blue
int culoare_player2 = 2; //default e green

//configurari caseta schimbare nume la selectia playerilor
caseta_nume input1, input2;
int input_focus = 0; //0 - nimic , 1 - player 1 , 2 - player 2

// cronometrare joc
time_t timp_start;
time_t timp_final;

//alocari dinamice
punct* puncte = NULL;
segment* segmente = NULL;

//variabile de joc
int capacitate_maxima = 1000;
int numar_puncte = 0;
int numar_segmente = 0;
int jucator_curent = 1;
int index_punct = -1;
bool joc_terminat = false;
int castigator = 0;

//alocari dinamice
void alocare() {
	if (puncte == NULL) {
		puncte = new punct[capacitate_maxima];
	}
	if (segmente == NULL) {
		segmente = new segment[capacitate_maxima];
	}

	numar_puncte = 0;
	numar_segmente = 0;
	jucator_curent = 1;
	joc_terminat = false;
	castigator = 0;
	index_punct = -1;
}
void dealocare() {
	if (puncte != NULL) {
		delete[] puncte;
		puncte = NULL;
	}
	if (segmente != NULL) {
		delete[] segmente;
		segmente = NULL;
	}
	numar_puncte = 0;
	numar_segmente = 0;
}

//functii matematice
int minim(int a, int b) {
	return (a < b) ? a : b;
}
int maxim(int a, int b) {
	return (a > b) ? a : b;
}
long long produs_vectorial(punct a, punct b, punct c) {
	//functie de produs vectorial 
	return (long long)(b.x - a.x) * (c.y - a.y) - (long long)(b.y - a.y) * (c.x - a.x);
}
bool pe_segment(punct a, punct b, punct c) {
	//functie care verifica daca punctul c se afla pe segemntul ab (pentru pct coliniare)
	return c.x >= minim(a.x, b.x) && c.x <= maxim(a.x, b.x) && c.y >= minim(a.y, b.y) && c.y <= maxim(a.y, b.y);
}
double distanta_punct_segment(punct p, punct a, punct b) {
	//functie pentru a verifica daca linia trece peste un punct
	double l = pow(a.x - b.x, 2) + pow(a.y - b.y, 2);
	if (l == 0) {
		return sqrt(pow(p.x - a.x, 2) + pow(p.y - a.y, 2));
	}
	double t = ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l;

	if (t < 0) {
		t = 0;
	}
	else if (t > 1) {
		t = 1;
	}
	double prjx = a.x + t * (b.x - a.x);
	double prjy = a.y + t * (b.y - a.y);

	return sqrt(pow(p.x - prjx, 2) + pow(p.y - prjy, 2));
}
bool se_intersecteaza(punct p1, punct q1, punct p2, punct q2) {
	//functie care verifica daca doua segmente se intersecteaza (p1q1 si p2q2)
	long long a1 = produs_vectorial(p1, q1, p2);
	long long a2 = produs_vectorial(p1, q1, q2);
	long long a3 = produs_vectorial(p2, q2, p1);
	long long a4 = produs_vectorial(p2, q2, q1);

	//caz general : capetele unui segment sunt de parti opuse ale celuilalt segment
	if (((a1 > 0 && a2 < 0) || (a1 < 0 && a2>0)) && ((a3 > 0 && a4 < 0) || (a3 < 0 && a4 >0))) return true;
	//cazuri particulare : coliniaritate si suprapunere
	if (a1 == 0 && pe_segment(p1, q1, p2)) return true;
	if (a2 == 0 && pe_segment(p1, q1, q2)) return true;
	if (a3 == 0 && pe_segment(p2, q2, p1)) return true;
	if (a4 == 0 && pe_segment(p2, q2, q1)) return true;

	return false;
}
bool mutare_valida(int i1, int i2) {
	//functie care verifica validitatea unei mutari intre indicii i1 si i2
	for (int i = 0; i < numar_segmente; i++) {
		//verific intersectia cu toate segmentele deja trasate
		if (se_intersecteaza(puncte[i1], puncte[i2], segmente[i].p1, segmente[i].p2))
			return false;
	}
	//verificam coliziunea cu alte puncte , linia sa nu treaca prea aproape de un punct
	for (int i = 0; i < numar_puncte; i++) {
		if (i == i1 || i == i2) continue;
		double distanta = distanta_punct_segment(puncte[i], puncte[i1], puncte[i2]);
		if (distanta < MARJA_COLIZIUNE) {
			return false;
		}
	}
	return true;
}
bool mai_exista_mutari() {
	//functie care verifica conditia de victorie , daca mai exista mutari posibile
	for (int i = 0; i < numar_puncte; i++) {
		if (puncte[i].folosit) continue;
		for (int j = i + 1; j < numar_puncte; j++) {
			if (puncte[j].folosit) continue;
			if (mutare_valida(i, j)) return true;
		}
	}
	return false;
}
//initializare logica jocului
void logica_joc() {
	//incepem jocul cu variabilele default
	numar_segmente = 0;
	jucator_curent = 1;
	joc_terminat = false;
	castigator = 0;
	index_punct = -1;
	
	for (int i = 0; i < numar_puncte; i++) {
		puncte[i].folosit = false;
	}
}
// interfata (butoane)
void init_buton(buton* b, int x, int y, int w, int h, const char* text) {
	b->x = x;
	b->y = y;
	b->w = w;
	b->h = h;
	strcpy(b->text, text);
}
void desen_buton(buton b) {
	int mx = mousex();
	int my = mousey();
	bool este_sub_mouse = (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h);
	int culoare_fill = este_sub_mouse ? BUTON_CULOARE_HOVER : BUTON_CULOARE;
	/// fundal buton
	setfillstyle(SOLID_FILL, culoare_fill);
	bar(b.x, b.y, b.x + b.w, b.y + b.h);
	///contur buton
	setcolor(WHITE);
	rectangle(b.x, b.y, b.x + b.w, b.y + b.h);
	///text
	setbkcolor(culoare_fill);
	setcolor(BUTON_CULOARE_TEXT);
	int marime_curenta = BUTON_FONT_SIZE;
	//settextstyle(BUTON_FONT, HORIZ_DIR, BUTON_FONT_SIZE);
	
	while ((textwidth(b.text) > b.w - 5 || textheight(b.text) > b.h - 2) && marime_curenta > 1) {
		marime_curenta--;
		settextstyle(BUTON_FONT, HORIZ_DIR, marime_curenta);
	}
	///centrare text
	int text_latime = textwidth(b.text);
	int text_inaltime = textheight(b.text);
	int tx = b.x + (b.w - text_latime) / 2;
	int ty = b.y + (b.h - text_inaltime) / 2;
	
	outtextxy(tx, ty, b.text);
}
bool este_click_pe_buton(buton b, int mx, int my) {
	return (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h);
}
//interfata (meniuri)
void init_meniu_principal() {
	int xcentru = (W - WB) / 2;
	int ystart = 250;
	int spatiu = 90;
	init_buton(&buton_start , xcentru , ystart , WB , HB , "START");
	init_buton(&buton_setari , xcentru , ystart + spatiu , WB , HB , "SETARI");
	init_buton(&buton_instr, xcentru, ystart + spatiu*2, WB, HB, "INSTRUCTIUNI");
	init_buton(&buton_iesire, xcentru, ystart + spatiu*3, WB, HB, "IESIRE");

}
void desen_meniu_principal() {
	setbkcolor(CULOARE_FUNDAL);
	cleardevice();
	//titlu
	setcolor(YELLOW);
	settextstyle(BOLD_FONT, HORIZ_DIR, 6);
	char titlu[] = "JOC SEGMENTE";
	outtextxy((W - textwidth(titlu)) / 2, 100, titlu);

	desen_buton(buton_start);
	desen_buton(buton_setari);
	desen_buton(buton_instr);
	desen_buton(buton_iesire);
}
void init_instructiuni() {
	init_buton(&buton_inapoi, (W - WB) / 2, H - 100, WB, HB, "INAPOI");
}
void desen_instructiuni() {
	setbkcolor(CULOARE_FUNDAL);
	cleardevice();
	//titlu pagina
	setcolor(YELLOW);
	settextstyle(BOLD_FONT, HORIZ_DIR, 5);
	char titlu[] = "REGULI DE JOC";
	outtextxy((W - textwidth(titlu)) / 2, 80, titlu);
	//text instructiuni
	setcolor(WHITE);
	settextstyle(BOLD_FONT, HORIZ_DIR, 4);
	int ytext = 175;
	int xtext = 100;
	int spatiu_linii = 50;

	outtextxy(xtext, ytext, " - Doi jucatori unesc alternativ oricare doua puncte libere.");
	ytext += spatiu_linii;
	outtextxy(xtext, ytext, " - Un punct poate fi capatul unui singur segment ( o singura data).");
	ytext += spatiu_linii;
	outtextxy(xtext, ytext, " - Segmentele nu se pot intersecta.");
	ytext += spatiu_linii;
	outtextxy(xtext, ytext, " - Castiga jucatorul care face ultima mutare posibila.");
	ytext += spatiu_linii;

	desen_buton(buton_inapoi);
}
void init_meniu_mod_joc() {
	int xcentru = (W - WB) / 2;
	int ystart = 250;
	int spatiu = 100;

	init_buton(&buton_pvp, xcentru, ystart, WB, HB, "PLAYER VS PLAYER");
	init_buton(&buton_pvb, xcentru, ystart + spatiu, WB, HB, "PLAYER VS BOT");

	init_buton(&buton_inapoi, (W - 200) / 2, H - 150, 200, 50, "INAPOI");
}
void desen_meniu_mod_joc() {
	setbkcolor(CULOARE_FUNDAL);
	cleardevice();

	setcolor(YELLOW);
	settextstyle(BOLD_FONT, HORIZ_DIR, 5);
	char titlu[] = "SELECTARE MOD DE JOC";
	outtextxy((W - textwidth(titlu)) / 2, 100, titlu);
	
	desen_buton(buton_pvp);
	desen_buton(buton_pvb);
	desen_buton(buton_inapoi);
}
//functii pentru setup ul jocului
void init_setup_joc() {
	int w_caseta = 300, h_caseta = 50;

	//playerul 1
	input1.x = 200;
	input1.y = 200;
	input1.w = w_caseta;
	input1.h = h_caseta;
	input1.activ = false;

	//playerul 2
	input2.x = W - 200 - w_caseta;
	input2.y = 200;
	input2.w = w_caseta;
	input2.h = h_caseta;
	input2.activ = false;

	init_buton(&buton_start_joc, (W - WB) / 2, H - 200, WB, HB, "INCEPE JOCUL");
	init_buton(&buton_inapoi, (W - 150) / 2, H - 100, 150, 40, "INAPOI");
}
void paleta_culori(int start_x, int start_y, int& culoare_selectata) {
	int size = 40, gap = 10;
	for (int i = 0; i < NUMAR_CULORI; i++) {
		int x = start_x + (i % 4) * (size + gap);
		int y = start_y + (i / 4) * (size + gap);

		setfillstyle(SOLID_FILL , CULORI_DISPONIBILE[i]);
		bar(x, y, x + size, y + size);

		if (CULORI_DISPONIBILE[i] == culoare_selectata) {
			setcolor(WHITE);
			setlinestyle(SOLID_LINE, 0, 3);
			rectangle(x - 2, y - 2, x + size + 2, y + size + 2);
			setlinestyle(SOLID_LINE, 0, 1);
		}
	}
}
void desen_setup_joc() {
	setbkcolor(CULOARE_FUNDAL);
	cleardevice();

	setcolor(WHITE);
	settextstyle(BOLD_FONT, HORIZ_DIR, 4);
	char titlu[] = "CONFIGURARE JUCATORI";
	outtextxy((W - textwidth(titlu)) / 2, 50, titlu);

	//player 1
	settextstyle(BUTON_FONT, HORIZ_DIR, 4);
	setcolor(culoare_player1);
	outtextxy(input1.x, input1.y - 40, "PLAYER 1");
	//caseta nume player 1
	setcolor(input_focus == 1 ? YELLOW : WHITE);
	rectangle(input1.x, input1.y, input1.x + input1.w, input1.y + input1.h);
	setcolor(WHITE);
	outtextxy(input1.x + 10, input1.y + 10, nume_player1);
	//paleta culori 1
	paleta_culori(input1.x, input1.y + 80, culoare_player1);

	//player 2
	settextstyle(BUTON_FONT, HORIZ_DIR, 4);
	setcolor(culoare_player2);
	outtextxy(input2.x, input2.y - 40, (mod_joc == 1 ? "COMPUTER" : "PLAYER 2"));
	//caseta nume player 2
	setcolor(input_focus == 2 ? YELLOW : WHITE);
	rectangle(input2.x, input2.y, input2.x + input2.w, input2.y + input2.h);
	setcolor(WHITE);
	outtextxy(input2.x + 10, input2.y + 10, (mod_joc == 1 ? "BOT" : nume_player2));
	//paleta culori 2
	paleta_culori(input2.x, input2.y + 80, culoare_player2);

	desen_buton(buton_start_joc);
	desen_buton(buton_inapoi);
}
void este_click_pe_paleta(int mx, int my, int xstart, int ystart, int& culoare_selectata) {
	int size = 40, gap = 10;
	for (int i = 0; i < NUMAR_CULORI; i++) {
		int x = xstart + (i % 4) * (size + gap);
		int y = ystart + (i / 4) * (size + gap);
		if (mx >= x && mx <= x + size && my >= y && my <= y + size) {
			culoare_selectata = CULORI_DISPONIBILE[i];
		}
	}
}
//functii legate de puncte
void init_plasare_puncte() {
	init_buton(&buton_gata, W - 170 , H - 70, 150, 50, "GATA");
	init_buton(&buton_inapoi, 20 , H - 70 , 150 , 50 , "INAPOI");
	init_buton(&buton_undo, (W - 100) / 2, H - 90, 150, 30, "UNDO");
}
void desen_plasare_puncte() {
	setbkcolor(CULOARE_FUNDAL);
	cleardevice();

	setcolor(WHITE);

	settextstyle(BOLD_FONT, HORIZ_DIR, 4);
	char titlu[] = "PLASARE PUNCTE";
	outtextxy((W - textwidth(titlu)) / 2, 10, titlu);

	settextstyle(BOLD_FONT, HORIZ_DIR, 2);
	char subtitlu[] = " Click pe ecran pentru a adauga puncte . Minim 4 puncte necesare ";
	outtextxy((W - textwidth(subtitlu)) / 2, 50, subtitlu);

	char counter[50];
	sprintf(counter, "Puncte: %d", numar_puncte);
	outtextxy((W - textwidth(counter)) / 2, H - 50, counter);
	//desenare puncte curente
	for (int i = 0; i < numar_puncte; i++) {
		setcolor(WHITE);
		setfillstyle(SOLID_FILL, WHITE);
		fillellipse(puncte[i].x, puncte[i].y, 8, 8);
	}
	desen_buton(buton_gata);
	desen_buton(buton_inapoi);
	desen_buton(buton_undo);
}
//functii legate de joc
void desen_joc() {
	setbkcolor(BLACK);
	cleardevice();

	init_buton(&buton_inapoi, W - 170, H - 70, 150, 50, "MENIU");
	desen_buton(buton_inapoi);

	init_buton(&buton_undo, 20, H - 70 , 150, 50, "UNDO");
	desen_buton(buton_undo);

	setbkcolor(BLACK);
	setcolor(WHITE);
	settextstyle(BOLD_FONT, HORIZ_DIR, 3);
	char info[300];
	
	if (!joc_terminat) {
		settextstyle(BOLD_FONT, HORIZ_DIR, 4);
		sprintf(info, "Randul lui: %s ", (jucator_curent == 1 ? nume_player1 : nume_player2));
		setcolor(jucator_curent == 1 ? culoare_player1 : culoare_player2);
	}
	else {
		settextstyle(BOLD_FONT, HORIZ_DIR, 4);
		sprintf(info, "CASTIGATORUL este: %s", (jucator_curent == 1 ? nume_player1 : nume_player2));
		setcolor(GREEN);
	}
	outtextxy(W / 2 - textwidth(info) / 2, 25, info);
	//adaugare timp scurs 
	setcolor(WHITE);
	settextstyle(BOLD_FONT, HORIZ_DIR, 3);

	time_t timp_curent;
	double secunde_trecute;
	if (joc_terminat) {
		secunde_trecute = difftime(timp_final, timp_start);
	}
	else {
		time(&timp_curent);
		secunde_trecute = difftime(timp_curent, timp_start);
	}
	int minute = (int)secunde_trecute / 60;
	int secunde = (int)secunde_trecute % 60;
	char timp_trecut[50];
	sprintf(timp_trecut, " %02d:%02d ", minute, secunde);
	outtextxy(W - textwidth(timp_trecut) - 30, 25, timp_trecut);
	//desenare segmente
	setlinestyle(SOLID_LINE, 0, 4);
	for (int i = 0; i < numar_segmente; i++) {
		setcolor(segmente[i].jucator == 1 ? culoare_player1 : culoare_player2);
		line(segmente[i].p1.x, segmente[i].p1.y, segmente[i].p2.x, segmente[i].p2.y);
	}
	//desenare puncte
	for (int i = 0; i < numar_puncte; i++) {
		if (puncte[i].folosit) {
			setcolor(DARKGRAY);
			setfillstyle(SOLID_FILL, DARKGRAY);
		}
		else if (i == index_punct) {
			setcolor(YELLOW);
			setfillstyle(SOLID_FILL, YELLOW);
		}
		else {
			setcolor(WHITE);
			setfillstyle(SOLID_FILL, WHITE);
		}
		fillellipse(puncte[i].x, puncte[i].y, 8, 8);
	}
}
//functii pentru sistemul de logging
void init_log() {
	time_t t = time(NULL);
	struct tm* tm = localtime(&t);
	char nume_fisier[100];

	sprintf(nume_fisier, "meci_%04d%02d%02d_%02d%02d%02d.txt", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	fisier_log = fopen(nume_fisier, "w");

	if (fisier_log) {
		fprintf(fisier_log, " JOC SEGMENTE - LOG MECI \n");
		fprintf(fisier_log, " Data: %02d-%02d-%04d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
		fprintf(fisier_log, "Ora inceperii: %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
		fprintf(fisier_log, "Jucator 1: %s\n", nume_player1);
		fprintf(fisier_log, "Jucator 2: %s\n", nume_player2);
		fprintf(fisier_log, "\n");
		fprintf(fisier_log, "MUTARI:\n");
	}
}
void log_mutare(int idx1, int idx2, int jucator) {
	if (!fisier_log) return;
	
	time_t t = time(NULL);
	struct tm* tm = localtime(&t);
	char* nume_jucator = (jucator == 1) ? nume_player1 : nume_player2;

	fprintf(fisier_log, "[%02d:%02d:%02d] %s a trasat segment intre (%d , %d) si (%d , %d)\n" , tm->tm_hour , tm->tm_min , tm->tm_sec , nume_jucator , puncte[idx1].x , puncte[idx1].y , puncte[idx2].x , puncte[idx2].y);
	fflush(fisier_log);
}
void inchide_log(bool terminat_cu_succes) {
	if (!fisier_log) return;

	time_t t = time(NULL);
	struct tm* tm = localtime(&t);

	fprintf(fisier_log, "\n");
	if (terminat_cu_succes) {
		char* nume_castigator = (castigator == 1) ? nume_player1 : nume_player2;
		fprintf(fisier_log, "Meci incheiat la: %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
		fprintf(fisier_log, "CASTIGATOR: %s\n", nume_castigator);
	}
	else {
		fprintf(fisier_log,"Meci intrerupt la: %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
		fprintf(fisier_log, "Fara castigator.\n");
	}
	fclose(fisier_log);
	fisier_log = NULL;
}
void log_undo(int x1, int y1, int x2, int y2, int jucator) {
	if (!fisier_log) return;
	fprintf(fisier_log, "Segmentul (%d , %d) - (%d , %d) a fost sters de %s \n", x1, y1, x2, y2, (jucator == 1 ? nume_player1 : nume_player2));
	fflush(fisier_log);
}
int main() {
	initwindow(W , H , "Joc Segmente");

	init_meniu_principal();
	init_instructiuni();
	init_meniu_mod_joc();
	init_setup_joc();
	init_plasare_puncte();

	int pagina = 0;
	bool ruleaza = true;

	while (ruleaza) {
		setactivepage(pagina);
		setvisualpage(1 - pagina);

		if (stare_curenta == MENIU_PRINCIPAL) {
			desen_meniu_principal();
		}
		else if (stare_curenta == INSTRUCTIUNI) {
			desen_instructiuni();
		}
		else if (stare_curenta == MENIU_MOD_JOC) {
			desen_meniu_mod_joc();
		}
		else if (stare_curenta == SETUP_JOC) {
			desen_setup_joc();
		}
		else if (stare_curenta == PLASARE_PUNCTE) {
			desen_plasare_puncte();
		}
		else if (stare_curenta == JOC) {
			desen_joc();
		}

		if (stare_curenta == SETUP_JOC && kbhit()) {
			char c = getch();
			char* nume_activ = NULL;
			if (input_focus == 1) nume_activ = nume_player1;
			else if (input_focus == 2) nume_activ = nume_player2;

			if (nume_activ != NULL) {
				int lung = strlen(nume_activ);
				if (c == 8) {
					if (lung > 0) nume_activ[lung - 1] = '\0';
				}
				else if (lung < 18 && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ')) {
					nume_activ[lung] = c;
					nume_activ[lung + 1] = '\0';
				}
			}
		}

		if (ismouseclick(WM_LBUTTONDOWN)) {
			int mx, my;
			getmouseclick(WM_LBUTTONDOWN, mx, my);
			
			if (stare_curenta == MENIU_PRINCIPAL) {
				if (este_click_pe_buton(buton_iesire, mx, my)) {
					ruleaza = false;
				}
				else if (este_click_pe_buton(buton_instr, mx, my)) {
					stare_curenta = INSTRUCTIUNI;
				}
				else if (este_click_pe_buton(buton_start, mx, my)) {
					stare_curenta = MENIU_MOD_JOC;
				}
			}
			else if (stare_curenta == MENIU_MOD_JOC) {
				if (este_click_pe_buton(buton_inapoi, mx, my)) {
					stare_curenta = MENIU_PRINCIPAL;
				}
				else if (este_click_pe_buton(buton_pvp, mx, my)) {
					mod_joc = 0; 
					stare_curenta = SETUP_JOC;
				}
				else if (este_click_pe_buton(buton_pvb, mx, my)) {
					mod_joc = 1; 
					stare_curenta = SETUP_JOC;
				}
			}
			else if (stare_curenta == INSTRUCTIUNI) {
				if (este_click_pe_buton(buton_inapoi, mx, my)) {
					stare_curenta = MENIU_PRINCIPAL;
				}
			}
			else if (stare_curenta == SETUP_JOC) {
				if (este_click_pe_buton(buton_inapoi, mx, my)) {
					stare_curenta = MENIU_MOD_JOC;
				}
				else if (este_click_pe_buton(buton_start_joc, mx, my)) {
					alocare();
					stare_curenta = PLASARE_PUNCTE;
					init_plasare_puncte();
				}

				if (mx >= input1.x && mx <= input1.x + input1.w && my >= input1.y && my <= input1.y + input1.h) {
					input_focus = 1;
				}
				else if (mx >= input2.x && mx <= input2.x + input2.w && my >= input2.y && my <= input2.y + input2.h) {
					input_focus = 2;
				}
				else {
					input_focus = 0;
				}
				este_click_pe_paleta(mx, my, input1.x, input1.y + 80, culoare_player1);
				if (mod_joc == 0) {
					este_click_pe_paleta(mx, my, input2.x, input2.y + 80, culoare_player2);
				}
			}
			else if (stare_curenta == PLASARE_PUNCTE) {
				if (este_click_pe_buton(buton_inapoi, mx, my)) {
					dealocare();
					stare_curenta = SETUP_JOC;
				}
				else if (este_click_pe_buton(buton_gata, mx, my)) {
					if (numar_puncte >= 4) {
						logica_joc(); //reset variabile de joc
						time(&timp_start);
						init_log();
						stare_curenta = JOC;
					}
				}
				else if (este_click_pe_buton(buton_undo, mx, my)) {
					if (numar_puncte > 0) {
						numar_puncte--;
					}
				}
				else {
					//cand dau click pe ecran
					if (my > 100 && my < H - 120 && numar_puncte < capacitate_maxima) {
						//cat timp pot pune puncte
						bool prea_aproape = false;
						for (int i = 0; i < numar_puncte; i++) {
							if ((mx - puncte[i].x) * (mx - puncte[i].x) + (my - puncte[i].y) * (my - puncte[i].y) < 400) {
								prea_aproape = true;
								break;
							}
						}
						if (!prea_aproape) {
							puncte[numar_puncte].x = mx;
							puncte[numar_puncte].y = my;
							puncte[numar_puncte].folosit = false;
							numar_puncte++;
						}
					}
				}
			}
			else if (stare_curenta == JOC) {
				if (este_click_pe_buton(buton_inapoi, mx, my)) {
					inchide_log(false);
					dealocare();
					stare_curenta = MENIU_PRINCIPAL;
				}
				else if (este_click_pe_buton(buton_undo, mx, my)) {
					if (numar_segmente > 0) {
						segment s = segmente[numar_segmente - 1];

						log_undo(s.p1.x, s.p1.y, s.p2.x, s.p2.y, s.jucator);

						for (int i = 0; i < numar_puncte; i++) {
							if ((puncte[i].x == s.p1.x && puncte[i].y == s.p1.y) || (puncte[i].x == s.p2.x && puncte[i].y == s.p2.y)) {
								puncte[i].folosit = false;
							}
						}
						numar_segmente--;
						if (joc_terminat) {
							jucator_curent = (jucator_curent == 1) ? 1 : 2;
						}
						else {
							jucator_curent = (jucator_curent == 1) ? 2 : 1;
						}
						joc_terminat = false;
						castigator = 0;
						index_punct = -1;
					}
				}
				else if (!joc_terminat) {
					for (int i = 0; i < numar_puncte; i++) {
						//verific daca click e pe punct
						if ((mx - puncte[i].x) * (mx - puncte[i].x) + (my - puncte[i].y) * (my - puncte[i].y) <= 225) {
							if (!puncte[i].folosit) {
								if (index_punct == -1) {
									//selectam punctul
									index_punct = i;
								}
								else if (index_punct == i) {
									//deselectare
									index_punct = -1;
								}
								else {
									if (mutare_valida(index_punct, i)) {

										log_mutare(index_punct, i, jucator_curent);
										segmente[numar_segmente].p1 = puncte[index_punct];
										segmente[numar_segmente].p2 = puncte[i];
										segmente[numar_segmente].jucator = jucator_curent;
										numar_segmente++;
										//marcam punctele ca folosite
										puncte[index_punct].folosit = true;
										puncte[i].folosit = true;
										index_punct = -1;
										//verific conditia de victorie
										if (!mai_exista_mutari()) {
											joc_terminat = true;
											castigator = jucator_curent;
											time(&timp_final);
											inchide_log(true);
										}
										else {
											jucator_curent = (jucator_curent == 1) ? 2 : 1;
										}
									}
									else {
										//mutare invalida
										index_punct = -1;
									}
								}
							}
						}
					}
				}
			}
		}
		pagina = 1 - pagina;
		delay(30);
	}
	
	closegraph();
	return 0;
}