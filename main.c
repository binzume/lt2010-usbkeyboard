/**
 *  キー入力テスト
 * @auther binzume
 * @license GPL v2
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "usbdrv.h"



static uchar    reportBuffer[2];    /* buffer for HID reports */
static uchar    idleRate;           /* in 4 ms units */

PROGMEM char usbHidReportDescriptor[35+18] = { /* USB report descriptor */
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x90,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)

    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)

    0xc0                           // END_COLLECTION
};

/* We use a simplifed keyboard report descriptor which does not support the
 * boot protocol. We don't allow setting status LEDs and we only allow one
 * simultaneous key press (except modifiers). We can therefore use short
 * 2 byte input reports.
 * The report descriptor has been created with usb.org's "HID Descriptor Tool"
 * which can be downloaded from http://www.usb.org/developers/hidpage/.
 * Redundant entries (such as LOGICAL_MINIMUM and USAGE_PAGE) have been omitted
 * for the second INPUT item.
 */

/* Keyboard usage values, see usb.org's HID-usage-tables document, chapter
 * 10 Keyboard/Keypad Page for more codes.
 */
#define MOD_CONTROL_LEFT    (1<<0)
#define MOD_SHIFT_LEFT      (1<<1)
#define MOD_ALT_LEFT        (1<<2)
#define MOD_GUI_LEFT        (1<<3)
#define MOD_CONTROL_RIGHT   (1<<4)
#define MOD_SHIFT_RIGHT     (1<<5)
#define MOD_ALT_RIGHT       (1<<6)
#define MOD_GUI_RIGHT       (1<<7)

#define KEY_A       4
#define KEY_B       5
#define KEY_C       6
#define KEY_D       7
#define KEY_E       8
#define KEY_F       9
#define KEY_G       10
#define KEY_H       11
#define KEY_I       12
#define KEY_J       13
#define KEY_K       14
#define KEY_L       15
#define KEY_M       16
#define KEY_N       17
#define KEY_O       18
#define KEY_P       19
#define KEY_Q       20
#define KEY_R       21
#define KEY_S       22
#define KEY_T       23
#define KEY_U       24
#define KEY_V       25
#define KEY_W       26
#define KEY_X       27
#define KEY_Y       28
#define KEY_Z       29
#define KEY_1       30
#define KEY_2       31
#define KEY_3       32
#define KEY_4       33
#define KEY_5       34
#define KEY_6       35
#define KEY_7       36
#define KEY_8       37
#define KEY_9       38
#define KEY_0       39

#define KEY_F1      58
#define KEY_F2      59
#define KEY_F3      60
#define KEY_F4      61
#define KEY_F5      62
#define KEY_F6      63
#define KEY_F7      64
#define KEY_F8      65
#define KEY_F9      66
#define KEY_F10     67
#define KEY_F11     68
#define KEY_F12     69

#define KEY_ENTER     0x28
#define KEY_ESC     0x29
#define KEY_BS     0x2A
#define KEY_TAB     0x2B

static const uchar  symbol[32][2] PROGMEM = {
        {0, 0x2c}, // ' '
        {MOD_SHIFT_LEFT, 0x1e}, // '!'
        {MOD_SHIFT_LEFT, 0x1f}, // '"'
        {MOD_SHIFT_LEFT, 0x20}, // '#'
        {MOD_SHIFT_LEFT, 0x21}, // '$'
        {MOD_SHIFT_LEFT, 0x22}, // '%'
        {MOD_SHIFT_LEFT, 0x23}, // '&'
        {MOD_SHIFT_LEFT, 0x24}, // '''
        {MOD_SHIFT_LEFT, 0x25}, // '('
        {MOD_SHIFT_LEFT, 0x26}, // ')'
        {MOD_SHIFT_LEFT, 0x34}, // '*'
        {MOD_SHIFT_LEFT, 0x33}, // '+'
        {0, 0x36}, // ','
        {0, 0x2d}, // '-'
        {0, 0x37}, // '.'
        {0, 0x38}, // '/'

        {0, KEY_0}, // '0'
        {0, KEY_1}, // '1'
        {0, KEY_2}, // '1'
        {0, KEY_3}, // '1'
        {0, KEY_4}, // '1'
        {0, KEY_5}, // '1'
        {0, KEY_6}, // '1'
        {0, KEY_7}, // '1'
        {0, KEY_8}, // '1'
        {0, KEY_9}, // '1'
        {0, 0x34}, // ':'
        {0, 0x33}, // ';'
        {MOD_SHIFT_LEFT, 0x36}, // '<'
        {MOD_SHIFT_LEFT, 0x2d}, // '='
        {MOD_SHIFT_LEFT, 0x37}, // '>'
        {MOD_SHIFT_LEFT, 0x38}, // '?'
};


prog_char pagedata_0[] PROGMEM = "\x05r\anotepad\n\a" "\04of\t\t5\n\a"
    "var f='test01.js -',args=WScript.Arguments,ws=new ActiveXObject('WScript.Shell');\n"
    "if(args.count()==0){"
    "ws.SendKeys('{CAPSLOCK}');\n"
    "var ie=new ActiveXObject('InternetExplorer.Application');\n"
    "ie.Navigate('about:blank');while(ie.Busy)WScript.Sleep(5);\n"
    "var iew=ie.Document.parentWindow;\n"
    "function cbget(){try{return iew.clipboardData.getData('text')}catch(e){}}\n"
    "function cbset(s){try{iew.clipboardData.setData('text',s)}catch(e){}}\n"
    "ws.run('wscript.exe \"'+WScript.ScriptFullName+'\" a');\n"

    "cbset('*');WScript.Sleep(500);ws.SendKeys('%{tab}');WScript.Sleep(100);ws.AppActivate(f);ws.SendKeys('{CAPSLOCK}');\n"
    "for(;;){WScript.Sleep(50);s = cbget();if(s=='end' || s=='')break;if(s=='*') continue;\n"
    "cbset(decodeURI(s));ws.AppActivate(f);ws.SendKeys('^a^v');WScript.Sleep(10);cbset('*');}ie.Quit();\n"
    "}else{for(i=0;i<500;i++){WScript.Sleep(9);if(ws.AppActivate('Internet Explorer')){ws.SendKeys('a');}}}\n"

    "\x03s\a\a%TEMP%\\test01.js\n\ay\a"
    "\x05r\a%TEMP%\\test01.js\n"

    "\a\a"

    "\03a ready.\03a\03c";
prog_char pagedata_end[] PROGMEM =
     "\04of\t\t9\n\a\04 r" "\03a\aend\03a\a\03c";


prog_char pagedata_1[] PROGMEM =
    "\03a\n\n USBデバイスを作ってみた\n\n   研開６セク 川平航介\n        (@binzume)\03a\03c\a"
    "\04 x\a\04of\t\t50\n\a" // 最大化してフォントの設定
    ;
prog_char pagedata_2[] PROGMEM =
    "\03a\n自己紹介\n\n・名前:川平航介\n・所属:研究開発部\n       第六開発セクション\n・2009年入社\n  13Fにいます\03a\03c\a";
prog_char pagedata_3[] PROGMEM =
    "\03a\n\n  id: binzume\n\n  好きな言語: C++/Perl\n  好きになれない言語:PHP\03a\03c";

prog_char pagedata_4[] PROGMEM =
    "\03a\nUSBデバイスを作りたい\03a\03c";

prog_char pagedata_5[] PROGMEM =
    "\n・近頃のPCと周辺機器の\n   インターフェイスは複雑\03a\03c";

prog_char pagedata_6[] PROGMEM =
    "\n・昔は良かった…\n   ISAバス, パラレルポート\03a\03c";

prog_char pagedata_7[] PROGMEM =
    "\03a\n\n  USBなら今後も安心\03a\03c\a";

PGM_P  pagedata[] = {
    pagedata_0, pagedata_1,pagedata_2, pagedata_3, pagedata_4, pagedata_5, pagedata_6, pagedata_7, pagedata_end,
};




uint16_t sendpos=0;
uint8_t strstate=0;
uint8_t page=0;
static uint8_t buildReport2(uchar key)
{
    reportBuffer[0] = 0;
    reportBuffer[1] = 0;
    uint8_t optkey = 0;

    uint8_t c = pgm_read_byte(&pagedata[page][sendpos]);
    if (c==0x03) {
        ++sendpos;
        optkey |= MOD_CONTROL_LEFT;
        c = pgm_read_byte(&pagedata[page][sendpos]);
    }
    if (c==0x04) {
        ++sendpos;
        optkey |= MOD_ALT_LEFT;
        c = pgm_read_byte(&pagedata[page][sendpos]);
    }
    if (c==0x05) {
        ++sendpos;
        optkey |= MOD_GUI_LEFT;
        c = pgm_read_byte(&pagedata[page][sendpos]);
    }

    // 全角文字はエンコード
    if (strstate) {
        if (strstate==2) {
            c>>=4;
        } else {
            c&=0xf;
        }
        c+= (c<10) ? '0' : ('A'-10);
        strstate--;
    }
    if (c>=0x80) {
        strstate = 2;
        c='%';
    }
    if (c==0) return 0;


    if (c=='\n') {
        reportBuffer[1] = KEY_ENTER;
    } else if (c=='\t') {
        reportBuffer[1] = KEY_TAB;
    } else if (c>='a' && c<='z') {
        reportBuffer[1] = c-'a'+KEY_A;
    } else if (c>='A' && c<='Z') {
        reportBuffer[0] = MOD_SHIFT_LEFT;
        reportBuffer[1] = c-'A'+KEY_A;
    } else if (c>=' ' && c<='?') {
        *(int*)reportBuffer = pgm_read_word(&symbol[c-' ']);
    } else if (c=='[') {
        reportBuffer[1] = 0x30;
    } else if (c==']') {
        reportBuffer[1] = 0x32;
    } else if (c=='{') {
        reportBuffer[0] = MOD_SHIFT_LEFT;
        reportBuffer[1] = 0x30;
    } else if (c=='}') {
        reportBuffer[0] = MOD_SHIFT_LEFT;
        reportBuffer[1] = 0x32;
    } else if (c=='\\') {
        reportBuffer[1] = 0x87;
    } else if (c=='^') {
        reportBuffer[1] = 0x2e;
    } else if (c=='|') {
        reportBuffer[0] = MOD_SHIFT_LEFT;
        reportBuffer[1] = 0x89;
    }
    if (!strstate) {
        sendpos++;
    }
    reportBuffer[0] |= optkey;
    return c;
}

volatile uint8_t led=0xff;


uchar   usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    usbMsgPtr = reportBuffer;
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
          //  buildReport(keyPressed());
            return sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_SET_REPORT){
            if (rq->wLength.word == 1) {
                // led = 1;
            }
            return USB_NO_MSG;
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
    led = data[0]&2; // 2:CapsLock
    return 1;
}


// 指定された時間だけ待つ
void delay_ms(uint16_t w){
    while (w-->0) _delay_ms(1);
}

/* ------------------------------------------------------------------------- */

int main(void)
{
    DDRB = 0x04;
    PCMSK0=1;

    DDRD = 0xff;  // LCD

    DDRC = 0x00;
    PORTC = 0x38;

    delay_ms(10);
//  while(PINC&0x08);

//  wdt_enable(WDTO_2S);
    PORTB |= 0x04; //USB ENABLE
    usbInit();
    sei();
    uint16_t count=0;
    uint16_t count2=300;
    static uchar nullReport[2]={0,0};

    reportBuffer[0]=0;
    reportBuffer[1]=0;
    uchar old=1,ret;

    for(;;){    /* main event loop */
//      wdt_reset();
        usbPoll();

        if(usbInterruptIsReady()){
            /* use last key and not current key status in order to avoid lost
               changes in key status. */
            if ((count&0x1f)==0x00 && led==0) {
                if (count2) {
                    count2--;
                    usbSetInterrupt(nullReport, sizeof(reportBuffer));
                } else {
                    if (old == reportBuffer[1]) {
                        usbSetInterrupt(nullReport, sizeof(reportBuffer));
                        old = 0xff;
                    } else {
                        usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
                        old = reportBuffer[1];
                        ret = buildReport2(1);
                        if (ret=='\a') {
                            count2=100;
                        } else if (ret=='\0') {
                            if (page<sizeof(pagedata)/sizeof(PGM_P) && (PINC&0x08)==0) {
                                page++;
                                sendpos=0;
                            }
                            if (page>0 && (PINC&0x10)==0) {
                                page--;
                                sendpos=0;
                            }
                        }
                    }

                }
            }
            count++;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
