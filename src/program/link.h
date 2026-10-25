/* Local link (version 5) between the game module (dinput8.dll inside the game) and the program "TeraCoop": one shared-memory block (the program makes it 4096).
 * The program creates it, the module opens it. Same user, same Windows session, both 32-bit. All fields are 32-bit. */
#define LINK_NAME L"Local\\TeraCoopLink"
#define LINK_MAGIC 0x4D485A5Au /* 'ZZHM' */
#define LINK_VERSION 5u
#define ST_BUILD_OK 1u   /* the game build is the supported one */
#define ST_FOREGROUND 2u /* the game window is the active window */
#define ST_P2_ACTIVE 4u  /* the second player is in the game */
#define ST_INJECTING 8u  /* the module is holding keys for player 2 right now */
#define ST_TEST_WAIT 16u /* a check is waiting for the game window to become active */
#define ST_TEST_RUN 32u  /* a check is running */
#define BT_ACTION 1u
#define BT_SPECIAL 2u
#define BT_JUMP 4u
#define EN_MAX 24u
#define EN_VALID 1u /* the entry exists */
#define EN_ALIVE 2u /* the enemy is still in the host's world (0 = gone: died or the level was left) */
typedef struct { u32 flags; i32 x, y, z, yaw; i32 sx, sz; u32 type_epoch; u32 tick; } EnPkt; /* type_epoch: low 16 bits the enemy type (node+0x20), high 16 bits the epoch (grows at every new level); tick: when this entry was updated (module: its own tick, program: when the packet arrived) */
typedef struct {
  u32 magic, version;
  /* program -> module */
  u32 prog_tick;   /* GetTickCount of the program, refreshed every 25 ms; the module lets go of all keys when it gets old */
  u32 want_p2;     /* 1 = join the second player (the module presses the game's own F2 while player 2 is absent) */
  u32 inject;      /* 1 = hold the player-2 keys that match ax, ay, buttons */
  i32 ax, ay;      /* -100..100, x to the right, y up */
  u32 buttons;     /* BT_* */
  u32 test_seq, test_kind; /* check request: test_seq grows by one per request; kinds: 1 join player 2, 2 move right, 3 jump, 4 action */
  /* module -> program */
  u32 plug_tick, plug_state, plug_ver, f2_presses, keys_down, test_done_seq, test_result; /* test_result: 1 ok, 2 failed */
  /* module -> program: what the local player 1 is doing right now (read from the game memory, about every 10 ms) */
  u32 cap_tick, cap_valid; /* cap_valid: 1 = player 1 found in the game */
  i32 cap_ax, cap_ay;      /* -100..100, x to the right, y up (the same convention as ax, ay above) */
  u32 cap_buttons;         /* BT_* held right now */
  /* version 3: where player 1 is standing (world position and heading, read from the game memory) */
  u32 cap_pos_valid;       /* 1 = the four numbers below are good */
  float cap_px, cap_py, cap_pz, cap_yaw; /* ch+0x190, +0x194, +0x198 and the heading angle ch+0x1b4 (radians) */
  /* version 3, program -> module: where the friend is standing (the module moves player 2 there, with smoothing) */
  u32 rem_tick;            /* GetTickCount when the newest sample arrived (the module ignores it when older than 500 ms) */
  u32 rem_valid;
  float rem_px, rem_py, rem_pz, rem_yaw;
  /* version 4: the world (enemies). EN_MAX enemies at most. Positions in millimetres, heading in milliradians. */
  u32 role;                /* program -> module: 0 not in a room, 1 host (the game's enemies are the truth), 2 guest (the enemies follow the host) */
  u32 en_out_tick, en_out_n; /* module -> program (host only): GetTickCount of the newest enemy table and how many entries are valid */
  EnPkt en_out[EN_MAX];    /* indexed by the enemy id the host gave it */
  u32 en_in_tick;          /* program -> module (guest only): GetTickCount when the newest enemy packet arrived */
  EnPkt en_in[EN_MAX];
  /* version 5 */
  u32 room_role;           /* program -> module: 0 not in a room, 1 host, 2 guest - as soon as the room exists (role above needs the friend to be present too); only used to name the log files */
} Link;
