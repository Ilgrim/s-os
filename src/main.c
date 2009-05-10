#include "Z80.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <conio.h>
#endif

#ifndef	FALSE
#define	FALSE	(0)
#define	TRUE	(1)
#endif

#define	WRD		0x1FAC
#define	WOPEN	0x1FAF
#define	PRTHL	0x1FBE
#define	PRTHX	0x1FC1
#define	BELL	0x1FC4
#define	PAUSE	0x1FC7
#define	GETLKN	0x1FD3
#define	LETNL	0x1FEE
#define	PRINT	0x1FF4

#define	BRKEY	0x1FCD	// ブレークキーが押された？
#define	GETKY	0x1FD0	// １文字入力？
#define	MPRNT	0x1FE2	// メッセージプリント (CALL直後の文字列出力)
#define	MSX		0x1FE5	// メッセージ (DEの文字列出力)
#define	MSG		0x1FE8	// メッセージ (DEの文字列出力)
#define	SETCUR	0x201E	// カーソル位置セット？
#define	GETKEY	0x2021	// キー入力？
#define	WIDCH	0x2030	// 画面横幅変更？


#ifdef _WIN32
#define KBHIT()  _kbhit()
#define GETCH()  _getch()
#else
#define KBHIT()  1
#define GETCH()  _getch()

int _getch() {
	int c;
	read(0, &c, sizeof(char));
	return c;
}
#endif

#define OpZ80(A) RdZ80(A)
#define M_RET R->PC.B.l=OpZ80(R->SP.W++);R->PC.B.h=OpZ80(R->SP.W++);JumpZ80(R->PC.W)
#define M_POP(Rg)      \
  R->Rg.B.l=OpZ80(R->SP.W++);R->Rg.B.h=OpZ80(R->SP.W++)


#define	RamSize		(256 * 256)

byte ram[RamSize];

void WrZ80(register word Addr,register byte Value) {
	ram[Addr] = Value;
}

byte RdZ80(register word Addr) {
	return ram[Addr];
}

void OutZ80(register word Port,register byte Value) {
}

byte InZ80(register word Port) {
	return 0;
}

void PatchZ80(register Z80 *R) {
}

static char line[1024];
static int lp = 0;

static void print_char(int c) {
	if (0xa0 <= c && c <= 0xdf) {
#if 1
		// utf-8
		int uc = 0xff60 + (c - 0xa0);
		putchar(uc);
#else
		putchar(c);
#endif
	} else {
		switch (c) {
		default:
			putchar(c);
			line[lp++] = c;
			break;
		case 0x0c:  // cls
			printf("\x1b[2J\x1b[0;0H");
			break;
		case 0x0d:
			putchar('\n');
			lp = 0;
			break;
		case 28:  // up?
			printf("\x1b[1A");
			break;
		case 29:  // left
			printf("\x1b[1D");
			break;
		case 30:  // right?
			printf("\x1b[1C");
			break;
		case 31:  // down
			printf("\x1b[1B");
			break;
		}
	}
}

static word print(word adr) {
	for (;;) {
		int c = RdZ80(adr);
		adr++;
		if (c == '\0')	break;
		print_char(c);
	}
	return adr;
}

static void print_buf(const char* str) {
	for (;;) {
		int c = *str++;
		if (c == '\0')	break;
		print_char(c);
	}
}

word LoopZ80(register Z80 *R) {
	word pc = R->PC.W;
	if (pc < 0x3000) {
		switch (pc) {
		default:
			M_RET		// dmy
			break;
		case WRD:		// 書き込み
			// DE: 開始番地？
			// HL: サイズ？
			{
				FILE* fp = fopen("__dmy", "wb");
				if (fp != NULL) {
					const char* p = &ram[R->DE.W];
					int size = R->HL.W;
					fwrite(p, 1, size, fp);
					fclose(fp);
				}
			}
			M_RET
			break;
		case WOPEN:		// 書き込みオープン
			// C flag ... エラー
			R->AF.B.l = 0;
			M_RET
			break;
		case PRTHX:
			printf("%02X", R->AF.B.h);
			lp += 2;
			M_RET
			break;
		case BELL:
			putchar('\a');
			M_RET
			break;
		case PAUSE:
			// pause?
			M_RET
			break;
		case GETLKN:
			{
				char* buf = &ram[R->DE.W ];
				int bufsiz=  R->AF.B.h > 0 ? R->AF.B.h : 256;
				char* p = fgets(buf + lp, bufsiz - lp, stdin);
				int len;
				memcpy(buf, line, lp);
				len = strlen(p);
				if (len > 0 && buf[len-1] == '\n') {
					buf[--len] = '\0';
				}
				buf[len] = 0x0d;
				buf[len+1] = '\0';
				lp = 0;
				// Ctrl+Cの処理
			}
			M_RET
			break;
		case LETNL:
			putchar('\n');
			lp = 0;
			M_RET
			break;
		case PRTHL:
			{
				int v = R->HL.W;
				char buf[16];
				sprintf(buf, "%04X", v);
				print_buf(buf);
			}
			M_RET
			break;
		case PRINT:
			{
				int c = R->AF.B.h;
				print_char(c);
			}
			M_RET
			break;

		// 半信半疑のもの
		case MPRNT:
			{
				M_POP(HL);
				R->HL.W = print(R->HL.W);
				R->PC.W = R->HL.W;
			}
			break;
		case MSG:
			{
				for (;;) {
					int c = RdZ80(R->DE.W);
					R->DE.W++;
					if (c == 0x0d)	break;
					print_char(c);
				}
			}
			M_RET
			break;
		case MSX:
			R->DE.W = print(R->DE.W);
			M_RET
			break;
		case GETKEY:
			{
				int c = getchar();
				R->AF.B.h = c;
			}
			M_RET
			break;
		case SETCUR:
			{
				int x = R->HL.B.h;
				int y = R->HL.B.l;
				printf("\x1b[%d;%dH", x + 1, y + 1);
			}
			M_RET
			break;
		case BRKEY:
			// @todo
			M_RET
			break;
		case GETKY:
			if (KBHIT()) {
				R->AF.B.h = GETCH();
			} else {
				R->AF.B.h = 0x00;
			}
			M_RET
			break;
		case WIDCH:
			{
				int w = R->AF.B.h;
				// なんか
			}
			M_RET
			break;
		}
	}

	R->IPeriod = -R->ICount;
	return INT_NONE;
}



void run(Z80* z80) {
	RunZ80(z80);
}

int hex(char c) {
	if ('0' <= c && c <= '9')	return c - '0';
	if ('A' <= c && c <= 'F')	return c - ('A' - 10);
	if ('a' <= c && c <= 'f')	return c - ('a' - 10);
	if (c == ' ')	return 0;	// patch
	return -1;
}

word fromhex(const char* s) {
	word v = 0;
	for (;;) {
		int h = hex(*s++);
		if (h < 0)	break;
		v = (v << 4) | h;
	}
	return v;
}

int load_obj(Z80* z80, const char* fn) {
	FILE* fp = fopen(fn, "rb");
	if (fp == NULL) {
		return FALSE;
	} else {
		const size_t HeaderSize = 18;
		size_t filesize, readsize;
		word load_adr, exec_adr;
		byte buf[32];
		size_t size = fread(buf, 1, HeaderSize, fp);
		if (size < HeaderSize) {
			return FALSE;
		}
		if (strncmp(buf, "_SOS ", 5) != 0) {
			return FALSE;
		}
		load_adr = fromhex(buf+8);
		exec_adr = fromhex(buf+13);

		fseek(fp, 0, SEEK_END);
		filesize = ftell(fp) - HeaderSize;
		fseek(fp, HeaderSize, SEEK_SET);
		if (load_adr + filesize >= RamSize) {
			return FALSE;
		}

		readsize = fread(&ram[load_adr], 1, filesize, fp);
		z80->PC.W = exec_adr;
		return readsize == filesize;
	}
}


int main(int argc, char* argv[]) {
	const char* fn;
	Z80 z80;

	if (argc < 2) {
		fprintf(stderr, "argc < 2\n");
		return 1;
	}

	memset(ram, 0x00, sizeof(ram));
	memset(&z80, 0x00, sizeof(z80));
	ResetZ80(&z80);
	z80.SP.W = 0x0000;

	fn = argv[1];
	if (!load_obj(&z80, fn)) {
		fprintf(stderr, "Load failed: %s\n", fn);
		return 1;
	}

	run(&z80);
	return 0;
}
