// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "traps.h"
#include "spinlock.h"
#include "file.h"
#include "memlayout.h"
#include "proc.h"
#include "x86.h"
#include "printf.h"

#include <stdarg.h>

static int search = 0;
static int search_len = 0;
static ushort ekran[80*25];
static int trenutnaPozicija;
static char search_buffer[80];
static int results[256];
static int resultat_count = 0;
static int selected = 0;

static void consputc(int);

static int panicked = 0;

static struct {
	struct spinlock lock;
	int locking;
} cons;

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
	int locking;
	va_list args;

	locking = cons.locking;
	if(locking)
		acquire(&cons.lock);

	if (fmt == 0)
		panic("null fmt");

	va_start(args, fmt);

	fnvprintf(consputc, fmt, args);

	va_end(args);

	if(locking)
		release(&cons.lock);
}

void
panic(char *s)
{
	int i;
	uint pcs[10];

	cli();
	cons.locking = 0;
	// use lapiccpunum so that we can call panic from mycpu()
	cprintf("lapicid %d: panic: ", lapicid());
	cprintf(s);
	cprintf("\n");
	getcallerpcs(&s, pcs);
	for(i=0; i<10; i++)
		cprintf(" %p", pcs[i]);
	panicked = 1; // freeze other CPU
	for(;;)
		;
}

#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory

static void
cgaputc(int c)
{
	int pos;

	// Cursor position: col + 80*row.
	outb(CRTPORT, 14);
	pos = inb(CRTPORT+1) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTPORT+1);

	if(c == '\n')
		pos += 80 - pos%80;
	else if(c == BACKSPACE){
		if(pos > 0) --pos;
	} else{
		crt[pos++] = (c&0xff) | 0x0700;  // black on white
	}
	if(pos < 0 || pos > 25*80)
		panic("pos under/overflow");

	if((pos/80) >= (search ? 25 : 24)){  // Scroll up.
		memmove(crt, crt+80, sizeof(crt[0])*23*80);
		pos -= 80;
		memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
	}
	outb(CRTPORT, 14);
	outb(CRTPORT+1, pos>>8);
	outb(CRTPORT, 15);
	outb(CRTPORT+1, pos);
	crt[pos] = ' ' | 0x0700;
	trenutnaPozicija = pos;

}
static int get_cursor(void){
	int pos;
	outb(CRTPORT, 14);
	pos = inb(CRTPORT+1) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTPORT+1);
	return pos;
}
static void set_cursor(int pos){
	outb(CRTPORT, 14);
	outb(CRTPORT+1, pos>>8);
	outb(CRTPORT, 15);
	outb(CRTPORT+1, pos);
}

static void search_menu(){
	int kolona;
	int pocetak_reda = 24*80;

	for(kolona = 0; kolona < 7; kolona++){
		crt[pocetak_reda+kolona] = 0x0200 | "Search:"[kolona];
	}
	for(kolona = 0; kolona < search_len; kolona++){
		crt[pocetak_reda + 7 + kolona] = 0x0700 | search_buffer[kolona];
	}
	set_cursor(pocetak_reda + 7 + search_len);
}
static void remove_search(){
	int kolona;
	int pocetak_reda = 24*80;

	for(kolona = 0; kolona < 7; kolona++){
		crt[pocetak_reda+kolona] = 0x0200 | "Search:"[kolona];
	}
	for(kolona = 0; kolona < search_len; kolona++){
		crt[pocetak_reda + 7 + kolona] = ' ';
	}
	search_len = 0;
	set_cursor(pocetak_reda + 7 + search_len);
}
void save_screen(){
	for (int i = 0; i < 80 * 25; i++) {
		ekran[i] = crt[i];
	}
}
void refresh(){
	for (int i = 0; i < 80 * 25; i++) {
		crt[i] = ekran[i];
	}
}
void pozadina(){
	for(int i = 0; i < 80 * 24; i++){
		crt[i] = (crt[i] & 0x00FF) | 0xF000;
	}
}
static int mala_slova(char c){
	if(c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';
	return c;
}
void oznaci_tekst(){
	resultat_count = 0;
	for(int i = 0; i < 80 * 24; i++){
		int podudaranje = 1;
		for(int j = 0; j < search_len; j++){
			if(mala_slova((crt[i+j] & 0x00FF)) != mala_slova(search_buffer[j])){
				podudaranje = 0;
				break;
			}
		}
		if(podudaranje && resultat_count < 256){
			results[resultat_count++] = i;
		}
	}


	for(int i = 0; i < resultat_count; i++){
		for(int j = 0; j < search_len; j++){
			crt[results[i]+j] = (crt[results[i]+j] & 0x00FF) | 0x5F00;
		}
	}
	selected = 0;
	if(resultat_count > 0){
		for(int j = 0; j < search_len; j++){
			crt[results[selected]+j] = (crt[results[selected]+j] & 0x0FF) | 0x2F00;
		}
	}
}

void ukloni_oznake(){
	for(int i = 0; i < 80 * 24; i++){
		crt[i] = (crt[i] & 0x0FF) | 0xF000;
	}
}

void
consputc(int c)
{
	if(panicked){
		cli();
		for(;;)
			;
	}

	if(c == BACKSPACE){
		uartputc('\b'); uartputc(' '); uartputc('\b');
	} else
		uartputc(c);
	cgaputc(c);
}

#define INPUT_BUF 128
struct {
	char buf[INPUT_BUF];
	uint r;  // Read index
	uint w;  // Write index
	uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x
#define A(x)  (x - 'A' + 128)

void
consoleintr(int (*getc)(void))
{
	int c, doprocdump = 0;

	acquire(&cons.lock);
	while((c = getc()) >= 0){
		switch(c){
			case C('P'):  // Process listing.
				// procdump() locks cons.lock indirectly; invoke later
				if(search == 1)
					break;
			doprocdump = 1;
			break;
			case C('U'):  // Kill line.
				while(input.e != input.w &&
					input.buf[(input.e-1) % INPUT_BUF] != '\n'){
					input.e--;
				consputc(BACKSPACE);
					}
					break;
			case C('H'): case '\x7f':  // Backspace
				if(search == 1){
					if(search_len > 0){
						search_len--;
						search_buffer[search_len] = 0;
						int pocetak_reda = 24*80;
						crt[pocetak_reda + 7 + search_len] = 0x0700 | ' ';
						set_cursor(pocetak_reda + 7 + search_len);
						ukloni_oznake();
						oznaci_tekst();
					}
					break;
				}
				if(input.e != input.w){
					input.e--;
					consputc(BACKSPACE);
				}
				break;
			case A('F'):
				if(search == 0){
					save_screen();
					trenutnaPozicija = get_cursor();
					pozadina();
					search = 1;
					search_menu();
					oznaci_tekst();
				}else{
					search = 0;
					refresh();
					set_cursor(trenutnaPozicija);
				}
				break;
			case A('N'):
				if(search == 1 && resultat_count > 0){
					for(int j = 0; j < search_len; j++){
						crt[results[selected]+j] = (crt[results[selected]+j] & 0x00FF) | 0x5F00;
					}
					selected = (selected + 1) % resultat_count;

					for(int j = 0; j < search_len; j++){
						crt[results[selected]+j] = (crt[results[selected]+j] & 0x00FF) | 0x2F00;
					}
				}
				break;
			case A('P'):
				if(search == 1 && resultat_count > 0){
					for(int j = 0; j < search_len; j++){
						crt[results[selected]+j] = (crt[results[selected]+j] & 0x00FF) | 0x5F00;
					}
					selected = (selected - 1 + resultat_count) % resultat_count;

					for(int j = 0; j < search_len; j++){
						crt[results[selected]+j] = (crt[results[selected]+j] & 0x00FF) | 0x2F00;
					}
				}
				break;
			case A('X'):
				if(search == 1){
					remove_search();
					ukloni_oznake();
				}
				break;
			default:
				if(search == 1){
					if(c >= 32 && c < 128 && search_len < 80 - 7 - 1){
						search_buffer[search_len] = c;
						int pocetak_reda = 24*80;
						crt[pocetak_reda + 7 + search_len] = 0x0700 | c;
						search_len++;
						set_cursor(pocetak_reda + 7 + search_len);
						ukloni_oznake();
						oznaci_tekst();
					}
					break;
				}
				if(c != 0 && input.e-input.r < INPUT_BUF){
					c = (c == '\r') ? '\n' : c;
					input.buf[input.e++ % INPUT_BUF] = c;
					consputc(c);
					if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
						input.w = input.e;
						wakeup(&input.r);
					}
				}
				break;
		}
	}
	release(&cons.lock);
	if(doprocdump) {
		procdump();  // now call procdump() wo. cons.lock held
	}
}

int
consoleread(struct inode *ip, char *dst, int n)
{
	uint target;
	int c;

	iunlock(ip);
	target = n;
	acquire(&cons.lock);
	while(n > 0){
		while(input.r == input.w){
			if(myproc()->killed){
				release(&cons.lock);
				ilock(ip);
				return -1;
			}
			sleep(&input.r, &cons.lock);
		}
		c = input.buf[input.r++ % INPUT_BUF];
		if(c == C('D')){  // EOF
			if(n < target){
				// Save ^D for next time, to make sure
				// caller gets a 0-byte result.
				input.r--;
			}
			break;
		}
		*dst++ = c;
		--n;
		if(c == '\n')
			break;
	}
	release(&cons.lock);
	ilock(ip);

	return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
	int i;

	iunlock(ip);
	acquire(&cons.lock);
	for(i = 0; i < n; i++)
		consputc(buf[i] & 0xff);
	release(&cons.lock);
	ilock(ip);

	return n;
}

void
consoleinit(void)
{
	initlock(&cons.lock, "console");

	devsw[CONSOLE].write = consolewrite;
	devsw[CONSOLE].read = consoleread;
	cons.locking = 1;

	ioapicenable(IRQ_KBD, 0);
}

