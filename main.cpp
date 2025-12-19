#include "graphics.h"
#include "winbgim.h"
#include "backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define W  1366
#define H 768
#define CULOARE_FUNDAL BLACK
#define FONT SANS_SERIF_FONT

#define BUTON_CULOARE BLACK
#define BUTON_CULOARE_HOVER YELLOW
#define BUTON_CULOARE_TEXT WHITE
#define BUTON_FONT SANS_SERIF_FONT
#define BUTON_FONT_SIZE 5
#define WB 220
#define HB 50

const int CULORI_DISPONIBILE[] = {1, 2, 3, 5, 9, 10, 11, 14};
const int NUMAR_CULORI = 8;
enum stare {
	MENIU_PRINCIPAL,
	MENIU_MOD_JOC,
	INSTRUCTIUNI,
	JOC,
	SETARI,
	SETUP_JOC
};
struct buton {
	int x, y, w, h;
	char text[50];
};
struct caseta_nume {
	int x, y, w, h;
	char text[21];
	bool activ;
};

buton buton_start, buton_setari, buton_instr, buton_iesire, buton_inapoi ;
buton buton_pvp, buton_pvb , buton_start_joc;

int stare_curenta = MENIU_PRINCIPAL;
int mod_joc = 0; //0 - pvp si 1 - pvb
//date despre jucatori
char nume_player1[21] = "Player 1";
char nume_player2[21] = "Player 2";
int culoare_player1 = 1; //default e blue
int culoare_player2 = 2; //default e green

caseta_nume input1, input2;
int input_focus = 0; //0 - nimic , 1 - player 1 , 2 - player 2
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
	settextstyle(BUTON_FONT, HORIZ_DIR, BUTON_FONT_SIZE);
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
	int wb = WB;
	int hb = HB; 
	int xcentru = (W - wb) / 2;
	int ystart = 250;
	int spatiu = 90;
	init_buton(&buton_start , xcentru , ystart , wb , hb , "START");
	init_buton(&buton_setari , xcentru , ystart + spatiu , wb , hb , "SETARI");
	init_buton(&buton_instr, xcentru, ystart + spatiu*2, wb, hb, "INSTRUCTIUNI");
	init_buton(&buton_iesire, xcentru, ystart + spatiu*3, wb, hb, "IESIRE");

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
	int wb = WB;
	int hb = HB;
	init_buton(&buton_inapoi, (W - wb) / 2, H - 100, wb, hb, "INAPOI");
}
void desen_instructiuni() {
	setbkcolor(CULOARE_FUNDAL);
	cleardevice();
	//titlu pagina
	setcolor(YELLOW);
	settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 5);
	char titlu[] = "REGULI DE JOC";
	outtextxy((W - textwidth(titlu)) / 2, 80, titlu);
	//text instructiuni
	setcolor(WHITE);
	settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 3);
	int ytext = 200;
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
	int wb = WB;
	int hb = HB;
	int xcentru = (W - wb) / 2;
	int ystart = 250;
	int spatiu = 100;

	init_buton(&buton_pvp, xcentru, ystart, wb, hb, "PLAYER VS PLAYER");
	init_buton(&buton_pvb, xcentru, ystart + spatiu, wb, hb, "PLAYER VS BOT");

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

	int wb = 250, hb = 60;
	init_buton(&buton_start_joc, (W - wb) / 2, H - 200, wb, hb, "INCEPE JOCUL");
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
	settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 3);
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
	settextstyle(SANS_SERIF_FONT, HORIZ_DIR, 3);
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
int main() {
	initwindow(W , H , "Joc Segmente");

	init_meniu_principal();
	init_instructiuni();
	init_meniu_mod_joc();
	init_setup_joc();

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
					//incepe jocul
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
		}
		pagina = 1 - pagina;
		delay(30);
	}
	
	closegraph();
	return 0;
}