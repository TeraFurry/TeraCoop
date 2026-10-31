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
#define MOD_VERSION 340u
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
static Link *lk; static Link loc_lk;
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
/* v2.14 (jitter): the other hero's speed from its last two packets; the puppet aims at "last position + speed x age" and moves there
   smoothly (25 % of the gap per frame), a hard jump only above 1.5 m. Runs on the game thread (PeekMessageA hook), once per frame. */
static volatile float loc_vx, loc_vy, loc_vz; static volatile u32 pp_in_game_thread;
/* Part 5.3: player 2 follows the friend. Called every frame from the game thread. Returns 1 when it handled the frame. */
static int puppet_net(void) {
  u32 c2, i; float p2[3], ex, ey, ez, dist, g, gy, dx, dy, dz, yaw, m00, m02;
  if (!lk || !lk->rem_valid || (i32)(GetTickCount() - lk->rem_tick) >= 500) return 0;
  c2 = pp_char(1); if (!c2) { pp_net_none++; return 1; }
  if (!rd(c2 + 0x190, p2, 12)) return 1;
  { float tx = lk->rem_px, ty = lk->rem_py, tz = lk->rem_pz;
    if (lk == &loc_lk) { u32 age = GetTickCount() - lk->rem_tick; float a; if (age > 120u) age = 120u; a = (float)age * 0.001f; tx += loc_vx * a; ty += loc_vy * a; tz += loc_vz * a; }
    ex = tx - p2[0]; ey = ty - p2[1]; ez = tz - p2[2]; }
  dist = __builtin_sqrtf(ex * ex + ez * ez); pp_net_dist_mm = (i32)(dist * 1000.0f);
  g = dist > 8.0f ? 1.0f : dist < 0.03f ? 0.0f : 0.2f;
  gy = (ey > 2.0f || ey < -2.0f) ? 1.0f : 0.2f;
  if (lk == &loc_lk) { g = dist > 1.5f ? 1.0f : dist < 0.02f ? 0.0f : 0.25f; gy = (ey > 1.0f || ey < -1.0f) ? 1.0f : (ey > 0.02f || ey < -0.02f) ? 0.25f : 0.0f; }   /* v2.14: smooth instead of exact jumps */
  if (g == 1.0f) pp_net_snaps++;
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
static u32 __stdcall puppet_thread(PV arg) { u32 last = 0; (void)arg; for (;;) { u32 n; Sleep(10); n = GetTickCount(); if (last) { u32 g = n - last; if (g > pp_gap_max) pp_gap_max = g; } last = n; if (!pp_in_game_thread) puppet_frame(); enemy_mirror_frame(); pad_inject_frame(); } return 0; }

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
static volatile u32 ov_no_panel, ov_on, ov_frames, ov_errs, ov_hint_until, ov_last_w, ov_last_h;
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
/* ---------------- v2.2: window mode (file teracoop_windowed.txt in the game folder) ----------------
 * The game's own "-Windowed" option is dead in this build (the flag 0x0157c820 is written but never read, checked in the exe).
 * Instead the module asks Direct3D 9 for a windowed device: in CreateDevice / Reset the documented D3DPRESENT_PARAMETERS fields
 * Windowed (index 8) = 1 and FullScreen_RefreshRateInHz (index 12) = 0; after CreateDevice the game window gets a normal frame and size.
 * The file may hold "width height [left top]" (default 1280 800, centred on the main screen). */
K int SetWindowLongW(PV, int, i32);
K int SetWindowPos(PV, PV, i32, i32, i32, i32, u32);
K int GetSystemMetrics(int);
K int AdjustWindowRect(i32 *, u32, int);
static volatile u32 multi_set;
static volatile u32 win_force, win_w = 1280, win_h = 800, win_cd_n, win_reset_n, win_fix_n, win_hwnd; static volatile i32 win_x = -1, win_y = -1;
/* v2.12: "own screen" test (file teracoop_view.txt: "left" or "left wide"). The split screen stays on inside the game (so zone
   transitions work), the game is kept in its ForcedVertical split submode (player 1 = left half), and the window shows only the left
   half of the picture (Present with a source rectangle; needs swap effect COPY and no multisampling). "wide" also makes the picture
   twice as wide as the window, so the half is not squeezed (not checked yet). F6 = left half on/off. */
static volatile u32 view_split_now, view_full, view_sc_left, view_sc_right; static volatile u32 view_on, view_wide, view_crop, view_bb_w, view_bb_h, view_pres_n, view_pres_fail; static volatile i32 view_last_hr;
static void win_fix_pp(void *pp) { u32 *p = (u32 *)pp; if (!p) return;
  if (win_force) { p[8] = 1; p[12] = 0; }
  if (view_on && !view_full && win_force) { p[0] = view_wide ? win_w * 2u : win_w; p[1] = win_h; p[4] = 0; p[5] = 0; p[6] = 3 /* D3DSWAPEFFECT_COPY */; view_bb_w = p[0]; view_bb_h = p[1]; } }
static void win_fix_window(PV hw) {
  i32 r[4]; i32 w, h, x, y;
  if (!win_force || !hw) return;
  r[0] = 0; r[1] = 0; r[2] = (i32)win_w; r[3] = (i32)win_h; AdjustWindowRect(r, 0x00CF0000u /* WS_OVERLAPPEDWINDOW */, 0);
  w = r[2] - r[0]; h = r[3] - r[1];
  x = win_x >= 0 ? win_x : (GetSystemMetrics(0) - w) / 2; y = win_y >= 0 ? win_y : (GetSystemMetrics(1) - h) / 2; if (x < 0) x = 0; if (y < 0) y = 0;
  SetWindowLongW(hw, -16 /* GWL_STYLE */, (i32)0x10CF0000u /* WS_OVERLAPPEDWINDOW | WS_VISIBLE */);
  SetWindowPos(hw, 0, x, y, w, h, 0x0004u /* NOZORDER */ | 0x0020u /* FRAMECHANGED */ | 0x0040u /* SHOWWINDOW */);
  win_hwnd = (u32)hw; win_fix_n++;
}
K int FlushInstructionCache(PV, const void *, u32);
K PV GetCurrentProcess(void);
#define VA_SUBMODE_SET 0x008463d0u   /* FUN_008463d0(N): split manager +0xb1c = N (the split submode the game wants this frame; 1 = ForcedVertical) */
static u32 view_patched;
static void view_load(void) { u16 fp[460]; PV h; char t[64]; u32 got = 0, i;
  mkpath(fp, L"teracoop_view.txt"); if (GetFileAttributesW(fp) == 0xFFFFFFFFu) return;
  h = CreateFileW(fp, 0x80000000u, 7, 0, 3, 0x80, 0); if (h && h != (PV)-1) { ReadFile(h, t, 63, &got, 0); CloseHandle(h); }
  for (i = 0; i + 3 < got; i++) if ((t[i] | 32) == 'l' && (t[i+1] | 32) == 'e' && (t[i+2] | 32) == 'f' && (t[i+3] | 32) == 't') view_on = 1;
  for (i = 0; i + 3 < got; i++) if ((t[i] | 32) == 'w' && (t[i+1] | 32) == 'i' && (t[i+2] | 32) == 'd' && (t[i+3] | 32) == 'e') view_wide = 1;
  for (i = 0; i + 3 < got; i++) if ((t[i] | 32) == 'f' && (t[i+1] | 32) == 'u' && (t[i+2] | 32) == 'l' && (t[i+3] | 32) == 'l') { view_on = 1; view_full = 1; }
  view_crop = view_on && !view_full; }
static void view_patch(void) {   /* mov eax,[esp+4]; mov [ecx+0xb1c],eax; ret 4  ->  mov dword [ecx+0xb1c],1; ret 4 (same 13 bytes) */
  static const u8 want[13] = { 0x8B, 0x44, 0x24, 0x04, 0x89, 0x81, 0x1C, 0x0B, 0x00, 0x00, 0xC2, 0x04, 0x00 }, put[13] = { 0xC7, 0x81, 0x1C, 0x0B, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC2, 0x04, 0x00 };
  u8 *f = (u8 *)(base - IMAGEBASE + VA_SUBMODE_SET); u32 old, i;
  if (!view_on) return;
  for (i = 0; i < 13; i++) if (f[i] != want[i]) { lbegin(); bs("view: split submode NOT patched (unexpected bytes at "); bh((u32)f); bs(")"); lend(); return; }
  if (!VirtualProtect(f, 13, 0x40, &old)) { say("view: split submode NOT patched (VirtualProtect)"); return; }
  for (i = 0; i < 13; i++) f[i] = put[i];
  VirtualProtect(f, 13, old, &old); FlushInstructionCache(GetCurrentProcess(), f, 13); view_patched = 1;
  lbegin(); bs("view: FUN_008463d0 at "); bh((u32)f); bs(" patched - the split submode is always 1 (ForcedVertical, player 1 left)"); lend(); }
extern u32 tc_split_arg; extern volatile u32 tc_split_n;
#ifndef VA_CUR_LEVEL
#define VA_CUR_LEVEL 0x015bcc2cu
#endif
/* v2.12b "full": in the ForcedVertical submode FUN_008462a0(left, &p0, &p1) gives each player's place on the screen
   (x 0.25 / 0.75, y 0.5; the split line lies between them). The module makes it p0 = (0.5, 0.5) and p1 = (10, 0.5): the line moves far
   off the right edge, so player 1's part is the whole window (not squeezed), and the game still counts the screen as split. */
#define VA_VSPLIT_POS 0x008462a0u
static void view_full_patch(void) {
  static const u8 want[8] = { 0x66, 0x83, 0x7C, 0x24, 0x04, 0x00, 0x8B, 0x44 };
  static const u8 put[35] = { 0x8B, 0x44, 0x24, 0x08, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xC7, 0x40, 0x04, 0x00, 0x00, 0x00, 0x3F,
                              0x8B, 0x44, 0x24, 0x0C, 0xC7, 0x00, 0x00, 0x00, 0x20, 0x41, 0xC7, 0x40, 0x04, 0x00, 0x00, 0x00, 0x3F, 0xC3 };
  u8 *f = (u8 *)(base - IMAGEBASE + VA_VSPLIT_POS); u32 old, i;
  if (!view_full) return;
  for (i = 0; i < 8; i++) if (f[i] != want[i]) { lbegin(); bs("view: split position NOT patched (unexpected bytes at "); bh((u32)f); bs(")"); lend(); return; }
  if (!VirtualProtect(f, 35, 0x40, &old)) { say("view: split position NOT patched (VirtualProtect)"); return; }
  for (i = 0; i < 35; i++) f[i] = put[i];
  VirtualProtect(f, 35, old, &old); FlushInstructionCache(GetCurrentProcess(), f, 35);
  lbegin(); bs("view: FUN_008462a0 at "); bh((u32)f); bs(" patched - player places 0.5 / 10.0: player 1's part of the split is the whole window"); lend(); }
/* v2.12d: x32dbg + decompilation (2026-10-31): the game renders each split view in FUN_005d67c0, which first calls
   FUN_00620c10(points, points2, count) with the view's screen polygon (vec4 points in -1..1, view+0xd80 / +0xf80, count view+0xd70).
   FUN_00620c10 makes the clip rectangle (DAT_01567ce0..cec, flag DAT_01568204) from it; count < 3 = no clipping (whole screen).
   In "full" mode: player 1's view (polygon on the left, x < 0) gets count 0 = whole window; player 2's view gets a tiny polygon in the
   bottom-right corner, so it draws (almost) nothing. */
#define VA_VIEW_POLY 0x00620c10u
u32 tc_poly_back; static volatile u32 view_poly_p1, view_poly_p2, view_poly_other;
static __attribute__((aligned(16))) float view_tiny[12] = { 0.998f, -0.998f, 0.0f, 1.0f,  1.0f, -0.998f, 0.0f, 1.0f,  1.0f, -1.0f, 0.0f, 1.0f };
static __attribute__((aligned(16))) float view_tiny2[12] = { 0.998f, -0.998f, 0.0f, 1.0f,  1.0f, -0.998f, 0.0f, 1.0f,  1.0f, -1.0f, 0.0f, 1.0f };
void __cdecl tc_on_poly(u32 *fr) { u32 n = fr[3], i; float sx = 0.0f; const float *pt = (const float *)fr[1];
  if (!view_full || !view_split_now || n < 3 || n > 32 || !pt) { view_poly_other++; return; }
  { float mn = 9.0f, mx = -9.0f; for (i = 0; i < n; i++) { float x = pt[i * 4]; sx += x; if (x < mn) mn = x; if (x > mx) mx = x; }
    if (mx <= 0.02f && mn < -0.5f) { fr[3] = 0; view_poly_p1++; }                                               /* left half = player 1 */
    else if (mn >= -0.02f && mx > 0.5f) { fr[1] = (u32)view_tiny; fr[2] = (u32)view_tiny2; fr[3] = 3; view_poly_p2++; } /* right half = player 2 */
    else view_poly_other++; } (void)sx; }
__attribute__((naked)) void tc_stub_poly(void) {
  __asm__(
    "pushfl\n\t" "pushal\n\t"
    "leal 36(%esp), %eax\n\t" "pushl %eax\n\t"
    "call _tc_on_poly\n\t" "addl $4, %esp\n\t"
    "popal\n\t" "popfl\n\t"
    "pushl %ebp\n\t" "movl %esp, %ebp\n\t" "andl $-16, %esp\n\t"
    "jmp *_tc_poly_back\n\t");
}
static void view_poly_install(void) {
  u8 *f = (u8 *)(base - IMAGEBASE + VA_VIEW_POLY); u32 old; static const u8 want[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x81, 0xEC, 0x64, 0x05 }; u32 i;
  if (!view_full) return;
  for (i = 0; i < sizeof want; i++) if (f[i] != want[i]) { lbegin(); bs("view: polygon hook NOT installed (unexpected bytes at "); bh((u32)f); bs(")"); lend(); return; }
  tc_poly_back = (u32)f + 6;
  if (!VirtualProtect(f, 6, 0x40, &old)) { say("view: polygon hook NOT installed (VirtualProtect)"); return; }
  { u32 rel = (u32)tc_stub_poly - ((u32)f + 5); f[1] = (u8)rel; f[2] = (u8)(rel >> 8); f[3] = (u8)(rel >> 16); f[4] = (u8)(rel >> 24); f[5] = 0x90; f[0] = 0xE9; }
  VirtualProtect(f, 6, old, &old); FlushInstructionCache(GetCurrentProcess(), f, 6);
  lbegin(); bs("view: polygon hook installed at "); bh((u32)f); bs(" (FUN_00620c10: player 1's view = whole window, player 2's = tiny corner)"); lend(); }
static void view_tick(u32 fg) { static u32 p6, sig, at; u32 k6 = (GetAsyncKeyState(0x75) & 0x8000) != 0, now = GetTickCount();
  if (!view_on) return;
  { static u32 was; u32 sp = tc_split_n && tc_split_arg == 0 && *(volatile const u32 *)(base - IMAGEBASE + VA_P2LIST) != 0 && rd32(base - IMAGEBASE + VA_CUR_LEVEL) != 0;
    view_split_now = sp; if (sp != was) { was = sp; lbegin(); bs(sp ? "view: screen is split (player 2 here) - showing the left half" : "view: no split / no player 2 - whole picture"); lend(); } }
  if (k6 && !p6 && fg) { view_crop = !view_crop; lbegin(); bs("view: F6 - left half only "); bs(view_crop ? "ON" : "OFF (whole picture)"); lend(); }
  p6 = k6;
  if ((i32)(now - at) >= 0) { u32 sg = view_pres_fail * 7 + (view_pres_n ? 1 : 0) + (view_poly_p1 ? 2 : 0) + (view_poly_p2 ? 4 : 0); at = now + 3000u;
    if (sg != sig) { sig = sg; lbegin(); bs("view: picture "); bd(view_bb_w); bs("x"); bd(view_bb_h); bs(", presents "); bd(view_pres_n); bs(", failed with the half rectangle "); bd(view_pres_fail); bs(", views: player 1 whole "); bd(view_poly_p1); bs(" player 2 tiny "); bd(view_poly_p2); bs(" other "); bd(view_poly_other); if (view_pres_fail) { bs(" (last hresult "); bh((u32)view_last_hr); bs(")"); } lend(); } } }
/* v2.13 "own screen" (file teracoop_view.txt with the word own; use together with teracoop_split.txt = off, one screen):
   found 2026-10-31 with x32dbg + the decompilation, both checked in the game by the owner:
   1) the one-screen camera FUN_00b85440 frames player 2 too: at VA 0x00b86081 `mov edi,[0x015ca000]` (player 2 list) -> `xor edi,edi`:
      the camera follows only this copy's player 1;
   2) the teleport step FUN_0092c390 waits FUN_009775d0(camera of the player) == 1, which never comes in one screen with two players
      (FUN_009775d0 is called only there, FUN_0084f860 only from it): FUN_009775d0 -> `mov al,1; ret` - the transition does not wait. */
#define VA_OWN_CAM  0x00b86081u
#define VA_OWN_TELE 0x009775d0u
static u32 own_on;
static void own_load(void) { u16 fp[460]; PV h; char t[64]; u32 got = 0, i;
  mkpath(fp, L"teracoop_view.txt"); if (GetFileAttributesW(fp) == 0xFFFFFFFFu) return;
  h = CreateFileW(fp, 0x80000000u, 7, 0, 3, 0x80, 0); if (h && h != (PV)-1) { ReadFile(h, t, 63, &got, 0); CloseHandle(h); }
  for (i = 0; i + 2 < got; i++) if ((t[i] | 32) == 'o' && (t[i+1] | 32) == 'w' && (t[i+2] | 32) == 'n') own_on = 1; }
static int own_patch1(u32 va, const u8 *want, u32 nw, const u8 *put, u32 np, const char *what) { u8 *f = (u8 *)(base - IMAGEBASE + va); u32 old, i;
  for (i = 0; i < nw; i++) if (f[i] != want[i]) { lbegin(); bs("own screen: "); bs(what); bs(" NOT patched (unexpected bytes at "); bh((u32)f); bs(")"); lend(); return 0; }
  if (!VirtualProtect(f, np, 0x40, &old)) { lbegin(); bs("own screen: "); bs(what); bs(" NOT patched (VirtualProtect)"); lend(); return 0; }
  for (i = 0; i < np; i++) f[i] = put[i];
  VirtualProtect(f, np, old, &old); FlushInstructionCache(GetCurrentProcess(), f, np);
  lbegin(); bs("own screen: "); bs(what); bs(" patched at "); bh((u32)f); lend(); return 1; }
static void own_patch(void) {
  static const u8 cam_w[6] = { 0x8B, 0x3D, 0x00, 0xA0, 0x5C, 0x01 }, cam_p[6] = { 0x31, 0xFF, 0x90, 0x90, 0x90, 0x90 };
  static const u8 tel_w[6] = { 0x8B, 0x81, 0x30, 0x07, 0x00, 0x00 }, tel_p[3] = { 0xB0, 0x01, 0xC3 };
  if (!own_on) return;
  own_patch1(VA_OWN_CAM, cam_w, 6, cam_p, 6, "camera follows only player 1 (FUN_00b85440)");
  own_patch1(VA_OWN_TELE, tel_w, 6, tel_p, 3, "transition does not wait for the camera (FUN_009775d0)"); }
static void win_load(void) {
  u16 fp[460]; PV h; char t[64]; u32 got = 0; i32 v[4] = { 0, 0, -1, -1 }; int n = 0, i = 0, neg = 0, in = 0;
  mkpath(fp, L"teracoop_windowed.txt"); if (GetFileAttributesW(fp) == 0xFFFFFFFFu) return;
  win_force = 1;
  h = CreateFileW(fp, 0x80000000u, 7, 0, 3, 0x80, 0);
  if (h && h != (PV)-1) { ReadFile(h, t, 63, &got, 0); CloseHandle(h); }
  for (i = 0; i < (int)got && n < 4; i++) { char c = t[i];
    if (c == '-' && !in) { neg = 1; continue; }
    if (c >= '0' && c <= '9') { if (!in) { in = 1; v[n] = 0; } v[n] = v[n] * 10 + (c - '0'); }
    else if (in) { if (neg) v[n] = -v[n]; n++; in = 0; neg = 0; } }
  if (in && n < 4) { if (neg) v[n] = -v[n]; n++; }
  if (n >= 2 && v[0] >= 320 && v[0] <= 7680 && v[1] >= 200 && v[1] <= 4320) { win_w = (u32)v[0]; win_h = (u32)v[1]; }
  if (n >= 4) { win_x = v[2]; win_y = v[3]; }
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
  win_fix_pp(pp); if (win_force) win_reset_n++;
  hr = ((F_reset)ov_orig_reset_r)(dev, pp); ov_reset_hr = (u32)hr; ov_reset_seq++;
  return hr;
}
static void *view_orig_present;
/* v2.12c: x32dbg (2026-10-31): while split, the game draws every half with the FULL viewport 1280x800 and cuts it with
   SetScissorRect (caller 0x0066b265): left half (0,0,640,800), right half (640,0,1280,800). In "full" mode the module makes the left
   (player 1) cut the whole window and the right (player 2) cut empty, so player 1's picture fills the window, not squeezed. */
static void *view_orig_scissor;
static long __stdcall hk_scissor_v(PV dev, const i32 *r) {
  if (view_full && view_split_now && r && win_w && win_h && r[1] == 0 && r[3] == (i32)win_h) { i32 h = (i32)(win_w / 2u), t[4];
    if (r[0] == 0 && r[2] == h) { t[0] = 0; t[1] = 0; t[2] = (i32)win_w; t[3] = (i32)win_h; view_sc_left++; return ((long (__stdcall *)(PV, const i32 *))view_orig_scissor)(dev, t); }
    if (r[0] == h && r[2] == (i32)win_w) { t[0] = 0; t[1] = 0; t[2] = 0; t[3] = 0; view_sc_right++; return ((long (__stdcall *)(PV, const i32 *))view_orig_scissor)(dev, t); } }
  return ((long (__stdcall *)(PV, const i32 *))view_orig_scissor)(dev, r); }
static long __stdcall hk_present_v(PV dev, const void *a, const void *b, PV w, const void *d) {
  long hr; view_pres_n++;
  if (view_crop && view_split_now && view_bb_w && view_bb_h && !a) { i32 src[4]; src[0] = 0; src[1] = 0; src[2] = (i32)(view_bb_w / 2u); src[3] = (i32)view_bb_h;
    hr = ((F_present)view_orig_present)(dev, src, 0, w, d); if (hr < 0) { view_pres_fail++; view_last_hr = hr; hr = ((F_present)view_orig_present)(dev, a, b, w, d); } return hr; }
  return ((F_present)view_orig_present)(dev, a, b, w, d); }
static long __stdcall hk_cd(PV d3d, u32 ad, u32 dt, PV hw, u32 fl, void *pp, PV *ppd) {
  long hr; win_fix_pp(pp); if (win_force) win_cd_n++;
  hr = ((long (__stdcall *)(PV, u32, u32, PV, u32, void *, PV *))ov_orig_cd)(d3d, ad, dt, hw, fl, pp, ppd);
  ov_n_cd++; ov_cd_flags = fl; ov_cd_hr = (u32)hr;
  if (hr >= 0 && win_force) win_fix_window(pp && ((u32 *)pp)[7] ? (PV)((u32 *)pp)[7] : hw);
  if (hr >= 0 && ppd && *ppd) { PV sc = 0; ov_dev_ptr = (u32)*ppd; ov_real_vt = (u32)VT(*ppd);
    if ((u32)VT(*ppd) != ov_dummy_vt) { void **rv = VT(*ppd); u32 old; /* the game makes its own table (pure device): patch that one with its own originals */
      if (ov_no_panel) { if (rv[16] != (void *)hk_reset_r && VirtualProtect(&rv[16], 4, 4, &old)) { ov_orig_reset_r = rv[16]; rv[16] = (void *)hk_reset_r; VirtualProtect(&rv[16], 4, old, &old); }
        if (view_on && rv[17] != (void *)hk_present_v && VirtualProtect(&rv[17], 4, 4, &old)) { view_orig_present = rv[17]; rv[17] = (void *)hk_present_v; VirtualProtect(&rv[17], 4, old, &old); }
        if (0 && view_full && rv[75] != (void *)hk_scissor_v && VirtualProtect(&rv[75], 4, 4, &old)) { view_orig_scissor = rv[75]; rv[75] = (void *)hk_scissor_v; VirtualProtect(&rv[75], 4, old, &old); } }
      else if (rv[17] != (void *)hk_present_r && VirtualProtect(&rv[16], 8, 4, &old)) { ov_orig_reset_r = rv[16]; ov_orig_present_r = rv[17]; rv[16] = (void *)hk_reset_r; rv[17] = (void *)hk_present_r; VirtualProtect(&rv[16], 8, old, &old); ov_patched_vt = (u32)rv; } }
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
  win_fix_pp(pp); if (win_force) win_reset_n++;
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
  if (ov_no_panel) { void **dv = VT(d3d); if (VirtualProtect(&dv[16], 4, 4, &old)) { ov_orig_cd = dv[16]; dv[16] = (void *)hk_cd; VirtualProtect(&dv[16], 4, old, &old); }
    ((F_rel)VT(dev)[2])(dev); ((F_rel)VT(d3d)[2])(d3d); DestroyWindow(hw); return 0; }
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

/* ---------------- v2.2: character events of the game (found 2026-10-27, steps 92-94 of the notes) ----------------
 * FUN_0095d300(obj)     = the game broadcasts "game object added"  (called when a character is created / re-added)
 * FUN_0095d380(obj, by) = the game broadcasts "game object killed" (by = killer, 0 = nobody)
 * Both start with "push -1; push imm32; mov eax, fs:[0]". The first 7 bytes are replaced by a jump to a small stub that queues the
 * event and then runs those 7 bytes itself. The queue is read by the worker thread, which writes the log (the game thread never writes files). */
static void lv_str(u32 p);
#define VA_LEVEL_TAB 0x015bcc28u
#define VA_LEVEL_CNT 0x015bcc34u
#define VA_EV_ADDED  0x0095d300u
#define VA_EV_KILLED 0x0095d380u
typedef struct { u32 kind, obj, by, t; float p[3]; u32 hp; } TcEv;
#define TCEV_N 256u
static TcEv tcev[TCEV_N]; static volatile u32 tcev_w, tcev_r, tcev_lost, tcev_lock;   /* w / r only change under the lock */
u32 tc_added_imm, tc_added_back, tc_killed_imm, tc_killed_back;
static void tc_lock(void) { while (__sync_lock_test_and_set(&tcev_lock, 1u)) { } }
static void tc_unlock(void) { __sync_lock_release(&tcev_lock); }
static void tcev_put(u32 kind, u32 obj, u32 by) {
  TcEv e; u8 h[2] = { 0, 0 };
  e.kind = kind; e.obj = obj; e.by = by; e.t = GetTickCount(); e.p[0] = e.p[1] = e.p[2] = 0.0f;
  if (obj && kind <= 2) { rd(obj + 0x190, e.p, 12); rd(obj + 0x706, h, 2); }
  e.hp = ((u32)h[0] << 8) | h[1];   /* max (+0x706), current (+0x707) */
  tc_lock();
  if (tcev_w - tcev_r < TCEV_N) { tcev[tcev_w % TCEV_N] = e; tcev_w++; } else tcev_lost++;
  tc_unlock();
}
void __cdecl tc_on_added(u32 obj) { tcev_put(1, obj, 0); }
void __cdecl tc_on_killed(u32 obj, u32 by) { tcev_put(2, obj, by); }
__attribute__((naked)) void tc_stub_added(void) {
  __asm__(
    "pushfl\n\t" "pushal\n\t"
    "movl 40(%esp), %eax\n\t" "pushl %eax\n\t"
    "call _tc_on_added\n\t" "addl $4, %esp\n\t"
    "popal\n\t" "popfl\n\t"
    "pushl $-1\n\t" "pushl _tc_added_imm\n\t" "jmp *_tc_added_back\n\t");
}
__attribute__((naked)) void tc_stub_killed(void) {
  __asm__(
    "pushfl\n\t" "pushal\n\t"
    "movl 44(%esp), %eax\n\t" "pushl %eax\n\t"
    "movl 44(%esp), %eax\n\t" "pushl %eax\n\t"
    "call _tc_on_killed\n\t" "addl $8, %esp\n\t"
    "popal\n\t" "popfl\n\t"
    "pushl $-1\n\t" "pushl _tc_killed_imm\n\t" "jmp *_tc_killed_back\n\t");
}
/* v2.2g: every level change of the story goes through FUN_00b169d0(index, flag) (thiscall, this = 0x013B7B6C); same entry bytes as above */
#define VA_EV_GOTO 0x00b169d0u
u32 tc_goto_imm, tc_goto_back; static volatile u32 tc_goto_mine;
static u32 loc_role;
static volatile u32 lg_idx = 0xFFFFFFFFu, lg_flag, lg_t, lg_n, lg_any_t, lg_any_n, lg_any_idx = 0xFFFFFFFFu, lg_prev_idx = 0xFFFFFFFFu, lg_dur;   /* last level request made by the game itself (not by the module) */
void __cdecl tc_on_goto(u32 idx, u32 flag) { u32 t = GetTickCount(); lg_any_t = t; lg_any_n++; lg_any_idx = idx; if (!tc_goto_mine || loc_role == 1) { lg_prev_idx = lg_idx; lg_dur = lg_n ? t - lg_t : 0; lg_idx = idx; lg_flag = flag; lg_t = GetTickCount(); lg_n++; } tcev_put(tc_goto_mine ? 4 : 3, idx, flag); }
__attribute__((naked)) void tc_stub_goto(void) {
  __asm__(
    "pushfl\n\t" "pushal\n\t"
    "movl 44(%esp), %eax\n\t" "pushl %eax\n\t"
    "movl 44(%esp), %eax\n\t" "pushl %eax\n\t"
    "call _tc_on_goto\n\t" "addl $8, %esp\n\t"
    "popal\n\t" "popfl\n\t"
    "pushl $-1\n\t" "pushl _tc_goto_imm\n\t" "jmp *_tc_goto_back\n\t");
}
/* v2.7: the split-screen manager. FUN_0085b9a0(mode) (thiscall, this = [game object + 0x68], object class cNewSplitScreenManager, current mode at this+0x13f8)
 * is called at every level start by FUN_009d6120 (mode 0 or 1). Its first bytes 56 57 8B 7C 24 0C (push esi; push edi; mov edi,[esp+0xC]) are
 * replaced by a jump to a stub that remembers this + mode and then runs those 6 bytes itself. */
#define VA_SPLIT_FN 0x0085b9a0u
u32 tc_split_mgr, tc_split_arg, tc_split_back; volatile u32 tc_split_n, tc_split_mine;
__attribute__((naked)) void tc_stub_split(void) {
  __asm__(
    "pushl %eax\n\t"
    "movl %ecx, _tc_split_mgr\n\t"
    "movl 8(%esp), %eax\n\t" "movl %eax, _tc_split_arg\n\t"
    "incl _tc_split_n\n\t"
    "popl %eax\n\t"
    "pushl %esi\n\t" "pushl %edi\n\t" "movl 12(%esp), %edi\n\t"
    "jmp *_tc_split_back\n\t");
}
/* v2.9a: who calls the game's hero change FUN_00bd7ab0(char, new type, old type, a4, a5, a6, a7) (cdecl)? First bytes 55 8B EC 83 E4 F0
   (push ebp; mov ebp,esp; and esp,-16) are replaced by a jump to a stub that queues the arguments, the return address and every
   code address found in the next 96 stack dwords, then runs those 6 bytes itself. */
#define VA_SWAP_HOOK 0x00bd7ab0u
typedef struct { u32 a[8]; u32 st[12]; u32 nst, t, mine; } TcSw;
static TcSw tcsw[16]; static volatile u32 tcsw_w, tcsw_r, tcsw_lost; u32 tc_swap_back; static volatile u32 tc_swap_mine;
void __cdecl tc_on_swap(u32 *fr) { TcSw e; u32 i, v; e.t = GetTickCount(); e.mine = tc_swap_mine; e.nst = 0;
  for (i = 0; i < 8; i++) e.a[i] = fr[i];   /* a[0] = return address, a[1..7] = arguments */
  for (i = 1; i < 96 && e.nst < 12; i++) { v = fr[8 + i - 1]; if (v >= base + 0x1000u && v < base + 0x949000u) e.st[e.nst++] = v; }
  tc_lock(); if (tcsw_w - tcsw_r < 16u) { tcsw[tcsw_w % 16u] = e; tcsw_w++; } else tcsw_lost++; tc_unlock(); }
__attribute__((naked)) void tc_stub_swap(void) {
  __asm__(
    "pushfl\n\t" "pushal\n\t"
    "leal 36(%esp), %eax\n\t" "pushl %eax\n\t"
    "call _tc_on_swap\n\t" "addl $4, %esp\n\t"
    "popal\n\t" "popfl\n\t"
    "pushl %ebp\n\t" "movl %esp, %ebp\n\t" "andl $-16, %esp\n\t"
    "jmp *_tc_swap_back\n\t");
}
K int FlushInstructionCache(PV, const void *, u32);
K PV GetCurrentProcess(void);
static int tc_hook(u32 va, void (*stub)(void), u32 *imm, u32 *back, const char *what) {
  u8 *f = (u8 *)(base - IMAGEBASE + va); u32 old; int ok;
  ok = f[0] == 0x6a && f[1] == 0xff && f[2] == 0x68 && f[7] == 0x64 && f[8] == 0xa1 && f[9] == 0 && f[10] == 0 && f[11] == 0 && f[12] == 0;
  lbegin(); bs("events: hook "); bs(what); bs(" at "); bh((u32)f);
  if (!ok) { bs(" NOT installed (unexpected bytes "); bh(*(u32 *)f); bs(")"); lend(); return 0; }
  *imm = *(u32 *)(f + 3); *back = (u32)f + 7;
  if (!VirtualProtect(f, 7, 0x40 /* PAGE_EXECUTE_READWRITE */, &old)) { bs(" NOT installed (VirtualProtect failed)"); lend(); return 0; }
  { u32 rel = (u32)stub - ((u32)f + 5); f[1] = (u8)rel; f[2] = (u8)(rel >> 8); f[3] = (u8)(rel >> 16); f[4] = (u8)(rel >> 24); f[5] = 0x90; f[6] = 0x90; f[0] = 0xE9; }
  VirtualProtect(f, 7, old, &old); FlushInstructionCache(GetCurrentProcess(), f, 7);
  bs(" installed"); lend(); return 1;
}
static int swap_install(void) {
  u8 *f = (u8 *)(base - IMAGEBASE + VA_SWAP_HOOK); u32 old; static const u8 want[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0, 0x6A, 0xFF }; u32 i;
  for (i = 0; i < sizeof want; i++) if (f[i] != want[i]) { lbegin(); bs("hero: swap hook NOT installed (unexpected bytes at "); bh((u32)f); bs(")"); lend(); return 0; }
  tc_swap_back = (u32)f + 6;
  if (!VirtualProtect(f, 6, 0x40, &old)) { say("hero: swap hook NOT installed (VirtualProtect)"); return 0; }
  { u32 rel = (u32)tc_stub_swap - ((u32)f + 5); f[1] = (u8)rel; f[2] = (u8)(rel >> 8); f[3] = (u8)(rel >> 16); f[4] = (u8)(rel >> 24); f[5] = 0x90; f[0] = 0xE9; }
  VirtualProtect(f, 6, old, &old); FlushInstructionCache(GetCurrentProcess(), f, 6);
  lbegin(); bs("hero: swap hook installed at "); bh((u32)f); bs(" (FUN_00bd7ab0, every hero change of the game is logged as SWAP)"); lend(); return 1;
}
static void swap_drain(void) { TcSw got[16]; u32 n = 0, i, k;
  tc_lock(); while (n < 16 && tcsw_r != tcsw_w) { got[n++] = tcsw[tcsw_r % 16u]; tcsw_r++; } tc_unlock();
  for (i = 0; i < n; i++) { TcSw *e = &got[i];
    lbegin(); bs("SWAP "); bs(e->mine ? "(module F8) " : "(game) "); bs("char "); bh(e->a[1]); bs(" type "); bi((i32)e->a[3]); bs(" -> "); bi((i32)e->a[2]);
    bs(" args "); bd(e->a[4]); bs(" "); bd(e->a[5]); bs(" "); bd(e->a[6]); bs(" "); bd(e->a[7]); bs(" | called from "); bh(e->a[0] - base + IMAGEBASE); lend();
    lbegin(); bs("  stack code addresses:"); for (k = 0; k < e->nst; k++) { bs(" "); bh(e->st[k] - base + IMAGEBASE); } lend(); }
}
static int split_install(void) {
  u8 *f = (u8 *)(base - IMAGEBASE + VA_SPLIT_FN); u32 old; static const u8 want[] = { 0x56, 0x57, 0x8B, 0x7C, 0x24, 0x0C, 0x8B, 0xF1, 0x39, 0xBE, 0xF8, 0x13 }; u32 i;
  for (i = 0; i < sizeof want; i++) if (f[i] != want[i]) { lbegin(); bs("split: hook NOT installed (unexpected bytes at "); bh((u32)f); bs(")"); lend(); return 0; }
  tc_split_back = (u32)f + 6;
  if (!VirtualProtect(f, 6, 0x40, &old)) { say("split: hook NOT installed (VirtualProtect)"); return 0; }
  { u32 rel = (u32)tc_stub_split - ((u32)f + 5); f[1] = (u8)rel; f[2] = (u8)(rel >> 8); f[3] = (u8)(rel >> 16); f[4] = (u8)(rel >> 24); f[5] = 0x90; f[0] = 0xE9; }
  VirtualProtect(f, 6, old, &old); FlushInstructionCache(GetCurrentProcess(), f, 6);
  lbegin(); bs("split: hook installed at "); bh((u32)f); bs(" - F7 = one screen for every level on/off, teracoop_split.txt with the word off = on from the start"); lend(); return 1;
}
static void tc_who(u32 a) { u32 i; if (!a) { bs("nobody"); return; } bh(a); for (i = 0; i < 4; i++) if (rd32(base - IMAGEBASE + VA_PTABLE + i * 4) == a) { bs(" (player table ["); bd(i); bs("]"); bs(i == 0 ? " = this PC's hero)" : ")"); return; } }
static void tcev_drain(void) {
  static TcEv got[64]; static u32 lost_seen; u32 n = 0, i, lost;
  tc_lock(); while (n < 64 && tcev_r != tcev_w) { got[n++] = tcev[tcev_r % TCEV_N]; tcev_r++; } lost = tcev_lost; tc_unlock();
  for (i = 0; i < n; i++) { TcEv *e = &got[i];
    if (e->kind >= 3) { u32 tab = rd32(base - IMAGEBASE + VA_LEVEL_TAB), cnt = rd32(base - IMAGEBASE + VA_LEVEL_CNT);
      lbegin(); bs(e->kind == 4 ? "GOTO request (module F5) index " : "GOTO request (game) index "); bd(e->obj); bs(" ");
      if (e->obj < cnt) lv_str(rd32(tab + e->obj * 0xE0u + 0x3c)); else bs("(out of table)"); bs(" flag "); bd(e->by); lend(); continue; }
    lbegin(); bs(e->kind == 1 ? "ADDED  obj " : "KILLED obj "); tc_who(e->obj);
    if (e->kind == 2) { bs(" by "); tc_who(e->by); }
    bs(" pos mm "); bi(f2mm(e->p[0])); bs(" "); bi(f2mm(e->p[1])); bs(" "); bi(f2mm(e->p[2]));
    bs(" hearts "); bd(e->hp & 0xFFu); bs("/"); bd(e->hp >> 8); lend(); }
  if (lost != lost_seen) { lost_seen = lost; lbegin(); bs("events: queue full, lost "); bd(lost); lend(); }
}

/* ---------------- v2.2d: current level (read only) ----------------
 * FUN_00b8aa20 (runs when a level starts) keeps the previous level in 0x015bc178 and puts the new one in DAT_015bcc2c = level instance + 0x10.
 * That is an entry of the level table DAT_015bcc28 (0xE0 bytes each, count DAT_015bcc34, loaded from levels\levels.txt):
 * +0x38 dir (char *), +0x3c name (char *), +0x58 index (short). FUN_00b169d0(index) asks the game to go to a level by index. */
#define VA_CUR_LEVEL 0x015bcc2cu
static void lv_str(u32 p) { char t[64]; int i; if (!p || !rd(p, t, 63)) { bs("?"); return; } t[63] = 0; for (i = 0; t[i]; i++) if (t[i] < 32 || t[i] > 126) { bs("?"); return; } bs(t); }
/* v2.2f: the game's own "current level" accessor (used by FUN_00b61c40, FUN_00b8aa20 and others):
 *   entry = DAT_013b7b94 (= level object 0x013b7b6c + 0x28, written by FUN_00b169d0 when a level is redirected) if not 0,
 *   else FUN_009ee6a0(): mgr = DAT_015ae0a0; p = mgr+0xbc; name = *(p + 0x30) (bytes: call 0x814c50; mov ecx,[eax+0xbc]; call 0x814c90 (= ecx+0x24); mov ebx,[eax+0xc]),
 *   then the level table is searched by that name. The module copies this with safe reads (no call into the game). */
#define VA_LEVEL_OVR 0x013b7b94u
#define VA_STREAM_MGR 0x015ae0a0u
static u32 lv_by_name(u32 name, u32 tab, u32 cnt) { char a[64], b[64]; u32 i; int k;
  if (!name || !tab || cnt > 4096 || !rd(name, a, 63)) return 0; a[63] = 0;
  for (i = 0; i < cnt; i++) { u32 e = tab + i * 0xE0u, n = rd32(e + 0x3c); if (!n || !rd(n, b, 63)) continue; b[63] = 0;
    for (k = 0; a[k] && b[k]; k++) { char x = a[k], y = b[k]; if (x >= 'a' && x <= 'z') x -= 32; if (y >= 'a' && y <= 'z') y -= 32; if (x != y) break; }
    if (!a[k] && !b[k]) return e; }
  return 0; }
static void lv_line(const char *what, u32 e, u32 tab, u32 cnt) { u16 idx = 0xFFFF;
  lbegin(); bs(what);
  if (!e) { bs("none"); lend(); return; }
  rd(e + 0x58, &idx, 2); bs("index "); bd(idx); bs(" name "); lv_str(rd32(e + 0x3c)); bs(" dir "); lv_str(rd32(e + 0x38));
  if (tab && e >= tab && (e - tab) % 0xE0u == 0) { bs(" (slot "); bd((e - tab) / 0xE0u); bs(" of "); bd(cnt); bs(")"); } else bs(" (not inside the level table!)");
  lend(); }
static u32 hm_reset_seq;
static void level_tick(void) {
  static u32 prev = 0xFFFFFFFFu, prev2 = 0xFFFFFFFFu, at; u32 e = rd32(base - IMAGEBASE + VA_CUR_LEVEL), tab, cnt, e2, ovr, mgr, p, nm; u32 now = GetTickCount();
  tab = rd32(base - IMAGEBASE + VA_LEVEL_TAB); cnt = rd32(base - IMAGEBASE + VA_LEVEL_CNT);
  if (e != prev) { prev = e; hm_reset_seq++; lv_line("LEVEL (old field 0x015bcc2c) now ", e, tab, cnt); }
  if ((i32)(now - at) < 0) return; at = now + 100u;   /* the name search is done 10 times a second */
  ovr = rd32(base - IMAGEBASE + VA_LEVEL_OVR); e2 = ovr;
  if (!e2) { mgr = rd32(base - IMAGEBASE + VA_STREAM_MGR); p = mgr ? rd32(mgr + 0xbc) : 0; nm = p ? rd32(p + 0x30) : 0; e2 = lv_by_name(nm, tab, cnt); }
  if (e2 != prev2) { prev2 = e2; lv_line(ovr ? "CURLEVEL (game accessor, override 0x013b7b94) now " : "CURLEVEL (game accessor) now ", e2, tab, cnt); }
}

/* ---------------- v2.2e: go to level N (test: F5 + teracoop_goto.txt) ----------------
 * The game changes level with FUN_00b169d0, a thiscall on the static object 0x013B7B6C with (index, 0) - seen in the bytes of FUN_00b5bc30
 * ("push 0; push index; mov ecx, 0x013B7B6C; call 0x00B169D0"); x32dbg showed every level change of the story going through it.
 * It must run on the game's main thread, so the module replaces the PeekMessageA entry of the game's import table (VA 0x00D492B0):
 * the message pump FUN_00670990 runs between frames on the main thread, and there a waiting request is carried out. */
#define VA_GOTO_FN   0x00b169d0u
#define VA_GOTO_THIS 0x013b7b6cu
#define VA_IAT_PEEK  0x00d492b0u
typedef int (__stdcall *F_peek)(PV, PV, u32, u32, u32);
typedef u32 (__thiscall *F_goto)(PV, i32, i32);
static F_peek goto_orig_peek; static volatile i32 goto_req = -1, goto_flag = 0, goto_done_idx = -1; static volatile u32 goto_done_ret, goto_seq, goto_skipped;
static volatile u32 kbi_extra_dik, kbi_extra_until; static volatile u32 kbi_on, kbi_mask, kbi_f2_until, kbi_st_calls, kbi_dt_calls, kbi_dt_events, kbi_writes, kbi_fixed, kbi_acq_calls, kbi_acq_fail;
static volatile i32 split_req = -1, split_want = -1; static volatile u32 split_off_on; static volatile u32 split_done_seq;
#define VA_WIN_ACTIVE 0x01352838u   /* the game's "my window is active" flag (WndProc: WM_ACTIVATE / WM_ACTIVATEAPP); FUN_0066ec90 ignores every key while it is 0 */
/* v2.9: change player 1's hero by name, the way the game's own script command "SwapCharacterType" (FUN_00515fc0) does it:
   type = FUN_009d9a40(name) (cdecl, -1 = unknown), old = FUN_0087ad10() (thiscall, ECX = character), FUN_00bd7ab0(char, type, old, 1, 0, 0, 0) (cdecl). */
#define VA_TYPE_BY_NAME 0x009d9a40u
#define VA_CUR_TYPE     0x0087ad10u
#define VA_SWAP_TYPE    0x00bd7ab0u
static volatile u32 hero_req, hero_req_slot, hero_seq, hero_res_ch, hero_res_slot; static volatile i32 hero_res_type = -9, hero_res_old = -9; static volatile u32 hero_res_ret; static char hero_want[48];
static u32 pp_char(u32 slot);
/* v2.9b: the swap alone does nothing while the new hero's model is not loaded (test v2.9). The game itself (FUN_0092fe90) makes a
   CharLoaderUtilSwap object (0x24 bytes, constructor FUN_00a52b50, vtable 0x0112ba48), asks vt[1] = FUN_00bf27b0(player, type) to load
   the hero, then every frame FUN_0098b5a0 (calls vt[2] = FUN_00bf2920 for players 0 and 1: swaps when loaded) until FUN_0098b5c0 says done
   (+4 and +8 both -1). The module does the same with its own object. */
#define VA_CLU_CTOR   0x00a52b50u
#define VA_CLU_TICK   0x0098b5a0u
#define VA_CLU_VT     0x0112ba48u
#define VA_CLU_ASK    0x00bf27b0u
static u32 hsw[16]; static u32 hsw_made, hsw_busy, hsw_t0; static volatile u32 hsw_done_seq, hsw_done_ms, hsw_timeout, hsw_frames;
static void hero_do(void) { u32 slot = hero_req_slot & 1u, ch = pp_char(slot); i32 t, o;
  hero_res_slot = slot; hero_res_ch = ch; hero_res_type = -9; hero_res_old = -9; hero_res_ret = 0;
  if (!ch) return;
  t = ((i32 (__cdecl *)(const char *))(base - IMAGEBASE + VA_TYPE_BY_NAME))(hero_want); hero_res_type = t; if (t < 0) return;
  o = (i32)(short)((u32 (__thiscall *)(PV))(base - IMAGEBASE + VA_CUR_TYPE))((PV)ch); hero_res_old = o;
  if (!hsw_made) { ((void (__thiscall *)(PV))(base - IMAGEBASE + VA_CLU_CTOR))((PV)hsw); hsw_made = 1; }
  if (hsw[0] != base - IMAGEBASE + VA_CLU_VT || rd32(hsw[0] + 4) != base - IMAGEBASE + VA_CLU_ASK) { hero_res_ret = 0xBAD0u; return; }
  ((void (__thiscall *)(PV, i32, i32))rd32(hsw[0] + 4))((PV)hsw, (i32)slot, t);
  hero_res_ret = 1; hsw_busy = 1; hsw_t0 = GetTickCount(); hsw_frames = 0; }
static void hero_loader_tick(void) {
  static u32 last; u32 now = GetTickCount();
  if (!hsw_busy || now - last < 15u) return; last = now;
  hsw_frames++; tc_swap_mine = 1; ((void (__thiscall *)(PV))(base - IMAGEBASE + VA_CLU_TICK))((PV)hsw); tc_swap_mine = 0;
  if (hsw[1] == 0xFFFFFFFFu && hsw[2] == 0xFFFFFFFFu) { hsw_busy = 0; hsw_timeout = 0; hsw_done_ms = GetTickCount() - hsw_t0; hsw_done_seq++; }
  else if (GetTickCount() - hsw_t0 > 15000u) { hsw_busy = 0; hsw_timeout = 1; hsw_done_ms = GetTickCount() - hsw_t0; hsw_done_seq++; } }
static int __stdcall hk_peek(PV msg, PV hw, u32 a, u32 b, u32 rm) {
  { static u32 pl; u32 n = GetTickCount(); if (n - pl >= 10u) { pl = n; pp_in_game_thread = 1; puppet_frame(); } }
  hero_loader_tick();
  if (hero_req && !hsw_busy) { hero_req = 0; if (*(volatile u32 *)(base - IMAGEBASE + VA_CUR_LEVEL)) hero_do(); else hero_res_ch = 0xFFFFFFFFu; hero_seq++; }
  if (kbi_on) *(volatile u32 *)(base - IMAGEBASE + VA_WIN_ACTIVE) = 1;
  if (split_req >= 0 && tc_split_mgr) { i32 m = split_req; split_req = -1; tc_split_mine = 1; ((void (__thiscall *)(PV, i32))(base - IMAGEBASE + VA_SPLIT_FN))((PV)tc_split_mgr, m); tc_split_mine = 0; split_done_seq++; }   /* v2.5a, host of the one-PC test only: keep reading keys while the guest window is the active one */
  if (goto_req >= 0) {
    i32 n = goto_req; u32 cur = *(volatile u32 *)(base - IMAGEBASE + VA_CUR_LEVEL);
    if (!cur) goto_skipped++;   /* no level running (menu / loading): keep waiting */
    else { goto_req = -1; tc_goto_mine = 1; goto_done_ret = ((F_goto)(base - IMAGEBASE + VA_GOTO_FN))((PV)(base - IMAGEBASE + VA_GOTO_THIS), n, goto_flag); tc_goto_mine = 0; goto_done_idx = n; goto_seq++; }
  }
  return goto_orig_peek(msg, hw, a, b, rm);
}
static void goto_install(void) {
  u32 *slot = (u32 *)(base - IMAGEBASE + VA_IAT_PEEK), old; u8 *f = (u8 *)(base - IMAGEBASE + VA_GOTO_FN);
  lbegin(); bs("goto: ");
  if (!(f[0] == 0x6a && f[1] == 0xff && f[2] == 0x68)) { bs("NOT installed (unexpected bytes at the level function)"); lend(); return; }
  if (!VirtualProtect(slot, 4, 4, &old)) { bs("NOT installed (VirtualProtect)"); lend(); return; }
  goto_orig_peek = (F_peek)*slot; *slot = (u32)hk_peek; VirtualProtect(slot, 4, old, &old);
  bs("PeekMessageA hook installed (original "); bh((u32)goto_orig_peek); bs("), F5 = go to the level number written in teracoop_goto.txt"); lend();
}
static void goto_tick(u32 fg) {
  static u32 f5_prev, seq_seen; u32 f5 = (GetAsyncKeyState(0x74) & 0x8000) != 0;
  if (f5 && !f5_prev && fg && !cen_on && goto_orig_peek) {
    u16 fp[460]; PV h; char t[16]; u32 got = 0; i32 n = -1; int i; u32 tab = rd32(base - IMAGEBASE + VA_LEVEL_TAB), cnt = rd32(base - IMAGEBASE + VA_LEVEL_CNT);
    mkpath(fp, L"teracoop_goto.txt"); h = CreateFileW(fp, 0x80000000u, 7, 0, 3, 0x80, 0);
    if (h && h != (PV)-1) { ReadFile(h, t, 15, &got, 0); CloseHandle(h); }
    for (i = 0; i < (int)got; i++) { if (t[i] >= '0' && t[i] <= '9') { if (n < 0) n = 0; n = n * 10 + (t[i] - '0'); } else if (n >= 0) break; }
    lbegin();
    if (n < 0 || (u32)n >= cnt) { bs("goto: F5 pressed, but teracoop_goto.txt has no valid level number (0.."); bd(cnt ? cnt - 1 : 0); bs(")"); }
    else { bs("goto: F5 pressed, request level "); bd((u32)n); bs(" "); lv_str(rd32(tab + (u32)n * 0xE0u + 0x3c)); goto_flag = 0; goto_req = n; }
    lend();
  }
  f5_prev = f5;
  if (goto_seq != seq_seen) { seq_seen = goto_seq; lbegin(); bs("goto: game thread called the level change for "); bd((u32)goto_done_idx); bs(", result "); bd(goto_done_ret & 0xFFu); bs(" (1 = accepted), waited frames "); bd(goto_skipped); lend(); goto_skipped = 0; }
}

/* ---------------- v2.3: two copies on one PC talk directly over 127.0.0.1 (test without the program and Steam) ----------------
 * teracoop_local.txt in the game folder: "host" or "guest". The host sends every level request of its game (FUN_00b169d0 index + flag) to
 * UDP 127.0.0.1:47802 at once and repeats the last one every second. The guest listens there and, when its own game is quiet
 * (a level is running and its game asked for no level change in the last 8 s), asks its game for the same level exactly like F5.
 * Packet 16 bytes: 'TCLV', sequence number, level index, flag. */
K int __stdcall WSAStartup(u16, void *);
K u32 __stdcall socket(int, int, int);
K int __stdcall bind(u32, const void *, int);
K int __stdcall sendto(u32, const void *, int, int, const void *, int);
K int __stdcall recvfrom(u32, void *, int, int, void *, int *);
K int __stdcall ioctlsocket(u32, long, u32 *);
#define LOC_PORT_GUEST 47802u
#define LOC_MAGIC 0x564C4354u   /* "TCLV" */
#define LOC_PORT_HOST 47801u
#define LOC_MAGIC_P2 0x32504354u /* "TCP2": guest -> host, where the guest's hero is and what it presses (36 bytes) */
static u32 loc_p2_sent_at, loc_p2_seq, loc_p2_rx, loc_p2_log_at, loc_p2_rx_logged;
static u32 loc_pend_prev = 0xFFFFFFFFu, loc_pend_dur, loc_sched_logged; static u32 loc_sock = 0xFFFFFFFFu, loc_sent_n, loc_resend_at, loc_rx_seq, loc_pend_idx = 0xFFFFFFFFu, loc_pend_flag, loc_pend_n, loc_wait_log_at;
static void loc_addr(u8 *a, u32 port) { int i; for (i = 0; i < 16; i++) a[i] = 0; a[0] = 2; a[2] = (u8)(port >> 8); a[3] = (u8)port; a[4] = 127; a[7] = 1; }
static void loc_start(void) {
  u16 fp[460]; PV h; char t[16]; u32 got = 0; u8 wsa[400], ad[16]; u32 nb = 1;
  mkpath(fp, L"teracoop_local.txt"); if (GetFileAttributesW(fp) == 0xFFFFFFFFu) { say("local link: off (create teracoop_local.txt with the word host or guest to link two copies on this PC)"); return; }
  h = CreateFileW(fp, 0x80000000u, 7, 0, 3, 0x80, 0); if (h && h != (PV)-1) { ReadFile(h, t, 15, &got, 0); CloseHandle(h); }
  if (got >= 4 && (t[0] | 32) == 'h') loc_role = 1; else if (got >= 5 && (t[0] | 32) == 'g') loc_role = 2;
  if (!loc_role) { say("local link: teracoop_local.txt must start with host or guest - link off"); return; }
  if (WSAStartup(0x0202, wsa) != 0) { say("local link: WSAStartup failed - link off"); loc_role = 0; return; }
  loc_sock = socket(2 /* AF_INET */, 2 /* SOCK_DGRAM */, 17 /* UDP */);
  if (loc_sock == 0xFFFFFFFFu) { say("local link: socket failed - link off"); loc_role = 0; return; }
  ioctlsocket(loc_sock, (long)0x8004667Eu /* FIONBIO */, &nb);
  if (loc_role == 2) { loc_addr(ad, LOC_PORT_GUEST); if (bind(loc_sock, ad, 16) != 0) { say("local link: GUEST could not take port 47802 (another guest running?) - link off"); loc_role = 0; return; } }
  if (loc_role == 1) { loc_addr(ad, LOC_PORT_HOST); if (bind(loc_sock, ad, 16) != 0) say("local link: HOST could not take port 47801 - the guest's hero will not arrive"); }
  /* v2.4: no program in this test - the module fills its own copy of the program link, so the existing player-2 code (join with F2, puppet, pad writes, capture) works unchanged */
  if (!lk) { memset(&loc_lk, 0, sizeof loc_lk); loc_lk.magic = LINK_MAGIC; loc_lk.version = LINK_VERSION; loc_lk.room_role = loc_role; loc_lk.prog_tick = GetTickCount();
    loc_lk.want_p2 = 0; loc_lk.inject = 0; inj_pad = 0; kbi_on = 1;   /* v2.6: both copies show the other player as their player 2 */
    lk = &loc_lk; say("local link: my hero (player 1) is sent to the other copy 50 times a second; the other hero plays player 2 here with keys inside this game only (F2 too); position corrected softly above 0.5 m"); }
  lbegin(); bs("local link: this copy is the "); bs(loc_role == 1 ? "HOST (sends its level requests to 127.0.0.1:47802)" : "GUEST (listens on 127.0.0.1:47802 and follows the host's level)"); lend();
}
/* v2.8: which hero is which. Both copies load the same level with the same party, so a hero is named by its place in the game's
 * character list (head 0x015b31e8, next +0x244; only nodes with a vtable count). Each copy sends the places of its player 1 and player 2. */
static i32 hero_index(u32 ch) { u32 nd[64], n, i, k = 0; if (!ch) return -1; n = cen_nodes(nd, 64); for (i = 0; i < n; i++) { if (!rd32(nd[i])) continue; if (nd[i] == ch) return (i32)k; k++; } return -1; }
static volatile i32 loc_oth_p1 = -1, loc_oth_p2 = -1;
#define HM_ENABLED 0   /* v2.8b: hero matching by list place switched off (the place is not the same in both copies); to come back by hero name */
static u32 hm_try, hm_key, hm_next, hm_prev_ch, hm_done, hm_start; static const u8 HM_KEYS[3] = { 0x25 /* K */, 0x18 /* O */, 0x17 /* I */ };
/* v2.10: hero sync by NAME. Each copy sends the name of its player 1 hero (+0x24 text) in the TCP2 packet; when player 2 here
   carries another name, the module asks the game's loader (same way as F8) to make player 2 that hero. At most one request every
   2 s; a name the game does not know, or that did not stick after 3 requests, is left alone until the other copy's hero changes. */
static int strcmp_s(const char *a, const char *b); static int hn_text(u32 p, char *out); static int hero_fn_ok(void);
static char loc_oth_name[32], hs_name_tried[32]; static u32 hs_next, hs_tries, hs_names_rx, hs_pending;
/* v2.10a: the same hero twice -> the game names the second one with a number ("FrodoBaggins01"): compare without digits at the end */
static int hero_same(const char *mine, const char *want) { while (*want && *mine == *want) { mine++; want++; } if (*want) return 0; while (*mine >= '0' && *mine <= '9') mine++; return *mine == 0; }
static void hero_name_of(u32 ch, char *out) { char t[48]; int i; out[0] = 0; if (!ch || !hn_text(rd32(ch + 0x24), t)) return; for (i = 0; i < 31 && t[i]; i++) out[i] = t[i]; out[i] = 0; }
static void hero_sync_tick(u32 now) { char mine[32]; u32 c2; int i;
  if (!hs_pending || !loc_oth_name[0] || !rd32(base - IMAGEBASE + VA_CUR_LEVEL) || !goto_orig_peek || !hero_fn_ok()) return;
  c2 = pp_char(1); if (!c2) return;
  hero_name_of(c2, mine); if (!mine[0]) return;
  if (hero_same(mine, loc_oth_name)) { lbegin(); bs("hero sync: player 2 is \""); bs(mine); bs("\" like the other copy's player 1 - done"); lend(); hs_tries = 0; hs_name_tried[0] = 0; hs_pending = 0; return; }
  if (strcmp_s(hs_name_tried, loc_oth_name)) { hs_tries = 0; for (i = 0; i < 31 && loc_oth_name[i]; i++) hs_name_tried[i] = loc_oth_name[i]; hs_name_tried[i] = 0; }
  if (hs_tries >= 3) { if ((i32)(now - hs_next) >= 0) { hs_pending = 0; hs_tries = 0; say("hero sync: 3 tries did not work - left alone until the other player changes hero again"); } return; }
  if (hero_req || hsw_busy || (i32)(now - hs_next) < 0) return;
  hs_tries++; hs_next = now + 2000u;
  for (i = 0; i < 31 && loc_oth_name[i]; i++) hero_want[i] = loc_oth_name[i]; hero_want[i] = 0;
  lbegin(); bs("hero sync: the other copy's player 1 is \""); bs(loc_oth_name); bs("\", my player 2 is \""); bs(mine); bs("\" - asking the game (try "); bd(hs_tries); bs(")"); lend();
  hero_req_slot = 1; hero_req = 1; }
static void loc_p2_in(u32 *pk, u32 now) {   /* the other copy's hero -> player 2 of this game */
  memcpy(&loc_lk.rem_px, &pk[2], 4); memcpy(&loc_lk.rem_py, &pk[3], 4); memcpy(&loc_lk.rem_pz, &pk[4], 4); memcpy(&loc_lk.rem_yaw, &pk[5], 4);
  { static float px, py, pz; static u32 pt; float nx, ny, nz; u32 dt = now - pt; memcpy(&nx, &pk[2], 4); memcpy(&ny, &pk[3], 4); memcpy(&nz, &pk[4], 4);
    if (pt && dt >= 5u && dt <= 200u) { float k = 1000.0f / (float)dt, vx = (nx - px) * k, vy = (ny - py) * k, vz = (nz - pz) * k;
      if (vx > -20.0f && vx < 20.0f && vy > -20.0f && vy < 20.0f && vz > -20.0f && vz < 20.0f) { loc_vx = 0.5f * loc_vx + 0.5f * vx; loc_vy = 0.5f * loc_vy + 0.5f * vy; loc_vz = 0.5f * loc_vz + 0.5f * vz; } }
    else if (dt > 200u) { loc_vx = loc_vy = loc_vz = 0.0f; }
    px = nx; py = ny; pz = nz; pt = now; }
  loc_lk.ax = (i32)pk[6]; loc_lk.ay = (i32)pk[7]; loc_lk.buttons = pk[8]; loc_lk.rem_valid = 1; loc_lk.rem_tick = now; loc_p2_rx++; loc_oth_p1 = (i32)pk[9]; loc_oth_p2 = (i32)pk[10];
  /* v2.10a: act only when the other player's hero CHANGES (not on the first packet / level start, when both copies have the save's own heroes) */
  { const char *nm = (const char *)&pk[11]; char t[32]; int i; for (i = 0; i < 31 && nm[i]; i++) t[i] = nm[i]; t[i] = 0;
    if (t[0] && strcmp_s(t, loc_oth_name)) { int first = loc_oth_name[0] == 0; for (i = 0; i < 32; i++) loc_oth_name[i] = t[i];
      lbegin(); bs("hero sync: the other copy's player 1 is now \""); bs(t); bs(first ? "\" (first packet - nothing changed here)" : "\" - my player 2 follows"); lend();
      if (!first) { hs_pending = 1; hs_tries = 0; hs_next = 0; } }
    hs_names_rx++; } }
static void loc_level_in(u32 *pk) {
  if (pk[1] == loc_rx_seq) return;
  loc_rx_seq = pk[1]; loc_pend_idx = pk[2]; loc_pend_flag = pk[3]; loc_pend_prev = pk[4]; loc_pend_dur = pk[5]; loc_pend_n++; loc_sched_logged = 0;
  lbegin(); bs("local link: host is going to level "); bd(pk[2]); bs(" flag "); bd(pk[3]); bs(" (host request #"); bd(pk[1]); bs(", host spent "); bd(pk[5] / 1000u); bs(" s in level "); bd(pk[4]); bs(")"); lend(); }
static void loc_tick(u32 now) {
  static u32 hm_seen;
  if (!loc_role) return;
  if (hm_reset_seq != hm_seen) { hm_seen = hm_reset_seq; hm_done = 0; hm_try = 0; hm_key = 0; hm_start = 0; hm_next = 0; }
  if (lk == &loc_lk) loc_lk.prog_tick = now;
  /* v2.6: both copies send their own hero (player 1) to the other one, 50 times a second */
  if (lk == &loc_lk && (i32)(now - loc_p2_sent_at) >= 20) { u32 pk[19]; u8 ad[16];
    loc_p2_sent_at = now;
    if (loc_lk.cap_pos_valid && (i32)(now - loc_lk.cap_tick) < 200) { pk[0] = LOC_MAGIC_P2; pk[1] = ++loc_p2_seq; memcpy(&pk[2], &loc_lk.cap_px, 4); memcpy(&pk[3], &loc_lk.cap_py, 4); memcpy(&pk[4], &loc_lk.cap_pz, 4); memcpy(&pk[5], &loc_lk.cap_yaw, 4);
      pk[6] = (u32)loc_lk.cap_ax; pk[7] = (u32)loc_lk.cap_ay; pk[8] = loc_lk.cap_buttons; pk[9] = (u32)hero_index(pp_char(0)); pk[10] = (u32)hero_index(pp_char(1)); memset(&pk[11], 0, 32); hero_name_of(pp_char(0), (char *)&pk[11]); loc_addr(ad, loc_role == 1 ? LOC_PORT_GUEST : LOC_PORT_HOST); sendto(loc_sock, pk, 76, 0, ad, 16); } }
  /* receive: hero packets (36 bytes, both sides) and level packets (24 bytes, guest only) */
  { u32 pk[19]; u8 from[16]; int fl = 16, r, k;
    for (k = 0; k < 64; k++) { r = recvfrom(loc_sock, pk, 76, 0, from, &fl); fl = 16; if (r <= 0) break;
      if (r == 76 && pk[0] == LOC_MAGIC_P2 && lk == &loc_lk) loc_p2_in(pk, now);
      else if (r == 24 && pk[0] == LOC_MAGIC && loc_role == 2) loc_level_in(pk); } }
  /* the other player's actions as player-2 keys inside this game, F2 while player 2 is absent */
  if (lk == &loc_lk) {
    { u32 m = 0; static u32 f2_next; u32 p2in = *(volatile const u32 *)(base - IMAGEBASE + VA_P2LIST) != 0;
      if (loc_lk.rem_valid && (i32)(now - loc_lk.rem_tick) < 300) { i32 ax = loc_lk.ax, ay = loc_lk.ay; u32 b = loc_lk.buttons;
        if (ax < -35) m |= 1u << KB_LEFT; if (ax > 35) m |= 1u << KB_RIGHT; if (ay > 35) m |= 1u << KB_UP; if (ay < -35) m |= 1u << KB_DOWN;
        if (b & BT_ACTION) m |= 1u << KB_ACTION; if (b & BT_SPECIAL) m |= 1u << KB_SPECIAL; if (b & BT_JUMP) m |= 1u << KB_JUMP;
        if (!p2in && rd32(base - IMAGEBASE + VA_CUR_LEVEL)) {
          int ready = 1;
          /* guest: before player 2 joins, my player 1 must be the hero that is player 2 at the host */
          if (HM_ENABLED && loc_role == 2 && !hm_done && loc_oth_p2 >= 0 && pp_char(0)) { u32 c1 = pp_char(0); i32 my = hero_index(c1); ready = 0;
            if (!hm_start) { hm_start = now; lbegin(); bs("hero: host has player 1 = list #"); bi(loc_oth_p1); bs(", player 2 = list #"); bi(loc_oth_p2); bs("; my player 1 is #"); bi(my); bs(my == loc_oth_p2 ? " - already the right hero" : " - switching"); lend(); }
            if (my == loc_oth_p2) { hm_done = 1; ready = 1; lbegin(); bs("hero: my player 1 is now the host's player 2 (list #"); bi(my); bs(") - player 2 may join"); lend(); }
            else if ((i32)(now - hm_next) >= 0) {
              if (hm_try && hm_prev_ch == c1) { lbegin(); bs("hero: key "); bh(HM_KEYS[hm_key]); bs(" did not change my hero"); lend(); hm_key++; }
              if (hm_key >= 3 || hm_try >= 12 || (i32)(now - hm_start) > 30000) { hm_done = 1; ready = 1; say("hero: giving up switching - player 2 joins as it is"); }
              else { hm_prev_ch = c1; hm_try++; kbi_extra_dik = HM_KEYS[hm_key]; kbi_extra_until = now + 150u; hm_next = now + 2000u; lbegin(); bs("hero: pressing player-1 key "); bh(HM_KEYS[hm_key]); bs(" inside the game (try "); bd(hm_try); bs(")"); lend(); } } }
          if (ready && (i32)(now - f2_next) >= 0) { f2_next = now + 3000u; kbi_f2_until = now + 150u; say("local link: player 2 is absent - F2 put into this game's keyboard"); } } }
      kbi_mask = p2in ? m : 0; }
    if ((i32)(now - loc_p2_log_at) >= 0) { loc_p2_log_at = now + 3000u; if (loc_p2_rx != loc_p2_rx_logged) { loc_p2_rx_logged = loc_p2_rx; lbegin(); bs("local link: other hero packets "); bd(loc_p2_rx); bs(", other hero at mm "); bi(f2mm(loc_lk.rem_px)); bs(" "); bi(f2mm(loc_lk.rem_py)); bs(" "); bi(f2mm(loc_lk.rem_pz)); bs(", buttons "); bbtn(loc_lk.buttons); bs(", player 2 joined: "); bs(*(volatile const u32 *)(base - IMAGEBASE + VA_P2LIST) ? "yes" : "no"); bs(", drift (mm) "); bd((u32)pp_net_dist_mm); bs(" | keyboard reads: state "); bd(kbi_st_calls); bs(", key writes "); bd(kbi_writes); bs(", refused reads replaced "); bd(kbi_fixed); bs(", Acquire calls "); bd(kbi_acq_calls); bs(" refused "); bd(kbi_acq_fail); lend(); } } }
  if (loc_role == 1) {
    u32 n = lg_n; if (n && (n != loc_sent_n || (i32)(now - loc_resend_at) >= 0)) { u32 pk[6]; u8 ad[16];
      pk[0] = LOC_MAGIC; pk[1] = n; pk[2] = lg_idx; pk[3] = lg_flag; pk[4] = lg_prev_idx; pk[5] = lg_dur; loc_addr(ad, LOC_PORT_GUEST); sendto(loc_sock, pk, 24, 0, ad, 16);
      if (n != loc_sent_n) { lbegin(); bs("local link: sent level "); bd(lg_idx); bs(" flag "); bd(lg_flag); bs(" (request #"); bd(n); bs(", spent "); bd(lg_dur / 1000u); bs(" s in level "); bd(lg_prev_idx); bs(")"); lend(); }
      loc_sent_n = n; loc_resend_at = now + 1000u; }
    return;
  }
  if (loc_pend_idx != 0xFFFFFFFFu && goto_req < 0) {
    u32 cur = rd32(base - IMAGEBASE + VA_CUR_LEVEL); u32 lmgr = rd32(base - IMAGEBASE + VA_STREAM_MGR), loading = lmgr ? rd32(lmgr + 0xbc) : 1;
    /* v2.3d: "quiet" = the game's loader is idle (stream manager +0xbc == 0: the CURLEVEL tests showed it is set when a level starts loading and
       cleared when loading ends) and at least 1.5 s since the last level request on this copy */
    int quiet = !loading && (lg_any_n == 0 || (i32)(now - lg_any_t) > 1500);
    if (lg_any_n && lg_any_idx == loc_pend_idx) { lbegin(); bs("local link: the last level request on this copy is already level "); bd(loc_pend_idx); bs(" - nothing to do"); lend(); loc_pend_idx = 0xFFFFFFFFu; }
    else if (loc_pend_flag == 1 && lg_any_idx == loc_pend_prev && cur && goto_orig_peek) {   /* the host goes on from the part I am in now */
      u32 at = lg_any_t + loc_pend_dur + 1500u;
      if (!loc_sched_logged) { loc_sched_logged = 1; lbegin(); bs("local link: host left level "); bd(loc_pend_prev); bs(" after "); bd(loc_pend_dur / 1000u); bs(" s - I follow after the same time + 1.5 s ("); bd((i32)(at - now) > 0 ? (at - now) / 1000u : 0); bs(" s from now) unless my game moves on by itself first"); lend(); }
      if ((i32)(now - at) >= 0 && !loading) { lbegin(); bs("local link: time is up, following the host: request level "); bd(loc_pend_idx); bs(" flag "); bd(loc_pend_flag); lend(); goto_flag = (i32)loc_pend_flag; goto_req = (i32)loc_pend_idx; loc_pend_idx = 0xFFFFFFFFu; } }
    else if (cur && quiet && goto_orig_peek) { lbegin(); bs("local link: following the host: request level "); bd(loc_pend_idx); bs(" "); lv_str(rd32(rd32(base - IMAGEBASE + VA_LEVEL_TAB) + loc_pend_idx * 0xE0u + 0x3c)); bs(" flag "); bd(loc_pend_flag); lend();
      goto_flag = (i32)loc_pend_flag; goto_req = (i32)loc_pend_idx; loc_pend_idx = 0xFFFFFFFFu; }
    else if ((i32)(now - loc_wait_log_at) >= 0) { loc_wait_log_at = now + 5000u; lbegin(); bs("local link: waiting to follow the host (level running: "); bs(cur ? "yes" : "no - load your save"); bs(", my game not loading: "); bs(quiet ? "yes" : "no"); bs(")"); lend(); }
  }
}

/* ---------------- v2.8a: what names does a hero carry? (discovery, read only) ----------------
 * When player 1 or player 2 becomes another hero (switch key, story, menu), the first 0xA60 bytes of the character are followed:
 * every pointer to a readable name-like text is logged (depth 1: char+off -> text; depth 2: char+off -> struct+off2 -> text).
 * Comparing the lines of two heroes shows which field holds the hero's type / name. */
static int strcmp_s(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a != *b; }
static int hn_text(u32 p, char *out) { char t[48]; int i; if (p < 0x100000u || p > 0x7F000000u || !rd(p, t, 47)) return 0; t[47] = 0;
  if (!(t[0] >= 'A' && t[0] <= 'Z')) return 0;
  for (i = 0; t[i]; i++) { char c = t[i]; if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == ' ')) return 0; }
  if (i < 3 || i > 40) return 0; for (i = 0; i < 48; i++) out[i] = t[i]; return 1; }
static void hero_names(const char *who, u32 ch) {
  static u8 buf[0xA60], b2[0x100]; u32 o, o2, lines = 0; char t[48];
  if (!ch || !rd(ch, buf, sizeof buf)) return;
  lbegin(); bs("HERO "); bs(who); bs(" is now character "); bh(ch); bs(" (list #"); bi(hero_index(ch)); bs(")"); lend();
  for (o = 0; o + 4 <= sizeof buf && lines < 60; o += 4) { u32 p = *(u32 *)(buf + o);
    if (hn_text(p, t)) { lbegin(); bs("  name +"); bh(o); bs(" -> \""); bs(t); bs("\""); lend(); lines++; continue; }
    if (p > 0x100000u && p < 0x7F000000u && !(p & 3) && rd(p, b2, sizeof b2))
      for (o2 = 0; o2 + 4 <= sizeof b2 && lines < 60; o2 += 4) if (hn_text(*(u32 *)(b2 + o2), t)) { lbegin(); bs("  name +"); bh(o); bs(" ->+"); bh(o2); bs(" -> \""); bs(t); bs("\""); lend(); lines++; } }
}
/* v2.11: where does the split screen keep the size of the halves? (discovery, read only)
   F3 = remember the split-screen manager (tc_split_mgr, 0x1440 bytes) and the objects at its +0x5c and +0x1424 (0x400 bytes each);
   F4 = compare with now and log every dword that changed (hex and value*1000 as a float). */
#define SPW_N 0x1440u
#define SPW_M 0x400u
static u8 spw_a[SPW_N], spw_b[SPW_N], spw_a2[2][SPW_M], spw_b2[2][SPW_M]; static u32 spw_have, spw_ptr[2];
static void spw_diff(const char *what, const u8 *a, const u8 *b, u32 n, u32 *lines) { u32 o;
  for (o = 0; o + 4 <= n && *lines < 160; o += 4) { u32 x = *(const u32 *)(a + o), y = *(const u32 *)(b + o); float fx, fy; if (x == y) continue;
    memcpy(&fx, &x, 4); memcpy(&fy, &y, 4);
    lbegin(); bs("  "); bs(what); bs(" +"); bh(o); bs(": "); bh(x); bs(" -> "); bh(y);
    if ((x & 0x7F800000u) != 0x7F800000u && (y & 0x7F800000u) != 0x7F800000u && fx > -100000.0f && fx < 100000.0f && fy > -100000.0f && fy < 100000.0f) { bs("   as float*1000: "); bi(f2mm(fx)); bs(" -> "); bi(f2mm(fy)); }
    lend(); (*lines)++; } }
static void spw_tick(u32 fg) { static u32 p3, p4; u32 k3 = (GetAsyncKeyState(0x72) & 0x8000) != 0, k4 = (GetAsyncKeyState(0x73) & 0x8000) != 0, m = tc_split_mgr, i;
  if (k3 && !p3 && fg && !cen_on) {
    if (!m || !rd(m, spw_a, SPW_N)) say("split watch: F3 - the split-screen manager is not known yet (no level?)");
    else { for (i = 0; i < 2; i++) { spw_ptr[i] = rd32(m + (i ? 0x1424u : 0x5cu)); if (!spw_ptr[i] || !rd(spw_ptr[i], spw_a2[i], SPW_M)) memset(spw_a2[i], 0, SPW_M); }
      spw_have = 1; lbegin(); bs("split watch: F3 - remembered the manager "); bh(m); bs(" (mode "); bd(rd32(m + 0x13f8)); bs("), +0x5c -> "); bh(spw_ptr[0]); bs(", +0x1424 -> "); bh(spw_ptr[1]); bs(" - now change the screen and press F4"); lend(); } }
  if (k4 && !p4 && fg && !cen_on) {
    if (!spw_have || !m || !rd(m, spw_b, SPW_N)) say("split watch: F4 - press F3 first");
    else { u32 lines = 0; lbegin(); bs("split watch: F4 - changes since F3 (mode now "); bd(rd32(m + 0x13f8)); bs("):"); lend();
      spw_diff("mgr", spw_a, spw_b, SPW_N, &lines);
      for (i = 0; i < 2; i++) if (spw_ptr[i] && rd32(m + (i ? 0x1424u : 0x5cu)) == spw_ptr[i] && rd(spw_ptr[i], spw_b2[i], SPW_M)) spw_diff(i ? "[+0x1424]" : "[+0x5c]", spw_a2[i], spw_b2[i], SPW_M, &lines);
      lbegin(); bs("split watch: "); bd(lines); bs(lines >= 160 ? " lines (cut at 160)" : " lines"); lend(); } }
  p3 = k3; p4 = k4; }
/* v2.9: F8 = read teracoop_hero.txt (a hero name like Legolas) and give it to player 1 on the game thread */
static int hero_fn_ok(void) { const u8 *a = (const u8 *)(base - IMAGEBASE + VA_TYPE_BY_NAME), *b = (const u8 *)(base - IMAGEBASE + VA_CUR_TYPE), *c = (const u8 *)(base - IMAGEBASE + VA_SWAP_TYPE);
  return a[0] == 0x8B && a[1] == 0x44 && a[2] == 0x24 && a[3] == 0x04 && a[4] == 0x8B && a[5] == 0x0D && b[0] == 0x56 && b[1] == 0x8B && b[2] == 0xF1 && b[3] == 0xE8
      && (tc_swap_back ? 1 : c[0] == 0x55) && (tc_swap_back || (c[1] == 0x8B && c[2] == 0xEC && c[3] == 0x83 && c[4] == 0xE4 && c[5] == 0xF0)) && c[6] == 0x6A && c[7] == 0xFF
      && ((const u8 *)(base - IMAGEBASE + VA_CLU_CTOR))[0] == 0x6A && ((const u8 *)(base - IMAGEBASE + VA_CLU_CTOR))[2] == 0x68 && ((const u8 *)(base - IMAGEBASE + VA_CLU_CTOR))[3] == 0x78
      && ((const u8 *)(base - IMAGEBASE + VA_CLU_TICK))[0] == 0x56 && ((const u8 *)(base - IMAGEBASE + VA_CLU_TICK))[1] == 0x57 && ((const u8 *)(base - IMAGEBASE + VA_CLU_TICK))[2] == 0x8B && ((const u8 *)(base - IMAGEBASE + VA_CLU_TICK))[3] == 0xF9; }
static void hero_f8(void) { u16 fp[460]; PV h; char t[64]; u32 got = 0, i, k = 0;
  lbegin(); bs("hero: F8 pressed - ");
  if (!goto_orig_peek) { bs("no PeekMessageA hook, nothing done"); lend(); return; }
  if (!hero_fn_ok()) { bs("the game's hero functions have unexpected bytes, nothing done"); lend(); return; }
  mkpath(fp, L"teracoop_hero.txt"); h = CreateFileW(fp, 0x80000000u, 7, 0, 3, 0x80, 0);
  if (h && h != (PV)-1) { ReadFile(h, t, 63, &got, 0); CloseHandle(h); }
  for (i = 0; i < got && k < 47; i++) { char c = t[i]; if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') hero_want[k++] = c; else if (k) break; }
  hero_want[k] = 0;
  if (!k) { bs("teracoop_hero.txt has no hero name (example: Legolas)"); lend(); return; }
  bs("asking the game to make player 1 \""); bs(hero_want); bs("\""); lend(); hero_req_slot = 0; hero_req = 1; }
static void hero_result(void) { static u32 seen; if (hero_seq == seen) return; seen = hero_seq;
  lbegin(); bs("hero: game thread: ");
  if (hero_res_ch == 0xFFFFFFFFu) bs("no level running, nothing done");
  else if (!hero_res_ch) { bs("no player "); bd(hero_res_slot + 1); bs(" character, nothing done"); }
  else if (hero_res_type < 0) { bs("the game does not know the hero \""); bs(hero_want); bs("\" (type -1)"); }
  else if (hero_res_ret == 0xBAD0u) bs("the loader object is not the expected CharLoaderUtilSwap, nothing done");
  else { bs("player "); bd(hero_res_slot + 1); bs(" character "); bh(hero_res_ch); bs(" type "); bi(hero_res_old); bs(" -> "); bi(hero_res_type); bs(" (\""); bs(hero_want); bs("\"): loading asked, waiting"); }
  lend(); }
static void hero_loader_result(void) { static u32 seen; if (hsw_done_seq == seen) return; seen = hsw_done_seq;
  lbegin(); bs("hero: loader "); bs(hsw_timeout ? "gave up after " : "finished after "); bd(hsw_done_ms); bs(" ms, "); bd(hsw_frames); bs(" frames"); lend(); }
/* v2.8c: switching hero from the in-game menu keeps the SAME character object and only changes its data (log v2.8b: char 0x1ec1d800
   stayed, +0x24 name pointer / +0x20 / +0x40 changed) - so a hero change = other object OR other name text at +0x24. */
static void hero_one(const char *who, u32 c, u32 *lastc, char *lastn, u32 *last20) { char t[48]; u32 v20;
  if (!c) return; if (!hn_text(rd32(c + 0x24), t)) t[0] = 0; v20 = rd32(c + 0x20);
  if (c == *lastc && !strcmp_s(t, lastn) && v20 == *last20) return;
  lbegin(); bs("HERO "); bs(who); bs(c == *lastc ? " same character " : " other character "); bh(c); bs(" name +0x24 \""); bs(t); bs("\" (+0x20 = "); bh(v20); bs(", +0x40 = "); bh(rd32(c + 0x40)); bs(")"); lend();
  if (c != *lastc) hero_names(who, c);
  *lastc = c; *last20 = v20; { int i; for (i = 0; i < 47 && t[i]; i++) lastn[i] = t[i]; lastn[i] = 0; } }
static void hero_watch(void) { static u32 l1, l2, a1, a2; static char n1[48], n2[48];
  hero_one("player 1", pp_char(0), &l1, n1, &a1); hero_one("player 2", pp_char(1), &l2, n2, &a2); }

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
  lbegin(); bs("teracoop module v2.14 (smooth other hero: speed prediction + 25%/frame on the game thread; OWN SCREEN: teracoop_view.txt own = one screen camera on player 1 only + transitions do not wait for the camera; tiny polygon 16-byte aligned - the game reads it with movaps; full: player 1's view polygon = whole window, player 2's = tiny corner; teracoop_view.txt full = split line moved off screen, no crop; own-screen test, left half only while the screen is split with player 2: teracoop_view.txt left / left wide, F6 = left half on/off, split submode forced vertical; split watch: F3 remember / F4 compare the split-screen manager; HERO SYNC by name both ways, only when the other player CHANGES hero: my player 2 = the other copy's player 1; F8 = hero from teracoop_hero.txt through the game's CharLoaderUtilSwap: load, then swap; SWAP = every hero change of the game logged with its callers; F8 = make player 1 the hero named in teracoop_hero.txt; HERO name at +0x24 logged on every change; hero matching off; one screen: F7 / teracoop_split.txt = off; local link both ways, exact position and heading: the other copy's hero = my player 2; of two copies via teracoop_local.txt; GOTO requests in the log; CURLEVEL = the game's own current-level accessor; F5 = go to level from teracoop_goto.txt; LEVEL changes and ADDED/KILLED character events in the log; window mode via teracoop_windowed.txt; second copy via teracoop_multi.txt; F11 data recording, F3/F4/F5 markers, F6 pad injection test, F7 list mirror (OFF by default), controls from Mappings.dat) started, exe base "); bh(base); bs(" timestamp "); bh(ts); bs(" size "); bh(soi); lend();
  build_ok = (ts == TIMESTAMP && soi == SIZEOFIMAGE);
  /* overlay: install at once (the game has not created its Direct3D device yet), retry a few times */
  win_load(); view_load(); own_load();
  say(multi_set ? "second copy: allowed (teracoop_multi.txt - the game's own single-copy check is skipped)" : "second copy: blocked as usual (create teracoop_multi.txt in the game folder to allow it)");
  if (win_force) { lbegin(); bs("window mode: ON (teracoop_windowed.txt), window "); bd(win_w); bs("x"); bd(win_h); if (win_x >= 0) { bs(" at "); bd((u32)win_x); bs(","); bd((u32)win_y); } else bs(" centred"); lend(); }
  else say("window mode: off (create the file teracoop_windowed.txt in the game folder to play in a window)");
  { u16 fp[460]; mkpath(fp, L"teracoop_overlay.txt"); if (GetFileAttributesW(fp) == 0xFFFFFFFFu) { ov_no_panel = 1; if (!win_force) ov_tries = 99; say("overlay: panel is switched off (create the file teracoop_overlay.txt in the game folder to try it)"); } }
  if (ov_tries != 99) { long hr = 0; int r = ov_install(&hr); lbegin(); bs("overlay: hook install step "); bd((u32)r); bs(" hresult "); bh((u32)hr); lend(); ov_state = r == 0; ov_tries = 1; ov_hint_until = GetTickCount() + 120000u; }
  if (build_ok) { CreateThread(0, 0, puppet_thread, 0, 0, 0); pe_ranges(); load_bindings("start"); }
  if (build_ok) { goto_install(); loc_start(); split_install(); swap_install(); view_patch(); view_full_patch(); view_poly_install(); own_patch();
    { u16 fp[460]; PV h; char t[8]; u32 got = 0; mkpath(fp, L"teracoop_split.txt"); h = CreateFileW(fp, 0x80000000u, 7, 0, 3, 0x80, 0);
      if (h && h != (PV)-1) { ReadFile(h, t, 7, &got, 0); CloseHandle(h); if (got >= 3 && (t[0] | 32) == 'o' && (t[1] | 32) == 'f') { split_off_on = 1; say("split: teracoop_split.txt = off - one screen for every level (levels.txt flag split_off)"); } } } }
  if (build_ok) { tc_hook(VA_EV_ADDED, tc_stub_added, &tc_added_imm, &tc_added_back, "ADDED (FUN_0095d300)"); tc_hook(VA_EV_KILLED, tc_stub_killed, &tc_killed_imm, &tc_killed_back, "KILLED (FUN_0095d380)"); tc_hook(VA_EV_GOTO, tc_stub_goto, &tc_goto_imm, &tc_goto_back, "GOTO (FUN_00b169d0)"); }
  say(build_ok ? "game build supported" : "game build NOT supported - the module will not touch the game");
  for (;;) {
    u32 now, state = 0, p2 = 0, fg, prog_alive, want = 0, want_join = 0;
    Sleep(10); now = GetTickCount();
    /* the F11 dump of step 5.1 is finished and switched off (F11 must stay free for the game and for screenshots) */
    { static u32 nlog_at, nsig; if ((i32)(now - nlog_at) >= 0) { u32 sg = pp_net_writes + pp_net_snaps * 3 + pp_net_none * 5; nlog_at = now + 3000u; if (sg != nsig) { nsig = sg; lbegin(); bs("puppet-net: writes "); bd(pp_net_writes); bs(" snaps "); bd(pp_net_snaps); bs(" no-p2-frames "); bd(pp_net_none); bs(" distance (mm) "); bd((u32)pp_net_dist_mm); lend(); } } }
    { u32 f8 = (GetAsyncKeyState(0x77) & 0x8000) != 0; static u32 f8_prev, pp_log_at, pp_sig;
      if (f8 && !f8_prev && game_fg()) hero_f8();
      f8_prev = f8;
      if (pp_mode && (i32)(now - pp_log_at) >= 0) { u32 sig = pp_writes + pp_kept * 3 + pp_reverted * 5 + pp_noP2 * 7; pp_log_at = now + 2000u;
        if (sig != pp_sig) { pp_sig = sig; lbegin(); bs("puppet: writes "); bd(pp_writes); bs(" kept "); bd(pp_kept); bs(" reverted "); bd(pp_reverted); bs(" no-p2-frames "); bd(pp_noP2); bs(" last revert dx/dz (mm) "); if (pp_lastdx < 0) { bs("-"); bd((u32)-pp_lastdx); } else bd((u32)pp_lastdx); bs("/"); if (pp_lastdz < 0) { bs("-"); bd((u32)-pp_lastdz); } else bd((u32)pp_lastdz); bs(" p2 char "); bh(pp_lastch); lend(); } } }
    /* overlay housekeeping (runs even before the program is connected) */
    if (!ov_state && ov_tries && ov_tries < 40 && (i32)(now - ov_try_at) >= 0) { long hr = 0; int r = ov_install(&hr); ov_tries++; ov_try_at = now + 500u; if (r == 0) { ov_state = 1; lbegin(); bs("overlay: hook installed on try "); bd(ov_tries); lend(); } else if (ov_tries == 40) { lbegin(); bs("overlay: giving up, last step "); bd((u32)r); bs(" hresult "); bh((u32)hr); lend(); } }
    if (ov_state) {
      u32 f9 = (GetAsyncKeyState(0x78) & 0x8000) != 0; ov_lk = (PV)lk;
      if (f9 && !f9_prev && game_fg() && !ov_no_panel) { ov_on = !ov_on; }
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
    if (build_ok) { tcev_drain(); level_tick(); goto_tick((u32)game_fg()); loc_tick(now); hero_watch(); hero_result(); hero_loader_result(); swap_drain(); if (loc_role) hero_sync_tick(now); spw_tick((u32)game_fg()); view_tick((u32)game_fg()); }
    if (build_ok && tc_split_back) { u32 f7 = (GetAsyncKeyState(0x76) & 0x8000) != 0; static u32 sn, f7p, last_arg = 0xFFFFFFFFu, last_mgr, so_next; static u8 so_orig[1024]; static u32 so_have;
      /* the game calls FUN_0085b9a0 every frame (v2.7 log: thousands of calls): 0 = split screen, 1 = one screen (FUN_009d6120 picks 1 when the
         level has the levels.txt flag split_off = entry +0xd4 bit 0x800, or in the hub, or when DAT_015b4f01 is 0). Only changes are logged. */
      if (tc_split_n != sn) { sn = tc_split_n; if (tc_split_arg != last_arg || tc_split_mgr != last_mgr) { last_arg = tc_split_arg; last_mgr = tc_split_mgr; lbegin(); bs("SPLIT: game now uses mode "); bd(tc_split_arg); bs(tc_split_arg == 0 ? " (split screen)" : tc_split_arg == 1 ? " (one screen)" : ""); bs(" (manager "); bh(tc_split_mgr); bs(", calls so far "); bd(sn); bs(")"); lend(); } }
      /* v2.7a: "split off" = the game's own levels.txt flag put on every level (F7 switches it on/off; teracoop_split.txt = off switches it on at start) */
      if (f7 && !f7p && game_fg()) { split_off_on = !split_off_on; lbegin(); bs("split: F7 - one screen for every level "); bs(split_off_on ? "ON" : "OFF (the levels' own setting again)"); lend(); so_next = 0; }
      f7p = f7;
      if ((i32)(now - so_next) >= 0) { u32 tab = rd32(base - IMAGEBASE + VA_LEVEL_TAB), cnt = rd32(base - IMAGEBASE + VA_LEVEL_CNT), i, ch = 0; so_next = now + 2000u;
        if (tab && cnt && cnt <= 1024) {
          if (!so_have) { for (i = 0; i < cnt; i++) so_orig[i] = (u8)((rd32(tab + i * 0xE0u + 0xd4) >> 11) & 1u); so_have = 1; }
          for (i = 0; i < cnt; i++) { u32 a = tab + i * 0xE0u + 0xd4, v = rd32(a), want = split_off_on ? (v | 0x800u) : so_orig[i] ? (v | 0x800u) : (v & ~0x800u);
            if (want != v) { *(volatile u32 *)a = want; ch++; } }
          if (ch) { lbegin(); bs("split: levels.txt flag split_off "); bs(split_off_on ? "put on " : "given back on "); bd(ch); bs(" of "); bd(cnt); bs(" levels"); lend(); } } } }
    { static u32 wsig; u32 sg = win_cd_n + win_reset_n * 7 + win_fix_n * 31; if (win_force && sg != wsig) { wsig = sg; lbegin(); bs("window mode: CreateDevice "); bd(win_cd_n); bs(" Reset "); bd(win_reset_n); bs(" window set "); bd(win_fix_n); bs(" hwnd "); bh(win_hwnd); lend(); } }
    if (build_ok) { u32 fg0 = (u32)game_fg(); census_tick(now, fg0); if (lk != &loc_lk) enemy_tick(now, fg0); }   /* the old enemy-list mirror is not used with the local link */
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
      if (f6 && !f6_prev && fg && !view_on) { inj_pad = !inj_pad; lbegin(); bs("pad-inject mode "); bs(inj_pad ? "ON (the friend's actions are written straight into the pad of player 2, no keys)" : "OFF (keys of this PC's Mappings.dat)"); bs(" (F6)"); lend(); }
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
typedef struct { u32 vt; void *st, *dt, *acq; } DIHK;
static DIHK dihk[6]; static volatile u32 dihk_n;
static DIHK *dihk_find(PV dev) { u32 i, v = (u32)VT(dev); for (i = 0; i < dihk_n; i++) if (dihk[i].vt == v) return &dihk[i]; return 0; }
/* v2.5: player 2 of the host is played by keys that exist only inside this game: the keyboard state the game reads through
 * DirectInput (GetDeviceState, 256 bytes, indexed by DIK codes) gets the player-2 keys of Mappings.dat set while the guest presses them,
 * plus F2 to let player 2 join. Works when the window is not active (if DirectInput refuses then, an empty keyboard is returned instead). */
static long __stdcall hk_dstate(PV dev, u32 cb, void *data) {
  DIHK *h = dihk_find(dev); long hr; u32 i;
  if (!h) return (long)0x80004005;
  hr = ((long (__stdcall *)(PV, u32, void *))h->st)(dev, cb, data);
  kbi_st_calls++;
  if (hr >= 0 && ov_on && data) for (i = 0; i < cb; i++) ((volatile u8 *)data)[i] = 0;
  if (kbi_on && cb == 256 && data) { u32 m = kbi_mask; volatile u8 *k = (volatile u8 *)data;
    if (hr < 0) { for (i = 0; i < 256; i++) k[i] = 0; hr = 0; kbi_fixed++; }
    for (i = 0; i < KB_N; i++) if (m & (1u << i)) k[(KSC[i] & 0x7Fu) | (KEXT[i] ? 0x80u : 0u)] = 0x80;
    if ((i32)(GetTickCount() - kbi_f2_until) < 0) k[0x3C] = 0x80;
    if (kbi_extra_dik && (i32)(GetTickCount() - kbi_extra_until) < 0) k[kbi_extra_dik & 0xFFu] = 0x80;
    if (m) kbi_writes++; }
  return hr;
}
static long __stdcall hk_ddata(PV dev, u32 cbo, void *dod, u32 *cnt, u32 fl) {
  DIHK *h = dihk_find(dev); long hr;
  if (!h) return (long)0x80004005;
  hr = ((long (__stdcall *)(PV, u32, void *, u32 *, u32))h->dt)(dev, cbo, dod, cnt, fl);
  kbi_dt_calls++; if (hr >= 0 && cnt && dod && *cnt) kbi_dt_events += *cnt;
  if (hr >= 0 && ov_on && cnt) *cnt = 0;
  return hr;
}
/* v2.5b: in a window that is not active DirectInput refuses Acquire and the game then does not read the keyboard at all;
 * for the host of the one-PC test a refused Acquire is reported as success, so the game goes on to GetDeviceState (whose refusal is replaced above). */
static long __stdcall hk_acquire(PV dev) {
  DIHK *h = dihk_find(dev); long hr;
  if (!h) return (long)0x80004005;
  hr = ((long (__stdcall *)(PV))h->acq)(dev); kbi_acq_calls++;
  if (hr < 0) { kbi_acq_fail++; if (kbi_on) hr = 0; }
  return hr;
}
static void *dihk_orig_create;
static long __stdcall hk_dicreate(PV di, const void *guid, PV *ppdev, PV outer) {
  long hr = ((long (__stdcall *)(PV, const void *, PV *, PV))dihk_orig_create)(di, guid, ppdev, outer);
  if (hr >= 0 && ppdev && *ppdev && !dihk_find(*ppdev) && dihk_n < 6) {
    void **vt = VT(*ppdev); u32 old;
    if (VirtualProtect(&vt[7], 16, 4, &old)) { DIHK *h = &dihk[dihk_n]; h->vt = (u32)vt; h->st = vt[9]; h->dt = vt[10]; h->acq = vt[7]; vt[9] = (void *)hk_dstate; vt[10] = (void *)hk_ddata; vt[7] = (void *)hk_acquire; VirtualProtect(&vt[7], 16, old, &old); dihk_n++; }
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
/* ---------------- v2.2c: second copy of the game on the same PC (file teracoop_multi.txt in the game folder) ----------------
 * WinMain refuses a second copy: FUN_0067ce60 creates the mutex "TTALES-{<title>}" and quits when it already exists, unless the game's own
 * flag DAT_0157c840 is set (its command-line option 0x44 sets it; the option's text is not known). dinput8.dll is in the import table of the exe,
 * so this DllMain runs before WinMain and can set the flag. The flag has exactly one reader (0x0067ceb3) and one writer (0x0067d647). */
#define VA_MULTI_OK 0x0157c840u
static void multi_allow(void) {
  u16 p[420]; int n; u32 b = (u32)GetModuleHandleW(0), pe, ts, soi; static const u16 f[] = L"teracoop_multi.txt"; int k;
  pe = *(const u32 *)(b + 0x3c); ts = *(const u32 *)(b + pe + 8); soi = *(const u32 *)(b + pe + 24 + 56);
  if (ts != TIMESTAMP || soi != SIZEOFIMAGE) return;
  n = (int)GetModuleFileNameW(0, p, 380); while (n > 0 && p[n - 1] != '\\') n--;
  for (k = 0; f[k]; k++) p[n + k] = f[k]; p[n + k] = 0;
  if (GetFileAttributesW(p) == 0xFFFFFFFFu) return;
  *(volatile u32 *)(b - IMAGEBASE + VA_MULTI_OK) = 1; multi_set = 1;
}
int __stdcall DllMain(PV h, u32 reason, PV res) {
  (void)res;
  if (reason == 1) { DisableThreadLibraryCalls(h); multi_allow(); AddVectoredExceptionHandler(1, exc_handler); CreateThread(0, 0, worker, 0, 0, 0); }
  return 1;
}
