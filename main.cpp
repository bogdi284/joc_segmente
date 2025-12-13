#include "graphics.h"
#include "winbgim.h"
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
// 
enum stare {
	MENIU_PRINCIPAL,
	MENIU_MOD_JOC,
	INSTRUCTIUNI,
	JOC,
	SETARI
};
struct buton {
	int x, y, w, h;
	char text[50];
};

buton buton_start, buton_setari, buton_instr, buton_iesire, buton_inapoi ;
buton buton_pvp, buton_pvb;

int stare_curenta = MENIU_PRINCIPAL;
int mod_joc = 0; //0 - pvp si 1 - pvb

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
int main() {
	initwindow(W , H , "Joc Segmente");

	init_meniu_principal();
	init_instructiuni();
	init_meniu_mod_joc();

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
				///aici va veni setari :))
			}
			else if (stare_curenta == MENIU_MOD_JOC) {
				if (este_click_pe_buton(buton_inapoi, mx, my)) {
					stare_curenta = MENIU_PRINCIPAL;
				}
				else if (este_click_pe_buton(buton_pvp, mx, my)) {
					mod_joc = 0; // player vs player
					//aici va veni start joc
					//stare_curenta = joc_pvp
				}
				else if (este_click_pe_buton(buton_pvb, mx, my)) {
					mod_joc = 1; // player vs bot
				}
			}
			else if (stare_curenta == INSTRUCTIUNI) {
				if (este_click_pe_buton(buton_inapoi, mx, my)) {
					stare_curenta = MENIU_PRINCIPAL;
				}
			}
			/// aici va veni setari
		}
		pagina = 1 - pagina;
		delay(30);
	}
	
	closegraph();
	return 0;
}