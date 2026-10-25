/* "Тера Кооп" (teracoop) v0.9 (the actions of the friend go over the network: the guest sends ACT packets, the host puts them onto player 2); v0.8 (shows what the local player 1 is doing - walking axes and buttons read by the game module - and has a "mirror" test button: player 2 repeats player 1 on the same PC); v0.6 (the program may sit right in the game folder next to the mod and the uninstaller: no steam_appid.txt is ever written or deleted, own folder TeraCoop_logs; the installer just writes dinput8.dll, no backup/restore of a foreign one); v0.5 (steam_api.dll inside the exe, AppID 480 set by the program itself, --install-mod / --remove-mod for the MSI installer; v0.4: embeds the game module as a resource, talks to it over shared memory; v0.3: dark owner-drawn look, two screens: connect / in the room with nicks and ping; invites from the Steam friends menu; log hidden by default): window program for the LEGO LOTR co-op mod. Separate program, NOT part of the game, never started by the game.
 * Part 1 of the roadmap: window instead of console, Unicode everywhere (no broken letters), short room code, Steam friends with invite, second way "By IP" (Radmin VPN).
 * Steam way = repo pattern playbook/networking-steam-spacewar.md (Spacewar 480, modern flat steam_api.dll next to this program, manual callback dispatch, ISteamNetworkingMessages).
 * IP way = plain UDP between two computers (host listens on port 27015); the messages are the same PING / PONG as in the Steam way.
 * It does not touch the game or its files. Log: TeraCoop_logs\teracoop_DATE_TIME.log next to the program (one file per run). */
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef int i32; typedef unsigned long long u64; typedef void *PV;
#define K __declspec(dllimport) __stdcall
#include "link.h"
/* kernel32 */
K PV GetModuleHandleW(const u16 *);
K u32 GetModuleFileNameW(PV, u16 *, u32);
K PV LoadLibraryW(const u16 *);
K PV GetProcAddress(PV, const char *);
K PV CreateFileW(const u16 *, u32, u32, PV, u32, u32, PV);
K int MoveFileW(const u16 *, const u16 *);
K int WriteFile(PV, const void *, u32, u32 *, PV);
K u32 SetFilePointer(PV, i32, i32 *, u32);
K int CloseHandle(PV);
K int CreateDirectoryW(const u16 *, PV);
K int SetCurrentDirectoryW(const u16 *);
K u32 GetFileAttributesW(const u16 *);
K u32 GetTickCount(void);
K void GetLocalTime(void *);
K void GetSystemTime(void *);
K int MultiByteToWideChar(u32, u32, const char *, i32, u16 *, i32);
K int WideCharToMultiByte(u32, u32, const u16 *, i32, char *, i32, const char *, int *);
K PV GlobalAlloc(u32, u32);
K PV GlobalLock(PV);
K int GlobalUnlock(PV);
K void ExitProcess(u32);
K PV CreateFileMappingW(PV, PV, u32, u32, u32, const u16 *);
K PV MapViewOfFile(PV, u32, u32, u32, u32);
K PV FindResourceW(PV, const u16 *, const u16 *);
K PV LoadResource(PV, PV);
K PV LockResource(PV);
K u32 SizeofResource(PV, PV);
K const u16 *GetCommandLineW(void);
K int SetEnvironmentVariableW(const u16 *, const u16 *);
K u32 GetEnvironmentVariableW(const u16 *, u16 *, u32);
K int DeleteFileW(const u16 *);
K int RemoveDirectoryW(const u16 *);
K int CopyFileW(const u16 *, const u16 *, int);
K int MoveFileExW(const u16 *, const u16 *, u32);
K int ReadFile(PV, void *, u32, u32 *, PV);
K u32 GetFileSize(PV, u32 *);
K PV FindFirstFileW(const u16 *, void *);
K int FindNextFileW(PV, void *);
K int FindClose(PV);
/* user32 */
K u16 RegisterClassExW(const void *);
K PV CreateWindowExW(u32, const u16 *, const u16 *, u32, i32, i32, i32, i32, PV, PV, PV, PV);
K i32 DefWindowProcW(PV, u32, u32, i32);
K int GetMessageW(void *, PV, u32, u32);
K int TranslateMessage(const void *);
K i32 DispatchMessageW(const void *);
K void PostQuitMessage(i32);
K int ShowWindow(PV, i32);
K int UpdateWindow(PV);
K i32 SendMessageW(PV, u32, u32, i32);
K int SetWindowTextW(PV, const u16 *);
K int GetWindowTextW(PV, u16 *, i32);
K i32 GetWindowTextLengthW(PV);
K u32 SetTimer(PV, u32, u32, PV);
K PV LoadCursorW(PV, const u16 *);
K PV LoadIconW(PV, const u16 *);
K int EnableWindow(PV, int);
K PV GetFocus(void);
K int IsDialogMessageW(PV, void *);
K int OpenClipboard(PV);
K int CloseClipboard(void);
K int EmptyClipboard(void);
K PV SetClipboardData(u32, PV);
K PV GetDC(PV);
K int ReleaseDC(PV, PV);
K int AdjustWindowRectEx(void *, u32, int, u32);
K int DestroyWindow(PV);
K i32 MessageBoxW(PV, const u16 *, const u16 *, u32);
K int MoveWindow(PV, i32, i32, i32, i32, int);
K int SetWindowPos(PV, PV, i32, i32, i32, i32, u32);
K int InvalidateRect(PV, const void *, int);
K PV BeginPaint(PV, void *);
K int EndPaint(PV, const void *);
K int GetClientRect(PV, void *);
K int FillRect(PV, const void *, PV);
K int DrawTextW(PV, const u16 *, i32, void *, u32);
K int GetCursorPos(void *);
K PV WindowFromPoint(i32, i32);
K int DrawIconEx(PV, i32, i32, PV, i32, i32, u32, PV, u32);
K PV LoadImageW(PV, const u16 *, u32, i32, i32, u32);
/* gdi32 */
K PV CreateFontW(i32, i32, i32, i32, i32, u32, u32, u32, u32, u32, u32, u32, u32, const u16 *);
K PV GetStockObject(i32);
K i32 GetDeviceCaps(PV, i32);
K PV CreateSolidBrush(u32);
K PV CreatePen(i32, i32, u32);
K PV SelectObject(PV, PV);
K int DeleteObject(PV);
K int RoundRect(PV, i32, i32, i32, i32, i32, i32);
K int Ellipse(PV, i32, i32, i32, i32);
K i32 SetBkMode(PV, i32);
K u32 SetTextColor(PV, u32);
K u32 SetBkColor(PV, u32);
K PV CreateCompatibleDC(PV);
K PV CreateCompatibleBitmap(PV, i32, i32);
K int BitBlt(PV, i32, i32, i32, i32, PV, i32, i32, u32);
K int DeleteDC(PV);
/* ws2_32 */
K i32 WSAStartup(u16, void *);
K u32 socket(i32, i32, i32);
K i32 bind(u32, const void *, i32);
K i32 sendto(u32, const void *, i32, i32, const void *, i32);
K i32 recvfrom(u32, void *, i32, i32, void *, i32 *);
K i32 ioctlsocket(u32, i32, u32 *);
K i32 closesocket(u32);
K i32 gethostname(char *, i32);
K void *gethostbyname(const char *);

int _fltused = 1;
void *memcpy(void *d, const void *s, u32 n) { u8 *a = d; const u8 *b = s; while (n--) *a++ = *b++; return d; }
void *memset(void *d, int c, u32 n) { u8 *a = d; while (n--) *a++ = (u8)c; return d; }

/* ---------------- ids ---------------- */
enum { ID_R_STEAM = 101, ID_R_IP = 102, ID_CREATE = 110, ID_COPY = 112, ID_JOINED = 121, ID_JOIN = 122,
       ID_LEAVE = 141, ID_LOG = 150, ID_LOGBTN = 151, ID_T_JOIN = 160, ID_T_RIGHT = 161, ID_T_JUMP = 162, ID_T_ACT = 163, ID_T_REC = 164 };

static PV hInst, hMain, hFont, hFontB, hFontT, hFontS, hFontC, hLogFile;
static PV C_STEAM, C_IP, C_CREATE, C_COPY, C_JOINED, C_JOIN, C_LEAVE, C_LOG, C_LOGBTN, C_T1, C_T2, C_T3, C_T4, C_T5;
static PV btns[16]; static int nbtns; static PV hover_wnd;
static PV brBG, brCard, brCard2;
static u32 dpi = 96, t0;
#define S(x) ((i32)(((x) * (i32)dpi + 48) / 96))
/* colors (COLORREF = 0x00BBGGRR): dark brown + gold */
#define RGBc(r, g, b) ((u32)(r) | ((u32)(g) << 8) | ((u32)(b) << 16))
#define COL_BG RGBc(24, 22, 20)
#define COL_HEAD RGBc(32, 29, 25)
#define COL_CARD RGBc(38, 35, 31)
#define COL_CARD2 RGBc(26, 24, 21)
#define COL_LINE RGBc(70, 63, 52)
#define COL_GOLD RGBc(214, 170, 74)
#define COL_GOLDHI RGBc(236, 198, 112)
#define COL_GOLDDK RGBc(32, 25, 10)
#define COL_TEXT RGBc(238, 232, 216)
#define COL_MUTED RGBc(156, 147, 128)
#define COL_GREEN RGBc(112, 196, 118)
#define COL_RED RGBc(222, 96, 84)
#define COL_BTN2 RGBc(60, 55, 47)
#define COL_BTN2HI RGBc(78, 71, 60)
/* layout (96-dpi units) */
#define LAY_W 560
#define H_BASE 384
#define H_LOG 580

/* ---------------- wide text builder ---------------- */
static u16 wb[700]; static int wl;
static void wr(void) { wl = 0; wb[0] = 0; }
static void ws(const u16 *s) { while (*s && wl < 660) wb[wl++] = *s++; wb[wl] = 0; }
static void wa(const char *s) { while (*s && wl < 660) wb[wl++] = (u8)*s++; wb[wl] = 0; }
static void wd(u32 v) { char t[12]; int n = 0; do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v); while (n && wl < 660) wb[wl++] = (u16)t[--n]; wb[wl] = 0; }
static void wd64(u64 v) { /* decimal without 64-bit division: long division on four 16-bit pieces */
  u32 w[4], q[4], rem, i; char t[24]; int n = 0;
  w[0] = (u32)(v >> 48) & 0xffff; w[1] = (u32)(v >> 32) & 0xffff; w[2] = (u32)(v >> 16) & 0xffff; w[3] = (u32)v & 0xffff;
  for (;;) { int zero = 1; rem = 0; for (i = 0; i < 4; i++) { u32 cur = rem * 65536u + w[i]; q[i] = cur / 10u; rem = cur % 10u; if (q[i]) zero = 0; } t[n++] = (char)('0' + rem); for (i = 0; i < 4; i++) w[i] = q[i]; if (zero) break; }
  while (n && wl < 660) wb[wl++] = (u16)t[--n]; wb[wl] = 0;
}
static void wutf8(const char *s) { int n; if (!s) return; n = MultiByteToWideChar(65001, 0, s, -1, wb + wl, 660 - wl); if (n > 0) wl += n - 1; wb[wl] = 0; }

static void wi(i32 v) { if (v < 0) { if (wl < 660) wb[wl++] = '-'; wb[wl] = 0; wd((u32)(-v)); } else wd((u32)v); }
/* log: window + file (UTF-8). log_nowin = 1: only into the file (the detailed lines would flood the window) */
static int log_nowin;
static u16 logdir[470], logstamp[28], logcur[500]; static u32 logrole;
static void log_name(u16 *out, u32 role) { /* <logdir><solo|host|guest>_program_<stamp>.log */
  const u16 *r = role == 1 ? L"host" : role == 2 ? L"guest" : L"solo", *kind = L"_program_"; int n = 0, i;
  for (i = 0; logdir[i]; i++) out[n++] = logdir[i]; for (i = 0; r[i]; i++) out[n++] = r[i]; for (i = 0; kind[i]; i++) out[n++] = kind[i];
  for (i = 0; logstamp[i]; i++) out[n++] = logstamp[i]; out[n++] = '.'; out[n++] = 'l'; out[n++] = 'o'; out[n++] = 'g'; out[n] = 0;
  for (i = 0; i <= n; i++) logcur[i] = out[i];
}
static void log_set_role(u32 role) { u16 old[500], np[500]; int i; if (!role || logrole || !hLogFile || hLogFile == (PV)-1) return; for (i = 0; logcur[i]; i++) old[i] = logcur[i]; old[i] = 0; logrole = role; log_name(np, role); if (!MoveFileW(old, np)) { for (i = 0; old[i]; i++) logcur[i] = old[i]; logcur[i] = 0; } }
static void log_line(void) {
  u32 w; char u[1500]; int n, i; i32 len; u16 line[720];
  for (i = 0; i < wl; i++) line[i] = wb[i];
  line[wl] = '\r'; line[wl + 1] = '\n'; line[wl + 2] = 0;
  if (C_LOG && !log_nowin) {
    len = GetWindowTextLengthW(C_LOG); if (len > 30000) { SetWindowTextW(C_LOG, L""); len = 0; }
    SendMessageW(C_LOG, 0xB1 /* EM_SETSEL */, (u32)len, len); SendMessageW(C_LOG, 0xC2 /* EM_REPLACESEL */, 0, (i32)(u32)line); SendMessageW(C_LOG, 0xB7 /* EM_SCROLLCARET */, 0, 0);
  }
  if (hLogFile != (PV)-1 && hLogFile) {
    char pre[48]; u32 v = (GetTickCount() - t0) / 1000u; int pl = 0, k = 0; char t[12]; u16 st[8];
    do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v); while (k) pre[pl++] = t[--k]; pre[pl++] = ' '; pre[pl++] = 's'; pre[pl++] = ' ';
    GetSystemTime(st); pre[pl++] = '['; pre[pl++] = 'U'; pre[pl++] = 'T'; pre[pl++] = 'C'; pre[pl++] = ' ';
    pre[pl++] = (char)('0' + (st[4] / 10) % 10); pre[pl++] = (char)('0' + st[4] % 10); pre[pl++] = ':'; pre[pl++] = (char)('0' + (st[5] / 10) % 10); pre[pl++] = (char)('0' + st[5] % 10); pre[pl++] = ':'; pre[pl++] = (char)('0' + (st[6] / 10) % 10); pre[pl++] = (char)('0' + st[6] % 10);
    pre[pl++] = ']'; pre[pl++] = ' '; pre[pl++] = ' ';
    n = WideCharToMultiByte(65001, 0, line, wl + 2, u, 1400, 0, 0);
    WriteFile(hLogFile, pre, (u32)pl, &w, 0); if (n > 0) WriteFile(hLogFile, u, (u32)n, &w, 0);
  }
}
static void say_ru(const u16 *s) { wr(); ws(s); log_line(); }
static void log_file(void) { log_nowin = 1; log_line(); log_nowin = 0; }
static u16 status_txt[420], code_txt[64], peer_name[64];
static void redraw(void) { if (hMain) InvalidateRect(hMain, 0, 0); }
static void set_status(const u16 *s) { int i; for (i = 0; s[i] && i < 410; i++) status_txt[i] = s[i]; status_txt[i] = 0; redraw(); }
static void set_code(const u16 *s) { int i; for (i = 0; s[i] && i < 60; i++) code_txt[i] = s[i]; code_txt[i] = 0; redraw(); }

/* ---------------- state ---------------- */
enum { M_STEAM = 0, M_IP = 1 };
static int mode = M_STEAM;
static int in_room, is_host, peer_present, steam_ready, steam_tried;
static u32 last_ping, ping_seq, last_rtt, peer_seen;

/* ---------------- steam (flat api, cdecl) ---------------- */
typedef int (__cdecl *F_InitFlat)(char *);
typedef void (__cdecl *F_v)(void);
typedef int (__cdecl *F_pipe)(void);
typedef void (__cdecl *F_pipev)(int);
typedef u8 (__cdecl *F_next)(int, void *);
typedef u8 (__cdecl *F_result)(int, u64, void *, int, int, u8 *);
typedef u32 (__cdecl *F_get)(void);
typedef u64 (__cdecl *F_id)(u32);
typedef const char *(__cdecl *F_name)(u32);
typedef u64 (__cdecl *F_create)(u32, int, int);
typedef u64 (__cdecl *F_join)(u32, u64);
typedef u8 (__cdecl *F_setdata)(u32, u64, const char *, const char *);
typedef const char *(__cdecl *F_getdata)(u32, u64, const char *);
typedef void (__cdecl *F_leave)(u32, u64);
typedef int (__cdecl *F_nummem)(u32, u64);
typedef u64 (__cdecl *F_owner)(u32, u64);
typedef u64 (__cdecl *F_memidx)(u32, u64, int);
typedef void (__cdecl *F_setid)(void *, u64);
typedef u64 (__cdecl *F_getid64)(const void *);
typedef int (__cdecl *F_send)(u32, const void *, const void *, u32, int, int);
typedef int (__cdecl *F_recv)(u32, int, void **, int);
typedef u8 (__cdecl *F_accept)(u32, const void *);
typedef void (__cdecl *F_rel)(void *);
typedef const char *(__cdecl *F_fname)(u32, u64);

static PV sdll;
static u32 mm, usr, fr, nm; static u64 myid, lobby; static u32 lobby_set, entered, joining;
static F_setdata fSetData; static F_getdata fGetData; static F_nummem fNumMem; static F_leave fLeave; static F_owner fOwner; static F_memidx fMemIdx;
static F_setid fSetId; static F_getid64 fGetId64; static F_send fSend; static F_recv fRecv; static F_accept fAccept; static F_rel fRel;
static F_fname fFName; static F_name fMyName; static F_create fCreate; static F_join fJoin; static F_v fShutdown;
static F_pipe fGetPipe; static F_pipev fRunFrame, fFree; static F_next fNext; static F_result fResult;
static u64 peer_steam;
typedef u8 (__cdecl *F_setrp)(u32, const char *, const char *); typedef void (__cdecl *F_clrrp)(u32); static F_setrp fSetRP; static F_clrrp fClrRP;

static u64 rd64(const u8 *p) { return (u64)(*(const u32 *)p) | ((u64)(*(const u32 *)(p + 4)) << 32); }

/* ---------------- short room code ---------------- */
#define LOBBY_HI 0x0186000000000000ull
static const char ALPH[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
static void make_code(u64 id) { /* "XXXX-XXX" (low 32 bits) or the full number if the high part is unusual */
  wr();
  if ((id & 0xFFFFFFFF00000000ull) == LOBBY_HI) {
    u32 low = (u32)id; char c[7]; int i;
    for (i = 6; i >= 0; i--) { c[i] = ALPH[low & 31]; low >>= 5; }
    for (i = 0; i < 7; i++) { if (i == 4) wb[wl++] = '-'; wb[wl++] = (u16)(u8)c[i]; }
    wb[wl] = 0;
  } else wd64(id);
}
static int parse_code(const u16 *t, u64 *out) {
  char c[40]; int n = 0, i, alldig = 1; u64 v = 0;
  for (i = 0; t[i] && n < 39; i++) {
    u16 ch = t[i]; if (ch >= 'a' && ch <= 'z') ch = (u16)(ch - 32);
    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z')) { c[n++] = (char)ch; if (ch < '0' || ch > '9') alldig = 0; }
  }
  if (n >= 10 && alldig) { for (i = 0; i < n; i++) v = (v << 3) + (v << 1) + (u64)(c[i] - '0'); *out = v; return 1; }
  if (n == 7) {
    u32 low = 0;
    for (i = 0; i < 7; i++) { int k, idx = -1; for (k = 0; k < 32; k++) if (ALPH[k] == c[i]) { idx = k; break; } if (idx < 0) return 0; if (i == 0 && idx > 3) return 0; low = (low << 5) | (u32)idx; }
    *out = LOBBY_HI | low; return 1;
  }
  return 0;
}

/* ---------------- messages (same for steam and ip) ---------------- */
#define TAG_PING 0x474E4950u
#define TAG_PONG 0x474E4F50u
#define TAG_HELO 0x4F4C4548u
#define TAG_ACT 0x20544341u /* "ACT ": the actions of the friend. seq = running number; tick = packed: ax (signed byte), ay (signed byte), buttons (BT_*), valid flag in the top byte */
#define TAG_PXZ 0x205A5850u /* "PXZ ": where the sender stands: x and z (float bits in the two words after the tag) */
#define TAG_PYA 0x20415950u /* "PYA ": height y and heading yaw (float bits) */
#define TAG_BTN 0x204E5442u /* "BTN ": the buttons the sender holds right now. w1 = BT_* bits (action, special, jump), w2 = running number (older ones are dropped) */
static u32 rx_btn, rx_btn_seq, rx_btn_tick, rx_btn_have, dbg_tx_btn, dbg_rx_btn, dbg_rx_btn_old;
static void net_got_btn(u32 b, u32 seq) { u32 now = GetTickCount(); if (!now) now = 1; dbg_rx_btn++; if (rx_btn_have && (i32)(seq - rx_btn_seq) <= 0 && (i32)(now - rx_btn_tick) < 1000) { dbg_rx_btn_old++; return; } rx_btn_have = 1; rx_btn = b; rx_btn_seq = seq; rx_btn_tick = now; }
static u32 rx_xz_tick, rx_ya_tick, rx_xb, rx_zb, rx_yb, rx_yawb, dbg_tx_pos, dbg_rx_pos;
static void net_got_pos(u32 tag, u32 a, u32 b) { u32 now = GetTickCount(); if (!now) now = 1; if (tag == TAG_PXZ) { rx_xb = a; rx_zb = b; rx_xz_tick = now; } else { rx_yb = a; rx_yawb = b; rx_ya_tick = now; } }
/* part 5.5, enemies: four message kinds, tag = (kind << 16) | enemy id. ENA = x and z (millimetres), ENB = y and heading (millimetres / milliradians), ENI = spawn x and z, ENS = flags (EN_VALID / EN_ALIVE) and type | epoch << 16.
 * The host sends them (ENA / ENB ten times a second without waiting for delivery, ENS on every change and twice a second, ENI once a second), the guest collects them in enr[] and hands them to the module. */
#define EN_KIND_A 0x4E45u
#define EN_KIND_B 0x4E46u
#define EN_KIND_I 0x4E49u
#define EN_KIND_S 0x4E53u
static EnPkt enr[EN_MAX]; static u32 en_dirty, dbg_tx_en, dbg_rx_en;
static int is_en_tag(u32 tag) { u32 h = tag >> 16; return (h == EN_KIND_A || h == EN_KIND_B || h == EN_KIND_I || h == EN_KIND_S) && (tag & 0xFFFFu) < EN_MAX; }
static void net_got_en(u32 tag, u32 a, u32 b) {
  u32 id = tag & 0xFFFFu, h = tag >> 16, now = GetTickCount(); EnPkt *e; if (id >= EN_MAX) return; e = &enr[id]; dbg_rx_en++;
  if (h == EN_KIND_A) { e->x = (i32)a; e->z = (i32)b; e->tick = now ? now : 1; } else if (h == EN_KIND_B) { e->y = (i32)a; e->yaw = (i32)b; }
  else if (h == EN_KIND_I) { e->sx = (i32)a; e->sz = (i32)b; } else { e->flags = a; e->type_epoch = b; }
  en_dirty = 1;
}
static u32 net_seq, net_have, net_tick, net_pack, net_sent_seq, net_last_send, net_tx_on, net_rx_on;
static void net_got_act(u32 seq, u32 packed) { /* host side: remember the newest actions from the friend (older / repeated packets are dropped) */
  u32 now = GetTickCount();
  if (net_have && (i32)(seq - net_seq) <= 0 && (i32)(now - net_tick) < 1000) return;
  net_have = 1; net_seq = seq; net_pack = packed; net_tick = now;
}
static u32 dbg_tx_act, dbg_tx_err, dbg_tx_res, dbg_rx_act, dbg_rx_rej, dbg_rx_ping, dbg_rx_pong, dbg_rx_other, dbg_last;
static int steam_send_f(u64 id, u32 tag, u32 seq, u32 tick, i32 flags) {
  u8 ident[160]; u32 pl[3]; int r; memset(ident, 0, sizeof ident); fSetId(ident, id); pl[0] = tag; pl[1] = seq; pl[2] = tick;
  r = fSend(nm, ident, pl, 12, flags, 0);
  if (tag == TAG_ACT) { dbg_tx_act++; if (r != 1) { dbg_tx_err++; dbg_tx_res = (u32)r; } }
  return r;
}
static int steam_send_to(u64 id, u32 tag, u32 seq, u32 tick) { return steam_send_f(id, tag, seq, tick, 9 /* reliable + no nagle */); }
/* udp */
#define IP_PORT 27015
static u32 usock = (u32)-1; static u8 peer_addr[16]; static int peer_addr_set, ip_joining; static u32 ip_hello_t, ip_join_t0;
static u16 bswap16(u16 v) { return (u16)((v << 8) | (v >> 8)); }
static int ip_open(u16 port) {
  u8 wsa[512]; u8 sa[16]; u32 one = 1;
  if (WSAStartup(0x0202, wsa) != 0) return 0;
  usock = socket(2, 2, 17); if (usock == (u32)-1) return 0;
  ioctlsocket(usock, (i32)0x8004667E, &one);
  memset(sa, 0, 16); *(u16 *)sa = 2; *(u16 *)(sa + 2) = bswap16(port);
  if (bind(usock, sa, 16) != 0) { closesocket(usock); usock = (u32)-1; return 0; }
  return 1;
}
static void ip_close(void) { if (usock != (u32)-1) { closesocket(usock); usock = (u32)-1; } peer_addr_set = 0; ip_joining = 0; }
static void ip_send(u32 tag, u32 seq, u32 tick) { u32 pl[3]; if (usock == (u32)-1 || !peer_addr_set) return; pl[0] = tag; pl[1] = seq; pl[2] = tick; sendto(usock, pl, 12, 0, peer_addr, 16); }
/* HELO also carries our nick (UTF-8, only if Steam is connected) so the other side can show it next to the ping */
static void ip_send_helo(void) {
  u8 pl[64]; u32 *h = (u32 *)pl; int n = 12, i; if (usock == (u32)-1 || !peer_addr_set) return;
  h[0] = TAG_HELO; h[1] = 0; h[2] = 0;
  if (steam_ready && fMyName) { const char *nm8 = fMyName(fr); for (i = 0; nm8 && nm8[i] && i < 40; i++) pl[n++] = (u8)nm8[i]; /* may cut a letter in half: the receiver tolerates it */ }
  sendto(usock, pl, n, 0, peer_addr, 16);
}
/* the best address of this PC to show: Radmin VPN starts with 26. */
static int ip_my_addresses(u8 *best, int *radmin) {
  char host[256]; u8 *he; u8 **list; int i, have = 0; *radmin = 0;
  if (gethostname(host, 255) != 0) return 0;
  he = gethostbyname(host); if (!he) return 0;
  list = *(u8 ***)(he + 12);
  for (i = 0; list && list[i] && i < 12; i++) {
    u8 *a = list[i];
    if (a[0] == 26) { best[0] = a[0]; best[1] = a[1]; best[2] = a[2]; best[3] = a[3]; *radmin = 1; return 1; }
    if (!have && a[0] != 127 && !(a[0] == 169 && a[1] == 254)) { best[0] = a[0]; best[1] = a[1]; best[2] = a[2]; best[3] = a[3]; have = 1; }
  }
  return have;
}
static int parse_ip(const u16 *t, u8 *a4, u16 *port) {
  int i = 0, part = 0; u32 v = 0; int digits = 0; u32 p = 0; int hasport = 0;
  while (t[i] == ' ') i++;
  for (; t[i]; i++) {
    u16 ch = t[i];
    if (ch >= '0' && ch <= '9') { if (hasport) { p = p * 10 + (ch - '0'); if (p > 65535) return 0; } else { v = v * 10 + (ch - '0'); digits++; if (v > 255) return 0; } }
    else if (ch == '.' && !hasport) { if (!digits || part > 2) return 0; a4[part++] = (u8)v; v = 0; digits = 0; }
    else if (ch == ':' && !hasport && part == 3 && digits) { a4[part++] = (u8)v; hasport = 1; }
    else if (ch == ' ') continue;
    else return 0;
  }
  if (!hasport) { if (part != 3 || !digits) return 0; a4[3] = (u8)v; *port = IP_PORT; return 1; }
  if (p == 0) return 0; *port = (u16)p; return 1;
}

/* ---------------- ui helpers ---------------- */
static int log_open;
static void layout(void) { /* two screens: not in a room (tabs, create, join) / in the room (code, nicks, ping, leave) */
  int rm = in_room != 0, a = rm ? 0 : 5, b = rm ? 5 : 0;
  ShowWindow(C_STEAM, a); ShowWindow(C_IP, a); ShowWindow(C_CREATE, a); ShowWindow(C_JOINED, a); ShowWindow(C_JOIN, a);
  ShowWindow(C_COPY, b); ShowWindow(C_LEAVE, b);
  ShowWindow(C_LOG, log_open ? 5 : 0); ShowWindow(C_T1, log_open ? 5 : 0); ShowWindow(C_T2, log_open ? 5 : 0); ShowWindow(C_T3, log_open ? 5 : 0); ShowWindow(C_T4, log_open ? 5 : 0); ShowWindow(C_T5, log_open ? 5 : 0);
  SetWindowTextW(C_LOGBTN, log_open ? L"Скрыть журнал" : L"Показать журнал");
  redraw();
}
static void apply_size(void) {
  int rc[4]; rc[0] = 0; rc[1] = 0; rc[2] = S(LAY_W); rc[3] = S(log_open ? H_LOG : H_BASE);
  AdjustWindowRectEx(rc, 0x02CA0000u, 0, 0);
  SetWindowPos(hMain, 0, 0, 0, rc[2] - rc[0], rc[3] - rc[1], 0x16 /* NOMOVE | NOZORDER | NOACTIVATE */);
}
static void refresh_ui(void) {
  int steam = (mode == M_STEAM);
  SetWindowTextW(C_CREATE, (steam && !steam_ready) ? L"Подключиться к Steam" : L"Создать комнату");
  EnableWindow(C_JOIN, !in_room && (!steam || steam_ready));
  EnableWindow(C_JOINED, !in_room);
  { int i; for (i = 0; i < nbtns; i++) InvalidateRect(btns[i], 0, 0); }
  layout();
}
static void set_rtt(void) { redraw(); } /* the ping is drawn next to the nick in the room card */
static void reset_room(void) {
  in_room = is_host = peer_present = 0; lobby_set = entered = joining = 0; peer_steam = 0; last_rtt = 0xFFFFFFFFu; peer_seen = 0; peer_name[0] = 0; net_have = 0;
  set_code(L""); set_rtt(); refresh_ui();
}
static void status_idle(void) {
  if (mode == M_STEAM) {
    if (steam_ready) set_status(L"Всё готово. Создай комнату или введи код друга.");
    else set_status(L"Steam не найден. Запусти Steam, войди в аккаунт и нажми «Подключиться к Steam».");
  } else set_status(L"Включи Radmin VPN. Хост создаёт комнату и называет адрес, друг вводит его ниже.");
}

/* ---------------- files inside the exe (resources) ---------------- */
#define RES_MOD 100   /* the game module (dinput8.dll) */
#define RES_STEAM 101 /* steam_api.dll (modern, 32-bit) */
static int res_blob(u32 id, const void **d, u32 *sz) {
  PV r = FindResourceW(0, (const u16 *)id, (const u16 *)10 /* RT_RCDATA */), g; if (!r) return 0;
  g = LoadResource(0, r); *sz = SizeofResource(0, r); *d = g ? LockResource(g) : 0; return *d && *sz;
}
static int file_equals(const u16 *path, const void *d, u32 sz) {
  PV f = CreateFileW(path, 0x80000000u, 1, 0, 3, 0x80, 0); u8 b[4096]; u32 off = 0, got, i; int ok = 1;
  if (f == (PV)-1) return 0;
  if (GetFileSize(f, 0) != sz) { CloseHandle(f); return 0; }
  while (off < sz && ok) { u32 want = sz - off > 4096 ? 4096 : sz - off; if (!ReadFile(f, b, want, &got, 0) || got != want) { ok = 0; break; } for (i = 0; i < got; i++) if (b[i] != ((const u8 *)d)[off + i]) { ok = 0; break; } off += got; }
  CloseHandle(f); return ok;
}
static int write_file(const u16 *path, const void *d, u32 sz) { u32 w = 0; int ok; PV f = CreateFileW(path, 0x40000000u, 0, 0, 2, 0x80, 0); if (f == (PV)-1) return 0; ok = WriteFile(f, d, sz, &w, 0) && w == sz; CloseHandle(f); return ok; }
static int wlen(const u16 *s) { int n = 0; while (s[n]) n++; return n; }
static void wcopy(u16 *d, const u16 *s, int cap) { int i; for (i = 0; s[i] && i < cap - 1; i++) d[i] = s[i]; d[i] = 0; }
static void wcat(u16 *d, const u16 *s, int cap) { int n = wlen(d), i; for (i = 0; s[i] && n + i < cap - 1; i++) d[n + i] = s[i]; d[n + i] = 0; }
static void exe_dir(u16 *out, int cap) { int n = (int)GetModuleFileNameW(0, out, (u32)cap - 2); while (n > 0 && out[n - 1] != '\\') n--; out[n] = 0; } /* with the trailing backslash */
/* steam_api.dll is carried inside this exe: it is laid out in %LOCALAPPDATA%\TeraCoop\ (not next to the program, so nothing lies around in the game folder) */
static int ensure_steam_dll(u16 *out, int cap) {
  u16 dir[460]; const void *d; u32 sz; u32 n = GetEnvironmentVariableW(L"LOCALAPPDATA", dir, 400);
  if (!n || n >= 400 || !res_blob(RES_STEAM, &d, &sz)) return 0;
  wcat(dir, L"\\TeraCoop", 440); CreateDirectoryW(dir, 0);
  wcopy(out, dir, cap); wcat(out, L"\\steam_api.dll", cap);
  if (file_equals(out, d, sz)) return 1;
  return write_file(out, d, sz);
}

/* ---------------- steam logic ---------------- */
static int missing;
static void *need(const char *n) { void *p = GetProcAddress(sdll, n); if (!p) { wr(); ws(L"нет функции в steam_api.dll: "); wa(n); log_line(); missing = 1; } return p; }
static void rp_set(u64 id) { /* lets Steam show "Join game" / "Invite to game" for this room (connect = +connect_lobby <id>) */
  char c[48]; u32 v[4]; int n = 0, i, k = 0; const char *pre = "+connect_lobby "; char t[24];
  if (!fSetRP) return;
  for (i = 0; pre[i]; i++) c[n++] = pre[i];
  v[0] = (u32)(id >> 48) & 0xffff; v[1] = (u32)(id >> 32) & 0xffff; v[2] = (u32)(id >> 16) & 0xffff; v[3] = (u32)id & 0xffff;
  for (;;) { u32 q[4], rem = 0, z = 1, j; for (j = 0; j < 4; j++) { u32 cur = rem * 65536u + v[j]; q[j] = cur / 10u; rem = cur % 10u; if (q[j]) z = 0; } t[k++] = (char)('0' + rem); for (j = 0; j < 4; j++) v[j] = q[j]; if (z) break; }
  while (k) c[n++] = t[--k];
  c[n] = 0;
  fSetRP(fr, "connect", c); fSetRP(fr, "status", "Тера Кооп: в комнате");
}
static void rp_clear(void) { if (fClrRP && steam_ready) fClrRP(fr); }
static int is_member(u64 id) { int n = fNumMem(mm, lobby), i; for (i = 0; i < n; i++) if (fMemIdx(mm, lobby, i) == id) return 1; return 0; }
static void members_changed(void) {
  int n, i; u64 other = 0;
  if (!lobby_set) return;
  n = fNumMem(mm, lobby);
  for (i = 0; i < n; i++) { u64 w = fMemIdx(mm, lobby, i); if (w != myid) { other = w; break; } }
  if (other && other != peer_steam) {
    peer_steam = other; peer_present = 1; last_rtt = 0xFFFFFFFFu;
    wr(); ws(L"В комнате друг: "); wutf8(fFName(fr, other)); log_line();
    { wr(); wutf8(fFName(fr, other)); { int i; for (i = 0; wb[i] && i < 60; i++) peer_name[i] = wb[i]; peer_name[i] = 0; } }
    set_status(L"Друг в комнате."); set_rtt();
  } else if (!other && peer_present) {
    peer_present = 0; peer_steam = 0; peer_name[0] = 0; say_ru(L"Друг вышел из комнаты."); set_status(L"Друг вышел. Комната открыта — он может зайти снова."); set_rtt();
  }
}
static void on_lobby_created(int result, u64 id) {
  if (result != 1) { wr(); ws(L"Не удалось создать комнату, код ошибки "); wd((u32)result); log_line(); set_status(L"Не удалось создать комнату. Проверь Steam и попробуй ещё раз."); return; }
  if (lobby_set && lobby == id) return;
  lobby = id; lobby_set = 1; entered = 1; in_room = 1; is_host = 1; joining = 0;
  fSetData(mm, id, "legolotrcoop", "1"); rp_set(id);
  make_code(id); set_code(wb);
  wr(); ws(L"Комната создана. Код: "); ws(code_txt); log_line();
  set_status(L"Комната готова. Назови код другу или пригласи его в Steam: правый клик на друге → «Пригласить в игру».");
  set_rtt(); refresh_ui();
}
static void on_lobby_enter(u64 id, u32 response) {
  if (!joining) return;
  joining = 0;
  if (response != 1) { say_ru(L"Не получилось войти: неверный код, комната закрыта или полна, или вы не друзья в Steam."); set_status(L"Не получилось войти. Проверь код и что вы с другом друзья в Steam."); refresh_ui(); return; }
  { const char *v = fGetData(mm, id, "legolotrcoop");
    if (!v || v[0] != '1') { fLeave(mm, id); say_ru(L"Это не комната «Тера Кооп» (нет метки)."); set_status(L"Это не комната нашего мода. Проверь код."); refresh_ui(); return; } }
  lobby = id; lobby_set = 1; entered = 1; in_room = 1; is_host = 0; make_code(id); set_code(wb); rp_set(id);
  say_ru(L"Вошла в комнату друга."); set_status(L"Ты в комнате друга.");
  members_changed(); set_rtt(); refresh_ui();
}
static void steam_join(u64 id) {
  if (!steam_ready) return;
  joining = 1; { u64 call = fJoin(mm, id); (void)call; }
  set_status(L"Вхожу в комнату…"); say_ru(L"Вхожу в комнату по коду…");
}
static void on_message(const u8 *m) {
  const u8 *data = *(const u8 *const *)m; u32 size = *(const u32 *)(m + 4); u64 s12 = fGetId64(m + 12), from = s12 ? s12 : fGetId64(m + 16);
  if (size >= 12) {
    u32 tag = *(const u32 *)data, seq = *(const u32 *)(data + 4), tick = *(const u32 *)(data + 8);
    if (tag == TAG_PING) { dbg_rx_ping++; steam_send_to(from, TAG_PONG, seq, tick); }
    else if (tag == TAG_PONG) { dbg_rx_pong++; last_rtt = GetTickCount() - tick; set_rtt(); }
    else if (tag == TAG_ACT) { if (is_host && from == peer_steam) { dbg_rx_act++; net_got_act(seq, tick); } else dbg_rx_rej++; }
    else if (tag == TAG_PXZ || tag == TAG_PYA) { if (from == peer_steam) { dbg_rx_pos++; net_got_pos(tag, seq, tick); } else dbg_rx_rej++; }
    else if (tag == TAG_BTN) { if (from == peer_steam) net_got_btn(seq, tick); else dbg_rx_rej++; }
    else if (is_en_tag(tag)) { if (from == peer_steam && !is_host) net_got_en(tag, seq, tick); else dbg_rx_rej++; }
    else dbg_rx_other++;
  }
}
static void handle(int cb, const u8 *p) {
  if (cb == 513) on_lobby_created(*(const int *)p, rd64(p + 8));
  else if (cb == 504) on_lobby_enter(rd64(p), *(const u32 *)(p + 16));
  else if (cb == 506) members_changed();
  else if (cb == 333) { u64 l = rd64(p); if (!in_room && !joining) { say_ru(L"Друг позвал в комнату — вхожу."); steam_join(l); } }
  else if (cb == 337) { /* Join game from the Steam friends menu: connect = "+connect_lobby <id>" */
    const char *c = (const char *)(p + 8); u64 v = 0; int i = 0, any = 0;
    while (c[i] && i < 240 && !(c[i] >= '0' && c[i] <= '9')) i++; /* the first digit starts the lobby number */
    while (c[i] >= '0' && c[i] <= '9') { v = (v << 3) + (v << 1) + (u64)(c[i] - '0'); i++; any = 1; }
    if (any) { if (!in_room && !joining) { say_ru(L"«Присоединиться к игре» из Steam — вхожу в комнату."); steam_join(v); } else set_status(L"Ты уже в комнате. Сначала выйди из неё."); }
  }
  else if (cb == 1251) {
    u64 who = fGetId64(p);
    if (lobby_set && who != myid && is_member(who)) fAccept(nm, p);
  }
}
static u8 cbbuf[8192];
struct CBMsg { int user; int cb; u8 *param; int size; };
static void pump(void) {
  int pipe = fGetPipe(); struct CBMsg m;
  fRunFrame(pipe);
  while (fNext(pipe, &m)) {
    if (m.cb == 703) {
      u64 call = rd64(m.param); int icb = *(int *)(m.param + 8); u32 psz = *(u32 *)(m.param + 12); u8 failed = 0;
      if (psz > sizeof cbbuf) psz = sizeof cbbuf;
      if (fResult(pipe, call, cbbuf, (int)psz, icb, &failed) && !failed) handle(icb, cbbuf);
    } else handle(m.cb, m.param);
    fFree(pipe);
  }
  if (nm) { void *msgs[16]; int got = fRecv(nm, 0, msgs, 16), i; for (i = 0; i < got; i++) { on_message((const u8 *)msgs[i]); fRel(msgs[i]); } }
}
static void steam_start(void) {
  u16 path[420]; char err[1024]; int r;
  steam_tried = 1; missing = 0;
  SetEnvironmentVariableW(L"SteamAppId", L"480"); SetEnvironmentVariableW(L"SteamGameId", L"480"); /* the program tells Steam its AppID itself (Spacewar 480): no steam_appid.txt next to the program is needed */
  if (!ensure_steam_dll(path, 420)) path[0] = 0; /* never a steam_api.dll from the program folder: in the game folder that is the game's own old one */
  sdll = LoadLibraryW(path);
  if (!sdll) { say_ru(L"Не удалось подготовить steam_api.dll."); status_idle(); set_status(L"Не удалось подготовить файл для связи со Steam (steam_api.dll). Переустанови программу."); refresh_ui(); return; }
  { F_InitFlat fInit = (F_InitFlat)need("SteamAPI_InitFlat"); F_v fMD = (F_v)need("SteamAPI_ManualDispatch_Init");
    F_get fUser = (F_get)need("SteamAPI_SteamUser_v023"), fFriends = (F_get)need("SteamAPI_SteamFriends_v018"), fMM = (F_get)need("SteamAPI_SteamMatchmaking_v009"), fNM = (F_get)need("SteamAPI_SteamNetworkingMessages_SteamAPI_v002");
    F_id fGetId = (F_id)need("SteamAPI_ISteamUser_GetSteamID");
    fMyName = (F_name)need("SteamAPI_ISteamFriends_GetPersonaName"); fCreate = (F_create)need("SteamAPI_ISteamMatchmaking_CreateLobby"); fJoin = (F_join)need("SteamAPI_ISteamMatchmaking_JoinLobby");
    fShutdown = (F_v)need("SteamAPI_Shutdown"); fGetPipe = (F_pipe)need("SteamAPI_GetHSteamPipe"); fRunFrame = (F_pipev)need("SteamAPI_ManualDispatch_RunFrame");
    fNext = (F_next)need("SteamAPI_ManualDispatch_GetNextCallback"); fFree = (F_pipev)need("SteamAPI_ManualDispatch_FreeLastCallback"); fResult = (F_result)need("SteamAPI_ManualDispatch_GetAPICallResult");
    fSetData = (F_setdata)need("SteamAPI_ISteamMatchmaking_SetLobbyData"); fGetData = (F_getdata)need("SteamAPI_ISteamMatchmaking_GetLobbyData");
    fNumMem = (F_nummem)need("SteamAPI_ISteamMatchmaking_GetNumLobbyMembers"); fLeave = (F_leave)need("SteamAPI_ISteamMatchmaking_LeaveLobby");
    fOwner = (F_owner)need("SteamAPI_ISteamMatchmaking_GetLobbyOwner"); fMemIdx = (F_memidx)need("SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex");
    fSetId = (F_setid)need("SteamAPI_SteamNetworkingIdentity_SetSteamID64"); fGetId64 = (F_getid64)need("SteamAPI_SteamNetworkingIdentity_GetSteamID64");
    fSend = (F_send)need("SteamAPI_ISteamNetworkingMessages_SendMessageToUser"); fRecv = (F_recv)need("SteamAPI_ISteamNetworkingMessages_ReceiveMessagesOnChannel");
    fAccept = (F_accept)need("SteamAPI_ISteamNetworkingMessages_AcceptSessionWithUser"); fRel = (F_rel)need("SteamAPI_SteamNetworkingMessage_t_Release");
    fFName = (F_fname)need("SteamAPI_ISteamFriends_GetFriendPersonaName");
    fSetRP = (F_setrp)GetProcAddress(sdll, "SteamAPI_ISteamFriends_SetRichPresence"); fClrRP = (F_clrrp)GetProcAddress(sdll, "SteamAPI_ISteamFriends_ClearRichPresence");
    if (missing || !fInit || !fMD) { set_status(L"steam_api.dll не подходит (не хватает функций). Нужна версия из комплекта."); refresh_ui(); return; }
    memset(err, 0, sizeof err); r = fInit(err);
    if (r != 0) {
      wr(); ws(L"Steam не принял программу, код "); wd((u32)r); log_line();
      set_status(r == 2 ? L"Steam не запущен. Запусти Steam, войди в аккаунт и нажми «Обновить»." : L"Не удалось подключиться к Steam. Запусти Steam и нажми «Обновить».");
      refresh_ui(); return;
    }
    fMD(); usr = fUser(); fr = fFriends(); mm = fMM(); nm = fNM(); myid = fGetId(usr); steam_ready = 1;
    wr(); ws(L"Steam подключён. Имя: "); wutf8(fMyName(fr)); log_line();
    status_idle(); refresh_ui();
  }
}

/* ---------------- ip logic ---------------- */
static void ip_on_packet(const u8 *from, u32 tag, u32 seq, u32 tick, u32 now, const u8 *name, int nlen) {
  int i;
  if (is_host) {
    if (peer_addr_set) { for (i = 2; i < 8; i++) if (peer_addr[i] != from[i]) return; /* another player: ignore (2 players max) */ }
    else { for (i = 0; i < 16; i++) peer_addr[i] = from[i]; peer_addr_set = 1; }
    if (!peer_present) { peer_present = 1; last_rtt = 0xFFFFFFFFu; say_ru(L"Друг подключился по IP."); set_status(L"Друг в комнате."); }
  } else if (ip_joining) {
    ip_joining = 0; in_room = 1; is_host = 0; peer_present = 1; last_rtt = 0xFFFFFFFFu; { wr(); wd(peer_addr[4]); ws(L"."); wd(peer_addr[5]); ws(L"."); wd(peer_addr[6]); ws(L"."); wd(peer_addr[7]); set_code(wb); } say_ru(L"Хост ответил — ты в комнате."); set_status(L"Ты в комнате друга."); refresh_ui();
  }
  peer_seen = now;
  if (tag == TAG_HELO) { ip_send_helo(); if (nlen > 0) { int c = MultiByteToWideChar(65001, 0, (const char *)name, nlen > 50 ? 50 : nlen, peer_name, 60); if (c < 0) c = 0; peer_name[c] = 0; } }
  else if (tag == TAG_PING) ip_send(TAG_PONG, seq, tick);
  else if (tag == TAG_PONG) { last_rtt = now - tick; }
  else if (tag == TAG_ACT) { if (is_host) net_got_act(seq, tick); }
  else if (tag == TAG_PXZ || tag == TAG_PYA) { dbg_rx_pos++; net_got_pos(tag, seq, tick); }
  else if (tag == TAG_BTN) { net_got_btn(seq, tick); }
  else if (is_en_tag(tag)) { if (!is_host) net_got_en(tag, seq, tick); }
  if (tag != TAG_ACT && tag != TAG_PXZ && tag != TAG_PYA && tag != TAG_BTN && !is_en_tag(tag)) set_rtt();
}
static void ip_tick(u32 now) {
  int k;
  if (usock == (u32)-1) return;
  for (k = 0; k < 16; k++) {
    u8 pkt[96]; u8 from[16]; i32 fl = 16, n;
    n = recvfrom(usock, pkt, 96, 0, from, &fl);
    if (n < 0) break;
    if (n >= 12) ip_on_packet(from, *(u32 *)pkt, *(u32 *)(pkt + 4), *(u32 *)(pkt + 8), now, pkt + 12, n - 12);
  }
  if (ip_joining) {
    if (now - ip_hello_t >= 1000u) { ip_hello_t = now; ip_send_helo(); }
    if (now - ip_join_t0 > 10000u) { ip_close(); set_code(L""); say_ru(L"Хост не отвечает по этому адресу."); set_status(L"Хост не отвечает. Проверь адрес, Radmin VPN у обоих и что хост создал комнату."); refresh_ui(); }
  }
  if (peer_present && peer_seen && now - peer_seen > 8000u) {
    peer_present = 0; peer_name[0] = 0; say_ru(L"Связь с другом пропала.");
    if (!is_host) { ip_close(); reset_room(); set_status(L"Связь с хостом пропала. Можно войти снова."); }
    else { peer_addr_set = 0; set_status(L"Друг пропал. Комната ещё открыта — он может зайти снова."); set_rtt(); }
  }
}

/* ---------------- commands ---------------- */
static void leave_room(void) {
  if (mode == M_STEAM) { if (steam_ready && lobby_set) fLeave(mm, lobby); rp_clear(); }
  else ip_close();
  if (in_room || joining) say_ru(L"Вышла из комнаты.");
  reset_room(); status_idle();
}
static void on_create(void) {
  if (mode == M_STEAM) {
    if (!steam_ready) { steam_start(); status_idle(); refresh_ui(); return; }
    joining = 0; fCreate(mm, 1 /* friends only */, 2); set_status(L"Создаю комнату…"); say_ru(L"Создаю комнату (только для друзей, до 2 игроков)…");
  } else {
    u8 best[4]; int radmin = 0, have = ip_my_addresses(best, &radmin);
    if (!ip_open(IP_PORT)) { say_ru(L"Не удалось открыть порт 27015."); set_status(L"Порт 27015 занят (комната уже открыта в другом окне?) или Windows его не разрешила."); return; }
    in_room = 1; is_host = 1; peer_present = 0; last_rtt = 0xFFFFFFFFu;
    wr(); if (have) { wd(best[0]); ws(L"."); wd(best[1]); ws(L"."); wd(best[2]); ws(L"."); wd(best[3]); } else ws(L"адрес не найден");
    set_code(wb);
    wr(); ws(L"Комната открыта (UDP, порт 27015). Адрес для друга: "); ws(code_txt); log_line();
    wr(); if (radmin) ws(L"Комната открыта. Назови другу этот адрес. Если Windows спросит про сеть — разреши.");
    else ws(L"Адрес Radmin (26.…) не найден. Включи Radmin VPN и создай комнату заново.");
    set_status(wb); set_rtt(); refresh_ui();
  }
}
static void on_join(void) {
  u16 t[200];
  GetWindowTextW(C_JOINED, t, 190);
  if (mode == M_STEAM) {
    u64 id = 0;
    if (!steam_ready) { status_idle(); return; }
    if (!parse_code(t, &id)) { set_status(L"Код не распознан. Он выглядит так: AB3K-9QX (7 символов, буквы и цифры)."); return; }
    steam_join(id);
  } else {
    u8 a[4]; u16 port;
    if (!parse_ip(t, a, &port)) { set_status(L"Адрес не распознан. Он выглядит так: 26.12.34.56 (можно с портом: 26.12.34.56:27015)."); return; }
    if (!ip_open(0)) { say_ru(L"Не удалось открыть сетевой порт."); set_status(L"Не удалось открыть сетевой порт."); return; }
    memset(peer_addr, 0, 16); *(u16 *)peer_addr = 2; *(u16 *)(peer_addr + 2) = bswap16(port); peer_addr[4] = a[0]; peer_addr[5] = a[1]; peer_addr[6] = a[2]; peer_addr[7] = a[3];
    peer_addr_set = 1; ip_joining = 1; is_host = 0; ip_hello_t = 0; ip_join_t0 = GetTickCount();
    wr(); ws(L"Подключаюсь к "); wd(a[0]); ws(L"."); wd(a[1]); ws(L"."); wd(a[2]); ws(L"."); wd(a[3]); ws(L":"); wd(port); ws(L"…"); log_line(); set_status(wb);
  }
}
static void on_copy(void) {
  u16 *t = code_txt; int n; PV h; u16 *p; int i;
  n = 0; while (t[n]) n++; if (!n) return;
  if (!OpenClipboard(hMain)) return;
  EmptyClipboard(); h = GlobalAlloc(2 /* GMEM_MOVEABLE */, (u32)(n + 1) * 2);
  if (h) { p = (u16 *)GlobalLock(h); if (p) { for (i = 0; i <= n; i++) p[i] = t[i]; GlobalUnlock(h); SetClipboardData(13 /* CF_UNICODETEXT */, h); } }
  CloseClipboard(); say_ru(L"Код скопирован.");
}
static void set_mode(int m) {
  if (in_room || joining || ip_joining) leave_room();
  mode = m;
    SetWindowTextW(C_JOINED, L"");
  if (m == M_STEAM && !steam_tried) steam_start();
  layout(); status_idle(); refresh_ui();
}


/* ---------------- link to the game module ---------------- */
static u32 mirror_on, mirror_was, cap_dirty, cap_last_draw, cap_sv, cap_friend; static i32 cap_sx, cap_sy; static u32 cap_sb;
static u32 net_btn_last, net_btn_burst, net_btn_t, net_btn_seq;
static u32 en_sent_tick, en_s_flags[EN_MAX], en_s_te[EN_MAX], en_s_at[EN_MAX], en_i_at[EN_MAX], en_was_guest, dl_etx0, dl_erx0;
static u32 dl_t, dl_rx0, dl_tx0, dl_btx0, dl_brx0, dl_gap_max, dl_gaps, dl_gaps_tot, dl_upd, dl_loop_last, dl_loop_max, dl_last_xz, dl_my_b, dl_fr_b;
static void wbtn(u32 b) { if (!(b & 7u)) { ws(L"нет"); return; } if (b & BT_ACTION) ws(L"[действие]"); if (b & BT_SPECIAL) ws(L"[особое]"); if (b & BT_JUMP) ws(L"[прыжок]"); }
static Link *lk; static u32 mod_state, my_test_seq, test_reported; /* mod_state: 0 game/module not seen, 1 build not supported, 2 module in the game, 3 module in the game and player 2 present */
static void link_open(void) {
  PV h = CreateFileMappingW((PV)-1, 0, 4 /* PAGE_READWRITE */, 0, 4096, LINK_NAME);
  if (!h) return;
  lk = (Link *)MapViewOfFile(h, 0xF001Fu /* FILE_MAP_ALL_ACCESS */, 0, 0, 0);
  if (lk && lk->magic != LINK_MAGIC) { lk->version = LINK_VERSION; lk->magic = LINK_MAGIC; }
}
static void link_tick(u32 now) {
  u32 ns = 0;
  if (!lk) return;
  lk->prog_tick = now;
  { /* part 5.5: who is who for the module, and the enemy traffic */
    u32 etx = in_room && peer_present && ((mode == M_STEAM && peer_steam) || (mode != M_STEAM && peer_addr_set)), id;
    lk->role = etx ? (is_host ? 1u : 2u) : 0u;
    lk->room_role = in_room ? (is_host ? 1u : 2u) : 0u; if (lk->room_role && !logrole) log_set_role(lk->room_role);
    if (etx && is_host && lk->en_out_tick && lk->en_out_tick != en_sent_tick) { /* host: the module has a new enemy table, send it */
      en_sent_tick = lk->en_out_tick;
      for (id = 0; id < EN_MAX; id++) {
        EnPkt e = lk->en_out[id]; u32 tg = id;
        if (!(e.flags & EN_VALID)) { if (en_s_flags[id]) { en_s_flags[id] = 0; if (mode == M_STEAM) steam_send_f(peer_steam, (EN_KIND_S << 16) | tg, 0, 0, 9); else ip_send((EN_KIND_S << 16) | tg, 0, 0); dbg_tx_en++; } continue; }
        if (mode == M_STEAM) { steam_send_f(peer_steam, (EN_KIND_A << 16) | tg, (u32)e.x, (u32)e.z, 1); steam_send_f(peer_steam, (EN_KIND_B << 16) | tg, (u32)e.y, (u32)e.yaw, 1); }
        else { ip_send((EN_KIND_A << 16) | tg, (u32)e.x, (u32)e.z); ip_send((EN_KIND_B << 16) | tg, (u32)e.y, (u32)e.yaw); }
        dbg_tx_en += 2;
        if (e.flags != en_s_flags[id] || e.type_epoch != en_s_te[id] || (i32)(now - en_s_at[id]) >= 500) {
          en_s_flags[id] = e.flags; en_s_te[id] = e.type_epoch; en_s_at[id] = now;
          if (mode == M_STEAM) steam_send_f(peer_steam, (EN_KIND_S << 16) | tg, e.flags, e.type_epoch, 9); else ip_send((EN_KIND_S << 16) | tg, e.flags, e.type_epoch); dbg_tx_en++;
        }
        if ((i32)(now - en_i_at[id]) >= 1000) {
          en_i_at[id] = now;
          if (mode == M_STEAM) steam_send_f(peer_steam, (EN_KIND_I << 16) | tg, (u32)e.sx, (u32)e.sz, 9); else ip_send((EN_KIND_I << 16) | tg, (u32)e.sx, (u32)e.sz); dbg_tx_en++;
        }
      }
    }
    if (etx && !is_host) { /* guest: what arrived goes to the module */
      if (en_dirty) { en_dirty = 0; for (id = 0; id < EN_MAX; id++) lk->en_in[id] = enr[id]; lk->en_in_tick = now; en_was_guest = 1; }
    } else if (en_was_guest) { /* left the room: the module must not keep pulling enemies */
      en_was_guest = 0; for (id = 0; id < EN_MAX; id++) { enr[id].flags = 0; lk->en_in[id].flags = 0; } lk->en_in_tick = now;
    }
  }
  if (lk->plug_tick && (i32)(now - lk->plug_tick) < 1500) ns = !(lk->plug_state & ST_BUILD_OK) ? 1 : (lk->plug_state & ST_P2_ACTIVE) ? 3 : 2;
  if (ns != mod_state) {
    mod_state = ns;
    say_ru(ns == 0 ? L"Мод в игре: связи нет (игра закрыта или мода нет)." : ns == 1 ? L"Мод в игре: эта версия игры не поддерживается." : ns == 2 ? L"Мод в игре: на связи." : L"Мод в игре: второй игрок подключён.");
    redraw();
  }
  { u32 alive = lk->plug_tick && (i32)(now - lk->plug_tick) < 1500, fresh = alive && lk->cap_valid && (i32)(now - lk->cap_tick) < 500;
    i32 ax = fresh ? lk->cap_ax : 0, ay = fresh ? lk->cap_ay : 0; u32 b = fresh ? lk->cap_buttons : 0;
    /* host: the actions of the friend that came over the network */
    /* part 5.3: the friend's position stream (both roles). The friend's own player 1 shows up here as player 2: the module puts player 2 where the friend stands (smoothed);
     * while the friend is moving, the program also holds "forward" on player 2 so that the game plays the walking animation. (The old action stream ACT is no longer used.) */
    static float sp_x, sp_z; static u32 sp_t, sp_moving;
    u32 nfresh = in_room && peer_present && rx_xz_tick && rx_ya_tick && (i32)(now - rx_xz_tick) < 400 && (i32)(now - rx_ya_tick) < 400;
    i32 nax = 0, nay = sp_moving ? 100 : 0; u32 nb = 0;
    if (nfresh) {
      float x = *(float *)&rx_xb, z = *(float *)&rx_zb;
      if (!sp_t || (i32)(now - sp_t) >= 150) { float ddx = x - sp_x, ddz = z - sp_z, dt = (float)(now - sp_t) / 1000.0f, lim = 0.35f * dt; sp_moving = sp_t && (ddx * ddx + ddz * ddz) > lim * lim && dt < 1.0f; sp_x = x; sp_z = z; sp_t = now; }
      nay = sp_moving ? 100 : 0;
      lk->rem_px = x; lk->rem_pz = z; lk->rem_py = *(float *)&rx_yb; lk->rem_yaw = *(float *)&rx_yawb; lk->rem_tick = rx_xz_tick; lk->rem_valid = 1;
    } else { lk->rem_valid = 0; sp_t = 0; sp_moving = 0; nay = 0; }
    /* part 5.4: the friend's action / special buttons are held on player 2 (the jump is not: the height already comes with the position). A stream that went silent lets the buttons go. */
    nb = (nfresh && rx_btn_have && (i32)(now - rx_btn_tick) < 400) ? (rx_btn & (BT_ACTION | BT_SPECIAL)) : 0;
    { /* a short tap must still be a whole key press in the game: every button that goes down is held for at least 200 ms */
      static u32 lt_prev, lt_act, lt_spc; u32 rise = nb & ~lt_prev; lt_prev = nb;
      if (rise & BT_ACTION) lt_act = now + 200u; if (rise & BT_SPECIAL) lt_spc = now + 200u;
      if (!nfresh) { lt_act = lt_spc = 0; }
      if ((i32)(lt_act - now) > 0) nb |= BT_ACTION; if ((i32)(lt_spc - now) > 0) nb |= BT_SPECIAL; }
    u32 src = mirror_on ? 1u : nfresh ? 2u : 0u;
    i32 dx = nfresh ? nax : ax, dy = nfresh ? nay : ay; u32 db = nfresh ? nb : b, dv = nfresh ? 1u : fresh;
    if (nfresh != net_rx_on) { net_rx_on = nfresh; if (nfresh) { say_ru(L"Получаю положение друга: второй игрок в игре стоит там, где он."); set_status(L"Друг играет: второй игрок повторяет его положение."); } else { say_ru(L"Положение друга перестало приходить."); } }
    if (dx != cap_sx || dy != cap_sy || db != cap_sb || dv != cap_sv || nfresh != cap_friend) { cap_sx = dx; cap_sy = dy; cap_sb = db; cap_sv = dv; cap_friend = nfresh; cap_dirty = 1; }
    if (cap_dirty && log_open && (i32)(now - cap_last_draw) >= 80) { cap_dirty = 0; cap_last_draw = now; redraw(); }
    if (src) { lk->ax = src == 1 ? ax : nax; lk->ay = src == 1 ? ay : nay; lk->buttons = src == 1 ? b : nb; lk->want_p2 = 1; lk->inject = 1; mirror_was = 1; }
    else if (mirror_was) { mirror_was = 0; lk->inject = 0; lk->want_p2 = 0; lk->ax = lk->ay = 0; lk->buttons = 0; }
    /* guest: send our own actions to the host (when they change, and at least every 100 ms so that a lost packet does not matter) */
    { u32 tx = in_room && peer_present && ((mode == M_STEAM && peer_steam) || (mode != M_STEAM && peer_addr_set));
      u32 posok = alive && lk->cap_pos_valid && (i32)(now - lk->cap_tick) < 500;
      if (tx != net_tx_on) { net_tx_on = tx; if (tx) say_ru(L"Отправляю своё положение другу: у него я буду вторым игроком."); }
      if (tx && posok && (i32)(now - net_last_send) >= 33) {
        u32 xb = *(u32 *)&lk->cap_px, zb = *(u32 *)&lk->cap_pz, yb = *(u32 *)&lk->cap_py, ab = *(u32 *)&lk->cap_yaw;
        net_last_send = now; net_sent_seq++; dbg_tx_pos++;
        if (mode == M_STEAM) { steam_send_f(peer_steam, TAG_PXZ, xb, zb, 9 /* reliable + no nagle */); steam_send_f(peer_steam, TAG_PYA, yb, ab, 9); }
        else { ip_send(TAG_PXZ, xb, zb); ip_send(TAG_PYA, yb, ab); }
      }
      /* part 5.4: our buttons (action / special / jump) to the friend: at once on every change (three times, 33 ms apart, so that a lost UDP packet does not matter), otherwise every 100 ms */
      { u32 mine = fresh ? (lk->cap_buttons & (BT_ACTION | BT_SPECIAL | BT_JUMP)) : 0;
        if (mine != net_btn_last) { net_btn_last = mine; net_btn_burst = 3; }
        if (tx && alive && ((net_btn_burst && (i32)(now - net_btn_t) >= 33) || (i32)(now - net_btn_t) >= 100)) {
          net_btn_t = now; if (net_btn_burst) net_btn_burst--; net_btn_seq++; dbg_tx_btn++;
          if (mode == M_STEAM) steam_send_f(peer_steam, TAG_BTN, mine, net_btn_seq, 9); else ip_send(TAG_BTN, mine, net_btn_seq);
        } } }
  /* detailed log lines (file only): button changes on both sides, pauses of the friend's position stream, stalls of this program, and a summary line every second */
  { u32 mb = (lk->cap_valid && (i32)(now - lk->cap_tick) < 500) ? (lk->cap_buttons & 7u) : 0, fb = (in_room && peer_present && rx_btn_have && (i32)(now - rx_btn_tick) < 400) ? (rx_btn & 7u) : 0;
    if (dl_loop_last) { u32 g = now - dl_loop_last; if (g > dl_loop_max) dl_loop_max = g; if (g > 250u) { wr(); ws(L"подробно: программа простояла "); wd(g); ws(L" мс между двумя проверками (окно или Steam тормозили)"); log_file(); } } dl_loop_last = now;
    if (mb != dl_my_b) { wr(); ws(L"подробно: свои кнопки в игре: "); wbtn(mb); ws(L" (были: "); wbtn(dl_my_b); ws(L")"); log_file(); dl_my_b = mb; }
    if (fb != dl_fr_b) { wr(); ws(L"подробно: кнопки друга пришли: "); wbtn(fb); ws(L" (были: "); wbtn(dl_fr_b); ws(L"); отправлено мной BTN "); wd(dbg_tx_btn); ws(L", принято BTN "); wd(dbg_rx_btn); log_file(); dl_fr_b = fb; }
    if (in_room && peer_present && rx_xz_tick && rx_xz_tick != dl_last_xz) {
      if (dl_last_xz) { u32 g = rx_xz_tick - dl_last_xz; if (g < 100000u) { if (g > dl_gap_max) dl_gap_max = g; if (g > 300u) { dl_gaps++; dl_gaps_tot++; wr(); ws(L"подробно: поток положения друга молчал "); wd(g); ws(L" мс (пауза)"); log_file(); } } }
      dl_last_xz = rx_xz_tick; dl_upd++;
    }
    if (in_room && peer_present && (i32)(now - dl_t) >= 1000) {
      dl_t = now;
      wr(); ws(L"подробно: за секунду позиций принято "); wd(dbg_rx_pos - dl_rx0); ws(L" пакетов (обновлений "); wd(dl_upd); ws(L"), самая длинная пауза "); wd(dl_gap_max); ws(L" мс, пауз>300мс "); wd(dl_gaps); ws(L" (всего "); wd(dl_gaps_tot);
      ws(L"); отправлено позиций "); wd(dbg_tx_pos - dl_tx0); ws(L"; BTN принято "); wd(dbg_rx_btn - dl_brx0); ws(L" отправлено "); wd(dbg_tx_btn - dl_btx0); ws(L" (устаревших отброшено всего "); wd(dbg_rx_btn_old); ws(L")");
      ws(L"; враги: отправлено пакетов "); wd(dbg_tx_en - dl_etx0); ws(L", принято "); wd(dbg_rx_en - dl_erx0); ws(L" (роль "); wd(lk->role); ws(L")");
      ws(L"; простой программы макс. "); wd(dl_loop_max); ws(L" мс; кнопки свои "); wbtn(mb); ws(L", друга "); wbtn(fb);
      if (nfresh) { ws(L"; друг (мм) x="); wi((i32)(*(float *)&rx_xb * 1000.0f)); ws(L" y="); wi((i32)(*(float *)&rx_yb * 1000.0f)); ws(L" z="); wi((i32)(*(float *)&rx_zb * 1000.0f)); ws(L" курс(мрад)="); wi((i32)(*(float *)&rx_yawb * 1000.0f)); } else ws(L"; положение друга не свежее");
      if (lk->cap_pos_valid) { ws(L"; я (мм) x="); wi((i32)(lk->cap_px * 1000.0f)); ws(L" y="); wi((i32)(lk->cap_py * 1000.0f)); ws(L" z="); wi((i32)(lk->cap_pz * 1000.0f)); }
      ws(L"; пинг "); if (last_rtt == 0xFFFFFFFFu) ws(L"?"); else { wd(last_rtt); ws(L" мс"); } ws(L"; ходьба для анимации="); wd((u32)sp_moving);
      log_file();
      dl_rx0 = dbg_rx_pos; dl_tx0 = dbg_tx_pos; dl_brx0 = dbg_rx_btn; dl_btx0 = dbg_tx_btn; dl_etx0 = dbg_tx_en; dl_erx0 = dbg_rx_en; dl_gap_max = 0; dl_gaps = 0; dl_upd = 0; dl_loop_max = 0;
    }
  } }
  if (my_test_seq && lk->test_done_seq == my_test_seq && test_reported != my_test_seq) {
    test_reported = my_test_seq;
    if (lk->test_result == 1) { say_ru(L"Проверка: получилось."); set_status(L"Проверка прошла: мод выполнил действие в игре."); }
    else { say_ru(L"Проверка: не получилось (смотри logs\\teracoop_mod.log в папке игры)."); set_status(L"Проверка не удалась: игра не была в фокусе, не в уровне или версия игры не подходит. Подробности в logs\\teracoop_mod.log в папке игры."); }
  }
}
static void toggle_mirror(void) {
  mirror_on = !mirror_on;
  SetWindowTextW(C_T5, mirror_on ? L"Зеркало: вкл" : L"Зеркало: выкл");
  if (mirror_on) { say_ru(L"Зеркало включено: второй игрок повторяет действия первого. Переключись в игру (в уровне) и поиграй."); set_status(L"Зеркало включено: в игре второй игрок будет повторять то, что делает первый (проверка захвата действий на одном компьютере)."); }
  else { say_ru(L"Зеркало выключено."); set_status(L"Зеркало выключено."); }
}
static void send_test(u32 kind) {
  if (!lk) return;
  lk->test_kind = kind; lk->test_seq++; my_test_seq = lk->test_seq;
  say_ru(L"Проверка отправлена в игру: переключись в окно игры (25 секунд).");
  set_status(L"Проверка отправлена. Переключись в окно игры (в уровне): мод сам выполнит действие за второго игрока.");
}
/* ---------------- install / remove the game module (called by the MSI installer, no window) ---------------- */
/* the game module is carried inside this program as a resource:
 *   --extract-mod "<folder>"  writes <folder>\dinput8.dll and exits
 *   --install-mod "<game folder>"  (called by the installer) checks that LEGOLOTR.exe is there and writes our dinput8.dll
 *   --remove-mod "<game folder>"   (called by the uninstaller) deletes our dinput8.dll and the program's own leftovers (logs, extracted steam_api.dll) */
static void with_slash(u16 *d, const u16 *s, int cap) { wcopy(d, s, cap); { int n = wlen(d); if (n && d[n - 1] != '\\' && n + 1 < cap) { d[n] = '\\'; d[n + 1] = 0; } } }
static void say_box(const u16 *t, u32 icon) { MessageBoxW(0, t, L"Тера Кооп", icon | 0x10000u /* MB_SETFOREGROUND */); }
static int extract_mod(const u16 *folder) {
  const void *d; u32 sz; u16 p[460];
  if (!res_blob(RES_MOD, &d, &sz)) return 2;
  with_slash(p, folder, 440); wcat(p, L"dinput8.dll", 460);
  return write_file(p, d, sz) ? 0 : 5;
}
static int install_mod(const u16 *game) {
  const void *d; u32 sz; u16 g[460], t[460], tmp[470];
  if (!res_blob(RES_MOD, &d, &sz)) { say_box(L"Внутри программы нет модуля для игры. Установка отменена.", 0x10); return 2; }
  with_slash(g, game, 440);
  wcopy(t, g, 460); wcat(t, L"LEGOLOTR.exe", 460);
  if (GetFileAttributesW(t) == 0xFFFFFFFFu) { say_box(L"В выбранной папке нет файла LEGOLOTR.exe, значит это не папка игры LEGO Властелин колец. Установка отменена. Запусти установку снова и выбери папку, где лежит игра.", 0x10); return 10; }
  wcopy(t, g, 460); wcat(t, L"dinput8.dll", 460);
  wcopy(tmp, t, 470); wcat(tmp, L".zz", 470);
  if (!write_file(tmp, d, sz) || !MoveFileExW(tmp, t, 1 /* MOVEFILE_REPLACE_EXISTING */)) { DeleteFileW(tmp); say_box(L"Не удалось записать dinput8.dll в папку игры. Закрой игру и попробуй снова (или у папки игры нет прав на запись).", 0x10); return 11; }
  return 0;
}
static void delete_dir_files(const u16 *dir) { /* deletes the files of a folder, then the folder (if it is empty) */
  u16 pat[470], f[470]; u8 fd[592]; PV h;
  wcopy(pat, dir, 460); wcat(pat, L"\\*", 470);
  h = FindFirstFileW(pat, fd);
  if (h != (PV)-1) { do { const u16 *nm = (const u16 *)(fd + 44); if (!(*(u32 *)fd & 0x10)) { wcopy(f, dir, 460); wcat(f, L"\\", 470); wcat(f, nm, 470); DeleteFileW(f); } } while (FindNextFileW(h, fd)); FindClose(h); }
  RemoveDirectoryW(dir);
}
static int remove_mod(const u16 *game) {
  const void *d; u32 sz; u16 g[460], t[460], x[470]; int have = res_blob(RES_MOD, &d, &sz);
  with_slash(g, game, 440); wcopy(t, g, 460); wcat(t, L"dinput8.dll", 460);
  if (have && GetFileAttributesW(t) != 0xFFFFFFFFu && file_equals(t, d, sz)) { if (!DeleteFileW(t)) say_box(L"Не удалось удалить dinput8.dll из папки игры: похоже, игра запущена. Закрой игру и удали dinput8.dll вручную.", 0x30); }
  exe_dir(x, 440); wcat(x, L"TeraCoop_logs", 460); delete_dir_files(x);
  { u16 ml[470]; wcopy(ml, g, 460); wcat(ml, L"logs\\teracoop_mod.log", 470); DeleteFileW(ml); wcopy(ml, g, 460); wcat(ml, L"logs", 470); RemoveDirectoryW(ml); } /* the game module's own log (the folder goes only if empty) */
  { u16 dir[460]; u32 n = GetEnvironmentVariableW(L"LOCALAPPDATA", dir, 400);
    if (n && n < 400) { wcat(dir, L"\\TeraCoop", 440); wcopy(x, dir, 460); wcat(x, L"\\steam_api.dll", 470); DeleteFileW(x); RemoveDirectoryW(dir); } }
  return 0;
}
/* the value after a command-line option, quotes removed (returns 0 if the option is not there) */
static int cli_arg(const u16 *cl, const u16 *opt, u16 *out, int cap) {
  int k = 0;
  while (cl[k]) { int m = 0; while (opt[m] && cl[k + m] == opt[m]) m++;
    if (!opt[m]) { int q = 0; k += m; while (cl[k] == ' ' || cl[k] == '"') k++; while (cl[k] && cl[k] != '"' && q < cap - 1) out[q++] = cl[k++]; out[q] = 0; return 1; }
    k++; }
  return 0;
}

/* ---------------- drawing ---------------- */
static void box_px(PV dc, int l, int t, int r, int b, u32 fill, u32 line, int rad) {
  PV br = CreateSolidBrush(fill), pn = CreatePen(0, 1, line), ob = SelectObject(dc, br), op = SelectObject(dc, pn);
  RoundRect(dc, l, t, r, b, rad, rad);
  SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(br); DeleteObject(pn);
}
static void box(PV dc, int x, int y, int w, int h, u32 fill, u32 line, int rad) { box_px(dc, S(x), S(y), S(x + w) + 1, S(y + h) + 1, fill, line, S(rad)); }
static void dot(PV dc, int cx, int cy, int r, u32 col) {
  PV br = CreateSolidBrush(col), pn = CreatePen(0, 1, col), ob = SelectObject(dc, br), op = SelectObject(dc, pn);
  Ellipse(dc, S(cx - r), S(cy - r), S(cx + r) + 1, S(cy + r) + 1);
  SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(br); DeleteObject(pn);
}
static void txt(PV dc, PV font, u32 col, const u16 *t, int x, int y, int w, int h, u32 fl) {
  int rc[4]; rc[0] = S(x); rc[1] = S(y); rc[2] = S(x + w); rc[3] = S(y + h);
  SelectObject(dc, font); SetTextColor(dc, col); DrawTextW(dc, t, -1, rc, fl | 0x800 /* DT_NOPREFIX */);
}
#define DT_VC1 0x24 /* vcenter + single line */
static void cat(u16 *d, const u16 *a, int cap) { int n = 0; while (d[n]) n++; while (*a && n < cap - 1) d[n++] = *a++; d[n] = 0; }
static void my_name(u16 *out) {
  out[0] = 0;
  if (steam_ready && fMyName) { int c = MultiByteToWideChar(65001, 0, fMyName(fr), -1, out, 56); if (c <= 0) out[0] = 0; }
  if (!out[0]) { const u16 *f = L"Ты"; int i; for (i = 0; f[i]; i++) out[i] = f[i]; out[i] = 0; }
}
static void peer_nm(u16 *out) {
  int i; for (i = 0; peer_name[i] && i < 56; i++) out[i] = peer_name[i]; out[i] = 0;
  if (!out[0]) { const u16 *f = is_host ? L"Друг" : L"Хост"; for (i = 0; f[i]; i++) out[i] = f[i]; out[i] = 0; }
}
static void fmt_i(u16 *o, i32 v) { char t[12]; int n = 0, k = wlen(o); u32 u = v < 0 ? (u32)(-v) : (u32)v; if (v < 0) o[k++] = '-'; do { t[n++] = (char)('0' + u % 10); u /= 10; } while (u); while (n) o[k++] = (u16)t[--n]; o[k] = 0; }
static void fmt_ms(u16 *o, u32 v) { char t[12]; int n = 0, k = 0; do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v); while (n) o[k++] = (u16)t[--n]; o[k] = 0; cat(o, L" мс", 24); }
static void player_row(PV dc, int y, const u16 *name, const u16 *role, u32 dotc, u32 namec, const u16 *ping, u32 pingc) {
  box(dc, 24, y, 512, 28, COL_CARD2, COL_LINE, 8);
  dot(dc, 40, y + 14, 5, dotc);
  txt(dc, hFontB, namec, name, 58, y, 250, 28, DT_VC1 | 0x8000);
  txt(dc, hFontS, COL_MUTED, role, 314, y, 80, 28, DT_VC1);
  txt(dc, hFontB, pingc, ping, 400, y, 124, 28, DT_VC1 | 2);
}
static void card(PV dc, int y, int h, const u16 *title) {
  box(dc, 12, y, 536, h, COL_CARD, COL_LINE, 12);
  txt(dc, hFontB, COL_GOLD, title, 26, y + 5, 500, 22, DT_VC1);
}
#define COL_SUB RGBc(198, 190, 170)
static void paint(PV dc, int w, int h) {
  int rc[4]; int steam = (mode == M_STEAM), rm = in_room != 0; (void)h;
  SetBkMode(dc, 1 /* TRANSPARENT */);
  rc[0] = 0; rc[1] = 0; rc[2] = w; rc[3] = S(H_LOG) + S(40); FillRect(dc, rc, brBG);
  /* header: title and Steam state */
  { PV hb = CreateSolidBrush(COL_HEAD), gb = CreateSolidBrush(COL_GOLD); rc[3] = S(46); FillRect(dc, rc, hb); rc[1] = S(46); rc[3] = S(46) + 1; FillRect(dc, rc, gb); DeleteObject(hb); DeleteObject(gb); }
  txt(dc, hFontT, COL_GOLD, L"Тера Кооп", 16, 6, 340, 34, DT_VC1);
  if (steam) { int ok = steam_ready; dot(dc, 402, 14, 5, ok ? COL_GREEN : COL_RED); txt(dc, hFontS, ok ? COL_SUB : COL_RED, ok ? L"Steam подключён" : L"Steam не подключён", 412, 4, 136, 20, DT_VC1); }
  { u32 ms = mod_state; dot(dc, 402, 33, 5, ms >= 2 ? COL_GREEN : ms == 1 ? COL_RED : COL_MUTED);
    txt(dc, hFontS, ms >= 2 ? COL_SUB : ms == 1 ? COL_RED : COL_MUTED, ms == 0 ? L"Мод: игра не запущена" : ms == 1 ? L"Мод: версия игры не та" : ms == 2 ? L"Мод: в игре, на связи" : L"Мод: в игре, 2-й игрок есть", 412, 23, 136, 20, DT_VC1); }
  if (!rm) {
    txt(dc, hFont, COL_SUB, status_txt, 16, 94, 528, 38, 0x10 /* DT_WORDBREAK */);
    card(dc, 138, 72, L"Я создаю комнату");
    card(dc, 218, 72, steam ? L"Я вхожу в комнату друга — ввожу его код" : L"Я вхожу в комнату друга — ввожу адрес хоста");
    box(dc, 24, 248, 394, 34, COL_CARD2, COL_LINE, 8);
    txt(dc, hFontS, COL_MUTED, steam ? L"Как это работает: один создаёт комнату и называет код (или зовёт друга через Steam:\nправый клик на друге → «Пригласить в игру»). Второй вводит код и нажимает «Войти»."
                                     : L"Как это работает: оба включают Radmin VPN. Хост создаёт комнату и называет адрес,\nдруг вводит этот адрес и нажимает «Войти».", 16, 298, 528, 40, 0x10);
  } else {
    u16 me[64], pr[64], mp[40], a[40]; int k; u32 pc = COL_MUTED;
    txt(dc, hFont, COL_SUB, status_txt, 16, 54, 528, 38, 0x10);
    card(dc, 98, 198, steam ? L"Комната  ·  через Steam" : L"Комната  ·  по IP (Radmin VPN)");
    box(dc, 24, 128, 394, 40, COL_CARD2, COL_LINE, 8);
    txt(dc, hFontS, COL_MUTED, steam ? L"код" : L"адрес", 36, 128, 60, 40, DT_VC1);
    txt(dc, hFontC, COL_GOLDHI, code_txt, 90, 128, 320, 40, DT_VC1 | 1);
    my_name(me); cat(me, L"  (ты)", 62); peer_nm(pr); if (!peer_present) { pr[0] = 0; cat(pr, L"ждём друга…", 62); }
    mp[0] = 0; cat(mp, L"это ты", 36);
    a[0] = 0;
    if (!peer_present) cat(a, L"—", 30); else if (last_rtt == 0xFFFFFFFFu) cat(a, L"проверяем…", 30);
    else { fmt_ms(a, last_rtt); pc = last_rtt < 80 ? COL_GREEN : last_rtt < 160 ? COL_GOLDHI : COL_RED; }
    for (k = 0; k < 2; k++) {
      int mine = (k == 0) == (is_host != 0); int y = 178 + k * 32;
      if (mine) player_row(dc, y, me, is_host ? L"хост" : L"гость", COL_GREEN, COL_TEXT, mp, COL_MUTED);
      else player_row(dc, y, pr, is_host ? L"гость" : L"хост", peer_present ? COL_GREEN : COL_MUTED, peer_present ? COL_TEXT : COL_MUTED, a, peer_present ? pc : COL_MUTED);
    }
    txt(dc, hFontS, COL_MUTED, L"Не закрывай это окно, пока вы играете.", 16, 304, 528, 22, DT_VC1 | 1);
  }
  txt(dc, hFontS, COL_MUTED, L"v0.16", 460, 346, 88, 26, DT_VC1 | 2);
  if (log_open) { box(dc, 12, 380, 536, 190, COL_CARD2, COL_LINE, 10); txt(dc, hFontS, COL_MUTED, L"Проверка (F10 в игре = автонажатия)", 24, 384, 220, 18, DT_VC1);
    { u16 t[120]; t[0] = 0; if (!cap_sv) cat(t, L"Игрок 1: нет данных", 120); else { cat(t, cap_friend ? L"Друг:  X " : L"Игрок 1:  X ", 120); fmt_i(t, cap_sx); cat(t, L"  Y ", 120); fmt_i(t, cap_sy); if (cap_sb & BT_JUMP) cat(t, L"  прыжок", 120); if (cap_sb & BT_ACTION) cat(t, L"  действие", 120); if (cap_sb & BT_SPECIAL) cat(t, L"  особое", 120); }
      txt(dc, hFontS, cap_sv ? COL_GOLDHI : COL_MUTED, t, 244, 384, 296, 18, DT_VC1 | 2); } }
}
static void draw_button(const u8 *d) {
  u32 id = *(const u32 *)(d + 4), st = *(const u32 *)(d + 16); PV hw = *(PV const *)(d + 20), dc = *(PV const *)(d + 24); const int *rc = (const int *)(d + 28);
  int dis = (st & 4) != 0, press = (st & 1) != 0, hot = (hw == hover_wnd) && !dis, tab = (id == ID_R_STEAM || id == ID_R_IP || id == ID_LOGBTN), sel = 0, tst = (id >= ID_T_JOIN && id <= ID_T_REC);
  u32 fill, line, tcol; u16 t[90]; int rr[4];
  if (tab) {
    sel = (id != ID_LOGBTN) && ((id == ID_R_STEAM) == (mode == M_STEAM));
    if (sel) { fill = COL_GOLD; line = fill; tcol = COL_GOLDDK; }
    else { fill = hot ? COL_CARD : COL_CARD2; line = hot ? COL_GOLD : COL_LINE; tcol = hot ? COL_TEXT : COL_MUTED; }
  } else if (id == ID_CREATE || id == ID_JOIN) {
    fill = dis ? RGBc(64, 57, 40) : press ? RGBc(176, 138, 56) : hot ? COL_GOLDHI : COL_GOLD; line = fill; tcol = dis ? RGBc(122, 110, 84) : COL_GOLDDK;
  } else if (id == ID_LEAVE) {
    fill = press ? RGBc(98, 40, 36) : hot ? RGBc(160, 64, 56) : RGBc(132, 52, 46); line = fill; tcol = COL_TEXT;
  } else {
    fill = dis ? COL_CARD2 : press ? COL_CARD2 : hot ? COL_BTN2HI : COL_BTN2; line = hot ? COL_GOLD : COL_LINE; tcol = dis ? RGBc(104, 98, 86) : COL_TEXT;
  }
  rr[0] = rc[0]; rr[1] = rc[1]; rr[2] = rc[2]; rr[3] = rc[3];
  FillRect(dc, rr, tab ? brBG : tst ? brCard2 : brCard);
  box_px(dc, rc[0], rc[1], rc[2], rc[3], fill, line, S(10));
  GetWindowTextW(hw, t, 88);
  SetBkMode(dc, 1); SelectObject(dc, id == ID_LOGBTN ? hFontS : hFontB); SetTextColor(dc, tcol);
  rr[1] += press ? 1 : 0; DrawTextW(dc, t, -1, rr, DT_VC1 | 1 | 0x800);
}

/* ---------------- window ---------------- */
static PV mk(const u16 *cls, const u16 *txt_, u32 style, int x, int y, int w, int h, int id) {
  PV c = CreateWindowExW(0, cls, txt_, style | 0x50000000u /* WS_CHILD|WS_VISIBLE */, S(x), S(y), S(w), S(h), hMain, (PV)(u32)id, hInst, 0);
  if (c && hFont) SendMessageW(c, 0x30 /* WM_SETFONT */, (u32)hFont, 1);
  return c;
}
static PV mkbtn(const u16 *txt_, int x, int y, int w, int h, int id) { PV c = mk(L"BUTTON", txt_, 0xB /* BS_OWNERDRAW */ | 0x10000u /* WS_TABSTOP */, x, y, w, h, id); if (nbtns < 16) btns[nbtns++] = c; return c; }
static int is_btn(PV w) { int i; for (i = 0; i < nbtns; i++) if (btns[i] == w) return 1; return 0; }
static void dark_theme(PV h) {
  typedef i32 (__stdcall *F_swt)(PV, const u16 *, const u16 *); static PV ux; static F_swt f;
  if (!ux) { ux = LoadLibraryW(L"uxtheme.dll"); if (ux) f = (F_swt)GetProcAddress(ux, "SetWindowTheme"); }
  if (f) f(h, L"DarkMode_Explorer", 0);
}
static void dark_titlebar(PV h) {
  typedef i32 (__stdcall *F_dwm)(PV, u32, const void *, u32); PV d = LoadLibraryW(L"dwmapi.dll"); F_dwm f; u32 on = 1, c1 = COL_HEAD, c2 = COL_TEXT;
  if (!d) return; f = (F_dwm)GetProcAddress(d, "DwmSetWindowAttribute"); if (!f) return;
  if (f(h, 20, &on, 4) != 0) f(h, 19, &on, 4);
  f(h, 35, &c1, 4); f(h, 34, &c1, 4); f(h, 36, &c2, 4); /* Windows 11: caption, border, caption text colors */
}
static void set_cue(void) { SendMessageW(C_JOINED, 0x1501 /* EM_SETCUEBANNER */, 1, (i32)(u32)(mode == M_STEAM ? L"например AB3K-9QX" : L"например 26.12.34.56")); }
static void build_ui(void) {
  C_STEAM = mkbtn(L"Steam  ·  код или приглашение", 12, 54, 262, 32, ID_R_STEAM);
  C_IP = mkbtn(L"По IP  ·  Radmin VPN", 286, 54, 262, 32, ID_R_IP);
  C_CREATE = mkbtn(L"Создать комнату", 24, 168, 512, 34, ID_CREATE);
  C_JOINED = mk(L"EDIT", L"", 0x80 /* ES_AUTOHSCROLL */ | 0x10000u, 34, 255, 374, 20, ID_JOINED);
  C_JOIN = mkbtn(L"Войти", 430, 248, 106, 34, ID_JOIN);
  C_COPY = mkbtn(L"Копировать", 430, 128, 106, 40, ID_COPY);
  C_LEAVE = mkbtn(L"Выйти из комнаты", 24, 248, 512, 36, ID_LEAVE);
  C_LOGBTN = mkbtn(L"Показать журнал", 12, 346, 150, 26, ID_LOGBTN);
  C_T1 = mkbtn(L"2-й игрок (F2)", 22, 406, 98, 28, ID_T_JOIN);
  C_T2 = mkbtn(L"Идти вправо", 126, 406, 98, 28, ID_T_RIGHT);
  C_T3 = mkbtn(L"Прыжок", 230, 406, 98, 28, ID_T_JUMP);
  C_T4 = mkbtn(L"Действие", 334, 406, 98, 28, ID_T_ACT);
  C_T5 = mkbtn(L"Зеркало: выкл", 438, 406, 98, 28, ID_T_REC);
  C_LOG = mk(L"EDIT", L"", 4 | 0x40 | 0x800 | 0x200000u, 22, 442, 516, 122, ID_LOG);
  SendMessageW(C_LOG, 0x30, (u32)hFontS, 1);
  dark_theme(C_LOG);
  set_cue(); layout();
}
static void tick(void) {
  u32 now = GetTickCount();
  { i32 pt[2]; PV w = 0; if (GetCursorPos(pt)) w = WindowFromPoint(pt[0], pt[1]); if (!is_btn(w)) w = 0;
    if (w != hover_wnd) { PV old = hover_wnd; hover_wnd = w; if (old) InvalidateRect(old, 0, 0); if (w) InvalidateRect(w, 0, 0); } }
  if (!steam_tried && mode == M_STEAM) { steam_start(); status_idle(); }
  if (steam_ready) pump();
  if (usock != (u32)-1) ip_tick(now);
  link_tick(now);
  if (in_room && peer_present && (i32)(now - dbg_last) >= 5000) {
    dbg_last = now;
    wr(); ws(L"сеть: ACT принято "); wd(dbg_rx_act); ws(L" (отклонено "); wd(dbg_rx_rej); ws(L"), PING "); wd(dbg_rx_ping); ws(L", PONG "); wd(dbg_rx_pong); ws(L", другое "); wd(dbg_rx_other); ws(L"; позиций принято "); wd(dbg_rx_pos); ws(L", отправлено "); wd(dbg_tx_pos); ws(L"; кнопки BTN принято "); wd(dbg_rx_btn); ws(L", отправлено "); wd(dbg_tx_btn); ws(L"; ошибок отправки "); wd(dbg_tx_err); ws(L", код "); wd(dbg_tx_res); ws(L"; host="); wd((u32)is_host); ws(L" peer="); wd(peer_steam ? 1u : 0u); log_line();
  }
  if (in_room && peer_present && now - last_ping >= 2000u) {
    last_ping = now; ping_seq++;
    if (mode == M_STEAM) { if (peer_steam) steam_send_to(peer_steam, TAG_PING, ping_seq, now); }
    else ip_send(TAG_PING, ping_seq, now);
  }
}
static i32 __stdcall WndProc(PV h, u32 msg, u32 wp, i32 lp) {
  switch (msg) {
    case 0x113: /* WM_TIMER */ tick(); return 0;
    case 0x14: /* WM_ERASEBKGND */ return 1;
    case 0x0F: { /* WM_PAINT: everything is drawn into a memory bitmap first (no flicker) */
      u8 ps[64]; int rc[4]; PV dc = BeginPaint(h, ps), mdc, bmp, ob; int w, hh;
      GetClientRect(h, rc); w = rc[2]; hh = rc[3];
      mdc = CreateCompatibleDC(dc); bmp = CreateCompatibleBitmap(dc, w, hh); ob = SelectObject(mdc, bmp);
      paint(mdc, w, hh); BitBlt(dc, 0, 0, w, hh, mdc, 0, 0, 0x00CC0020 /* SRCCOPY */);
      SelectObject(mdc, ob); DeleteObject(bmp); DeleteDC(mdc); EndPaint(h, ps); return 0;
    }
    case 0x2B: { /* WM_DRAWITEM */
      const u8 *d = (const u8 *)(u32)lp;
      if (*(const u32 *)d == 4 /* ODT_BUTTON */) draw_button(d);
      return 1;
    }
    case 0x133: case 0x138: /* WM_CTLCOLOREDIT / STATIC: dark inputs */
      SetTextColor((PV)wp, COL_TEXT); SetBkColor((PV)wp, COL_CARD2); return (i32)(u32)brCard2;
    case 0x111: { /* WM_COMMAND */
      u32 id = wp & 0xFFFF, code = (wp >> 16) & 0xFFFF;
      if (id == 1 /* IDOK (Enter) */) { if (GetFocus() == C_JOINED && !in_room) on_join(); return 0; }
      if (code != 0 /* BN_CLICKED */) return 0;
      if (id == ID_R_STEAM) { set_mode(M_STEAM); set_cue(); SetWindowTextW(C_JOINED, L""); }
      else if (id == ID_R_IP) { set_mode(M_IP); set_cue(); SetWindowTextW(C_JOINED, L""); }
      else if (id == ID_CREATE) on_create();
      else if (id == ID_JOIN) on_join();
      else if (id == ID_COPY) on_copy();
      else if (id == ID_LEAVE) leave_room();
      else if (id == ID_LOGBTN) { log_open = !log_open; layout(); apply_size(); }
      else if (id == ID_T_JOIN) send_test(1);
      else if (id == ID_T_RIGHT) send_test(2);
      else if (id == ID_T_JUMP) send_test(3);
      else if (id == ID_T_ACT) send_test(4);
      else if (id == ID_T_REC) toggle_mirror();
      return 0;
    }
    case 0x10: /* WM_CLOSE */
      if (in_room || joining) { if (mode == M_STEAM) { if (steam_ready && lobby_set) fLeave(mm, lobby); } else ip_close(); }
      rp_clear();
      if (steam_ready && fShutdown) fShutdown();
      DestroyWindow(h); return 0;
    case 0x2: /* WM_DESTROY */ PostQuitMessage(0); return 0;
  }
  return DefWindowProcW(h, msg, wp, lp);
}
void WinMainCRTStartup(void) {
  u16 path[420], dir[420]; int n, i; u32 wc[12]; u8 msg[28]; u8 rc[16];
  t0 = GetTickCount(); hInst = GetModuleHandleW(0);
  { const u16 *cl = GetCommandLineW(); u16 a[420]; /* command-line modes for the installer: no window */
    if (cli_arg(cl, L"--install-mod", a, 420)) ExitProcess((u32)install_mod(a));
    if (cli_arg(cl, L"--remove-mod", a, 420)) ExitProcess((u32)remove_mod(a));
    if (cli_arg(cl, L"--extract-mod", a, 420)) ExitProcess((u32)extract_mod(a)); }
  n = (int)GetModuleFileNameW(0, path, 380); while (n > 0 && path[n - 1] != '\\') n--;
  for (i = 0; i < n; i++) dir[i] = path[i]; dir[n > 0 ? n - 1 : 0] = 0; SetCurrentDirectoryW(dir);
  { int k; const u16 *f = L"TeraCoop_logs";
    for (k = 0; f[k]; k++) path[n + k] = f[k]; path[n + k] = 0; CreateDirectoryW(path, 0);
    { /* one file per run: TeraCoop_logs\\<solo|host|guest>_program_YYYY-MM-DD_HHhMMmSSs.log (local time of this PC); renamed to host_ / guest_ as soon as the room exists */
      u16 st[8]; int m = 0; for (k = 0; k < n + 14; k++) logdir[k] = path[k]; /* "<program folder>TeraCoop_logs" */
      logdir[n + 13] = '\\'; logdir[n + 14] = 0;
      GetLocalTime(st);
      logstamp[m++] = (u16)('0' + (st[0] / 1000) % 10); logstamp[m++] = (u16)('0' + (st[0] / 100) % 10); logstamp[m++] = (u16)('0' + (st[0] / 10) % 10); logstamp[m++] = (u16)('0' + st[0] % 10); logstamp[m++] = '-';
      logstamp[m++] = (u16)('0' + (st[1] / 10) % 10); logstamp[m++] = (u16)('0' + st[1] % 10); logstamp[m++] = '-';
      logstamp[m++] = (u16)('0' + (st[3] / 10) % 10); logstamp[m++] = (u16)('0' + st[3] % 10); logstamp[m++] = '_';
      logstamp[m++] = (u16)('0' + (st[4] / 10) % 10); logstamp[m++] = (u16)('0' + st[4] % 10); logstamp[m++] = 'h';
      logstamp[m++] = (u16)('0' + (st[5] / 10) % 10); logstamp[m++] = (u16)('0' + st[5] % 10); logstamp[m++] = 'm';
      logstamp[m++] = (u16)('0' + (st[6] / 10) % 10); logstamp[m++] = (u16)('0' + st[6] % 10); logstamp[m++] = 's'; logstamp[m] = 0;
      log_name(path, 0); }
    hLogFile = CreateFileW(path, 0x40000000u, 5 /* read + delete sharing: renamed later */, 0, 2 /* CREATE_ALWAYS */, 0x80, 0); }
  { PV dc = GetDC(0); if (dc) { dpi = (u32)GetDeviceCaps(dc, 88 /* LOGPIXELSX */); ReleaseDC(0, dc); } if (dpi < 96 || dpi > 480) dpi = 96; }
  hFont = CreateFontW(-(i32)((9 * dpi + 36) / 72), 0, 0, 0, 400, 0, 0, 0, 1 /* DEFAULT_CHARSET */, 0, 0, 5 /* CLEARTYPE */, 0, L"Segoe UI");
  hFontB = CreateFontW(-(i32)((9 * dpi + 36) / 72), 0, 0, 0, 600, 0, 0, 0, 1, 0, 0, 5, 0, L"Segoe UI");
  hFontT = CreateFontW(-(i32)((17 * dpi + 36) / 72), 0, 0, 0, 700, 0, 0, 0, 1, 0, 0, 5, 0, L"Segoe UI");
  hFontS = CreateFontW(-(i32)((17 * dpi + 72) / 144), 0, 0, 0, 400, 0, 0, 0, 1, 0, 0, 5, 0, L"Segoe UI");
  hFontC = CreateFontW(-(i32)((15 * dpi + 36) / 72), 0, 0, 0, 700, 0, 0, 0, 1, 0, 0, 5, 0, L"Consolas");
  if (!hFont) hFont = GetStockObject(17 /* DEFAULT_GUI_FONT */);
  if (!hFontB) hFontB = hFont; if (!hFontT) hFontT = hFont; if (!hFontS) hFontS = hFont; if (!hFontC) hFontC = hFont;
  brBG = CreateSolidBrush(COL_BG); brCard = CreateSolidBrush(COL_CARD); brCard2 = CreateSolidBrush(COL_CARD2);
  wc[0] = 48; wc[1] = 3; wc[2] = (u32)WndProc; wc[3] = 0; wc[4] = 0; wc[5] = (u32)hInst; wc[6] = (u32)LoadIconW(hInst, (const u16 *)1); wc[7] = (u32)LoadCursorW(0, (const u16 *)32512);
  wc[8] = (u32)brBG; wc[9] = 0; wc[10] = (u32)L"TeraCoopWnd"; wc[11] = wc[6];
  RegisterClassExW(wc);
  *(i32 *)rc = 0; *(i32 *)(rc + 4) = 0; *(i32 *)(rc + 8) = S(LAY_W); *(i32 *)(rc + 12) = S(H_BASE);
  AdjustWindowRectEx(rc, 0x02CA0000u, 0, 0);
  hMain = CreateWindowExW(0, L"TeraCoopWnd", L"Тера Кооп", 0x02CA0000u /* caption, sysmenu, minimize box, clip children */, (i32)0x80000000, (i32)0x80000000, *(i32 *)(rc + 8) - *(i32 *)rc, *(i32 *)(rc + 12) - *(i32 *)(rc + 4), 0, 0, hInst, 0);
  dark_titlebar(hMain);
  build_ui(); link_open();
  say_ru(L"Тера Кооп v0.16 запущен.");
  reset_room(); status_idle();
  ShowWindow(hMain, 1); UpdateWindow(hMain);
  SetTimer(hMain, 1, 25, 0);
  while (GetMessageW(msg, 0, 0, 0) > 0) { if (!IsDialogMessageW(hMain, msg)) { TranslateMessage(msg); DispatchMessageW(msg); } }
  ExitProcess(0);
}
