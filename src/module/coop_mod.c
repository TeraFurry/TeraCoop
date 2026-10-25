/* TeraCoop - game module v1.0 (dinput8.dll proxy) for LEGO The Lord of the Rings (Steam 214510, build PE timestamp 0x50d201cc).
 * Clean version of the research plugin (v1.1 adds read-only capture of the local player 1: walking axes and buttons, written into the link block for the program): it does only what the co-op mod needs.
 *   1. forwards DirectInput8Create to the real system dinput8.dll (the game works as always);
 *   2. checks that the game build is the supported one - otherwise it does nothing at all;
 *   3. talks to the program over a shared-memory block (link.h);
 *   4. joins the second player by pressing the game's own F2 key (only when the program asks and player 2 is absent);
 *   5. captures the actions of the local player 1 (axes + jump/action/special) from the game memory, read only, for the program;
 *   6. holds the player-2 keys (arrows, scan codes 0x35 jump, 0x34 action, 0x28 special) that match the actions the program gives it.
 * Safety: keys are sent only while the game window is the active window, and are all released when the program stops talking, the window loses focus, or the game closes.
 * It never writes into game memory, no hooks, no breakpoints, no threads of the game are touched. Log: logs\teracoop_mod.log next to LEGOLOTR.exe (older logs are renamed teracoop_mod_NNN.log). */
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef int i32; typedef void *PV;
#define K __declspec(dllimport) __stdcall
K PV GetModuleHandleW(const u16 *);
K u32 GetModuleFileNameW(PV, u16 *, u32);
K PV CreateThread(PV, u32, u32 (__stdcall *)(PV), PV, u32, u32 *);
K void Sleep(u32);
K u32 GetTickCount(void);
K PV CreateFileW(const u16 *, u32, u32, PV, u32, u32, PV);
K int WriteFile(PV, const void *, u32, u32 *, PV);
K int CreateDirectoryW(const u16 *, PV);
K int MoveFileW(const u16 *, const u16 *);
K u32 GetFileAttributesW(const u16 *);
K u32 GetSystemDirectoryW(u16 *, u32);
K PV LoadLibraryW(const u16 *);
K PV GetProcAddress(PV, const char *);
K PV OpenFileMappingW(u32, int, const u16 *);
K PV MapViewOfFile(PV, u32, u32, u32, u32);
K u32 GetCurrentProcessId(void);
K int ReadProcessMemory(PV, const void *, void *, u32, u32 *);
K int WriteProcessMemory(PV, PV, const void *, u32, u32 *);
K int DisableThreadLibraryCalls(PV);
K PV GetForegroundWindow(void);
K u32 GetWindowThreadProcessId(PV, u32 *);
K u32 SendInput(u32, void *, int);
K u16 GetAsyncKeyState(int);
K void GetLocalTime(void *);
K int CloseHandle(PV);
K int ReadFile(PV, void *, u32, u32 *, PV);
K u32 GetLastError(void);
K int CopyFileW(const u16 *, const u16 *, int);
K u32 GetEnvironmentVariableW(const u16 *, u16 *, u32);
K void GetSystemTime(void *);
#include "link.h"

#define IMAGEBASE 0x400000u
#define SIZEOFIMAGE 0x15e6000u
#define TIMESTAMP 0x50d201ccu
#define VA_P2LIST 0x015ca000u /* head of the list that holds player 2 (0 = player 2 absent), found in step 31/33 of the notes */
#define MOD_VERSION 210u
#define VA_PTABLE 0x015b30e0u /* table of the four player characters; [0] = local player 1 (steps 61-63 of the notes) */
#define VT_PAD 0x010df7d8u    /* vtable of the character pad object (+4 owner, +0x18/+0x1c axes, +0x0C -> record whose +4 holds the held buttons) */

int _fltused = 1;
void *memcpy(void *d, const void *s, u32 n) { u8 *a = d; const u8 *b = s; while (n--) *a++ = *b++; return d; }
void *memset(void *d, int c, u32 n) { u8 *a = d; while (n--) *a++ = (u8)c; return d; }

/* ---------------- log ---------------- */
static PV hlog; static u32 t0, base; static u16 dirw[420]; static int dirn;
static char buf[1000]; static int bl;
static void bs(const char *s) { while (*s && bl < 960) buf[bl++] = *s++; }
static void bd(u32 v) { char t[12]; int n = 0; do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v); while (n && bl < 960) buf[bl++] = t[--n]; }
static void bh(u32 v) { const char *d = "0123456789abcdef"; int i; bs("0x"); for (i = 28; i >= 0; i -= 4) if (bl < 960) buf[bl++] = d[(v >> i) & 15]; }
static void bi(i32 v) { if (v < 0) { bs("-"); bd((u32)(-v)); } else bd((u32)v); }
static void bbtn(u32 b) { if (!(b & 7u)) { bs("none"); return; } if (b & 1u) bs("[action]"); if (b & 2u) bs("[special]"); if (b & 4u) bs("[jump]"); } /* BT_ACTION, BT_SPECIAL, BT_JUMP */
static void b2(u32 v) { if (bl < 960) buf[bl++] = (char)('0' + (v / 10) % 10); if (bl < 960) buf[bl++] = (char)('0' + v % 10); }
static void lbegin(void) { u16 st[8]; bl = 0; bd(GetTickCount() - t0); bs(" ms  "); GetSystemTime(st); bs("[UTC "); b2(st[4]); bs(":"); b2(st[5]); bs(":"); b2(st[6]); bs("] "); }
static void lend(void) { u32 w; buf[bl++] = '\r'; buf[bl++] = '\n'; if (hlog && hlog != (PV)-1) WriteFile(hlog, buf, (u32)bl, &w, 0); }
static void say(const char *s) { lbegin(); bs(s); lend(); }
static void mkpath(u16 *o, const u16 *name) { int i, k; for (i = 0; i < dirn; i++) o[i] = dirw[i]; for (k = 0; name[k]; k++) o[i++] = name[k]; o[i] = 0; }
/* a file name with the start time of this PC (local time) in it: logs\\<pre>YYYY-MM-DD_HH-MM-SS<ext>; the date is taken once so that all files of a run share it */
static u16 stamp[28]; static int stamp_n; static u32 cur_role; /* 0 solo (no room), 1 host, 2 guest: part of every log file name */
static void mk_stamp(void) {
  u16 st[8]; int n = 0; GetLocalTime(st);
  stamp[n++] = (u16)('0' + (st[0] / 1000) % 10); stamp[n++] = (u16)('0' + (st[0] / 100) % 10); stamp[n++] = (u16)('0' + (st[0] / 10) % 10); stamp[n++] = (u16)('0' + st[0] % 10); stamp[n++] = '-';
  stamp[n++] = (u16)('0' + (st[1] / 10) % 10); stamp[n++] = (u16)('0' + st[1] % 10); stamp[n++] = '-';
  stamp[n++] = (u16)('0' + (st[3] / 10) % 10); stamp[n++] = (u16)('0' + st[3] % 10); stamp[n++] = '_';
  stamp[n++] = (u16)('0' + (st[4] / 10) % 10); stamp[n++] = (u16)('0' + st[4] % 10); stamp[n++] = 'h';
  stamp[n++] = (u16)('0' + (st[5] / 10) % 10); stamp[n++] = (u16)('0' + st[5] % 10); stamp[n++] = 'm';
  stamp[n++] = (u16)('0' + (st[6] / 10) % 10); stamp[n++] = (u16)('0' + st[6] % 10); stamp[n++] = 's'; stamp[n] = 0; stamp_n = n;
}
/* logs\<host|guest|solo>_<kind>_<YYYY-MM-DD>_<HH>h<MM>m<SS>s<suffix> - the date is taken once, so all files of a run share it */
static void mk_logpath(u16 *out, const u16 *kind, const u16 *suffix) {
  u16 nm[120]; int n = 0, i; const u16 *r = cur_role == 1 ? L"host" : cur_role == 2 ? L"guest" : L"solo"; static const u16 pre[] = L"logs\\";
  if (!stamp_n) mk_stamp();
  for (i = 0; pre[i]; i++) nm[n++] = pre[i];
  for (i = 0; r[i]; i++) nm[n++] = r[i]; nm[n++] = '_';
  for (i = 0; kind[i]; i++) nm[n++] = kind[i]; nm[n++] = '_';
  for (i = 0; i < stamp_n; i++) nm[n++] = stamp[i];
  for (i = 0; suffix[i]; i++) nm[n++] = suffix[i];
  nm[n] = 0; mkpath(out, nm);
}
/* files named before the role was known (solo) get renamed when the program says host or guest */
typedef struct { u16 cur[460]; const u16 *kind, *suffix; int used; } RenFile; static RenFile renf[4];
static void ren_add(const u16 *path, const u16 *kind, const u16 *suffix) { int i, k; for (i = 0; i < 4; i++) if (!renf[i].used) { for (k = 0; path[k]; k++) renf[i].cur[k] = path[k]; renf[i].cur[k] = 0; renf[i].kind = kind; renf[i].suffix = suffix; renf[i].used = 1; return; } }
static void ren_all(void) { int i; for (i = 0; i < 4; i++) if (renf[i].used) { u16 np[460]; int k; mk_logpath(np, renf[i].kind, renf[i].suffix); if (MoveFileW(renf[i].cur, np)) { for (k = 0; np[k]; k++) renf[i].cur[k] = np[k]; renf[i].cur[k] = 0; } } }
static void open_log(void) {
  u16 p[460]; /* one file per run; every line also carries the UTC clock so that the logs of two PCs can be put side by side */
  mkpath(p, L"logs"); CreateDirectoryW(p, 0);
  mk_logpath(p, L"module", L".log");
  hlog = CreateFileW(p, 0x40000000u, 5 /* read + delete sharing: the file is renamed when the role (host / guest) becomes known */, 0, 2 /* CREATE_ALWAYS */, 0x80, 0);
  ren_add(p, L"module", L".log");
}

/* ---------------- keys ---------------- */
enum { KB_LEFT, KB_RIGHT, KB_UP, KB_DOWN, KB_ACTION, KB_SPECIAL, KB_JUMP, KB_N };
static u8 KSC[KB_N] = { 0x4B, 0x4D, 0x48, 0x50, 0x34, 0x28, 0x35 };   /* scan codes of the player-2 keys (arrows are "extended" keys); replaced by the keys of Mappings.dat of this PC, see load_bindings */
static u8 KEXT[KB_N] = { 1, 1, 1, 1, 0, 0, 0 };
static const u8 KSC_DEF[KB_N] = { 0x4B, 0x4D, 0x48, 0x50, 0x34, 0x28, 0x35 }, KEXT_DEF[KB_N] = { 1, 1, 1, 1, 0, 0, 0 };
static u32 keys_now;
static void send_sc(u8 sc, int down, int ext) { u32 in[7]; memset(in, 0, sizeof in); in[0] = 1; in[1] = ((u32)sc << 16); in[2] = 8u /* SCANCODE */ | (down ? 0u : 2u) | (ext ? 1u : 0u); SendInput(1, in, 28); }
static void set_keys(u32 want) {
  int i;
  if (want != keys_now) { static const char *kn[KB_N] = { "left", "right", "up", "down", "action", "special", "jump" };
    lbegin(); bs("keys: player-2 keys now held: "); if (!want) bs("none"); for (i = 0; i < KB_N; i++) if (want & (1u << i)) { bs(kn[i]); bs(" "); } bs("(was "); bh(keys_now); bs(" now "); bh(want); bs(")"); lend(); }
  for (i = 0; i < KB_N; i++) { u32 b = 1u << i; if ((want & b) != (keys_now & b)) send_sc(KSC[i], (want & b) != 0, KEXT[i]); }
  keys_now = want;
}
/* ---------------- the controls of THIS PC (Mappings.dat) ----------------
 * The program sends only "player 2 does: left / right / up / down / action / special / jump"; the module turns that into the keys that THIS game has bound to player 2 ("Keyboard2" block of
 * %APPDATA%\\Warner Bros. Interactive Entertainment\\LEGO The Lord of the Rings\\Mappings.dat), so nobody has to change his keys. File format (found on the 25.10 test): 'VPAM' header, then blocks of 0x9C bytes starting at +0x0C,
 * every block starts with its name ("Keyboard2", "Keyboard1", ...); at block+0x64 four axes of 3 bytes (0x0A, negative key, positive key; axis 0 = left/right, axis 1 = up/down), at block+0x70 button slots of 2 bytes
 * (0x07, key; slot 11 = special, 12 = jump, 13 = action). Keys are DirectInput codes (+0x80 = extended key). Anything unexpected = the built-in defaults stay and the log says so. */
static u8 cfgbuf[16384];
static int cfg_path(u16 *out, const u16 *file) { u16 app[260]; static const u16 dir[] = L"\\Warner Bros. Interactive Entertainment\\LEGO The Lord of the Rings\\"; u32 n = GetEnvironmentVariableW(L"APPDATA", app, 250); int a, b; if (!n || n >= 250) return 0; for (a = 0; app[a]; a++) out[a] = app[a]; for (b = 0; dir[b]; b++) out[a + b] = dir[b]; a += b; for (b = 0; file[b]; b++) out[a + b] = file[b]; out[a + b] = 0; return 1; }
static int cfg_read(const u16 *file, u32 *got, u32 *err) { u16 p[420]; PV h; int ok; *got = 0; *err = 0; if (!cfg_path(p, file)) { *err = 1; return 0; }
  h = CreateFileW(p, 0x80000000u, 7u /* share read + write + delete: the game may keep the file open */, 0, 3 /* OPEN_EXISTING */, 0x80, 0);
  if (h == (PV)-1 || !h) { *err = GetLastError(); return 0; }
  ok = ReadFile(h, cfgbuf, sizeof cfgbuf, got, 0); if (!ok) *err = GetLastError(); CloseHandle(h); return ok && *got > 0 && *got < sizeof cfgbuf; }
static void cfg_copy(const u16 *file, const char *what, const u16 *kind, const u16 *ext) { u32 got, err; if (cfg_read(file, &got, &err)) { u16 d[460]; PV h; mk_logpath(d, kind, ext); h = CreateFileW(d, 0x40000000u, 5, 0, 2, 0x80, 0); if (h != (PV)-1 && h) { u32 w; WriteFile(h, cfgbuf, got, &w, 0); CloseHandle(h); ren_add(d, kind, ext); lbegin(); bs("config copy: "); bs(what); bs(" "); bd(got); bs(" bytes copied into the logs folder"); lend(); return; } err = GetLastError(); }
  lbegin(); bs("config copy: "); bs(what); bs(" FAILED, windows error "); bd(err); lend(); }
static int blk_find(u32 n, const char *name, u32 nth, u32 *st) { u32 o; for (o = 12; o + 0x9C <= n; o += 0x9C) { int k = 0; while (name[k] && cfgbuf[o + k] == (u8)name[k]) k++; if (!name[k] && cfgbuf[o + k] == 0) { if (nth == 0) { *st = o; return 1; } nth--; } } return 0; }
static int blk_keys(u32 st, u8 *dik) { /* dik[KB_N] in the order of the KB_ enum; 0 = not found */
  u32 a; for (a = 0; a < 2; a++) { u32 q = st + 0x64 + 3 * a; if (cfgbuf[q] != 0x0A || !cfgbuf[q + 1] || !cfgbuf[q + 2]) return 0; dik[a ? KB_UP : KB_LEFT] = cfgbuf[q + 1]; dik[a ? KB_DOWN : KB_RIGHT] = cfgbuf[q + 2]; }
  { u32 sl[3] = { 11, 12, 13 }, k; int idx[3] = { KB_SPECIAL, KB_JUMP, KB_ACTION }; for (k = 0; k < 3; k++) { u32 q = st + 0x70 + 2 * sl[k]; if (cfgbuf[q] != 0x07 || !cfgbuf[q + 1]) return 0; dik[idx[k]] = cfgbuf[q + 1]; } }
  return 1; }
static const char *KBN[KB_N] = { "left", "right", "up", "down", "action", "special", "jump" };
static void load_bindings(const char *why) {
  u32 got, err, st, i; u8 p2[KB_N], p1[KB_N]; int ok2 = 0, ok1 = 0, diff = 0;
  if (keys_now) return; /* never change the table while a key is held */
  if (!cfg_read(L"Mappings.dat", &got, &err)) { lbegin(); bs("bindings ("); bs(why); bs("): Mappings.dat could not be read (windows error "); bd(err); bs(") - the default keys stay"); lend(); return; }
  if (got < 12 || cfgbuf[0] != 'V' || cfgbuf[1] != 'P' || cfgbuf[2] != 'A' || cfgbuf[3] != 'M') { lbegin(); bs("bindings ("); bs(why); bs("): Mappings.dat has an unknown format ("); bd(got); bs(" bytes) - the default keys stay"); lend(); return; }
  memset(p2, 0, sizeof p2); memset(p1, 0, sizeof p1);
  if (blk_find(got, "Keyboard2", 0, &st)) ok2 = blk_keys(st, p2);
  if (blk_find(got, "Keyboard1", 0, &st)) ok1 = blk_keys(st, p1);
  if (!ok2) { lbegin(); bs("bindings ("); bs(why); bs("): no usable Keyboard2 block in Mappings.dat - the default keys stay"); lend(); return; }
  for (i = 0; i < KB_N; i++) { u8 sc = (u8)(p2[i] & 0x7F), ex = (u8)((p2[i] & 0x80) != 0); if (sc != KSC_DEF[i] || ex != KEXT_DEF[i]) diff = 1; KSC[i] = sc; KEXT[i] = ex; }
  lbegin(); bs("bindings ("); bs(why); bs("): player-2 keys from Mappings.dat (DirectInput codes):"); for (i = 0; i < KB_N; i++) { bs(" "); bs(KBN[i]); bs("="); bh(p2[i]); } bs(diff ? " - DIFFERENT from the built-in defaults" : " - same as the built-in defaults"); lend();
  if (ok1) { int c = 0; lbegin(); bs("bindings: player-1 keys:"); for (i = 0; i < KB_N; i++) { u32 k; bs(" "); bs(KBN[i]); bs("="); bh(p1[i]); for (k = 0; k < KB_N; k++) if (p1[i] == p2[k]) c++; } bs(c ? " - WARNING: a key of player 1 is also a key of player 2 (injected keys will also reach player 1)" : " - no key is shared with player 2"); lend(); }
}
static int game_fg(void) { u32 pid = 0; PV w = GetForegroundWindow(); if (!w) return 0; GetWindowThreadProcessId(w, &pid); return pid == GetCurrentProcessId(); }

/* ---------------- capture of player 1 (read only; ReadProcessMemory on this process, so a bad address cannot crash the game) ---------------- */
static Link *lk;
static int rd(u32 a, void *o, u32 n) { u32 got = 0; if (!ReadProcessMemory((PV)-1, (const void *)a, o, n, &got) || got != n) return 0; return 1; }
static u32 rd32(u32 a) { u32 v = 0; rd(a, &v, 4); return v; }
static i32 axis100(u32 raw) { float f = *(float *)&raw; i32 v; if (!(f > -2.0f && f < 2.0f)) return 0; v = (i32)(f * 100.0f); return v > 100 ? 100 : v < -100 ? -100 : v; }
static u32 cap_was, cap_rec;
static void capture(u32 base, u32 now) {
  u32 ch = rd32(base - IMAGEBASE + VA_PTABLE), pad, rec, held, valid = 0, b = 0; i32 ax = 0, ay = 0;
  if (ch && (rd32(ch + 0x278) & 1u) && rd32(ch + 0x2f0)) {
    pad = rd32(ch + 0x4C0);
    if (pad && rd32(pad) == base - IMAGEBASE + VT_PAD && rd32(pad + 4) == ch) {
      ax = axis100(rd32(pad + 0x18)); ay = axis100(rd32(pad + 0x1c));
      rec = rd32(pad + 0xC); cap_rec = rec;
      if (rec) { held = rd32(rec + 4); if (held & 0x4000u) b |= BT_JUMP; if (held & 0x8000u) b |= BT_ACTION; if (held & 0x2000u) b |= BT_SPECIAL; }
      valid = 1;
    }
  }
  { float pos[3], yaw = 0.0f; u32 pv = 0;
    if (ch && (rd32(ch + 0x278) & 1u) && rd32(ch + 0x2f0) && rd(ch + 0x190, pos, 12) && rd(ch + 0x1b4, &yaw, 4)) {
      if (pos[0] > -100000.0f && pos[0] < 100000.0f && pos[1] > -100000.0f && pos[1] < 100000.0f && pos[2] > -100000.0f && pos[2] < 100000.0f && yaw > -20.0f && yaw < 20.0f) { pv = 1; lk->cap_px = pos[0]; lk->cap_py = pos[1]; lk->cap_pz = pos[2]; lk->cap_yaw = yaw; }
    }
    lk->cap_pos_valid = pv; }
  lk->cap_ax = ax; lk->cap_ay = ay; lk->cap_buttons = b; lk->cap_valid = valid; lk->cap_tick = now;
  { static u32 cap_b_prev; if (valid && b != cap_b_prev) { lbegin(); bs("capture: local player 1 buttons held: "); bbtn(b); bs(" (axes "); bi(ax); bs("/"); bi(ay); bs(")");
    if (cap_rec) { u32 q, d[8]; if (rd(cap_rec, d, 32)) { bs(" pad record dwords:"); for (q = 0; q < 8; q++) { bs(" "); bh(d[q]); } } } lend(); cap_b_prev = b; } }
  if (valid != cap_was) { cap_was = valid; say(valid ? "capture: local player 1 found" : "capture: local player 1 not available"); }
}

/* ---------------- test aid: F10 in the game runs a scripted series of player-1 key presses (walk, jump, action, special) ----------------
 * Only for the testing phase (the owner's keyboard keys are worn out); to be removed before the release. The keys are the game's default player-1 keys. */
typedef struct { u8 a, b; u16 hold, gap; } Step; /* a, b = scan codes (b = 0: none), hold / gap in ms */
static const Step SCRIPT[] = {
  { 0x20, 0, 1200, 300 }, { 0x1E, 0, 1200, 300 }, { 0x11, 0, 1000, 300 }, { 0x1F, 0, 1000, 300 },   /* D, A, W, S */
  { 0x39, 0, 150, 900 }, { 0x39, 0, 150, 900 }, { 0x23, 0, 150, 900 }, { 0x23, 0, 150, 900 }, { 0x24, 0, 150, 900 }, { 0x24, 0, 150, 900 }, /* space, H, J twice each */
  { 0x20, 0x39, 1200, 600 }, { 0x1E, 0x23, 1200, 600 }, { 0x11, 0x24, 1200, 600 }                    /* D+jump, A+action, W+special */
};
static u32 sc_idx, sc_next, sc_phase, sc_on, f10_prev;
static void test_script(u32 now) {
  int f10 = (GetAsyncKeyState(0x79) & 0x8000) != 0;
  if (f10 && !f10_prev && !sc_on && game_fg()) { sc_on = 1; sc_idx = 0; sc_phase = 0; sc_next = now + 1500u; say("test script started (F10)"); }
  f10_prev = (u32)f10;
  if (!sc_on || (i32)(now - sc_next) < 0) return;
  if (!game_fg()) { if (sc_phase == 1) { send_sc(SCRIPT[sc_idx].a, 0, 0); if (SCRIPT[sc_idx].b) send_sc(SCRIPT[sc_idx].b, 0, 0); sc_phase = 0; sc_idx++; } sc_next = now + 200u; return; }
  if (sc_phase == 0) { if (sc_idx >= sizeof SCRIPT / sizeof SCRIPT[0]) { sc_on = 0; say("test script done"); return; } send_sc(SCRIPT[sc_idx].a, 1, 0); if (SCRIPT[sc_idx].b) send_sc(SCRIPT[sc_idx].b, 1, 0); sc_phase = 1; sc_next = now + SCRIPT[sc_idx].hold; }
  else { send_sc(SCRIPT[sc_idx].a, 0, 0); if (SCRIPT[sc_idx].b) send_sc(SCRIPT[sc_idx].b, 0, 0); sc_phase = 0; sc_next = now + SCRIPT[sc_idx].gap; sc_idx++; }
}


/* ---------------- Part 5, step 5.1 (discovery, read-only): F11 records the local player-1 character while it walks ----------------
 * F11 (game in front, hero standing in open space): snapshot, then the module holds D, W, A, S for 1.5 s each (the default player-1 keys) with a snapshot after each phase.
 * A snapshot = the first 0x1000 bytes of the character object plus 0x100 bytes behind each pointer found in its first 0x400 bytes (at most 96). Written to logs\teracoop_dump.bin.
 * The cloud side compares the snapshots to find position, direction and animation fields. Nothing is written into the game. To be removed before the release. */
static PV hdump; static u32 dp_on, dp_phase, dp_next, dp_step, f11_prev; static u8 dbuf[0x1000]; static u8 pbuf[0x100];
static const u8 DKEYS[4] = { 0x20, 0x11, 0x1E, 0x1F };
static void dump_w(const void *p, u32 n) { u32 w; if (hdump && hdump != (PV)-1) WriteFile(hdump, p, n, &w, 0); }
static void dump_snap(u32 ch, u32 now) {
  u32 hd[4], i, j, np = 0, seen[96];
  hd[0] = 0x504D5544u; hd[1] = now - t0; hd[2] = dp_step; hd[3] = ch;
  dump_w(hd, 16);
  if (!rd(ch, dbuf, 0x1000)) { memset(dbuf, 0, sizeof dbuf); }
  dump_w(dbuf, 0x1000);
  for (i = 0; i < 0x400 && np < 96; i += 4) {
    u32 v = *(u32 *)(dbuf + i), r[4]; int dup = 0;
    if (v < 0x00100000u || v > 0x7F000000u || (v & 3)) continue;
    for (j = 0; j < np; j++) if (seen[j] == v) dup = 1;
    if (dup || !rd(v, pbuf, 0x100)) continue;
    seen[np++] = v; r[0] = 0x52545050u; r[1] = i; r[2] = v; r[3] = 0x100; dump_w(r, 16); dump_w(pbuf, 0x100);
  }
  lbegin(); bs("dump: snapshot "); bd(dp_step); bs(", pointers followed "); bd(np); lend();
}
static void dump_script(u32 now) {
  int f11 = (GetAsyncKeyState(0x7A) & 0x8000) != 0; u32 ch;
  if (f11 && !f11_prev && !dp_on && game_fg()) {
    u16 p[460]; mkpath(p, L"logs\\teracoop_dump.bin"); hdump = CreateFileW(p, 0x40000000u, 1, 0, 2, 0x80, 0);
    dp_on = 1; dp_phase = 0; dp_step = 0; dp_next = now + 1500u; say("dump: started (F11)");
  }
  f11_prev = (u32)f11;
  if (!dp_on || (i32)(now - dp_next) < 0) return;
  ch = rd32(base - IMAGEBASE + VA_PTABLE);
  if (!ch || !game_fg()) { if (dp_phase == 1) { send_sc(DKEYS[dp_step - 1], 0, 0); } dp_on = 0; if (hdump && hdump != (PV)-1) { u32 e[4]; e[0] = 0x444E4544u; e[1] = e[2] = e[3] = 0; dump_w(e, 16); } say("dump: aborted (no character or the game lost focus)"); return; }
  if (dp_phase == 0) { /* snapshot of the current state, then press the next key */
    dump_snap(ch, now);
    if (dp_step >= 4) { u32 e[4]; e[0] = 0x444E4544u; e[1] = e[2] = e[3] = 0; dump_w(e, 16); dp_on = 0; say("dump: done"); return; }
    send_sc(DKEYS[dp_step], 1, 0); dp_step++; dp_phase = 1; dp_next = now + 1500u;
  } else { send_sc(DKEYS[dp_step - 1], 0, 0); dp_phase = 0; dp_next = now + 600u; }
}


/* ---------------- Part 5, step 5.2: "puppet" on one PC (experiment) ----------------
 * F8 cycles the mode: off -> A -> B -> off. With the mode on, player 2 is put next to player 1 every frame (from the Present hook = the game thread, one frame after its update).
 * Mode A: only the world matrix translation of the P2 character (ch+0x190..0x198) is written.
 * Mode B: the same movement (delta) is added to EVERY copy of the position found in step 5.1: inside the character object (0x64/0x70/0x7c bounding box, 0x5d0.. copies, 0x744)
 *         and in the objects behind the pointers ch+0x40, +0x44, +0x58, +0x60, +0x1b0, +0x1f8.
 * Read-back check: the next frame the P2 world position is read and compared with what was written (kept / reverted counters, shown in the log). */
static volatile u32 pp_mode, pp_kept, pp_reverted, pp_writes, pp_noP2, pp_lastch; static volatile i32 pp_lastdx, pp_lastdz; static float pp_wx, pp_wy, pp_wz; static u32 pp_have;
static const u16 PP_SELF[] = { 0x64, 0x70, 0x7c, 0x190, 0x5d0, 0x5e0, 0x5f4, 0x600, 0x610, 0x620, 0x670, 0x744 };   /* x offsets inside the character (y = +4, z = +8) */
static const u16 PP_PTR[][2] = { { 0x40, 0x30 }, { 0x40, 0x70 }, { 0x44, 0x60 }, { 0x44, 0xf0 }, { 0x58, 0x54 }, { 0x60, 0x34 }, { 0x1b0, 0x70 }, { 0x1f8, 0x70 } }; /* {offset of the pointer in the character, x offset in the target} */
static u32 pp_char(u32 slot) { u32 ch = rd32(base - IMAGEBASE + VA_PTABLE + slot * 4); if (!ch || !(rd32(ch + 0x278) & 1u) || !rd32(ch + 0x2f0)) return 0; return ch; }
static int pp_rdf(u32 a, float *o) { return rd(a, o, 4); }
static void pw(u32 a, float v) { u32 w; WriteProcessMemory((PV)-1, (PV)a, &v, 4, &w); } /* the puppet runs in its own thread: a write to a freed address fails quietly instead of crashing the game */
static void pp_add(u32 a, float dx, float dy, float dz) { float v[3]; if (!rd(a, v, 12)) return; pw(a, v[0] + dx); pw(a + 4, v[1] + dy); pw(a + 8, v[2] + dz); }
static float pp_wrap(float a) { while (a > 3.14159265f) a -= 6.2831853f; while (a < -3.14159265f) a += 6.2831853f; return a; }
static float pp_sin(float x) { float x2; x = pp_wrap(x); x2 = x * x; return x * (1.0f - x2 / 6.0f * (1.0f - x2 / 20.0f * (1.0f - x2 / 42.0f * (1.0f - x2 / 72.0f * (1.0f - x2 / 110.0f * (1.0f - x2 / 156.0f)))))); }
static float pp_cos(float x) { return pp_sin(x + 1.5707963f); }
static volatile u32 pp_net_writes, pp_net_snaps, pp_net_none; static volatile i32 pp_net_dist_mm;
/* Part 5.3: player 2 follows the friend. Called every frame from the game thread. Returns 1 when it handled the frame. */
static int puppet_net(void) {
  u32 c2, i; float p2[3], ex, ey, ez, dist, g, gy, dx, dy, dz, yaw, m00, m02;
  if (!lk || !lk->rem_valid || (i32)(GetTickCount() - lk->rem_tick) >= 500) return 0;
  c2 = pp_char(1); if (!c2) { pp_net_none++; return 1; }
  if (!rd(c2 + 0x190, p2, 12)) return 1;
  ex = lk->rem_px - p2[0]; ey = lk->rem_py - p2[1]; ez = lk->rem_pz - p2[2];
  dist = __builtin_sqrtf(ex * ex + ez * ez); pp_net_dist_mm = (i32)(dist * 1000.0f);
  g = dist > 8.0f ? 1.0f : dist < 0.03f ? 0.0f : 0.2f; gy = (ey > 2.0f || ey < -2.0f) ? 1.0f : 0.2f; if (g == 1.0f) pp_net_snaps++;
  dx = ex * g; dz = ez * g; dy = ey * gy;
  for (i = 0; i < sizeof PP_SELF / sizeof PP_SELF[0]; i++) pp_add(c2 + PP_SELF[i], dx, dy, dz);
  for (i = 0; i < sizeof PP_PTR / sizeof PP_PTR[0]; i++) { u32 t = rd32(c2 + PP_PTR[i][0]); if (t > 0x100000u && t < 0x7F000000u && !(t & 3)) pp_add(t + PP_PTR[i][1], dx, dy, dz); }
  /* heading: the angle and the four matrix cells that depend on it (m00 = -cos, m02 = sin, m20 = -sin, m22 = -cos) */
  yaw = lk->rem_yaw; if (yaw > -20.0f && yaw < 20.0f) {
    m00 = -pp_cos(yaw); m02 = pp_sin(yaw);
    pw(c2 + 0x1b4, yaw);
    { static const u16 ms[] = { 0x160 }; u32 b = c2 + ms[0]; pw(b, m00); pw(b + 8, m02); pw(b + 0x20, -m02); pw(b + 0x28, m00); }
    { u32 t = rd32(c2 + 0x40); if (t > 0x100000u && t < 0x7F000000u && !(t & 3)) { u32 k, o[2] = { 0x00, 0x40 }; for (k = 0; k < 2; k++) { u32 b = t + o[k]; pw(b, m00); pw(b + 8, m02); pw(b + 0x20, -m02); pw(b + 0x28, m00); } } }
    { u32 t = rd32(c2 + 0x44); if (t > 0x100000u && t < 0x7F000000u && !(t & 3)) { u32 b = t + 0x30; pw(b, m00); pw(b + 8, m02); pw(b + 0x20, -m02); pw(b + 0x28, m00); } }
  }
  pp_net_writes++;
  return 1;
}
static void puppet_frame(void) {
  u32 c1, c2, i; float p1[3], p2[3], dx, dy, dz;
  if (!base) return;
  if (puppet_net()) return;
  if (!pp_mode) return;
  c1 = pp_char(0); c2 = pp_char(1);
  if (!c1 || !c2) { pp_noP2++; pp_have = 0; return; }
  if (!rd(c2 + 0x190, p2, 12) || !rd(c1 + 0x190, p1, 12)) return;
  if (pp_have) { float ex = p2[0] - pp_wx, ez = p2[2] - pp_wz; if (ex < 0) ex = -ex; if (ez < 0) ez = -ez; if (ex < 0.02f && ez < 0.02f) pp_kept++; else { pp_reverted++; pp_lastdx = (i32)((p2[0] - pp_wx) * 1000.0f); pp_lastdz = (i32)((p2[2] - pp_wz) * 1000.0f); } }
  dx = p1[0] + 1.2f - p2[0]; dy = p1[1] - p2[1]; dz = p1[2] + 1.2f - p2[2];
  if (pp_mode == 1) { pp_add(c2 + 0x190, dx, dy, dz); }
  else {
    for (i = 0; i < sizeof PP_SELF / sizeof PP_SELF[0]; i++) pp_add(c2 + PP_SELF[i], dx, dy, dz);
    for (i = 0; i < sizeof PP_PTR / sizeof PP_PTR[0]; i++) { u32 t = rd32(c2 + PP_PTR[i][0]); if (t > 0x100000u && t < 0x7F000000u && !(t & 3)) pp_add(t + PP_PTR[i][1], dx, dy, dz); }
  }
  pp_wx = p1[0] + 1.2f; pp_wy = p1[1]; pp_wz = p1[2] + 1.2f; pp_have = 1; pp_writes++; pp_lastch = c2;
  (void)pp_rdf; (void)pp_wy;
}

/* the puppet runs in its own thread, about 100 times a second (no hook on the picture, so nothing here can touch drawing, Reset or screenshots) */
static volatile u32 pp_gap_max; /* longest time between two runs of the puppet thread (read and reset by the worker once a second): shows freezes of the whole game */
/* Experiment (F6): put the friend's actions straight into the game's pad record of player 2 (held bits and the two axes) instead of pressing keys. Independent of every key binding and of the window focus.
 * Whether the game's own logic reacts to it is exactly what the test must show (watch: does player 2 hit / jump / walk?). The log shows "pad-inject" counters. Off by default. */
static volatile u32 inj_pad; static volatile u32 padi_writes, padi_fail;
static void pad_inject_frame(void) {
  u32 c2, pad, rec, held, w; float fx, fy; i32 ax, ay; u32 b;
  if (!inj_pad || !lk || !lk->inject || (i32)(GetTickCount() - lk->prog_tick) > 1500) return;
  c2 = pp_char(1); if (!c2) return;
  pad = rd32(c2 + 0x4C0); if (!pad || rd32(pad) != base - IMAGEBASE + VT_PAD || rd32(pad + 4) != c2) { padi_fail++; return; }
  rec = rd32(pad + 0xC); if (!rec) { padi_fail++; return; }
  ax = lk->ax; ay = lk->ay; b = lk->buttons;
  held = rd32(rec + 4) & ~0xE000u; if (b & BT_JUMP) held |= 0x4000u; if (b & BT_ACTION) held |= 0x8000u; if (b & BT_SPECIAL) held |= 0x2000u;
  WriteProcessMemory((PV)-1, (PV)(rec + 4), &held, 4, &w);
  fx = (float)ax / 100.0f; fy = (float)ay / 100.0f; WriteProcessMemory((PV)-1, (PV)(pad + 0x18), &fx, 4, &w); WriteProcessMemory((PV)-1, (PV)(pad + 0x1c), &fy, 4, &w);
  padi_writes++;
}
static void enemy_mirror_frame(void);
static u32 __stdcall puppet_thread(PV arg) { u32 last = 0; (void)arg; for (;;) { u32 n; Sleep(10); n = GetTickCount(); if (last) { u32 g = n - last; if (g > pp_gap_max) pp_gap_max = g; } last = n; puppet_frame(); enemy_mirror_frame(); pad_inject_frame(); } return 0; }

/* ---------------- Part 5: data collection (F11) - discovery for the world sync: enemies, health, deaths, coins ----------------
 * F11 (game window in front) starts a recording of up to 120 s, F11 again stops it. It works alone (no program, no friend needed: "solo"). Three files per recording, named <host|guest|solo>_<kind>_<date>_rec<N>:
 *  data    (.bin)  every 500 ms the first 0x400 bytes of every character in the game's list (the enemies) and the first 0x800 bytes of the two players; every 2 s also the first 0x60 bytes of everything those bytes point to
 *                  (with the class name of the target when it is an object with a vtable: this is how the health / hit-point objects of the characters are found); markers; the list of characters
 *  classes (.txt)  a scan of the whole heap of the game, again and again (a few seconds each): how many objects of every class (RTTI name) exist, and every change of that count (coins / pickups appearing and disappearing, enemies, doors, breakables...)
 *  objects (.bin)  after every scan: the first 0x100 bytes of the objects of the interesting classes (Pickup, Stud, Coin, Health, Hit, Door, Break, Destruct, Life, Brain, Collect, Gizmo)
 * Markers while recording: F3 = my health changed (hit / heart), F4 = an enemy died, F5 = I picked up a coin. They go into the log, into the data file and into the classes file. Only reads game memory. */
#define VA_CHARLIST 0x015b31e8u
K u32 VirtualQuery(const void *, void *, u32);
K int SetThreadPriority(PV, int);
K PV GetCurrentThread(void);
static PV hcen, hcls, hobj; static volatile u32 cen_on, scan_run, scan_abort;
static u32 cen_next, cen_end, cen_f11_prev, cen_txt_at, cen_n_prev, cen_samples, cen_idx, cen_ptr_next, cen_mk_prev[3], cen_ptrs_written;
static u32 ro_lo, ro_hi, tx_lo, tx_hi, im_lo, im_hi; static u8 cen_buf[0x800];
static void pe_ranges(void) {
  u32 pe, ns, os, sec, i; if (ro_lo || !base) return;
  pe = *(const u32 *)(base + 0x3c); ns = *(const u16 *)(base + pe + 6); os = *(const u16 *)(base + pe + 20); sec = base + pe + 24 + os;
  im_lo = base; im_hi = base + *(const u32 *)(base + pe + 24 + 56); ro_lo = 0xFFFFFFFFu; tx_lo = 0xFFFFFFFFu;
  for (i = 0; i < ns && i < 24; i++) { u32 va = *(const u32 *)(sec + i * 40 + 12), vs = *(const u32 *)(sec + i * 40 + 8), ch = *(const u32 *)(sec + i * 40 + 36), lo = base + va, hi = base + va + vs;
    if (ch & 0x20000000u) { if (lo < tx_lo) tx_lo = lo; if (hi > tx_hi) tx_hi = hi; }
    else if (!(ch & 0x80000000u) && !(ch & 0x02000000u) && (ch & 0x40000000u)) { /* readable, not writable, not discardable: .rdata (vtables and their RTTI); .rsrc is skipped */
      const char *nm = (const char *)(sec + i * 40); if (nm[0] == '.' && nm[1] == 'r' && nm[2] == 's') continue; if (lo < ro_lo) ro_lo = lo; if (hi > ro_hi) ro_hi = hi; } }
  if (ro_lo == 0xFFFFFFFFu) ro_lo = ro_hi = 0; if (tx_lo == 0xFFFFFFFFu) tx_lo = tx_hi = 0;
  lbegin(); bs("sections: code "); bh(tx_lo); bs("-"); bh(tx_hi); bs(" read-only data "); bh(ro_lo); bs("-"); bh(ro_hi); lend();
}
/* is v the address of a class vtable of the game's exe? gives the RTTI class name (without ".?AV" and "@@") */
static int vt_probe(u32 v, char *name) { /* the exe is mapped in this very process: the checks read it directly, no system call (the scan calls this a million times) */
  u32 f, col, td; int k = 0;
  if (!ro_hi || v < ro_lo + 4 || v >= ro_hi || (v & 3)) return 0;
  f = *(const volatile u32 *)v; if (f < tx_lo || f >= tx_hi) return 0;
  name[0] = '?'; name[1] = 0;
  col = *(const volatile u32 *)(v - 4);
  if (col >= ro_lo && col <= ro_hi - 16) { td = *(const volatile u32 *)(col + 12);
    if (td >= im_lo && td < im_hi - 64) { const volatile char *t = (const volatile char *)(td + 8); u32 i = 0; if (t[0] == '.' && t[1] == '?' && t[2] == 'A') i = 4; for (; t[i] && t[i] >= 32 && t[i] < 127 && k < 43; i++) name[k++] = t[i]; while (k > 0 && name[k - 1] == '@') k--; name[k] = 0; if (!k) { name[0] = '?'; name[1] = 0; } } }
  return 1;
}
static int has_sub(const char *s, const char *sub) { int i, j; for (i = 0; s[i]; i++) { for (j = 0; sub[j] && s[i + j] == sub[j]; j++) { } if (!sub[j]) return 1; } return 0; }
static int interesting(const char *n) { static const char *k[] = { "Pickup", "Stud", "Coin", "Health", "Hit", "Door", "Break", "Destruct", "Life", "Brain", "Collect", "Gizmo", "Smash", "Enemy", "Spawn", 0 }; int i; for (i = 0; k[i]; i++) if (has_sub(n, k[i])) return 1; return 0; }
/* separate small line buffer for the scan thread (the module log buffer is not thread safe) */
typedef struct { char b[400]; int n; } SB;
static void sb_s(SB *o, const char *s) { while (*s && o->n < 380) o->b[o->n++] = *s++; }
static void sb_d(SB *o, u32 v) { char t[12]; int n = 0; do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v); while (n && o->n < 380) o->b[o->n++] = t[--n]; }
static void sb_h(SB *o, u32 v) { const char *d = "0123456789abcdef"; int i; sb_s(o, "0x"); for (i = 28; i >= 0; i -= 4) if (o->n < 380) o->b[o->n++] = d[(v >> i) & 15]; }
static void sb_out(PV h, SB *o) { u32 w; o->b[o->n++] = '\r'; o->b[o->n++] = '\n'; if (h && h != (PV)-1) WriteFile(h, o->b, (u32)o->n, &w, 0); o->n = 0; }
/* the class table of the scan */
typedef struct { u32 vt, count, last, nobj, keep; char name[44]; u32 objs[40]; } VtEnt;
#define VT_MAX 3072u
static VtEnt vte[VT_MAX]; static u32 vte_n; static u32 ht_key[8192], ht_val[8192]; /* open addressing: only real classes are stored */
static u8 sbuf[0x100000];
static VtEnt *vt_get(u32 v) {
  u32 h = (v >> 2) * 2654435761u >> 19, i; char nm[44];
  for (i = 0; i < 8192; i++, h = (h + 1) & 8191u) { if (!ht_key[h]) break; if (ht_key[h] == v) return &vte[ht_val[h]]; }
  if (i >= 8192 || vte_n >= VT_MAX || !vt_probe(v, nm)) return 0; /* not a class: nothing is remembered about it (probing is cheap) */
  { VtEnt *e = &vte[vte_n]; int k; memset(e, 0, sizeof *e); e->vt = v; for (k = 0; k < 43 && nm[k]; k++) e->name[k] = nm[k]; e->keep = interesting(e->name); ht_key[h] = v; ht_val[h] = vte_n++; return e; }
}
static void scan_note(u32 v, u32 addr) { VtEnt *e = vt_get(v); if (!e) return; e->count++; if (e->nobj < 40) e->objs[e->nobj++] = addr; }
static u32 __stdcall scan_thread(PV arg) {
  u32 scan_no = 0, k, w; SB o; (void)arg; o.n = 0; scan_run = 1; SetThreadPriority(GetCurrentThread(), -2 /* lowest: the game comes first */);
  while (cen_on && !scan_abort) {
    u32 addr = 0x10000u, t_start = GetTickCount(), mb = 0, nclass = 0, changed = 0; u32 mbi[7]; u16 st[8];
    for (k = 0; k < vte_n; k++) { vte[k].count = 0; vte[k].nobj = 0; }
    while (addr < 0x7FFF0000u && cen_on && !scan_abort) {
      u32 rb, rs, p, e; if (!VirtualQuery((const void *)addr, mbi, 28)) break;
      rb = mbi[0]; rs = mbi[3]; if (!rs) break;
      if (mbi[4] == 0x1000u /* COMMIT */ && !(mbi[5] & 0x101u /* NOACCESS | GUARD */) && mbi[6] == 0x20000u /* private memory only (the heaps); images hold our own tables, mapped files are big and have no objects */) {
        e = rb + rs; if (e < rb) e = 0xFFFFFFFFu;
        for (p = rb; p < e && cen_on && !scan_abort; p += sizeof sbuf) {
          u32 n = e - p, i, got = 0; if (n > sizeof sbuf) n = sizeof sbuf;
          if (ReadProcessMemory((PV)-1, (const void *)p, sbuf, n, &got) && got >= 4) { u32 *d = (u32 *)sbuf; u32 cnt = got / 4; for (i = 0; i < cnt; i++) { u32 v = d[i]; if (v >= ro_lo && v < ro_hi && !(v & 3)) scan_note(v, p + i * 4); } mb += got; }
          else { /* a page of the chunk is not readable: take it page by page */
            u32 q; for (q = 0; q < n; q += 4096u) { u32 m = n - q > 4096u ? 4096u : n - q, g2 = 0; if (ReadProcessMemory((PV)-1, (const void *)(p + q), sbuf, m, &g2) && g2 >= 4) { u32 *d = (u32 *)sbuf; u32 cnt = g2 / 4; for (i = 0; i < cnt; i++) { u32 v = d[i]; if (v >= ro_lo && v < ro_hi && !(v & 3)) scan_note(v, p + q + i * 4); } mb += g2; } }
          }
          Sleep(1);
        }
      }
      addr = rb + rs; if (addr < rb) break;
    }
    if (!cen_on || scan_abort) break;
    scan_no++; GetSystemTime(st);
    for (k = 0; k < vte_n; k++) { if (vte[k].count) nclass++; if (vte[k].count != vte[k].last || scan_no == 1) changed++; }
    o.n = 0; sb_s(&o, "scan "); sb_d(&o, scan_no); sb_s(&o, " finished at "); sb_d(&o, GetTickCount() - t0); sb_s(&o, " ms [UTC "); sb_d(&o, st[4]); sb_s(&o, ":"); sb_d(&o, st[5]); sb_s(&o, ":"); sb_d(&o, st[6]); sb_s(&o, "], took "); sb_d(&o, GetTickCount() - t_start); sb_s(&o, " ms, read "); sb_d(&o, mb >> 20); sb_s(&o, " MB, classes with objects "); sb_d(&o, nclass); sb_s(&o, ", changed "); sb_d(&o, changed); sb_out(hcls, &o);
    for (k = 0; k < vte_n; k++) {
      VtEnt *e = &vte[k];
      if (e->count != e->last || scan_no == 1) {
        if (scan_no == 1 && !e->count) continue;
        sb_s(&o, e->keep ? "  * " : "    "); sb_s(&o, e->name); sb_s(&o, " vtable "); sb_h(&o, e->vt); sb_s(&o, " (exe+"); sb_h(&o, e->vt - base); sb_s(&o, "): "); if (scan_no > 1) { sb_d(&o, e->last); sb_s(&o, " -> "); } sb_d(&o, e->count); sb_out(hcls, &o);
        e->last = e->count;
      }
    }
    { /* the interesting objects, as they are right now */
      u32 total = 0;
      for (k = 0; k < vte_n && cen_on && !scan_abort; k++) { VtEnt *e = &vte[k]; u32 j;
        if (!e->keep || !e->nobj || e->count > 400u) continue;
        for (j = 0; j < e->nobj && total < 400u; j++) { u32 hd[4]; u8 ob[0x100]; hd[0] = 0x534A424Fu; /* 'OBJS' */ hd[1] = e->vt; hd[2] = e->objs[j]; hd[3] = scan_no; if (!rd(e->objs[j], ob, 0x100)) continue; WriteFile(hobj, hd, 16, &w, 0); WriteFile(hobj, ob, 0x100, &w, 0); total++; } }
    }
  }
  scan_run = 0; return 0;
}
static void cen_w(const void *p, u32 n) { u32 w; if (hcen && hcen != (PV)-1) WriteFile(hcen, p, n, &w, 0); }
static u32 cen_nodes(u32 *out, u32 max) {
  u32 n = 0, node = rd32(base - IMAGEBASE + VA_CHARLIST), i;
  while (node > 0x100000u && node < 0x7F000000u && n < max) { for (i = 0; i < n; i++) if (out[i] == node) return n; out[n++] = node; node = rd32(node + 0x244); }
  return n;
}
static void cen_fl(u32 a) { float f[3]; if (rd(a, f, 12) && f[0] > -1e6f && f[0] < 1e6f && f[1] > -1e6f && f[1] < 1e6f && f[2] > -1e6f && f[2] < 1e6f) { bi((i32)(f[0] * 1000.0f)); bs(" "); bi((i32)(f[1] * 1000.0f)); bs(" "); bi((i32)(f[2] * 1000.0f)); } else bs("n/a"); }
/* record the objects that the first bytes of an object point to (heap pointers), with the class name when the target starts with a vtable */
static void cen_ptrs(u32 k, const u8 *b, u32 len) {
  u32 off, done = 0;
  for (off = 0; off + 4 <= len && done < 100; off += 4) {
    u32 v = *(const u32 *)(b + off), hd[4]; char nm[44]; u8 tb[0x60];
    if (v < 0x00100000u || v >= 0x7F000000u || (v & 3) || (v >= im_lo && v < im_hi)) continue;
    if (!rd(v, tb, 0x60)) continue;
    memset(nm, 0, sizeof nm); { u32 vt0 = *(const u32 *)tb; if (!vt_probe(vt0, nm)) nm[0] = 0; }
    hd[0] = 0x44545050u; /* 'PPTD' */ hd[1] = (k << 16) | off; hd[2] = v; hd[3] = 0x60; cen_w(hd, 16); cen_w(nm, 44); cen_w(tb, 0x60); done++; cen_ptrs_written++;
  }
}
static void cen_stop(const char *why) {
  u32 t = GetTickCount();
  if (!cen_on) return;
  cen_on = 0; scan_abort = 1;
  while (scan_run && GetTickCount() - t < 4000u) Sleep(20);
  if (hcen && hcen != (PV)-1) CloseHandle(hcen); hcen = 0; if (hcls && hcls != (PV)-1) CloseHandle(hcls); hcls = 0; if (hobj && hobj != (PV)-1) CloseHandle(hobj); hobj = 0;
  lbegin(); bs("census: recording finished ("); bs(why); bs("), samples "); bd(cen_samples); bs(", pointer dumps "); bd(cen_ptrs_written); lend();
}
static void cen_start(u32 now) {
  u16 p[460], ex[24]; int k = 0;
  if (!ro_hi) pe_ranges();
  cen_idx++; ex[k++] = '_'; ex[k++] = 'r'; ex[k++] = 'e'; ex[k++] = 'c'; ex[k++] = (u16)('0' + (cen_idx / 10) % 10); ex[k++] = (u16)('0' + cen_idx % 10);
  { static const u16 e1[] = L".bin", e2[] = L".txt"; int i; u16 q[24];
    for (i = 0; i < k; i++) q[i] = ex[i]; for (i = 0; e1[i]; i++) q[k + i] = e1[i]; q[k + i] = 0; mk_logpath(p, L"data", q); hcen = CreateFileW(p, 0x40000000u, 5, 0, 2, 0x80, 0);
    for (i = 0; i < k; i++) q[i] = ex[i]; for (i = 0; e2[i]; i++) q[k + i] = e2[i]; q[k + i] = 0; mk_logpath(p, L"classes", q); hcls = CreateFileW(p, 0x40000000u, 5, 0, 2, 0x80, 0);
    for (i = 0; i < k; i++) q[i] = ex[i]; for (i = 0; e1[i]; i++) q[k + i] = e1[i]; q[k + i] = 0; mk_logpath(p, L"objects", q); hobj = CreateFileW(p, 0x40000000u, 5, 0, 2, 0x80, 0); }
  cen_on = 1; cen_next = now; cen_end = now + 120000u; cen_samples = 0; cen_ptr_next = now; cen_ptrs_written = 0; scan_abort = 0;
  { u32 i; for (i = 0; i < vte_n; i++) vte[i].last = 0; }
  lbegin(); bs("census: recording "); bd(cen_idx); bs(" started (F11 again stops it; markers F3 = health changed, F4 = enemy died, F5 = coin picked up), up to 120 seconds"); lend();
  CreateThread(0, 0, scan_thread, 0, 0, 0);
}
static void cen_marker(u32 id, const char *what) {
  u32 hd[4], w; SB o;
  hd[0] = 0x4B52414Du; /* 'MARK' */ hd[1] = id; hd[2] = GetTickCount() - t0; hd[3] = 0; cen_w(hd, 16);
  lbegin(); bs("MARKER "); bd(id); bs(": "); bs(what); lend();
  o.n = 0; sb_s(&o, "MARKER "); sb_d(&o, id); sb_s(&o, " at "); sb_d(&o, GetTickCount() - t0); sb_s(&o, " ms: "); sb_s(&o, what); sb_out(hcls, &o); (void)w;
}
static void census_tick(u32 now, u32 fg) {
  u32 nodes[64], n, i, pl0, pl1; int f11, mk[3];
  if (!base) return;
  n = cen_nodes(nodes, 64);
  { u32 c0 = rd32(base - IMAGEBASE + VA_PTABLE), c1 = rd32(base - IMAGEBASE + VA_PTABLE + 4); pl0 = c0 ? rd32(c0 + 0x1b0) : 0; pl1 = c1 ? rd32(c1 + 0x1b0) : 0; }
  if (n != cen_n_prev || (n > 2 && (i32)(now - cen_txt_at) >= 0) || (i32)(now - cen_txt_at) >= 15000) {
    cen_txt_at = now + 5000u; cen_n_prev = n;
    lbegin(); bs("census: "); bd(n); bs(" characters in the list; body of player 1 "); bh(pl0); bs(", of player 2 "); bh(pl1); lend();
    for (i = 0; i < n && i < 40; i++) {
      lbegin(); bs("census #"); bd(i); bs(" body "); bh(nodes[i]); bs(nodes[i] == pl0 ? " [P1]" : nodes[i] == pl1 ? " [P2]" : ""); bs(" vt "); bh(rd32(nodes[i])); bs(" pos mm "); cen_fl(nodes[i] + 0x70); lend();
    }
  }
  f11 = (GetAsyncKeyState(0x7A) & 0x8000) != 0;
  if (f11 && !cen_f11_prev && fg) { if (cen_on) cen_stop("stopped with F11"); else cen_start(now); }
  cen_f11_prev = (u32)f11;
  mk[0] = (GetAsyncKeyState(0x72) & 0x8000) != 0; mk[1] = (GetAsyncKeyState(0x73) & 0x8000) != 0; mk[2] = (GetAsyncKeyState(0x74) & 0x8000) != 0;
  for (i = 0; i < 3; i++) { if (mk[i] && !cen_mk_prev[i] && fg && cen_on) cen_marker(i + 1, i == 0 ? "my health changed (hit / heart)" : i == 1 ? "an enemy died" : "I picked up a coin"); cen_mk_prev[i] = (u32)mk[i]; }
  if (cen_on && ((i32)(now - cen_end) >= 0)) cen_stop("120 seconds over");
  if (cen_on && (i32)(now - cen_next) >= 0) {
    u32 hd[4], k, doptr = (i32)(now - cen_ptr_next) >= 0;
    cen_next = now + 500u; if (doptr) cen_ptr_next = now + 2000u;
    hd[0] = 0x534E4543u; hd[1] = now - t0; hd[2] = n; hd[3] = 0; cen_w(hd, 16); /* 'CENS': a new sample: time, number of nodes */
    for (k = 0; k < n; k++) { hd[0] = 0x45444F4Eu; hd[1] = k; hd[2] = nodes[k]; hd[3] = 0x400; cen_w(hd, 16); if (!rd(nodes[k], cen_buf, 0x400)) memset(cen_buf, 0, 0x400); cen_w(cen_buf, 0x400); if (doptr && rd32(nodes[k]) && nodes[k] != pl0 && nodes[k] != pl1) cen_ptrs(k, cen_buf, 0x400); } /* 'NODE' */
    { u32 sl; for (sl = 0; sl < 2; sl++) { u32 ch = rd32(base - IMAGEBASE + VA_PTABLE + sl * 4); hd[0] = 0x4C595050u; hd[1] = sl; hd[2] = ch; hd[3] = 0x800; cen_w(hd, 16); if (!ch || !rd(ch, cen_buf, 0x800)) memset(cen_buf, 0, 0x800); cen_w(cen_buf, 0x800); if (doptr && ch) cen_ptrs(100 + sl, cen_buf, 0x800); } } /* 'PPYL' player objects */
    cen_samples++;
  }
}

/* ---------------- Part 5.5: enemies (host-authoritative mirror, first stage: position and heading) ----------------
 * Facts from the logs of the 25.10 test: the characters in the list at VA 0x015b31e8 (next at +0x244) are the enemies / NPCs of the level, between two list markers (vtable 0). Their objects have the same layout as the player characters
 * (world position at +0x190, heading at +0x1b4, matrix at +0x160 with m00 = -cos(yaw), m02 = sin(yaw), position copies at +0x64/+0x70/+0x7c). Both PCs create the enemies at the same places (spawn positions agree to a few centimetres),
 * but every PC runs their AI by itself, so they drift apart.
 * Host (role 1): the module keeps a table of the enemies (slot = id), publishes it 10 times a second through the link; the program sends it to the guest.
 * Guest (role 2): every local enemy is matched to a host id by its spawn position (up to 3 m; otherwise by the current position up to 5 m), then the puppet thread pulls it to the host's position 100 times a second
 * (the same "move every copy of the position" method as for player 2, but a copy is only touched when it really holds the enemy's position, within 1 m). F7 switches the mirror off and on (kill switch).
 * Not done yet (no health / death field found, see the census): death, damage, animation. When the host's enemy is gone but the local one is still there, it is logged as "zombie". */
typedef struct { u32 node, vt, alive, hid, hepoch, gone_at, type, zombie_at; i32 sx, sz; float lx, ly, lz, lyaw; } Enemy;
static Enemy en[EN_MAX];
static u32 en_epoch, en_on = 0, en_scan_at, en_stat_at, en_pos_at, en_f7_prev, en_new_total, en_gone_total, en_match_total, en_zombie_total, en_nomatch_mask;
static volatile u32 enm_writes, enm_snaps, enm_cp_ok, enm_cp_skip, enm_vtbad, enm_dcnt, enm_dsum;
static i32 f2mm(float f) { return (i32)(f * 1000.0f); }
static float en_abs(float f) { return f < 0.0f ? -f : f; }
static void en_pos3(const char *what, u32 j) { Enemy *q = &en[j]; bs(what); bd(j); bs(" node "); bh(q->node); bs(" vt "); bh(q->vt); bs(" type "); bh(q->type); bs(" pos mm "); bi(f2mm(q->lx)); bs(" "); bi(f2mm(q->ly)); bs(" "); bi(f2mm(q->lz)); }
static void enemy_scan(u32 now) {
  u32 nodes[64], n, i, j, pl0 = 0, pl1 = 0, role = lk ? lk->role : 0; u8 seen[EN_MAX];
  memset(seen, 0, sizeof seen);
  n = cen_nodes(nodes, 64);
  { u32 c0 = rd32(base - IMAGEBASE + VA_PTABLE), c1 = rd32(base - IMAGEBASE + VA_PTABLE + 4); pl0 = c0 ? rd32(c0 + 0x1b0) : 0; pl1 = c1 ? rd32(c1 + 0x1b0) : 0; }
  for (i = 0; i < n; i++) {
    u32 node = nodes[i], vt = rd32(node); float p[3], yaw = 0.0f; int found = -1;
    if (!vt || node == pl0 || node == pl1) continue; /* list markers have no vtable */
    if (!rd(node + 0x190, p, 12) || !(p[0] > -1e6f && p[0] < 1e6f && p[1] > -1e6f && p[1] < 1e6f && p[2] > -1e6f && p[2] < 1e6f)) continue;
    for (j = 0; j < EN_MAX; j++) if (en[j].alive && en[j].node == node) { found = (int)j; break; }
    if (found < 0) {
      int any = 0;
      for (j = 0; j < EN_MAX; j++) if (en[j].alive) any = 1;
      for (j = 0; j < EN_MAX; j++) if (!en[j].node || (!en[j].alive && (i32)(now - en[j].gone_at) > 3000)) break;
      if (j == EN_MAX) continue;
      if (!any) en_epoch++;
      memset(&en[j], 0, sizeof en[j]);
      en[j].node = node; en[j].vt = vt; en[j].alive = 1; en[j].hid = role == 1 ? j : 0xFFFFFFFFu; en[j].type = rd32(node + 0x20) & 0xFFFFu;
      en[j].sx = f2mm(p[0]); en[j].sz = f2mm(p[2]); en[j].lx = p[0]; en[j].ly = p[1]; en[j].lz = p[2];
      en_new_total++; found = (int)j;
      lbegin(); en_pos3("enemy new: slot ", j); bs(" epoch "); bd(en_epoch); bs(" role "); bd(role); lend();
    }
    rd(node + 0x1b4, &yaw, 4);
    en[found].lx = p[0]; en[found].ly = p[1]; en[found].lz = p[2]; en[found].lyaw = yaw; seen[found] = 1;
  }
  for (j = 0; j < EN_MAX; j++) {
    if (en[j].alive && !seen[j]) { en[j].alive = 0; en[j].gone_at = now; en_gone_total++; lbegin(); en_pos3("enemy gone: slot ", j); bs(" (died or the level was left), host id "); if (en[j].hid < EN_MAX) bd(en[j].hid); else bs("-"); lend(); }
    else if (!en[j].alive && en[j].node && (i32)(now - en[j].gone_at) > 3000) en[j].node = 0;
  }
}
static void enemy_publish(u32 now) {
  u32 j;
  for (j = 0; j < EN_MAX; j++) {
    EnPkt *e = &lk->en_out[j]; Enemy *q = &en[j];
    e->x = f2mm(q->lx); e->y = f2mm(q->ly); e->z = f2mm(q->lz); e->yaw = f2mm(q->lyaw); e->sx = q->sx; e->sz = q->sz; e->type_epoch = q->type | (en_epoch << 16); e->tick = now;
    e->flags = (q->node ? EN_VALID : 0u) | (q->alive ? EN_ALIVE : 0u);
  }
  lk->en_out_n = EN_MAX; lk->en_out_tick = now;
}
static void enemy_match(u32 now) {
  u32 h, j;
  for (h = 0; h < EN_MAX; h++) {
    EnPkt *e = &lk->en_in[h]; u32 ep = e->type_epoch >> 16; int best = -1; i32 bd2 = 0x7FFFFFFF; int already = 0;
    if (!(e->flags & EN_VALID) || (i32)(now - e->tick) > 1500) continue;
    for (j = 0; j < EN_MAX; j++) if (en[j].alive && en[j].hid == h) { if (en[j].hepoch == ep) already = 1; else en[j].hid = 0xFFFFFFFFu; }
    if (!(e->flags & EN_ALIVE)) { /* the host's enemy is gone: the local twin, if any, is a "zombie" */
      for (j = 0; j < EN_MAX; j++) if (en[j].alive && en[j].hid == h && en[j].hepoch == ep) { if (!en[j].zombie_at) en[j].zombie_at = now ? now : 1; else if ((i32)(now - en[j].zombie_at) > 1500 && en[j].zombie_at != 1) { en[j].zombie_at = 1; en_zombie_total++; lbegin(); en_pos3("zombie: the host says enemy gone, the local twin is still there: slot ", j); bs(" host id "); bd(h); lend(); } }
      continue;
    }
    if (already) continue;
    for (j = 0; j < EN_MAX; j++) {
      float dx, dz; i32 d;
      if (!en[j].alive || en[j].hid < EN_MAX) continue;
      dx = (float)(en[j].sx - e->sx); dz = (float)(en[j].sz - e->sz); d = (i32)__builtin_sqrtf(dx * dx + dz * dz);
      if (d < 3000 && d < bd2) { bd2 = d; best = (int)j; }
    }
    if (best < 0) for (j = 0; j < EN_MAX; j++) {
      float dx, dz; i32 d;
      if (!en[j].alive || en[j].hid < EN_MAX) continue;
      dx = en[j].lx * 1000.0f - (float)e->x; dz = en[j].lz * 1000.0f - (float)e->z; d = (i32)__builtin_sqrtf(dx * dx + dz * dz);
      if (d < 5000 && d < bd2) { bd2 = d; best = (int)j; }
    }
    if (best >= 0) { en[best].hid = h; en[best].hepoch = ep; en[best].zombie_at = 0; en_match_total++; en_nomatch_mask &= ~(1u << h); lbegin(); en_pos3("enemy matched: local slot ", (u32)best); bs(" <-> host id "); bd(h); bs(" epoch "); bd(ep); bs(" distance mm "); bd((u32)bd2); lend(); }
    else if (!(en_nomatch_mask & (1u << h))) { en_nomatch_mask |= 1u << h; lbegin(); bs("enemy: no local twin for host id "); bd(h); bs(" type "); bh(e->type_epoch & 0xFFFFu); bs(" spawn mm "); bi(e->sx); bs(" "); bi(e->sz); bs(" now mm "); bi(e->x); bs(" "); bi(e->y); bs(" "); bi(e->z); lend(); }
  }
}
static int en_chk(u32 a, const float *c) { float v[3]; if (!rd(a, v, 12)) return 0; return en_abs(v[0] - c[0]) < 1.0f && en_abs(v[1] - c[1]) < 1.0f && en_abs(v[2] - c[2]) < 1.0f; }
static void en_apply(u32 node, const float *cur, float dx, float dy, float dz) {
  u32 i;
  for (i = 0; i < sizeof PP_SELF / sizeof PP_SELF[0]; i++) { u32 a = node + PP_SELF[i]; if (en_chk(a, cur)) { pp_add(a, dx, dy, dz); enm_cp_ok++; } else enm_cp_skip++; }
  for (i = 0; i < sizeof PP_PTR / sizeof PP_PTR[0]; i++) { u32 t = rd32(node + PP_PTR[i][0]); if (t > 0x100000u && t < 0x7F000000u && !(t & 3)) { u32 a = t + PP_PTR[i][1]; if (en_chk(a, cur)) { pp_add(a, dx, dy, dz); enm_cp_ok++; } else enm_cp_skip++; } }
}
static void enemy_mirror_frame(void) { /* puppet thread, about 100 times a second; guest only */
  u32 j, now;
  if (!lk || !base || !en_on || lk->role != 2) return;
  now = GetTickCount();
  for (j = 0; j < EN_MAX; j++) {
    Enemy *q = &en[j]; EnPkt *e; float cur[3], ex, ey, ez, dist, g, gy, yaw;
    if (!q->alive || !q->node || q->hid >= EN_MAX) continue;
    e = &lk->en_in[q->hid];
    if (!(e->flags & EN_ALIVE) || (e->type_epoch >> 16) != q->hepoch || (i32)(now - e->tick) > 400) continue;
    if (rd32(q->node) != q->vt) { enm_vtbad++; continue; }
    if (!rd(q->node + 0x190, cur, 12)) continue;
    ex = (float)e->x * 0.001f - cur[0]; ey = (float)e->y * 0.001f - cur[1]; ez = (float)e->z * 0.001f - cur[2];
    dist = __builtin_sqrtf(ex * ex + ez * ez);
    g = dist > 6.0f ? 1.0f : dist < 0.02f ? 0.0f : 0.3f; gy = (ey > 2.0f || ey < -2.0f) ? 1.0f : 0.3f; if (g == 1.0f) enm_snaps++;
    enm_dsum += (u32)(dist * 1000.0f); enm_dcnt++;
    if (g > 0.0f || en_abs(ey) > 0.02f) en_apply(q->node, cur, ex * g, ey * gy, ez * g);
    yaw = (float)e->yaw * 0.001f;
    if (yaw > -20.0f && yaw < 20.0f) { float m00 = -pp_cos(yaw), m02 = pp_sin(yaw); u32 b = q->node + 0x160; pw(q->node + 0x1b4, yaw); pw(b, m00); pw(b + 8, m02); pw(b + 0x20, -m02); pw(b + 0x28, m00); }
    enm_writes++;
  }
}
static void enemy_stats(u32 now) {
  u32 j, nl = 0, nm = 0, nh = 0, role = lk ? lk->role : 0;
  if ((i32)(now - en_stat_at) < 0) return;
  en_stat_at = now + 1000u;
  for (j = 0; j < EN_MAX; j++) { if (en[j].alive) { nl++; if (en[j].hid < EN_MAX) nm++; } if (lk && role == 2 && (lk->en_in[j].flags & EN_ALIVE) && (i32)(now - lk->en_in[j].tick) < 1500) nh++; }
  if (!nl && !nh && !en_new_total) return;
  lbegin(); bs("enemy: role "); bd(role); bs(" mirror "); bs(en_on ? "on" : "OFF"); bs(" | local alive "); bd(nl); bs(" matched "); bd(nm); bs(" host-known "); bd(nh); bs(" | epoch "); bd(en_epoch);
  bs(" new "); bd(en_new_total); bs(" gone "); bd(en_gone_total); bs(" zombies "); bd(en_zombie_total);
  bs(" | writes "); bd(enm_writes); bs(" snaps "); bd(enm_snaps); bs(" copies ok "); bd(enm_cp_ok); bs(" skipped "); bd(enm_cp_skip); bs(" vt-bad "); bd(enm_vtbad);
  bs(" | avg distance to the host's enemy (mm) "); if (enm_dcnt) bd(enm_dsum / enm_dcnt); else bs("-"); enm_dsum = 0; enm_dcnt = 0;
  if (lk && role == 1) { bs(" | published ticks "); bd(lk->en_out_tick & 0xFFFFu); }
  lend();
  if (role == 2 && (i32)(now - en_pos_at) >= 0) { /* every 3 s: where each matched enemy is, here and at the host */
    en_pos_at = now + 3000u;
    for (j = 0; j < EN_MAX; j++) if (en[j].alive && en[j].hid < EN_MAX) { EnPkt *e = &lk->en_in[en[j].hid]; lbegin(); bs("enemy-pos: host id "); bd(en[j].hid); bs(" host mm "); bi(e->x); bs(" "); bi(e->y); bs(" "); bi(e->z); bs(" local mm "); bi(f2mm(en[j].lx)); bs(" "); bi(f2mm(en[j].ly)); bs(" "); bi(f2mm(en[j].lz)); lend(); }
  }
}
static void enemy_tick(u32 now, u32 fg) {
  u32 f7;
  if (!base) return;
  f7 = (GetAsyncKeyState(0x76) & 0x8000) != 0;
  if (f7 && !en_f7_prev && fg) { en_on = !en_on; lbegin(); bs("enemy mirror switched "); bs(en_on ? "ON" : "OFF"); bs(" (F7)"); lend(); }
  en_f7_prev = f7;
  if ((i32)(now - en_scan_at) >= 0) {
    en_scan_at = now + 100u; enemy_scan(now);
    if (lk && lk->role == 1) enemy_publish(now); else if (lk && lk->role == 2) enemy_match(now);
  }
  enemy_stats(now);
}

/* ---------------- detailed statistics (log only, once a second) ----------------
 * Part 5 asked for very detailed logs: the position of both players, the remote target, how steadily the friend's stream arrives (updates per second, longest gap, every gap over 300 ms),
 * how long the module's own loop and the puppet thread were stalled (a frozen game shows up here), the buttons of both sides and the keys that are held. */
static u32 st_t, st_loop_last, st_loop_max, st_rem_last, st_rem_upd, st_rem_gapmax, st_rem_gaps, st_w0, st_s0, st_n0, st_rb_prev; static u32 st_rem_tot, st_gap_tot;
static void st_pos(u32 slot) { u32 c = pp_char(slot); float p[3]; float yaw = 0.0f;
  if (c && rd(c + 0x190, p, 12) && rd(c + 0x1b4, &yaw, 4)) { bi((i32)(p[0] * 1000.0f)); bs(" "); bi((i32)(p[1] * 1000.0f)); bs(" "); bi((i32)(p[2] * 1000.0f)); bs(" yaw(mrad) "); bi((i32)(yaw * 1000.0f)); } else bs("n/a"); }
/* the game's own pad object of a character (the same one capture() reads for player 1): what the game itself believes is held. For player 2 this shows whether an injected key really arrived. */
static int pad_read(u32 slot, i32 *ax, i32 *ay, u32 *btn) {
  u32 ch = pp_char(slot), pad, rec, held; *ax = *ay = 0; *btn = 0;
  if (!ch) return 0;
  pad = rd32(ch + 0x4C0);
  if (!pad || rd32(pad) != base - IMAGEBASE + VT_PAD || rd32(pad + 4) != ch) return 0;
  *ax = axis100(rd32(pad + 0x18)); *ay = axis100(rd32(pad + 0x1c));
  rec = rd32(pad + 0xC); if (rec) { held = rd32(rec + 4); if (held & 0x4000u) *btn |= BT_JUMP; if (held & 0x8000u) *btn |= BT_ACTION; if (held & 0x2000u) *btn |= BT_SPECIAL; }
  return 1;
}
/* field watcher (discovery for the animation state): every ~40 ms the first 0x800 bytes of player 1 and player 2 are compared with the last sample; the fields that changed are logged as "watch".
 * Float-looking values are skipped (positions, matrices), and a field that changes again and again (a timer) is silenced by a decaying counter, so mostly the state flags that move with attacks, jumps and so on are left. */
#define WN 512
static u32 w_prev[2][WN], w_ch[2], w_t, w_dec; static u8 w_cnt[2][WN]; static u32 w_lines_sec, w_sec_t, w_lines_dropped;
static int w_floatish(u32 v) { u32 e = (v >> 23) & 0xFFu; return e >= 100u && e <= 160u; }
static void watch_char(u32 slot, u32 now) {
  u32 ch = pp_char(slot), cur[WN], i, k = 0, ids[12], ov[12], nv[12];
  if (!ch) { w_ch[slot] = 0; return; }
  if (!rd(ch, cur, sizeof cur)) return;
  if (w_ch[slot] != ch) { w_ch[slot] = ch; for (i = 0; i < WN; i++) { w_prev[slot][i] = cur[i]; w_cnt[slot][i] = 0; } return; }
  for (i = 0; i < WN; i++) {
    if (cur[i] == w_prev[slot][i]) continue;
    if (!w_floatish(cur[i]) && !w_floatish(w_prev[slot][i]) && w_cnt[slot][i] <= 12u && k < 12) { ids[k] = i * 4; ov[k] = w_prev[slot][i]; nv[k] = cur[i]; k++; }
    { u32 c = (u32)w_cnt[slot][i] + 4u; w_cnt[slot][i] = (u8)(c > 120u ? 120u : c); }
    w_prev[slot][i] = cur[i];
  }
  if (k) {
    if (w_lines_sec >= 40u) { w_lines_dropped++; return; }
    w_lines_sec++; lbegin(); bs("watch P"); bd(slot + 1); bs(":");
    for (i = 0; i < k; i++) { bs(" +"); bh(ids[i]); bs(" "); bh(ov[i]); bs("->"); bh(nv[i]); }
    lend();
  }
}
static void watch_all(u32 now) {
  if ((i32)(now - w_t) < 40) return;
  w_t = now;
  if ((i32)(now - w_sec_t) >= 1000) { w_sec_t = now; w_lines_sec = 0; for (w_dec = 0; w_dec < 2; w_dec++) { u32 i; for (i = 0; i < WN; i++) w_cnt[w_dec][i] = w_cnt[w_dec][i] > 8u ? (u8)(w_cnt[w_dec][i] - 8u) : 0; } }
  watch_char(0, now); watch_char(1, now);
}
static void detail_stats(u32 now, u32 fg, u32 p2) {
  u32 g;
  if (!lk || !base) return;
  if (st_loop_last) { g = now - st_loop_last; if (g > st_loop_max) st_loop_max = g; } st_loop_last = now;
  if (lk->rem_valid && lk->rem_tick != st_rem_last) {
    if (st_rem_last) { g = lk->rem_tick - st_rem_last; if (g < 100000u) { if (g > st_rem_gapmax) st_rem_gapmax = g; if (g > 300u) { st_rem_gaps++; st_gap_tot++; lbegin(); bs("stream: the friend's position stream paused for "); bd(g); bs(" ms"); lend(); } } }
    st_rem_last = lk->rem_tick; st_rem_upd++; st_rem_tot++;
  }
  { i32 pax, pay; u32 pb; static u32 pb_prev, pb_ok_prev; int ok = pad_read(1, &pax, &pay, &pb);
    if (ok != (int)pb_ok_prev) { pb_ok_prev = (u32)ok; lbegin(); bs(ok ? "p2pad: the game's pad object of player 2 found" : "p2pad: player 2 pad not available"); lend(); }
    if (ok && pb != pb_prev) { lbegin(); bs("p2pad: the game sees player 2 holding: "); bbtn(pb); bs(" (axes "); bi(pax); bs("/"); bi(pay); bs(") injected keys "); bh(keys_now); lend(); pb_prev = pb; } }
  watch_all(now);
  if (lk->cap_buttons != st_rb_prev) st_rb_prev = lk->cap_buttons;
  if ((i32)(now - st_t) < 1000) return;
  st_t = now;
  if (!p2 && !lk->rem_valid) { st_loop_max = 0; pp_gap_max = 0; st_rem_upd = 0; st_rem_gapmax = 0; st_rem_gaps = 0; return; }
  lbegin(); bs("state: fg "); bd(fg); bs(" p2 "); bd(p2);
  bs(" | P1 mm x y z "); st_pos(0);
  bs(" | P2 mm x y z "); st_pos(1);
  if (lk->rem_valid) { bs(" | target mm x y z "); bi((i32)(lk->rem_px * 1000.0f)); bs(" "); bi((i32)(lk->rem_py * 1000.0f)); bs(" "); bi((i32)(lk->rem_pz * 1000.0f)); bs(" yaw(mrad) "); bi((i32)(lk->rem_yaw * 1000.0f)); bs(" age ms "); bd(now - lk->rem_tick); } else bs(" | target none");
  bs(" | dist mm "); bd((u32)pp_net_dist_mm);
  bs(" | stream updates/s "); bd(st_rem_upd); bs(" longest gap ms "); bd(st_rem_gapmax); bs(" gaps>300ms "); bd(st_rem_gaps); bs(" (total "); bd(st_gap_tot); bs(")");
  bs(" | puppet writes/s "); bd(pp_net_writes - st_w0); bs(" snaps/s "); bd(pp_net_snaps - st_s0); bs(" no-p2/s "); bd(pp_net_none - st_n0);
  bs(" | stall ms: module loop "); bd(st_loop_max); bs(" puppet thread "); bd(pp_gap_max);
  { i32 pax, pay; u32 pb; if (pad_read(1, &pax, &pay, &pb)) { bs(" | P2 pad: axes "); bi(pax); bs("/"); bi(pay); bs(" held "); bbtn(pb); } else bs(" | P2 pad n/a"); }
  bs(" | watch lines dropped "); bd(w_lines_dropped);
  bs(" | buttons: own "); bbtn(lk->cap_buttons); bs(" friend-to-P2 "); bbtn(lk->buttons); bs(" inject "); bd(lk->inject); bs(" keys "); bh(keys_now);
  lend();
  st_w0 = pp_net_writes; st_s0 = pp_net_snaps; st_n0 = pp_net_none; st_loop_max = 0; pp_gap_max = 0; st_rem_upd = 0; st_rem_gapmax = 0; st_rem_gaps = 0;
}

/* ---------------- Part 5, step A: in-game panel (overlay) ----------------
 * Draws our own panel on top of the game picture. The game is Direct3D 9 (imports d3d9.dll and d3dx9_41.dll). Method from the repo (playbook/methods.md, "draw with a swap-chain hook"):
 * create a throw-away device, read the vtable of IDirect3DDevice9 (every device of the d3d9.dll shares it), overwrite Present (17) and Reset (16) and draw from the Present hook.
 * The picture is drawn with Clear() rectangles and text with ID3DXFont; the render states are saved and restored with a state block. F9 opens and closes the panel.
 * The hooks log nothing (the log buffer belongs to the worker thread); they only count frames. Any failure: no hook, the game goes on untouched. */
K u32 VirtualProtect(PV, u32, u32, u32 *);
K PV CreateWindowExW(u32, const u16 *, const u16 *, u32, i32, i32, i32, i32, PV, PV, PV, PV);
K int DestroyWindow(PV);
typedef u32 (__stdcall *F_rel)(PV);
typedef long (__stdcall *F_present)(PV, const void *, const void *, PV, const void *);
typedef long (__stdcall *F_reset)(PV, void *);
static void *ov_orig_present, *ov_orig_reset;
static PV ov_font, ov_d3dx;
static volatile u32 ov_on, ov_frames, ov_errs, ov_hint_until, ov_last_w, ov_last_h;
static volatile PV ov_lk;
static void *ov_orig_end, *ov_orig_scp, *ov_orig_cd, *ov_orig_cdx;
static volatile u32 ov_n_end, ov_n_sc, ov_n_reset, ov_n_cd, ov_n_cdx, ov_cd_flags, ov_cd_hr, ov_real_vt, ov_dummy_vt, ov_real_scvt, ov_dummy_scvt, ov_in_draw, ov_dev_ptr;
#define VT(o) (*(void ***)(o))
static void ov_text(PV font, const u16 *s, i32 x, i32 y, u32 color) {
  i32 rc[4]; u32 n = 0; rc[0] = x; rc[1] = y; rc[2] = x + 1600; rc[3] = y + 100; while (s[n]) n++;
  ((long (__stdcall *)(PV, PV, const u16 *, i32, i32 *, u32, u32))VT(font)[15])(font, 0, s, (i32)n, rc, 0x100 /* DT_NOCLIP */, color);
}
static void ov_rect(PV dev, i32 x1, i32 y1, i32 x2, i32 y2, u32 color) {
  i32 r[4]; r[0] = x1; r[1] = y1; r[2] = x2; r[3] = y2;
  ((long (__stdcall *)(PV, u32, const i32 *, u32, u32, float, u32))VT(dev)[43])(dev, 1, r, 1 /* D3DCLEAR_TARGET */, color, 1.0f, 0);
}
static void ov_cat(u16 *d, const u16 *a) { int n = 0; while (d[n]) n++; while (*a) d[n++] = *a++; d[n] = 0; }
static void ov_draw2(PV dev) {
  PV bb = 0, sb = 0; u32 desc[8], now = GetTickCount(), W, H, fh; int panel = ov_on != 0, hint = !panel && (i32)(now - ov_hint_until) < 0;
  if (!panel && !hint) return;
  if (((long (__stdcall *)(PV, u32, u32, u32, PV *))VT(dev)[18])(dev, 0, 0, 0, &bb) < 0 || !bb) { ov_errs++; return; }
  if (((long (__stdcall *)(PV, u32 *))VT(bb)[12])(bb, desc) < 0) { ((F_rel)VT(bb)[2])(bb); ov_errs++; return; }
  W = desc[6]; H = desc[7]; ov_last_w = W; ov_last_h = H;
  if (W < 200 || H < 150) { ((F_rel)VT(bb)[2])(bb); return; }
  fh = H / 46; if (fh < 15) fh = 15; if (fh > 30) fh = 30;
  if (((long (__stdcall *)(PV, u32, PV *))VT(dev)[59])(dev, 1 /* D3DSBT_ALL */, &sb) >= 0 && sb) ((long (__stdcall *)(PV))VT(sb)[4])(sb); else sb = 0;
  ((long (__stdcall *)(PV))VT(dev)[41])(dev); /* BeginScene */
  { u32 vp[6]; vp[0] = 0; vp[1] = 0; vp[2] = W; vp[3] = H; vp[4] = 0; vp[5] = 0x3f800000u; /* full-picture viewport (the game may have a split-screen one set); the state block restores it */
    ((long (__stdcall *)(PV, const u32 *))VT(dev)[47])(dev, vp);
    ((long (__stdcall *)(PV, u32, u32))VT(dev)[57])(dev, 174 /* D3DRS_SCISSORTEST */, 0); }
  if (!ov_font) {
    typedef long (__stdcall *F_cf)(PV, i32, u32, u32, u32, i32, u32, u32, u32, u32, const u16 *, PV *);
    if (!ov_d3dx) ov_d3dx = LoadLibraryW(L"d3dx9_41.dll");
    if (ov_d3dx) { F_cf cf = (F_cf)GetProcAddress(ov_d3dx, "D3DXCreateFontW"); if (cf) { PV f = 0; if (cf(dev, (i32)fh, 0, 400, 1, 0, 1, 0, 4, 0, L"Segoe UI", &f) >= 0) ov_font = f; } }
    if (!ov_font) ov_errs++;
  }
  if (panel) {
    /* compact panel at the left edge, vertically a little above the middle (away from the corner HUD and the bottom hints) */
    i32 pad = (i32)fh / 2, line = (i32)fh + (i32)fh / 4, w = (i32)fh * 17, h = pad * 2 + line * 4 + (i32)fh / 2, x = (i32)(W / 60), y = (i32)(H * 3 / 10);
    u16 t[120]; PV lkp = ov_lk; Link *l = (Link *)lkp;
    ov_rect(dev, x - 4, y - 4, x + w + 4, y + h + 4, 0xFF5A4620u);   /* outer dark-gold edge */
    ov_rect(dev, x - 2, y - 2, x + w + 2, y + h + 2, 0xFFD2A84Au);   /* gold line */
    ov_rect(dev, x, y, x + w, y + h, 0xFF16130Eu);                    /* body */
    ov_rect(dev, x, y, x + w, y + line + pad, 0xFF2C2416u);           /* title bar */
    ov_rect(dev, x, y + line + pad, x + w, y + line + pad + 2, 0xFFD2A84Au);
    if (ov_font) {
      ov_text(ov_font, L"Тера Кооп", x + pad, y + pad / 2, 0xFFF0D070u);
      t[0] = 0; ov_cat(t, (l && (i32)(now - l->prog_tick) < 1500) ? L"Программа: на связи" : L"Программа: не запущена"); ov_text(ov_font, t, x + pad, y + pad + line + pad / 2, 0xFFE6E0D0u);
      ov_text(ov_font, pp_mode == 0 ? L"F8 — марионетка: выкл" : pp_mode == 1 ? L"F8 — марионетка: режим A" : L"F8 — марионетка: режим B", x + pad, y + pad + line * 2 + pad / 2, 0xFFA8A090u);
      ov_text(ov_font, L"F9 — закрыть", x + pad, y + pad + line * 3 + pad / 2, 0xFF908870u);
    }
  } else if (ov_font) {
    ov_text(ov_font, L"F9 — совместная игра (Тера Кооп)", (i32)(W / 40), (i32)(H / 40), 0xFFE8C860u);
  }
  ((long (__stdcall *)(PV))VT(dev)[42])(dev); /* EndScene */
  if (sb) { ((long (__stdcall *)(PV))VT(sb)[5])(sb); ((F_rel)VT(sb)[2])(sb); }
  ((F_rel)VT(bb)[2])(bb);
}
static void ov_draw(PV dev) {
  if (ov_in_draw) return;
  /* device lost / being reset (minimised window, alt-tab): draw nothing and let go of the font (it lives in the default pool and would make Reset fail) */
  if (((long (__stdcall *)(PV))VT(dev)[3])(dev) != 0) { if (ov_font) { PV f = ov_font; ov_font = 0; ((F_rel)VT(f)[2])(f); } return; }
  ov_in_draw = 1; ov_draw2(dev); ov_in_draw = 0;
}
static long __stdcall hk_endscene(PV dev) { if (!ov_in_draw) ov_n_end++; return ((long (__stdcall *)(PV))ov_orig_end)(dev); }
static long __stdcall hk_scpresent(PV sc, const void *a, const void *b, PV w, const void *d, u32 fl) {
  ov_n_sc++;
  if (!ov_frames) { PV dev = 0; if (((long (__stdcall *)(PV, PV *))VT(sc)[8])(sc, &dev) >= 0 && dev) { ov_draw(dev); ((F_rel)VT(dev)[2])(dev); } }
  return ((long (__stdcall *)(PV, const void *, const void *, PV, const void *, u32))ov_orig_scp)(sc, a, b, w, d, fl);
}
static void *ov_orig_present_r, *ov_orig_reset_r;
static volatile u32 ov_patched_vt;
static long __stdcall hk_present_r(PV dev, const void *a, const void *b, PV w, const void *d) { ov_frames++; ov_dev_ptr = (u32)dev; ov_draw(dev); return ((F_present)ov_orig_present_r)(dev, a, b, w, d); }
static volatile u32 ov_reset_seq, ov_reset_hr, ov_reset_pre;
static long __stdcall hk_reset_r(PV dev, void *pp) {
  long hr; ov_n_reset++; ov_reset_pre = ov_font != 0; if (ov_font) { PV f = ov_font; ov_font = 0; ((F_rel)VT(f)[2])(f); }
  hr = ((F_reset)ov_orig_reset_r)(dev, pp); ov_reset_hr = (u32)hr; ov_reset_seq++;
  return hr;
}
static long __stdcall hk_cd(PV d3d, u32 ad, u32 dt, PV hw, u32 fl, void *pp, PV *ppd) {
  long hr = ((long (__stdcall *)(PV, u32, u32, PV, u32, void *, PV *))ov_orig_cd)(d3d, ad, dt, hw, fl, pp, ppd);
  ov_n_cd++; ov_cd_flags = fl; ov_cd_hr = (u32)hr;
  if (hr >= 0 && ppd && *ppd) { PV sc = 0; ov_dev_ptr = (u32)*ppd; ov_real_vt = (u32)VT(*ppd);
    if ((u32)VT(*ppd) != ov_dummy_vt) { void **rv = VT(*ppd); u32 old; /* the game makes its own table (pure device): patch that one with its own originals */
      if (rv[17] != (void *)hk_present_r && VirtualProtect(&rv[16], 8, 4, &old)) { ov_orig_reset_r = rv[16]; ov_orig_present_r = rv[17]; rv[16] = (void *)hk_reset_r; rv[17] = (void *)hk_present_r; VirtualProtect(&rv[16], 8, old, &old); ov_patched_vt = (u32)rv; } }
    if (((long (__stdcall *)(PV, u32, PV *))VT(*ppd)[14])(*ppd, 0, &sc) >= 0 && sc) { ov_real_scvt = (u32)VT(sc); ((F_rel)VT(sc)[2])(sc); } }
  return hr;
}
static long __stdcall hk_cdx(PV d3d, u32 ad, u32 dt, PV hw, u32 fl, void *pp, void *fs, PV *ppd) {
  ov_n_cdx++; return ((long (__stdcall *)(PV, u32, u32, PV, u32, void *, void *, PV *))ov_orig_cdx)(d3d, ad, dt, hw, fl, pp, fs, ppd);
}
static long __stdcall hk_present(PV dev, const void *a, const void *b, PV w, const void *d) {
  ov_frames++; ov_dev_ptr = (u32)dev;
  ov_draw(dev);
  return ((F_present)ov_orig_present)(dev, a, b, w, d);
}
static long __stdcall hk_reset(PV dev, void *pp) {
  ov_n_reset++;
  if (ov_font) { PV f = ov_font; ov_font = 0; ((F_rel)VT(f)[2])(f); }
  return ((F_reset)ov_orig_reset)(dev, pp);
}
/* returns 0 = hooked, otherwise a step number (and the HRESULT in *hr) */
static int ov_install(long *hr) {
  typedef PV (__stdcall *F_create9)(u32);
  PV d3d9 = LoadLibraryW(L"d3d9.dll"), d3d, hw, dev = 0; F_create9 c9; u32 pp[14], old, i; void **vt;
  *hr = 0; if (!d3d9) return 1;
  c9 = (F_create9)GetProcAddress(d3d9, "Direct3DCreate9"); if (!c9) return 2;
  d3d = c9(32); if (!d3d) return 3;
  hw = CreateWindowExW(0, L"STATIC", L"", 0x80000000u /* WS_POPUP */, 0, 0, 2, 2, 0, 0, GetModuleHandleW(0), 0); if (!hw) { ((F_rel)VT(d3d)[2])(d3d); return 4; }
  for (i = 0; i < 14; i++) pp[i] = 0;
  pp[0] = 2; pp[1] = 2; pp[3] = 1; pp[6] = 1; pp[7] = (u32)hw; pp[8] = 1;
  *hr = ((long (__stdcall *)(PV, u32, u32, PV, u32, void *, PV *))VT(d3d)[16])(d3d, 0, 1 /* HAL */, hw, 0x22 /* SOFTWARE_VERTEXPROCESSING | FPU_PRESERVE */, pp, &dev);
  if (*hr < 0 || !dev) { DestroyWindow(hw); ((F_rel)VT(d3d)[2])(d3d); return 5; }
  vt = VT(dev);
  if (!VirtualProtect(&vt[16], 8, 4 /* PAGE_READWRITE */, &old)) { ((F_rel)VT(dev)[2])(dev); ((F_rel)VT(d3d)[2])(d3d); DestroyWindow(hw); return 6; }
  ov_orig_reset = vt[16]; ov_orig_present = vt[17]; vt[16] = (void *)hk_reset; vt[17] = (void *)hk_present;
  VirtualProtect(&vt[16], 8, old, &old);
  ov_dummy_vt = (u32)vt;
  if (VirtualProtect(&vt[42], 4, 4, &old)) { ov_orig_end = vt[42]; vt[42] = (void *)hk_endscene; VirtualProtect(&vt[42], 4, old, &old); }
  { void **dv = VT(d3d); if (VirtualProtect(&dv[16], 4, 4, &old)) { ov_orig_cd = dv[16]; dv[16] = (void *)hk_cd; VirtualProtect(&dv[16], 4, old, &old); } }
  { PV sc = 0; if (((long (__stdcall *)(PV, u32, PV *))VT(dev)[14])(dev, 0, &sc) >= 0 && sc) { void **sv = VT(sc); ov_dummy_scvt = (u32)sv;
      if (VirtualProtect(&sv[3], 4, 4, &old)) { ov_orig_scp = sv[3]; sv[3] = (void *)hk_scpresent; VirtualProtect(&sv[3], 4, old, &old); } ((F_rel)VT(sc)[2])(sc); } }
  { typedef long (__stdcall *F_c9x)(u32, PV *); F_c9x c9x = (F_c9x)GetProcAddress(d3d9, "Direct3DCreate9Ex"); PV dx = 0;
    if (c9x && c9x(32, &dx) >= 0 && dx) { void **xv = VT(dx); if (VirtualProtect(&xv[20], 4, 4, &old)) { ov_orig_cdx = xv[20]; xv[20] = (void *)hk_cdx; VirtualProtect(&xv[20], 4, old, &old); } ((F_rel)VT(dx)[2])(dx); } }
  ((F_rel)VT(dev)[2])(dev); ((F_rel)VT(d3d)[2])(d3d); DestroyWindow(hw);
  return 0;
}

/* ---------------- worker ---------------- */
static u32 __stdcall worker(PV arg) {
  u32 pe = 0, ts, soi, last_open = 0, last_f2 = 0, last_p2 = 0, prog_ok_prev = 0;
  u32 last_tseq = 0, tkind = 0, tdead = 0, trun = 0, tjoin = 0, tkeys = 0;
  u32 ov_cnt_sig = 0, ov_cnt_at = 0, ov_state = 0, ov_try_at = 0, ov_tries = 0, f9_prev = 0, ov_frames_logged = 0, ov_on_prev = 0; int build_ok = 0; (void)arg;
  t0 = GetTickCount();
  { u16 path[420]; int n = (int)GetModuleFileNameW(0, path, 380); while (n > 0 && path[n - 1] != '\\') n--; for (dirn = 0; dirn < n; dirn++) dirw[dirn] = path[dirn]; }
  open_log();
  base = (u32)GetModuleHandleW(0);
  pe = *(const u32 *)(base + 0x3c); ts = *(const u32 *)(base + pe + 8); soi = *(const u32 *)(base + pe + 24 + 56);
  /* the controls of this PC: copies of Mappings.dat and pcconfig.txt next to the log (opened with full sharing: the game may keep Mappings.dat open) */
  cfg_copy(L"Mappings.dat", "Mappings.dat", L"config-Mappings", L".dat");
  cfg_copy(L"pcconfig.txt", "pcconfig.txt", L"config-pcconfig", L".txt");
  lbegin(); bs("teracoop module v2.1c (F11 data recording, F3/F4/F5 markers, F6 pad injection test, F7 list mirror (OFF by default), controls from Mappings.dat) started, exe base "); bh(base); bs(" timestamp "); bh(ts); bs(" size "); bh(soi); lend();
  build_ok = (ts == TIMESTAMP && soi == SIZEOFIMAGE);
  /* overlay: install at once (the game has not created its Direct3D device yet), retry a few times */
  { u16 fp[460]; mkpath(fp, L"teracoop_overlay.txt"); if (GetFileAttributesW(fp) == 0xFFFFFFFFu) { ov_tries = 99; say("overlay: panel is switched off (create the file teracoop_overlay.txt in the game folder to try it)"); } }
  if (ov_tries != 99) { long hr = 0; int r = ov_install(&hr); lbegin(); bs("overlay: hook install step "); bd((u32)r); bs(" hresult "); bh((u32)hr); lend(); ov_state = r == 0; ov_tries = 1; ov_hint_until = GetTickCount() + 120000u; }
  if (build_ok) { CreateThread(0, 0, puppet_thread, 0, 0, 0); pe_ranges(); load_bindings("start"); }
  say(build_ok ? "game build supported" : "game build NOT supported - the module will not touch the game");
  for (;;) {
    u32 now, state = 0, p2 = 0, fg, prog_alive, want = 0, want_join = 0;
    Sleep(10); now = GetTickCount();
    /* the F11 dump of step 5.1 is finished and switched off (F11 must stay free for the game and for screenshots) */
    { static u32 nlog_at, nsig; if ((i32)(now - nlog_at) >= 0) { u32 sg = pp_net_writes + pp_net_snaps * 3 + pp_net_none * 5; nlog_at = now + 3000u; if (sg != nsig) { nsig = sg; lbegin(); bs("puppet-net: writes "); bd(pp_net_writes); bs(" snaps "); bd(pp_net_snaps); bs(" no-p2-frames "); bd(pp_net_none); bs(" distance (mm) "); bd((u32)pp_net_dist_mm); lend(); } } }
    { u32 f8 = (GetAsyncKeyState(0x77) & 0x8000) != 0; static u32 f8_prev, pp_log_at, pp_sig;
      if (f8 && !f8_prev && game_fg()) { pp_mode = (pp_mode + 1) % 3; pp_have = 0; lbegin(); bs("puppet: mode "); bd(pp_mode); bs(pp_mode == 0 ? " (off)" : pp_mode == 1 ? " (A: matrix only)" : " (B: all copies)"); lend(); }
      f8_prev = f8;
      if (pp_mode && (i32)(now - pp_log_at) >= 0) { u32 sig = pp_writes + pp_kept * 3 + pp_reverted * 5 + pp_noP2 * 7; pp_log_at = now + 2000u;
        if (sig != pp_sig) { pp_sig = sig; lbegin(); bs("puppet: writes "); bd(pp_writes); bs(" kept "); bd(pp_kept); bs(" reverted "); bd(pp_reverted); bs(" no-p2-frames "); bd(pp_noP2); bs(" last revert dx/dz (mm) "); if (pp_lastdx < 0) { bs("-"); bd((u32)-pp_lastdx); } else bd((u32)pp_lastdx); bs("/"); if (pp_lastdz < 0) { bs("-"); bd((u32)-pp_lastdz); } else bd((u32)pp_lastdz); bs(" p2 char "); bh(pp_lastch); lend(); } } }
    /* overlay housekeeping (runs even before the program is connected) */
    if (!ov_state && ov_tries && ov_tries < 40 && (i32)(now - ov_try_at) >= 0) { long hr = 0; int r = ov_install(&hr); ov_tries++; ov_try_at = now + 500u; if (r == 0) { ov_state = 1; lbegin(); bs("overlay: hook installed on try "); bd(ov_tries); lend(); } else if (ov_tries == 40) { lbegin(); bs("overlay: giving up, last step "); bd((u32)r); bs(" hresult "); bh((u32)hr); lend(); } }
    if (ov_state) {
      u32 f9 = (GetAsyncKeyState(0x78) & 0x8000) != 0; ov_lk = (PV)lk;
      if (f9 && !f9_prev && game_fg()) { ov_on = !ov_on; }
      f9_prev = f9;
      if (ov_on != ov_on_prev) { ov_on_prev = ov_on; lbegin(); bs(ov_on ? "overlay: panel opened (F9)" : "overlay: panel closed (F9)"); bs(", frames "); bd(ov_frames); bs(", errors "); bd(ov_errs); bs(", picture "); bd(ov_last_w); bs("x"); bd(ov_last_h); lend(); }
      if (ov_patched_vt) { /* something else (e.g. the Steam overlay) may overwrite the table entry after us: put ours back, keep the other as the original so the chain still works */
        u32 cur = rd32(ov_patched_vt + 17 * 4);
        if (cur && cur != (u32)hk_present_r) { u32 old; lbegin(); bs("overlay: Present entry was replaced by "); bh(cur); bs(" (ours "); bh((u32)hk_present_r); bs(", old original "); bh((u32)ov_orig_present_r); bs("), re-hooking"); lend();
          if (VirtualProtect((PV)(ov_patched_vt + 16 * 4), 8, 4, &old)) { ov_orig_present_r = (void *)cur; *(void **)(ov_patched_vt + 17 * 4) = (void *)hk_present_r; VirtualProtect((PV)(ov_patched_vt + 16 * 4), 8, old, &old); } }
      }
      { static u32 rseq; if (ov_reset_seq != rseq) { rseq = ov_reset_seq; lbegin(); bs("overlay: game called Reset, result "); bh(ov_reset_hr); bs(", font was alive "); bd(ov_reset_pre); lend(); } }
      if (ov_patched_vt) { u32 cr = rd32(ov_patched_vt + 16 * 4);
        if (cr && cr != (u32)hk_reset_r) { u32 old; lbegin(); bs("overlay: Reset entry was replaced by "); bh(cr); bs(", re-hooking"); lend();
          if (VirtualProtect((PV)(ov_patched_vt + 16 * 4), 4, 4, &old)) { ov_orig_reset_r = (void *)cr; *(void **)(ov_patched_vt + 16 * 4) = (void *)hk_reset_r; VirtualProtect((PV)(ov_patched_vt + 16 * 4), 4, old, &old); } } }
      if ((i32)(now - ov_cnt_at) >= 0) { u32 sig = ov_frames + ov_n_end * 3 + ov_n_sc * 5 + ov_n_reset * 7 + ov_n_cd * 11 + ov_n_cdx * 13; ov_cnt_at = now + 3000u;
        if (sig != ov_cnt_sig) { ov_cnt_sig = sig; lbegin(); bs("overlay: counts present "); bd(ov_frames); bs(" endscene "); bd(ov_n_end); bs(" swapchain-present "); bd(ov_n_sc); bs(" reset "); bd(ov_n_reset); bs(" createdevice "); bd(ov_n_cd); bs(" createdeviceex "); bd(ov_n_cdx);
          bs(" flags "); bh(ov_cd_flags); bs(" hr "); bh(ov_cd_hr); bs(" devvt "); bh(ov_real_vt); bs(" dummyvt "); bh(ov_dummy_vt); bs(" scvt "); bh(ov_real_scvt); bs(" dummyscvt "); bh(ov_dummy_scvt); bs(" dev "); bh(ov_dev_ptr); bs(" patched "); bh(ov_patched_vt); bs(" entry17 "); bh(ov_patched_vt ? rd32(ov_patched_vt + 68) : 0); bs(" orig17 "); bh((u32)ov_orig_present_r); lend(); } }
      if (!ov_frames_logged && ov_frames) { ov_frames_logged = 1; say("overlay: the game is presenting frames through the hook"); }
    }
    if (build_ok) { u32 fg0 = (u32)game_fg(); census_tick(now, fg0); enemy_tick(now, fg0); }
    if (!lk) {
      if (now - last_open < 500u && last_open) continue;
      last_open = now ? now : 1;
      { PV h = OpenFileMappingW(0xF001Fu /* FILE_MAP_ALL_ACCESS */, 0, LINK_NAME);
        if (h) { Link *m = (Link *)MapViewOfFile(h, 0xF001Fu, 0, 0, 0); if (m && m->magic == LINK_MAGIC && m->version == LINK_VERSION) { lk = m; say("connected to the program"); } } }
      continue;
    }
    if (lk->magic != LINK_MAGIC) { continue; }
    fg = (u32)game_fg();
    if (build_ok) p2 = (*(volatile const u32 *)(base - IMAGEBASE + VA_P2LIST) != 0);
    prog_alive = (i32)(now - lk->prog_tick) < 1500;
    if (build_ok) capture(base, now);
    if (build_ok) detail_stats(now, fg, p2);
    if (lk->room_role && lk->room_role != cur_role && !cur_role) { cur_role = lk->room_role; ren_all(); lbegin(); bs("role: this PC is the "); bs(cur_role == 1 ? "HOST" : "GUEST"); bs(" - log files renamed"); lend(); }
    if (build_ok) test_script(now);
    if (prog_alive != (prog_ok_prev != 0)) { prog_ok_prev = prog_alive; say(prog_alive ? "program is talking" : "program stopped talking - keys released"); }
    if (p2 != last_p2) { last_p2 = p2; say(p2 ? "player 2 is active" : "player 2 is not active"); }
    /* --- a check requested by the program (it waits until the game window is active) --- */
    if (lk->test_seq != last_tseq) { last_tseq = lk->test_seq; tkind = lk->test_kind; tdead = now + 25000u; trun = 0; tjoin = 0; lbegin(); bs("check requested, kind "); bd(tkind); lend(); lk->test_result = 0; }
    tkeys = 0;
    if (tkind) {
      if (!build_ok || !prog_alive) { lk->test_result = 2; lk->test_done_seq = last_tseq; tkind = 0; say("check failed: build not supported or program silent"); }
      else if ((i32)(now - tdead) > 0) { lk->test_result = 2; lk->test_done_seq = last_tseq; tkind = 0; say("check failed: the game window did not become active in time"); }
      else if (fg) {
        if (tkind == 1) {
          if (p2) { lk->test_result = 1; lk->test_done_seq = last_tseq; tkind = 0; say("check ok: player 2 joined"); }
          else { want_join = 1; if (!tjoin) tjoin = now + 8000u; if ((i32)(now - tjoin) > 0) { lk->test_result = 2; lk->test_done_seq = last_tseq; tkind = 0; say("check failed: player 2 did not join (is the game inside a level?)"); } }
        } else {
          if (!trun) { trun = now + 1500u; say("check running"); }
          if ((i32)(now - trun) < 0) tkeys = tkind == 2 ? (1u << KB_RIGHT) : tkind == 3 ? (1u << KB_JUMP) : (1u << KB_ACTION);
          else { lk->test_result = 1; lk->test_done_seq = last_tseq; tkind = 0; say("check done"); }
        }
      }
    }
    /* --- normal control from the program --- */
    if (build_ok && prog_alive && fg && lk->inject) {
      i32 ax = lk->ax, ay = lk->ay; u32 b = lk->buttons;
      if (ax < -35) want |= 1u << KB_LEFT; if (ax > 35) want |= 1u << KB_RIGHT; if (ay > 35) want |= 1u << KB_UP; if (ay < -35) want |= 1u << KB_DOWN;
      if (b & BT_ACTION) want |= 1u << KB_ACTION; if (b & BT_SPECIAL) want |= 1u << KB_SPECIAL; if (b & BT_JUMP) want |= 1u << KB_JUMP;
    }
    if (build_ok && prog_alive && fg && lk->want_p2) want_join = 1;
    if (!(build_ok && prog_alive && fg)) tkeys = 0;
    want |= tkeys;
    if (!(build_ok && fg)) want = 0;
    if (!prog_alive) want = 0;
    { static u32 f6_prev, padlog_at; u32 f6 = (GetAsyncKeyState(0x75) & 0x8000) != 0;
      if (f6 && !f6_prev && fg) { inj_pad = !inj_pad; lbegin(); bs("pad-inject mode "); bs(inj_pad ? "ON (the friend's actions are written straight into the pad of player 2, no keys)" : "OFF (keys of this PC's Mappings.dat)"); bs(" (F6)"); lend(); }
      f6_prev = f6;
      if (inj_pad && (i32)(now - padlog_at) >= 0) { padlog_at = now + 2000u; if (padi_writes || padi_fail) { lbegin(); bs("pad-inject: writes "); bd(padi_writes); bs(" failed (pad not found) "); bd(padi_fail); lend(); } } }
    if (inj_pad) want = 0;
    if (want != keys_now) set_keys(want);
    /* --- join player 2 with the game's own F2 --- */
    if (build_ok && prog_alive && fg && want_join && !p2 && (i32)(now - last_f2) > 2500) {
      last_f2 = now; load_bindings("before F2"); send_sc(0x3C /* F2 */, 1, 0); Sleep(40); send_sc(0x3C, 0, 0); lk->f2_presses++; say("F2 pressed (join player 2)");
    }
    state = (build_ok ? ST_BUILD_OK : 0) | (fg ? ST_FOREGROUND : 0) | (p2 ? ST_P2_ACTIVE : 0) | (keys_now ? ST_INJECTING : 0) | (tkind && !tkeys && !trun ? ST_TEST_WAIT : 0) | (tkind && (trun || tkeys) ? ST_TEST_RUN : 0);
    lk->plug_state = state; lk->plug_ver = MOD_VERSION; lk->keys_down = keys_now; lk->plug_tick = now;
  }
}

/* ---------------- swallow the game input while the panel is open ----------------
 * The game reads keyboard / mouse / pad through DirectInput 8. Every device we see being created gets its GetDeviceState / GetDeviceData table entries replaced (per table, with its own originals);
 * while the panel is open the result is emptied (all keys released, no events), so the game does not react to what is typed for the panel. */
typedef struct { u32 vt; void *st, *dt; } DIHK;
static DIHK dihk[6]; static volatile u32 dihk_n;
static DIHK *dihk_find(PV dev) { u32 i, v = (u32)VT(dev); for (i = 0; i < dihk_n; i++) if (dihk[i].vt == v) return &dihk[i]; return 0; }
static long __stdcall hk_dstate(PV dev, u32 cb, void *data) {
  DIHK *h = dihk_find(dev); long hr; u32 i;
  if (!h) return (long)0x80004005;
  hr = ((long (__stdcall *)(PV, u32, void *))h->st)(dev, cb, data);
  if (hr >= 0 && ov_on && data) for (i = 0; i < cb; i++) ((volatile u8 *)data)[i] = 0;
  return hr;
}
static long __stdcall hk_ddata(PV dev, u32 cbo, void *dod, u32 *cnt, u32 fl) {
  DIHK *h = dihk_find(dev); long hr;
  if (!h) return (long)0x80004005;
  hr = ((long (__stdcall *)(PV, u32, void *, u32 *, u32))h->dt)(dev, cbo, dod, cnt, fl);
  if (hr >= 0 && ov_on && cnt) *cnt = 0;
  return hr;
}
static void *dihk_orig_create;
static long __stdcall hk_dicreate(PV di, const void *guid, PV *ppdev, PV outer) {
  long hr = ((long (__stdcall *)(PV, const void *, PV *, PV))dihk_orig_create)(di, guid, ppdev, outer);
  if (hr >= 0 && ppdev && *ppdev && !dihk_find(*ppdev) && dihk_n < 6) {
    void **vt = VT(*ppdev); u32 old;
    if (VirtualProtect(&vt[9], 8, 4, &old)) { DIHK *h = &dihk[dihk_n]; h->vt = (u32)vt; h->st = vt[9]; h->dt = vt[10]; vt[9] = (void *)hk_dstate; vt[10] = (void *)hk_ddata; VirtualProtect(&vt[9], 8, old, &old); dihk_n++; }
  }
  return hr;
}
static void dihk_wrap(PV di) {
  void **vt; u32 old; if (!di) return; vt = VT(di);
  if (vt[3] == (void *)hk_dicreate) return;
  if (VirtualProtect(&vt[3], 4, 4, &old)) { dihk_orig_create = vt[3]; vt[3] = (void *)hk_dicreate; VirtualProtect(&vt[3], 4, old, &old); }
}

/* ---------------- crash notes: the first fatal exceptions are written to the log (address inside the game / our module), then Windows goes on as usual ---------------- */
K PV AddVectoredExceptionHandler(u32, long (__stdcall *)(void *));
static volatile u32 exc_n;
static long __stdcall exc_handler(void *ep) {
  u32 *rec = *(u32 **)ep; u32 code = rec[0], addr = rec[3];
  if ((code & 0xF0000000u) == 0xC0000000u && exc_n < 6 && hlog && hlog != (PV)-1) {
    char b[160]; int n = 0, i; u32 w, v[3]; const char *t = "EXCEPTION code ", *d = "0123456789abcdef"; exc_n++;
    v[0] = code; v[1] = addr; v[2] = (rec[0] == 0xC0000005u && rec[2 + 2] >= 2) ? rec[6] : 0;
    for (i = 0; t[i]; i++) b[n++] = t[i];
    for (i = 28; i >= 0; i -= 4) b[n++] = d[(v[0] >> i) & 15];
    t = " at "; for (i = 0; t[i]; i++) b[n++] = t[i];
    for (i = 28; i >= 0; i -= 4) b[n++] = d[(v[1] >> i) & 15];
    t = " (game base "; for (i = 0; t[i]; i++) b[n++] = t[i];
    for (i = 28; i >= 0; i -= 4) b[n++] = d[(base >> i) & 15];
    t = ") data address "; for (i = 0; t[i]; i++) b[n++] = t[i];
    for (i = 28; i >= 0; i -= 4) b[n++] = d[(v[2] >> i) & 15];
    b[n++] = '\r'; b[n++] = '\n'; WriteFile(hlog, b, (u32)n, &w, 0);
  }
  return 0; /* EXCEPTION_CONTINUE_SEARCH */
}

/* ---------------- dinput8 proxy ---------------- */
typedef long (__stdcall *DI8)(PV, u32, const void *, PV *, PV);
long __stdcall DirectInput8Create(PV hinst, u32 ver, const void *riid, PV *out, PV outer) {
  static DI8 real;
  if (!real) {
    u16 p[300]; u32 n = GetSystemDirectoryW(p, 260); const u16 *f = L"\\dinput8.dll"; u32 k; PV h;
    for (k = 0; f[k]; k++) p[n + k] = f[k]; p[n + k] = 0;
    h = LoadLibraryW(p); real = h ? (DI8)GetProcAddress(h, "DirectInput8Create") : 0;
    if (!real) return (long)0x80004005;
  }
  { long hr = real(hinst, ver, riid, out, outer); if (hr >= 0 && out && *out) dihk_wrap(*out); return hr; }
}
int __stdcall DllMain(PV h, u32 reason, PV res) {
  (void)res;
  if (reason == 1) { DisableThreadLibraryCalls(h); AddVectoredExceptionHandler(1, exc_handler); CreateThread(0, 0, worker, 0, 0, 0); }
  return 1;
}
