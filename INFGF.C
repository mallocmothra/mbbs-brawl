/***************************************************************************
 *  GUNFIGHTER, AKA BRAWL.                                                 *
 *                                                                         *
 *  Lost source, reconstructed from thin air and sweat.                    *
 *  This is version 2.0A, which is pre-worldgroup.                         *
 *                                                                         *
 *  Disassembled, decompiled, and beautified by mallocmothra.              *
 *                                                                         *
 *  Thanks to the original game authors - loved this classic!              *
 *  Thanks to the MBBSEmu crew and romhacking.net discord.                 *
 *  Thanks to the numerous sources that still host 16-bit PC info.         *
  ***************************************************************************/


#include "gcomm.h"
#include "majorbbs.h"
#include "infgfmsg.h"
#undef LEVEL6



/*******************************************************************************
 *  type definitions and type-specific constants.                              *
  ******************************************************************************/


#define BRAWL_HOLSTER_COUNT (3)
#define BRAWL_AIM_TABLE_COUNT (7)


typedef enum BRAWL_PLAYER_CONFIG_OPTION {
    BRAWL_PLAYER_CONFIG_OPTION_NONE = 0,
    BRAWL_PLAYER_CONFIG_OPTION_DISABLE_BRAWL = 1,
} BRAWL_PLAYER_CONFIG_OPTION;

typedef enum BRAWL_GAME_STATE {
    BRAWL_GAME_STATE_SALOON_IS_OPEN = 0,
    BRAWL_GAME_STATE_ROUND_IN_PROGRESS = 1,
    BRAWL_GAME_STATE_SHERIFF_IS_HERE = 2,
    BRAWL_GAME_STATE_SALOON_IS_CLOSED = 4,
} BRAWL_GAME_STATE;

typedef enum BRAWL_WEAPON {
    BRAWL_WEAPON_COLT45 = 0,
    BRAWL_WEAPON_REVOLVER = 1,
    BRAWL_WEAPON_SNEAKGUN = 2,
    BRAWL_WEAPON_PISTOL = 3,
    BRAWL_WEAPON_SHOTGUN = 4,
    BRAWL_WEAPON_GATLINGGUN = 5,
    BRAWL_WEAPON_BOTTLE = 6,
    BRAWL_WEAPON_CHAIR = 7,
    BRAWL_WEAPON_DYNAMITE = 8,
    BRAWL_WEAPON_COUNT = 9,
    BRAWL_WEAPON_NONE = -1,
} BRAWL_WEAPON;

typedef enum BRAWL_COMMAND {
    BRAWL_COMMAND_INV = 0,
    BRAWL_COMMAND_AIM = 1,
    BRAWL_COMMAND_SHOOT = 2,
    BRAWL_COMMAND_DRAW = 3,
    BRAWL_COMMAND_VIEW = 4,
    BRAWL_COMMAND_HOLSTER = 5,
    BRAWL_COMMAND_DROP = 6,
    BRAWL_COMMAND_LOAD = 7,
    BRAWL_COMMAND_GRAB = 8,
    BRAWL_COMMAND_ESC = 9,
    BRAWL_COMMAND_CALLOUT = 10,
    BRAWL_COMMAND_BRAWL = 11,
    BRAWL_COMMAND_HIT = 12,
    BRAWL_COMMAND_LIGHT = 13,
    BRAWL_COMMAND_THROW = 14,
    BRAWL_COMMAND_DEFUSE = 15,
    BRAWL_COMMAND_HELP = 16,
    BRAWL_COMMAND_COUNT = 17,
    BRAWL_COMMAND_NONE = -1,
} BRAWL_COMMAND;

typedef enum BRAWL_PLAYER_STATE {
    BRAWL_PLAYER_STATE_BRAWLING = 1,
    BRAWL_PLAYER_STATE_DEAD = 2,
    BRAWL_PLAYER_STATE_ESCAPED = 3,
    BRAWL_PLAYER_STATE_NOT_BRAWLING = -1,
} BRAWL_PLAYER_STATE;

typedef enum BRAWL_PLAYER_RANK {
    BRAWL_PLAYER_RANK_GREENHORN = 0,
    BRAWL_PLAYER_RANK_BORDER_RIDER = 1,
    BRAWL_PLAYER_RANK_SHARPSHOOTER = 2,
    BRAWL_PLAYER_RANK_GUNSLINGER = 3,
    BRAWL_PLAYER_RANK_COUNT = 4,
} BRAWL_PLAYER_RANK;

typedef enum BRAWL_DYNAMITE_STATE {
    BRAWL_DYNAMITE_STATE_SPAWNED = 0,
    BRAWL_DYNAMITE_STATE_LIT = 1,
    BRAWL_DYNAMITE_STATE_NOT_SPAWNED = -1,
} BRAWL_DYNAMITE_STATE;

typedef struct brawl_player_holster_data {
    enum BRAWL_WEAPON weapon;
    int ammo_in_weapon;
} brawl_player_holster_data;

typedef struct brawl_player_data {
    int player_index;
    enum BRAWL_PLAYER_STATE player_state;
    int current_hp;
    int callout_update_countdown_timer;
    int aim_update_timer;
    int aim_target_player_index;
    enum BRAWL_COMMAND queued_command;
    int queued_command_update_countdown_timer;
    int queued_weapon_to_grab_or_draw;
    enum BRAWL_WEAPON weapon_in_hand;
    int weapon_in_hand_ammo;
    int fatigue_points;
    int resting_status_update_countdown_timer;
    char name_of_last_killer[UIDSIZ];  // Can only really be seen when dead, but isn't cleared on round start
    int shots_fired_this_round;
    int shots_landed_this_round;
    int bounty;
    int kill_count;
    int money_held;
    struct brawl_player_holster_data holsters[BRAWL_HOLSTER_COUNT];
    char userid[UIDSIZ];
    unsigned int config_option_flags;
    BRAWL_PLAYER_RANK rank;
    int banked_money_held;
    int banked_kill_count;
    struct brawl_player_holster_data banked_holsters[BRAWL_HOLSTER_COUNT];
    unsigned char unused_padding[20];
} brawl_player_data;

typedef struct brawl_command_table_entry {
    char* command_name;
    int time_to_escape_if_interrupted;  // I think this value is bugged, and basically does nothing. See use of it in code below. TODO: Verify
    int can_do_when_dead_or_escaped_or_resting;
} brawl_command_table_entry;

typedef struct brawl_weapon_table_entry {
    char* weapon_name;
    char* weapon_description;
    int time_to_draw;
    int time_to_holster;
    int time_to_grab;
    int time_to_load;
    int ammo_capacity;
    int shoot_damage_weak;
    int shoot_damage_regular;
    int shoot_damage_crit;
    int max_hit_or_throw_damage_inclusive;
    int fatigue_cost_for_hit_or_throw;
    int is_destroyed_on_hit_or_throw;
    int spawn_chance;
    int max_spawn_count;
} brawl_weapon_table_entry;

typedef struct brawl_aim_table_entry {
    char* description;
    int min_aim_timer_value;
    int percent_chance_to_hit;
    int max_roll_weak_hit;
    int max_roll_regular_hit;
    int max_roll_crit;
} brawl_aim_table_entry;

typedef struct brawl_arena_state_t {
    enum BRAWL_GAME_STATE game_state;
    int sheriff_arrive_update_countdown_timer;
} brawl_arena_state_t;



/***************************************************************************
 *  constants.                                                             *
  ***************************************************************************/


#define BRAWL_HP_MAX (10)
#define BRAWL_HP_MIN_GOOD_SHAPE (7)
#define BRAWL_HP_MIN_HANGING_IN_THERE (4)

#define BRAWL_KILLS_UNTIL_OUTLAW_THRESHOLD (3)
#define BRAWL_BOUNTY_PER_KILL_AS_OUTLAW (50)
#define BRAWL_KILLS_UNTIL_MEANEST_THRESHOLD (5)
#define BRAWL_MONEY_UNTIL_RICHEST_THRESHOLD (101)
#define BRAWL_HP_DEADEST_THRESHOLD (-3)

#define BRAWL_TIME_TO_SHERIFF_MAX_RANDOM_TIME_TO_APPEND (9)
#define BRAWL_TIME_TO_SHERIFF_MAX_WHEN_ALL_BRAWLERS_DEAD_OR_ESCAPED (6)

#define BRAWL_STARTING_WEAPON (BRAWL_WEAPON_PISTOL)
#define BRAWL_STARTING_WEAPON_AMMO (6)

#define BRAWL_WEAPON_COUNT_MAX_TO_SPAWN_AT_ROUND_START (7)
#define BRAWL_WEAPON_COUNT_MIN_TO_SPAWN_AT_ROUND_START (2)
#define BRAWL_WEAPON_MID_ROUND_DESPAWN_COUNT_THRESHOLD (3)
#define BRAWL_WEAPON_MID_ROUND_CHANCE_DESPAWN (15)
#define BRAWL_WEAPON_MID_ROUND_SPAWN_COUNT_LIMIT (7)
#define BRAWL_WEAPON_MID_ROUND_CHANCE_SPAWN (20)
#define BRAWL_WEAPON_SPAWN_REROLL_COUNT_MAX (4)

#define BRAWL_CHANCE_TO_LAND_HIT_OR_THROW (70)
#define BRAWL_CHANCE_TO_LAND_SHOT_MAX (85)
#define BRAWL_DAMAGE_THRESHOLD_TO_TRIGGER_STUN (2)
#define BRAWL_FATIGUE_THRESHOLD_TO_TRIGGER_RESTING (5)
#define BRAWL_RESTING_STATUS_TIMER_ADDED_WHEN_FATIGUE_THRESHOLD_HIT (4)
#define BRAWL_THROW_MAX_RANDOM_FATIGUE_PENALTY (1)

#define BRAWL_BAREHANDED_DAMAGE_FOR_HIT_OR_THROW (2)
#define BRAWL_BAREHANDED_FATIGUE_FOR_HIT_OR_THROW (1)
#define BRAWL_GATLINGGUN_FIXED_AIM_TIMER_VALUE (3)
#define BRAWL_DYNAMITE_MIN_RANDOM_TIME_ON_LIGHT (3)
#define BRAWL_DYNAMITE_MAX_RANDOM_TIME_ON_LIGHT (9)
#define BRAWL_DYNAMITE_DIRECT_DAMAGE_MIN (4)
#define BRAWL_DYNAMITE_DIRECT_DAMAGE_MAX (8)
#define BRAWL_DYNAMITE_SPLASH_DAMAGE_RATIO (3)

#define BRAWL_BORDER_RIDER_GRAB_TIME_BONUS (2)
#define BRAWL_SHARPSHOOTER_CHANCE_TO_LAND_SHOT_BONUS (15)
#define BRAWL_GUNSLINGER_DRAW_TIME_BONUS (2)

#define BRAWL_NOTIFY_STILL_UNDER_AIM_THREAT_EVERY_NTH_UPDATE (2)
#define BRAWL_NOTIFY_DYNAMITE_STILL_LIT_EVERY_NTH_UPDATE (2)
#define BRAWL_NOTIFY_SHERIFF_ARRIVING_AT_TIMER_VALUE (5)

#define BRAWL_UPDATE_FREQUENCY_IN_SECONDS (5)
#define BRAWL_ADVERTISE_EVERY_NTH_USER_LOGIN (4)
#define BRAWL_DURATION_OF_MODULE_TRIAL_PERIOD_IN_DAYS (14)
#define BRAWL_HACK_OFFSET_TO_STASH_DAYS_MODULE_ACTIVE (2)
#define BRAWL_INITIAL_PLAYER_USERID_DB_FILE_LENGTH (70)

#define TELECONFERENCE_USER_SUBSTATE_ENTERING_TELECONFERENCE (0)
#define TELECONFERENCE_USER_SUBSTATE_NORMAL (1)
#define TELECONFERENCE_USER_SUBSTATE_PRIVATE_CHAT (2)


const struct brawl_command_table_entry brawl_command_table[BRAWL_COMMAND_COUNT] = {
    // command_name  time_to_escape_if_interrupted  can_do_when_dead_or_escaped_or_resting
    { "INV",      0, 1 },
    { "AIM",      0, 0 },
    { "SHOOT",    0, 0 },
    { "DRAW",    99, 0 },
    { "VIEW",     0, 1 },
    { "HOLSTER", 99, 0 },
    { "DROP",     0, 0 },
    { "LOAD",     3, 0 },
    { "GRAB",    99, 0 },
    { "ESC",      3, 0 },
    { "CALLOUT",  0, 0 },
    { "BRAWL",    0, 1 },
    { "HIT",      0, 0 },
    { "LIGHT",    0, 0 },
    { "THROW",    0, 0 },
    { "DEFUSE",   0, 0 },
    { "HELP",     0, 1 },
};

const struct brawl_weapon_table_entry brawl_weapon_table[BRAWL_WEAPON_COUNT] = {
    // weapon_name  weapon_description
    // time_to_draw  time_to_holster  time_to_grab  time_to_load
    // ammo_capacity  shoot_damage_weak  shoot_damage_regular  shoot_damage_crit
    // max_hit_or_throw_damage_inclusive  fatigue_cost_for_hit_or_throw  is_destroyed_on_hit_or_throw
    // spawn_chance  max_spawn_count
    { "COLT45",     "Colt 45",                    2,  1, 3,  3,    6, 1, 3, 4,   2, 2, 0,    10, 2, },
    { "REVOLVER",   "Smith & Wesson revolver",    1,  1, 3,  3,    6, 1, 2, 4,   2, 2, 0,    20, 2, },
    { "SNEAKGUN",   "sneak gun",                  0,  1, 3,  3,    5, 0, 1, 3,   2, 2, 0,    30, 2, },
    { "PISTOL",     "pistol",                     2,  1, 3,  3,    6, 1, 2, 3,   2, 2, 0,    40, 2, },
    { "SHOTGUN",    "double-barrelled shotgun",   3,  2, 4,  4,    2, 2, 3, 6,   3, 3, 0,    50, 2, },
    { "GATLINGGUN", "Gatling gun!",               0, -1, 3, 10,   10, 1, 1, 1,   3, 5, 0,    60, 1, },
    { "BOTTLE",     "whiskey bottle",             0, -1, 0, -1,    0, 0, 0, 0,   2, 1, 1,    80, 4, },
    { "CHAIR",      "chair",                      0, -1, 2, -1,    0, 0, 0, 0,   5, 5, 1,    90, 1, },
    { "DYNAMITE",   "dynamite",                   0, -1, 2, -1,    0, 0, 0, 0,   1, 1, 0,   100, 1, },
};

const struct brawl_aim_table_entry brawl_aim_table[BRAWL_AIM_TABLE_COUNT] = {
    // description  min_aim_timer_value  percent_chance_to_hit  max_roll_weak_hit  max_roll_regular_hit  max_roll_crit
    { "lousy",          1, 15, 80, 100, 101, },
    { "awright",        2, 30, 60,  90, 100, },
    { "good",           3, 45, 40,  80, 100, },
    { "darn good",      4, 60, 20,  70, 100, },
    { "deadeye",        5, 75, 10,  60, 100, },
    { "crackshot",      6, 70,  0,  50, 100, },
    { "getting shakey", 7, 65,  0,  40, 100, },
};

const char* brawl_player_rank_names[BRAWL_PLAYER_RANK_COUNT] = {
    "Greenhorn",
    "Border Rider",
    "Sharpshooter",
    "Gunslinger",
};



/***************************************************************************
 *  global variables.                                                      *
  ***************************************************************************/


// These forward declarations are here only so the table can be filled.
// Put other forward declarations below.
int brawl_handle_user_login(void);
int brawl_handle_user_input(void);
void brawl_handle_user_disconnect(void);
void brawl_handle_midnight_cleanup(void);
void brawl_handle_user_delete_account(char* user_id);
void brawl_handle_system_shutdown(void);


struct module brawl_module = {
    { 0 },
    brawl_handle_user_login,           // user log-on suplement
    brawl_handle_user_input,           // input routine if selected
    dfsthn,                            // status-input routine
    NULL,                              // "injoth" routine for this module
    NULL,                              // user logoff supplemental routine
    brawl_handle_user_disconnect,      // hang-up routine
    brawl_handle_midnight_cleanup,     // midnight clean-up
    brawl_handle_user_delete_account,  // delete account routine
    brawl_handle_system_shutdown,      // system shut-down routine
};

unsigned char brawl_advertisement_login_counter = 0;
int brawl_module_index = 0;

BRAWL_DYNAMITE_STATE brawl_dynamite_state = 0;
int brawl_dynamite_target_player_index = 0;
int brawl_dynamite_update_countdown_timer = 0;

int brawl_current_random_message_num = 0;
int brawl_weapon_current_lying_on_floor_count[BRAWL_WEAPON_COUNT] = { 0 };

struct brawl_player_data* brawl_current_aim_target_ptr = NULL;   // Temporary data pointer for the player that is currently being aimed at. Why this isn't a local variable? beats me!
struct brawl_player_data* brawl_current_player_data_ptr = NULL;  // Temporary data pointer for the player currently being updated. It needs to be set before some functions get called. Other functions set it themselves
struct brawl_player_data* brawl_player_data_array = NULL;        // Array of all player data (made w/ malloc)

int brawl_config_is_game_advertisement_enabled = 0;  // (BBCALL from INFGFMSG.MSG)   Whether or not to advertise brawl to users when they log on the BBS
int brawl_config_player_default_brawl_on_value = 0;  // (BBBRWLON from INFGFMSG.MSG) The default "brawl on" value when users enter teleconference
int brawl_config_time_sheriff_stay = 0;              // (BBSHFLEN from INFGFMSG.MSG) Time until the sheriff leaves after arriving (in updates. see BRAWL_UPDATE_FREQUENCY_IN_SECONDS)
int brawl_config_time_to_sheriff_arrival = 0;        // (BBGAMLEN from INFGFMSG.MSG) Time until the sheriff comes after a brawl starts (in updates. see BRAWL_UPDATE_FREQUENCY_IN_SECONDS)
int brawl_config_time_between_pages_to_draw = 0;     // (BBCALLIV from INFGFMSG.MSG) Time between callouts of other users to play brawl (in seconds)

int brawl_unused_1 = 0;  // Completely unused. Not read or written

struct brawl_arena_state_t brawl_arena_state = {
    BRAWL_GAME_STATE_SALOON_IS_OPEN,  // game_state
    0,                                // time_to_sheriff
};

BTVFILE* brawl_btrieve_db_player_userid_file = NULL;  // Database for saving player user ids
FILE* brawl_mcv_config_file = NULL;                   // Binary config file, compiled from INFGFMSG.MSG
int teleconference_module_index = 0;                  // Module index of the TC module, which this module will patch in an input overlay on top of



/***************************************************************************
 *  function forward declarations.                                         *
  ***************************************************************************/


void brawl_callout_player(void);
int brawl_can_aim_weapon(void);
int brawl_can_defuse_dynamite(void);
int brawl_can_draw_weapon(void);
int brawl_can_drop_weapon(void);
int brawl_can_grab_weapon(void);
int brawl_can_hit_or_throw(void);
int brawl_can_holster_weapon(void);
int brawl_can_light_dynamite(void);
int brawl_can_load_weapon(void);
int brawl_can_shoot(void);
void brawl_cancel_aim_and_queued_command(struct brawl_player_data* target_player_data_ptr);
int brawl_check_do_action_and_maybe_start_brawl(BRAWL_COMMAND command);
void brawl_clear_player_queued_command_and_timer(struct brawl_player_data* target_player_data_ptr);
void brawl_create_player_data(void);
void brawl_despawn_most_common_weapon(void);
void brawl_do_damage_for_hit_or_throw(int damage_amount);
void brawl_do_damage_for_shoot(int param_1);
int brawl_find_module_index_by_name(char* module_name);
BRAWL_WEAPON brawl_find_weapon_index_by_name(const char* weapon_name);
int brawl_get_aim_table_index_for_timer_value(int aim_timer);
int brawl_get_fatigue_cost_for_hit_or_throw(void);
const char* brawl_get_hp_description(int hit_points);
int brawl_get_user_count_matching_name(char* name_to_match);
const char* brawl_get_weapon_name(BRAWL_WEAPON weapon);
void brawl_handle_brawl_or_help_command(void);
void brawl_handle_death(struct brawl_player_data* victim, const char* name_of_last_killer);
void brawl_handle_defuse_dynamite(void);
void brawl_handle_draw_weapon(void);
void brawl_handle_drop_weapon(void);
void brawl_handle_exploding_dynamite(void);
int brawl_handle_grab_weapon(void);
void brawl_handle_hit_or_throw(void);
void brawl_handle_light_dynamite(void);
void brawl_handle_print_inventory(void);
void brawl_handle_sheriff_arrives(void);
void brawl_handle_shoot(void);
int brawl_handle_user_input_overlay_shim(void);
void brawl_handle_view_brawlers(void);
int brawl_handle_view_command(void);
void brawl_handle_view_items(void);
void brawl_init_teleconference_overlay(void);
void brawl_output_message_to_all_brawlers(void);
void brawl_output_message_to_all_but_current_and_target_brawlers(int user_index);
void brawl_output_message_to_specified_brawler(int user_index);
BRAWL_WEAPON brawl_pick_random_weapon_to_spawn(void);
void brawl_print_aim_message_for_aimer_and_target(void);
void brawl_print_round_result(void);
void brawl_queue_new_command(void);
void brawl_reset_player_data_for_round(int user_index);
int brawl_roll_and_check_for_shot_hit(int aim_table_index);
int brawl_roll_and_get_damage_for_aim(int aim_table_index, BRAWL_WEAPON weapon);
int brawl_roll_and_get_damage_for_hit_or_throw(void);
void brawl_score_kill_and_collect_bounty(struct brawl_player_data* victim, struct brawl_player_data* killer);
void brawl_spawn_random_weapon(void);
void brawl_update(void);
void brawl_update_weapon_spawns(void);



/***************************************************************************
 *  function implementations.                                              *
  ***************************************************************************/


void EXPORT init__brawl(void) {
    int brawl_player_data_array_size;

    stzcpy(brawl_module.descrp, gmdnam("INFGF.MDF"), MNMSIZ);
    brawl_module_index = register_module(&brawl_module);
    brawl_mcv_config_file = opnmsg("infgfmsg.mcv");
    brawl_btrieve_db_player_userid_file = opnbtv("infgf.dat", BRAWL_INITIAL_PLAYER_USERID_DB_FILE_LENGTH);

    brawl_player_data_array = (brawl_player_data*)alcmem(
        brawl_player_data_array_size = nterms * sizeof(struct brawl_player_data));  // Doing inline to match original binary
    setmem((char*)brawl_player_data_array, brawl_player_data_array_size, 0);

    setmem((char*)&brawl_arena_state, sizeof(brawl_arena_state_t), 0);
    brawl_arena_state.game_state = BRAWL_GAME_STATE_SALOON_IS_OPEN;
    if (ynopt(BBCLOSED)) {
        brawl_arena_state.game_state = BRAWL_GAME_STATE_SALOON_IS_CLOSED;
    }

    brawl_config_is_game_advertisement_enabled = ynopt(BBCALL);
    brawl_config_player_default_brawl_on_value = ynopt(BBBRWLON);
    brawl_config_time_between_pages_to_draw = numopt(BBCALLIV, 0, 32767);
    brawl_config_time_to_sheriff_arrival = numopt(BBGAMLEN, 1, 32767);
    brawl_config_time_sheriff_stay = numopt(BBSHFLEN, 1, 32767);

    rtkick(1, brawl_init_teleconference_overlay);
    rtkick(3, brawl_update);
}


void brawl_init_teleconference_overlay(void) {
    // patch the teleconference module, so its user input goes to brawl instead.
    // if brawl decides it cannot handle the input, it will feed it on to teleconference.
    if ((teleconference_module_index = brawl_find_module_index_by_name("Teleconference")) < 0) {  // Doing inline to match original binary
        catastro("Gunfighter. Couldn\'t overlay teleconference");
    }

    shocst("Gunfighter 2.0a-TD loaded", "Overlaying teleconference %d)", teleconference_module_index);

    module[brawl_module_index]->sttrou = module[teleconference_module_index]->sttrou;
    module[teleconference_module_index]->sttrou = brawl_handle_user_input_overlay_shim;
}


int brawl_find_module_index_by_name(char* module_name) {
    int current_module_index;

    for (current_module_index = 1; current_module_index < nmods; ++current_module_index) {
        if (sameto(module_name, module[current_module_index]->descrp)) {
            return current_module_index;
        }
    }

    return -1;
}


void brawl_spawn_weapons_and_set_bounty(void) {
    int index;
    struct brawl_player_data* current_player_data_ptr;
    int max_seen_player_kill_count_player_index;
    int max_seen_player_kill_count;
    int current_player_kill_count;
    int weapons_to_spawn_count;

    max_seen_player_kill_count_player_index = -1;
    max_seen_player_kill_count = -1;
    brawl_dynamite_state = BRAWL_DYNAMITE_STATE_NOT_SPAWNED;
    brawl_dynamite_target_player_index = -1;

    for (index = 0; index < BRAWL_WEAPON_COUNT; index++) {
        brawl_weapon_current_lying_on_floor_count[index] = 0;
    }

    weapons_to_spawn_count = rand() % (BRAWL_WEAPON_COUNT_MAX_TO_SPAWN_AT_ROUND_START + 1);

    if (weapons_to_spawn_count < BRAWL_WEAPON_COUNT_MIN_TO_SPAWN_AT_ROUND_START) {
        weapons_to_spawn_count = BRAWL_WEAPON_COUNT_MIN_TO_SPAWN_AT_ROUND_START;
    }

    if (0 < weapons_to_spawn_count) {
        for (index = 0; index < weapons_to_spawn_count; ++index) {
            brawl_spawn_random_weapon();
        }
    }

    for (index = 0, current_player_data_ptr = brawl_player_data_array; index < nterms; ++index, ++current_player_data_ptr) {
        if ((current_player_kill_count = current_player_data_ptr->kill_count) > max_seen_player_kill_count) {  // Doing inline to match original binary
            max_seen_player_kill_count = current_player_kill_count;
            max_seen_player_kill_count_player_index = index;
        }
    }

    if (BRAWL_KILLS_UNTIL_OUTLAW_THRESHOLD < max_seen_player_kill_count) {
        brawl_player_data_array[max_seen_player_kill_count_player_index].bounty =
            (brawl_player_data_array[max_seen_player_kill_count_player_index].kill_count - BRAWL_KILLS_UNTIL_OUTLAW_THRESHOLD) * BRAWL_BOUNTY_PER_KILL_AS_OUTLAW;

        prfmsg(BBWANTED, brawl_player_data_array[max_seen_player_kill_count_player_index].userid, brawl_player_data_array[max_seen_player_kill_count_player_index].bounty);
        brawl_output_message_to_all_brawlers();
    }
}


int brawl_handle_user_login(void) {
    int advertise_every_nth_user_login;

    advertise_every_nth_user_login = BRAWL_ADVERTISE_EVERY_NTH_USER_LOGIN;

    setbtv(brawl_btrieve_db_player_userid_file);
    brawl_current_player_data_ptr = &brawl_player_data_array[usrnum];

    if (strncmp(usaptr->userid, brawl_current_player_data_ptr->userid, UIDSIZ) != 0) {
        setmem(brawl_current_player_data_ptr, sizeof(brawl_player_data), 0);

        if (!acqbtv(&brawl_current_player_data_ptr->userid, usaptr->userid, 0)) {  // Check if the player userid record exists for this user
            brawl_create_player_data();
            insbtv(&brawl_current_player_data_ptr->userid);  // if not, then create it
        }

        brawl_reset_player_data_for_round(usrnum);

        if (brawl_config_is_game_advertisement_enabled && ++brawl_advertisement_login_counter % advertise_every_nth_user_login == 0) {
            // send advertisement to user
            brawl_advertisement_login_counter = 0;
            setmbk(brawl_mcv_config_file);
            prfmsg(BRAWLLOG);
            outprf(usrnum);
            prf("\0");
        }
    }

    return 0;
}


void brawl_create_player_data(void) {
    int index;

    setmem(brawl_current_player_data_ptr, sizeof(brawl_player_data), 0);
    movmem(usaptr->userid, &brawl_current_player_data_ptr->userid, UIDSIZ);

    if (brawl_config_player_default_brawl_on_value) {
        brawl_current_player_data_ptr->config_option_flags = BRAWL_PLAYER_CONFIG_OPTION_NONE;
    } else {
        brawl_current_player_data_ptr->config_option_flags = BRAWL_PLAYER_CONFIG_OPTION_DISABLE_BRAWL;
    }

    index = 0;
    do {
        brawl_current_player_data_ptr->banked_holsters[index].weapon = BRAWL_WEAPON_NONE;
        brawl_current_player_data_ptr->banked_holsters[index].ammo_in_weapon = -1;
        ++index;
    } while (index < BRAWL_HOLSTER_COUNT);

    brawl_current_player_data_ptr->banked_holsters[0].weapon = BRAWL_STARTING_WEAPON;
    brawl_current_player_data_ptr->banked_holsters[0].ammo_in_weapon = BRAWL_STARTING_WEAPON_AMMO;
}


void brawl_reset_player_data_for_round(int user_index) {
    struct brawl_player_data* current_player_data_ptr;
    int index;

    current_player_data_ptr = &brawl_player_data_array[user_index];
    current_player_data_ptr->player_index = user_index;


    if (current_player_data_ptr->config_option_flags & BRAWL_PLAYER_CONFIG_OPTION_DISABLE_BRAWL) {
        current_player_data_ptr->player_state = BRAWL_PLAYER_STATE_NOT_BRAWLING;
    } else {
        current_player_data_ptr->player_state = BRAWL_PLAYER_STATE_BRAWLING;
    }

    current_player_data_ptr->current_hp = BRAWL_HP_MAX;
    current_player_data_ptr->aim_update_timer = -1;
    current_player_data_ptr->aim_target_player_index = -1;
    current_player_data_ptr->queued_command = BRAWL_COMMAND_NONE;
    current_player_data_ptr->queued_command_update_countdown_timer = -1;
    current_player_data_ptr->queued_weapon_to_grab_or_draw = BRAWL_WEAPON_NONE;
    current_player_data_ptr->weapon_in_hand = BRAWL_WEAPON_NONE;
    current_player_data_ptr->weapon_in_hand_ammo = -1;
    current_player_data_ptr->fatigue_points = 0;
    current_player_data_ptr->resting_status_update_countdown_timer = 0;
    current_player_data_ptr->shots_fired_this_round = 0;
    current_player_data_ptr->shots_landed_this_round = 0;
    current_player_data_ptr->bounty = 0;

    index = 0;
    do {
        current_player_data_ptr->holsters[index].weapon = current_player_data_ptr->banked_holsters[index].weapon;
        current_player_data_ptr->holsters[index].ammo_in_weapon = current_player_data_ptr->banked_holsters[index].ammo_in_weapon;
        ++index;
    } while (index < BRAWL_HOLSTER_COUNT);

    current_player_data_ptr->kill_count = current_player_data_ptr->banked_kill_count;
    current_player_data_ptr->money_held = current_player_data_ptr->banked_money_held;
}


int brawl_handle_user_input_overlay_shim(void) {
    // This should check the `brawl_handle_user_input()` result directly, without ` == 1`, but the original binary didn't do that
    if (usrptr->substt == TELECONFERENCE_USER_SUBSTATE_NORMAL && brawl_handle_user_input() == 1) {
        clrprf();
        prf("");
        return 1;
    }

    return module[brawl_module_index]->sttrou();
}


int brawl_handle_user_input(void) {
    int current_command_index;

    if (margc != 0) {
        brawl_current_player_data_ptr = &brawl_player_data_array[usrnum];

        if (sameas(margv[0], "brawl")) {
            setmbk(brawl_mcv_config_file);
            brawl_handle_brawl_or_help_command();
            return 1;
        } else if (sameas(margv[0], "help")) {
            if (margc == 2 && sameas(margv[1], "brawl")) {
                setmbk(brawl_mcv_config_file);
                brawl_handle_brawl_or_help_command();
                return 1;
            } else {
                return 0;  // Not logically necessary, but the compiler fails to optimize this out, and the original binary had it
            }
        } else {
            current_command_index = BRAWL_COMMAND_INV;

            do {
                if (sameas(margv[0], brawl_command_table[current_command_index].command_name)) {
                    setmbk(brawl_mcv_config_file);

                    // The original code would skip commands if the module had been active on the BBS for more than 14 days
                    // The way this was implemented was a bit hacky. The value is stored in space reserved by the MBBS-API for future use.
                    // This is somewhat fine, since the code could just be upgraded when the BBS is versioned, though no version checks are done in this module.
                    // But it could also potentially conflict with other modules, if they happen to use the same trick with the same offset.

                    // The original unmodified code isn't quite this, as this code is somehow one byte short, but it's probably close to this.
                    // TODO: Figure out the original code, and put that here.
                    // if (sv.spare[BRAWL_HACK_OFFSET_TO_STASH_DAYS_MODULE_ACTIVE] >= BRAWL_DURATION_OF_MODULE_TRIAL_PERIOD_IN_DAYS) {
                    //     prfmsg(LEVEL4);
                    //     outprf(usrnum);
                    //     return 1;
                    // }
                    asm {
                        mov    ax, SEG sv
                        mov    es, ax
                        cmp    byte ptr es:[sv.(struct sysvbl)spare + BRAWL_HACK_OFFSET_TO_STASH_DAYS_MODULE_ACTIVE], BRAWL_DURATION_OF_MODULE_TRIAL_PERIOD_IN_DAYS

                        // This was probably originally `JL(E) skip_trial_timeout_error`, before a binary modification was applied
                        jmp    skip_trial_timeout_error

                        // This was probably originally `PUSH 0`, before a binary modification was applied, which would correspond to a LEVEL4 message
                        jmp    skip_trial_timeout_error
                        db     000h

                        // Can't call from normal C code since the opcode to push its param got clobbered by the binary modification
                        call   far ptr es:prfmsg
                        pop    cx
                    }

                    outprf(usrnum);
                    return 1;
skip_trial_timeout_error:

                    if (!brawl_check_do_action_and_maybe_start_brawl(current_command_index)) {
                        return 1;
                    }

                    switch(current_command_index) {

                    case BRAWL_COMMAND_INV:
                        brawl_handle_print_inventory();
                        return 1;

                    case BRAWL_COMMAND_AIM:
                        if (!brawl_can_aim_weapon()) {
                            return 1;
                        } else {
                            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                            brawl_print_aim_message_for_aimer_and_target();
                            return 1;
                        }

                    case BRAWL_COMMAND_SHOOT:
                        if (brawl_current_player_data_ptr->aim_target_player_index != -1) {
                            brawl_current_aim_target_ptr = &brawl_player_data_array[brawl_current_player_data_ptr->aim_target_player_index];
                        }

                        if (!brawl_can_shoot()) {
                            return 1;
                        } else {
                            brawl_handle_shoot();

                            if (2 <= margc && sameas(margv[1], "DBL") && brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_SHOTGUN) {
                                // This should check the `brawl_can_shoot()` result directly, without ` == 1`, but the original binary didn't do that
                                if (brawl_can_shoot() == 1) {
                                    brawl_handle_shoot();
                                } else {
                                    prf("You must have a shotgun to fire both barrels\r");
                                    brawl_output_message_to_specified_brawler(usrnum);
                                }
                            }

                            brawl_cancel_aim_and_queued_command(brawl_current_player_data_ptr);
                            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                            return 1;
                        }

                    case BRAWL_COMMAND_DRAW:
                        if (!brawl_can_draw_weapon()) {
                            return 1;
                        } else {
                            asm {
                                // This is a JMP queue_new_draw_or_grab_command, but opaque to the compiler
                                // TODO: Figure out how to get this exact code to be spit out without this hack
                                //       In fact, I'd like to just copy/paste the code here and not have a goto at all!
                                DB  0ebh, 00ch
                            }
                        }

                    case BRAWL_COMMAND_GRAB:
                        if (!brawl_can_grab_weapon()) {
                            return 1;
                        } else {
queue_new_draw_or_grab_command:
                            brawl_cancel_aim_and_queued_command(brawl_current_player_data_ptr);
                            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                            brawl_current_player_data_ptr->queued_command = current_command_index;
                            brawl_queue_new_command();
                            return 1;
                        }

                    case BRAWL_COMMAND_HOLSTER:
                        if (!brawl_can_holster_weapon()) {
                            return 1;
                        } else {
                            brawl_cancel_aim_and_queued_command(brawl_current_player_data_ptr);
                            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                            brawl_current_player_data_ptr->queued_command = current_command_index;
                            brawl_current_player_data_ptr->queued_command_update_countdown_timer = brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].time_to_holster;
                            prf("%s is holstering the %s\r", usaptr->userid, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
                            brawl_output_message_to_all_brawlers();
                            return 1;
                        }

                    case BRAWL_COMMAND_VIEW:
                        brawl_handle_view_command();
                        return 1;

                    case BRAWL_COMMAND_DROP:
                        if (!brawl_can_drop_weapon()) {
                            return 1;
                        } else {
                            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                            brawl_cancel_aim_and_queued_command(brawl_current_player_data_ptr);
                            brawl_handle_drop_weapon();
                            return 1;
                        }

                    case BRAWL_COMMAND_LIGHT:
                        if (!brawl_can_light_dynamite()) {
                            return 1;
                        } else {
                            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                            brawl_cancel_aim_and_queued_command(brawl_current_player_data_ptr);
                            brawl_handle_light_dynamite();
                            return 1;
                        }

                    case BRAWL_COMMAND_DEFUSE:
                        if (!brawl_can_defuse_dynamite()) {
                            return 1;
                        } else {
                            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                            brawl_cancel_aim_and_queued_command(brawl_current_player_data_ptr);
                            brawl_handle_defuse_dynamite();
                            return 1;
                        }

                    case BRAWL_COMMAND_THROW:
                        if (!brawl_can_hit_or_throw()) {
                            return 1;
                        } else {
                            if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_DYNAMITE && brawl_dynamite_state == BRAWL_DYNAMITE_STATE_LIT) {
                                brawl_dynamite_target_player_index = -1;
                            }

                            brawl_handle_hit_or_throw();
                            brawl_cancel_aim_and_queued_command(brawl_current_player_data_ptr);
                            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                            return 1;
                        }

                    case BRAWL_COMMAND_LOAD:
                        if (!brawl_can_load_weapon()) {
                            return 1;
                        } else {
                            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                            brawl_cancel_aim_and_queued_command(brawl_current_player_data_ptr);
                            brawl_current_player_data_ptr->queued_command = current_command_index;
                            brawl_current_player_data_ptr->queued_command_update_countdown_timer = brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].time_to_load;
                            prf("%s is reloading the %s\r", usaptr->userid, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
                            brawl_output_message_to_all_brawlers();
                            return 1;
                        }

                    case BRAWL_COMMAND_ESC:
                        brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                        brawl_cancel_aim_and_queued_command(brawl_current_player_data_ptr);
                        brawl_current_player_data_ptr->queued_command = current_command_index;
                        // I think this is a bug in the original code, and this is supposed to use the previous command index.
                        // Right now it uses `current_command_index`, which in this case I think is always going to be "esc"?
                        // TODO: Verify if this actually is always escape, or if it somehow grabs the previous command
                        brawl_current_player_data_ptr->queued_command_update_countdown_timer = brawl_command_table[current_command_index].time_to_escape_if_interrupted;
                        prf("%s is making a break for it!\r", usaptr->userid);
                        brawl_output_message_to_all_brawlers();
                        return 1;

                    case BRAWL_COMMAND_CALLOUT:
                        brawl_callout_player();
                        return 1;

                    case BRAWL_COMMAND_HIT:
                        if (!brawl_can_hit_or_throw()) {
                            return 1;
                        } else {
                            brawl_handle_hit_or_throw();
                            brawl_cancel_aim_and_queued_command(brawl_current_player_data_ptr);
                            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                            return 1;
                        }

                    default:
                        return 1;

                    }
                }

                ++current_command_index;
            } while (current_command_index < BRAWL_COMMAND_COUNT);
        }
    }

    return 0;
}


void brawl_queue_new_command(void) {
    BRAWL_WEAPON input_specified_weapon_type;

    input_specified_weapon_type = brawl_find_weapon_index_by_name(margv[1]);

    switch (brawl_current_player_data_ptr->queued_command) {

    case BRAWL_COMMAND_DRAW:
        brawl_current_player_data_ptr->queued_command_update_countdown_timer = brawl_weapon_table[input_specified_weapon_type].time_to_draw;
        brawl_current_player_data_ptr->queued_weapon_to_grab_or_draw = input_specified_weapon_type;

        if (brawl_current_player_data_ptr->rank == BRAWL_PLAYER_RANK_GUNSLINGER && input_specified_weapon_type != BRAWL_WEAPON_SHOTGUN) {
            brawl_current_player_data_ptr->queued_command_update_countdown_timer = brawl_current_player_data_ptr->queued_command_update_countdown_timer - BRAWL_GUNSLINGER_DRAW_TIME_BONUS;

            if (brawl_current_player_data_ptr->queued_command_update_countdown_timer < 0) {
                brawl_current_player_data_ptr->queued_command_update_countdown_timer = 0;
            }
        }

        if (!brawl_current_player_data_ptr->queued_command_update_countdown_timer) {
            brawl_handle_draw_weapon();
            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
        } else {
            prf("%s is drawing a %s!\r", usaptr, brawl_get_weapon_name(input_specified_weapon_type));
            brawl_output_message_to_all_brawlers();
        }

        break;

    case BRAWL_COMMAND_GRAB:
        brawl_current_player_data_ptr->queued_command_update_countdown_timer = brawl_weapon_table[input_specified_weapon_type].time_to_grab;
        brawl_current_player_data_ptr->queued_weapon_to_grab_or_draw = input_specified_weapon_type;

        if (BRAWL_PLAYER_RANK_BORDER_RIDER <= brawl_current_player_data_ptr->rank) {
            brawl_current_player_data_ptr->queued_command_update_countdown_timer = brawl_current_player_data_ptr->queued_command_update_countdown_timer - BRAWL_BORDER_RIDER_GRAB_TIME_BONUS;

            if (brawl_current_player_data_ptr->queued_command_update_countdown_timer < 0) {
                brawl_current_player_data_ptr->queued_command_update_countdown_timer = 0;
            }
        }

        if (!brawl_current_player_data_ptr->queued_command_update_countdown_timer) {
            brawl_handle_grab_weapon();
            brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
        } else {
            prf("%s is going for the %s\r", usaptr, brawl_get_weapon_name(input_specified_weapon_type));
            brawl_output_message_to_all_brawlers();
        }

        break;

    default:
        return;

    }
}


void brawl_cancel_aim_and_queued_command(struct brawl_player_data* target_player_data_ptr) {
    target_player_data_ptr->aim_update_timer = 0;
    target_player_data_ptr->aim_target_player_index = -1;

    if (target_player_data_ptr->queued_command == BRAWL_COMMAND_AIM) {
        target_player_data_ptr->queued_command = BRAWL_COMMAND_NONE;
    }
}


void brawl_clear_player_queued_command_and_timer(struct brawl_player_data* target_player_data_ptr) {
    target_player_data_ptr->queued_command = BRAWL_COMMAND_NONE;
    target_player_data_ptr->queued_command_update_countdown_timer = 0;
    target_player_data_ptr->queued_weapon_to_grab_or_draw = BRAWL_WEAPON_NONE;
}


void brawl_handle_brawl_or_help_command(void) {
    if (margc < 2) {
        prfmsg(BBBRFMT);
        outprf(usrnum);
        return;
    } else if (sameas(margv[1], "off")) {
        // turning brawl off successfully
        brawl_current_player_data_ptr->config_option_flags |= BRAWL_PLAYER_CONFIG_OPTION_DISABLE_BRAWL;
        brawl_current_player_data_ptr->player_state = BRAWL_PLAYER_STATE_NOT_BRAWLING;
        prfmsg(BBOFF);
        outprf(usrnum);
        return;
    } else if (sameas(margv[1], "on")) {
        if (brawl_current_player_data_ptr->current_hp <= 0) {
            // current player is already dead and cannot act
            prfmsg(BBONDEAD);
            outprf(usrnum);
            brawl_current_player_data_ptr->player_state = BRAWL_PLAYER_STATE_DEAD;
            return;
        } else if (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_ESCAPED) {
            // current player has already fled and cannot act
            prfmsg(BBONESC);
            outprf(usrnum);
            return;
        } else if (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING) {
            // current player is already brawling, and doesn't need to "brawl on" again
            prfmsg(BBONBRWL);
            outprf(usrnum);
            return;
        } else {
            // turning brawl on successfully
            brawl_current_player_data_ptr->config_option_flags &= ~BRAWL_PLAYER_CONFIG_OPTION_DISABLE_BRAWL;
            brawl_current_player_data_ptr->player_state = BRAWL_PLAYER_STATE_BRAWLING;
            prfmsg(BBON);
            outprf(usrnum);
            return;
        }
    } else if (sameas(margv[1], "help") ||
            (sameas(margv[0], "help") && (sameas(margv[1], "brawl")))) {
        // general help message
        prfmsg(BBHLPGEN);
        outprf(usrnum);
        return;
    } else if (sameto("act", margv[1])) {
        // actions help message
        prfmsg(BBHLPACT);
        outprf(usrnum);
        return;
    } else if (sameas(margv[1], "items")) {
        // items help message
        prfmsg(BBHLPITM);
        outprf(usrnum);
        return;
    } else if (sameas(margv[1], "scoring")) {
        // scoring help message
        prfmsg(BBHLPSCR);
        outprf(usrnum);
        return;
    } else if (sameas(margv[1], "new")) {
        // new brawl version features help message
        prfmsg(BBHLPNEW);
        outprf(usrnum);
        return;
    } else if (sameas(margv[1], "open") && (usrptr->flags & MASTER) != 0) {
        brawl_arena_state.game_state = BRAWL_GAME_STATE_SALOON_IS_OPEN;
        prf("OK, The saloon is open.\r");
        outprf(usrnum);
        return;
    } else if (sameas(margv[1], "close") && (usrptr->flags & MASTER) != 0) {
        brawl_arena_state.game_state = BRAWL_GAME_STATE_SALOON_IS_CLOSED;
        prf("OK, The saloon is closed.\r");
        outprf(usrnum);
        return;
    } else {
        prfmsg(BBBRFMT);
        outprf(usrnum);
        return;
    }
}


void brawl_handle_print_inventory(void) {
    int holster_index;

    if (brawl_current_player_data_ptr->aim_target_player_index < -1 || nterms < brawl_current_player_data_ptr->aim_target_player_index) {
        prf("aimtarget error\r");
        brawl_output_message_to_specified_brawler(usrnum);
        return;
    }

    if (BRAWL_WEAPON_COUNT <= brawl_current_player_data_ptr->weapon_in_hand || brawl_current_player_data_ptr->weapon_in_hand < BRAWL_WEAPON_NONE) {
        prf("handitem error\r");
        brawl_output_message_to_specified_brawler(usrnum);
        return;
    }

    prf(" %s (%s) %s\r", &brawl_current_player_data_ptr->userid, brawl_player_rank_names[brawl_current_player_data_ptr->rank], brawl_current_player_data_ptr->kill_count >= BRAWL_KILLS_UNTIL_OUTLAW_THRESHOLD ? "- OUTLAW" : "");

    if (0 < brawl_current_player_data_ptr->bounty) {
        prf("WANTED -- $%d reward!\r", brawl_current_player_data_ptr->bounty);
    }

    if (0 < brawl_current_player_data_ptr->kill_count) {
        prf("You\'ve killed %d gunfighter%s in your career.\r", brawl_current_player_data_ptr->kill_count, brawl_current_player_data_ptr->kill_count > 1 ? "s" : "");
    }

    if (0 < brawl_current_player_data_ptr->money_held) {
        prf("You have $%d in cash.\r", brawl_current_player_data_ptr->money_held);
    }

    switch (brawl_current_player_data_ptr->player_state) {

    case BRAWL_PLAYER_STATE_BRAWLING:
        prf("You are %s.\r", brawl_get_hp_description(brawl_current_player_data_ptr->current_hp));

        switch(brawl_current_player_data_ptr->queued_command) {

        case BRAWL_COMMAND_AIM:
            prf("You\'re AIMing at %s", brawl_current_player_data_ptr->aim_target_player_index == -1 ? "nobody.\r" : brawl_player_data_array[brawl_current_player_data_ptr->aim_target_player_index].userid);
            prf(".  Your aim is %s.\r", brawl_aim_table[brawl_get_aim_table_index_for_timer_value(brawl_current_player_data_ptr->aim_update_timer)].description);
            break;

        case BRAWL_COMMAND_LOAD:
            prf("You're LOADing your %s.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
            break;

        case BRAWL_COMMAND_HOLSTER:
            prf("You're HOLSTERing your %s.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
            break;

        case BRAWL_COMMAND_DRAW:
            prf("You're DRAWing your %s.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->queued_weapon_to_grab_or_draw));
            break;

        case BRAWL_COMMAND_GRAB:
            prf("You're GRABbing for the %s.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->queued_weapon_to_grab_or_draw));
            break;

        case BRAWL_COMMAND_ESC:
            prf("You're trying to ESCape.\r");
            break;

        default:
            if (brawl_current_player_data_ptr->resting_status_update_countdown_timer > 0) {
                prf("You're resting.\r");
            } else {
                prf("You're not doing anything.\r");
            }
            break;

        }

        if (brawl_current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_NONE) {
            if (brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].ammo_capacity > 0) {
                if (brawl_current_player_data_ptr->weapon_in_hand_ammo == 0) {
                    prf("Your %s is empty.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
                } else if (brawl_current_player_data_ptr->weapon_in_hand_ammo == brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].ammo_capacity) {
                    prf("Your %s is fully loaded.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
                } else {
                    prf("Your %s has %d shots left.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand), brawl_current_player_data_ptr->weapon_in_hand_ammo);
                }
            } else {
                prf("You're holding a %s.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
            }
        } else {
            prf("Your hands are empty.\r");
        }

        if (usrnum == brawl_dynamite_target_player_index && brawl_current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_DYNAMITE) {
            prf("There\'s a stick of lit DYNAMITE at your feet.\r");
            prf("You\'d better GRAB it so you can THROW it or DEFUSE it.\r");
            prf("      DO SOMETHING!\r");
        }

        break;

    case BRAWL_PLAYER_STATE_ESCAPED:
        prf("You have ESCAPED.\r");
        break;

    case BRAWL_PLAYER_STATE_DEAD:
        prf("You are DEAD.\r");
        break;

    }

    if (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING || brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_ESCAPED) {
        prf("In your holsters you have:\r");

        holster_index = 0;
        do {
            if (brawl_current_player_data_ptr->holsters[holster_index].weapon != BRAWL_WEAPON_NONE) {
                prf("   A %s ", brawl_get_weapon_name(brawl_current_player_data_ptr->holsters[holster_index].weapon));
                prf("with %d %s\r", brawl_current_player_data_ptr->holsters[holster_index].ammo_in_weapon,
                    brawl_current_player_data_ptr->holsters[holster_index].weapon == BRAWL_WEAPON_SHOTGUN ? "shells" : "bullets");
            } else {
                prf("   empty holster\r");
            }

            ++holster_index;
        } while (holster_index < BRAWL_HOLSTER_COUNT);
    }

    brawl_output_message_to_specified_brawler(usrnum);
}


void brawl_print_aim_message_for_aimer_and_target(void) {
    brawl_current_player_data_ptr->aim_update_timer = 0;
    brawl_current_player_data_ptr->queued_command = BRAWL_COMMAND_AIM;
    brawl_current_player_data_ptr->aim_target_player_index = othusn;

    brawl_current_random_message_num = rand() % (AIMLAST - AIM1) + AIM1;
    prfmsg(brawl_current_random_message_num, othuap);
    brawl_output_message_to_specified_brawler(usrnum);

    brawl_current_random_message_num = rand() % (TRGTLST - TARGET1) + TARGET1;
    prfmsg(brawl_current_random_message_num, usaptr, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
    brawl_output_message_to_specified_brawler(brawl_current_player_data_ptr->aim_target_player_index);

    prfmsg(BBISAIM, usaptr, othuap);
    brawl_output_message_to_all_but_current_and_target_brawlers(brawl_current_player_data_ptr->aim_target_player_index);
}


int brawl_can_aim_weapon(void) {
    int user_find_by_name_count;

    if (margc < 2) {
        // did not specify target name
        prfmsg(BBAIMFMT);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else {
        rstrin();

        if ((user_find_by_name_count = brawl_get_user_count_matching_name(margv[1])) == 0) {  // Doing inline to match original binary
            // could not find user with given name
            prfmsg(BBAIMNTF, margv[1]);
            brawl_output_message_to_specified_brawler(usrnum);
            return 0;
        } else if (user_find_by_name_count != 1) {
            // target name is ambigious
            prfmsg(AMBIGUS, margv[1]);
            brawl_output_message_to_specified_brawler(usrnum);
            return 0;
        } else {
            brawl_current_aim_target_ptr = &brawl_player_data_array[othusn];

            if (brawl_current_aim_target_ptr->player_state == BRAWL_PLAYER_STATE_NOT_BRAWLING) {
                // trying to aim at target who is not brawling
                prfmsg(BBNOTBWL, othuap->userid);
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            } else if (brawl_current_aim_target_ptr->player_state == BRAWL_PLAYER_STATE_DEAD) {
                // trying to aim when target is already dead
                prfmsg(BBALRDED, othuap->userid);
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            } else if (brawl_current_aim_target_ptr->player_state == BRAWL_PLAYER_STATE_ESCAPED) {
                // target already fled
                prfmsg(BBHASESC, othuap->userid);
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            } else if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
                // trying to aim with no weapon drawn
                prfmsg(BBAIMNAW);
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            } else if (brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].ammo_capacity == 0) {
                // trying to aim with no gun drawn
                prfmsg(BBAIMNAG);
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            } else {
                parsin();
                return 1;
            }
        }
    }
}


void brawl_handle_shoot(void) {
    int aim_table_index;
    int damage_done;

    if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_GATLINGGUN) {
        brawl_current_player_data_ptr->aim_update_timer = BRAWL_GATLINGGUN_FIXED_AIM_TIMER_VALUE;
    }

    aim_table_index = brawl_get_aim_table_index_for_timer_value(brawl_current_player_data_ptr->aim_update_timer);
    ++brawl_current_player_data_ptr->shots_fired_this_round;

    // This should check the `brawl_roll_and_check_for_shot_hit()` result directly, without ` == 1`, but the original binary didn't do that
    if (brawl_roll_and_check_for_shot_hit(aim_table_index) == 1) {
        damage_done = brawl_roll_and_get_damage_for_aim(aim_table_index, brawl_current_player_data_ptr->weapon_in_hand);

        if (0 < damage_done) {
            ++brawl_current_player_data_ptr->shots_landed_this_round;
            brawl_do_damage_for_shoot(damage_done);
        } else {
            prfmsg(BBJUSMIS, usaptr, uacoff(brawl_current_player_data_ptr->aim_target_player_index));
            brawl_output_message_to_all_brawlers();
        }
    } else {
        prfmsg(BBMISSED, usaptr, uacoff(brawl_current_player_data_ptr->aim_target_player_index));
        brawl_output_message_to_all_brawlers();
    }

    --brawl_current_player_data_ptr->weapon_in_hand_ammo;
}


int brawl_can_shoot(void) {
    if (brawl_current_player_data_ptr->aim_target_player_index == -1) {
        // not aiming
        prfmsg(BBSHTNAT);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
        // no weapon drawn
        prfmsg(BBDRWFST);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else if (brawl_current_player_data_ptr->weapon_in_hand_ammo <= 0) {
        // shooting with no ammo
        prfmsg(BBNOAMMO);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else if (brawl_current_aim_target_ptr->player_state == BRAWL_PLAYER_STATE_DEAD) {
        // aiming at dead player
        prfmsg(BBALRDE2, uacoff(brawl_current_player_data_ptr->aim_target_player_index));
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else if (brawl_current_aim_target_ptr->player_state == BRAWL_PLAYER_STATE_ESCAPED) {
        // aiming at player who fled
        prfmsg(BBHASES2, uacoff(brawl_current_player_data_ptr->aim_target_player_index));
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else {
        return 1;
    }
}


int brawl_get_aim_table_index_for_timer_value(int aim_timer) {
    int aim_table_index;

    aim_table_index = 0;
    do {
        if (aim_timer < brawl_aim_table[aim_table_index].min_aim_timer_value)
        {
            break;
        }

        ++aim_table_index;
    } while (aim_table_index < BRAWL_AIM_TABLE_COUNT);

    --aim_table_index;

    if (aim_table_index < 0) {
        aim_table_index = 0;
    }

    return aim_table_index;
}


int brawl_roll_and_check_for_shot_hit(int aim_table_index) {
    int percent_chance_to_hit;
    int random_value;

    percent_chance_to_hit = brawl_aim_table[aim_table_index].percent_chance_to_hit;
    random_value = rand() % 100 + 1;

    if (brawl_current_player_data_ptr->rank >= BRAWL_PLAYER_RANK_SHARPSHOOTER && brawl_current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_GATLINGGUN) {
        percent_chance_to_hit = percent_chance_to_hit + BRAWL_SHARPSHOOTER_CHANCE_TO_LAND_SHOT_BONUS;
    }

    if (BRAWL_CHANCE_TO_LAND_SHOT_MAX < percent_chance_to_hit) {
        percent_chance_to_hit = BRAWL_CHANCE_TO_LAND_SHOT_MAX;
    }

    return random_value <= percent_chance_to_hit;
}


int brawl_roll_and_get_damage_for_aim(int aim_table_index, BRAWL_WEAPON weapon) {
    int die_roll;
    const struct brawl_aim_table_entry* current_aim_table_entry;

    die_roll = rand() % 100 + 1;
    current_aim_table_entry = &brawl_aim_table[aim_table_index];

    if (current_aim_table_entry->max_roll_weak_hit >= die_roll) {
        return brawl_weapon_table[weapon].shoot_damage_weak;
    } else if (current_aim_table_entry->max_roll_regular_hit >= die_roll) {
        return brawl_weapon_table[weapon].shoot_damage_regular;
    } else  if (current_aim_table_entry->max_roll_crit >= die_roll) {
        return brawl_weapon_table[weapon].shoot_damage_crit;
    } else {
        // should only get here if the table is bugged
        prf("Invalid DMG_ROLL = %d", die_roll);
        brawl_output_message_to_all_brawlers();
        return 0;
    }
}


void brawl_do_damage_for_shoot(int damage_amount) {
    brawl_current_aim_target_ptr->current_hp = brawl_current_aim_target_ptr->current_hp - damage_amount;

    if (brawl_current_aim_target_ptr->current_hp <= 0) {
        brawl_score_kill_and_collect_bounty(brawl_current_aim_target_ptr, brawl_current_player_data_ptr);
    } else {
        brawl_current_random_message_num = rand() % (SHOOTLST - SHOOT1) + SHOOT1;
        prfmsg(brawl_current_random_message_num, uacoff(brawl_current_player_data_ptr->aim_target_player_index));
        brawl_output_message_to_specified_brawler(usrnum);

        brawl_current_random_message_num = rand() % (SHOTLAST - SHOT1) + SHOT1;
        prfmsg(brawl_current_random_message_num, usaptr);
        brawl_output_message_to_specified_brawler(brawl_current_player_data_ptr->aim_target_player_index);

        prfmsg(BBGOTHIT, usaptr, uacoff(brawl_current_player_data_ptr->aim_target_player_index));
        brawl_output_message_to_all_but_current_and_target_brawlers(brawl_current_player_data_ptr->aim_target_player_index);

        if (BRAWL_DAMAGE_THRESHOLD_TO_TRIGGER_STUN <= damage_amount) {
            brawl_cancel_aim_and_queued_command(brawl_current_aim_target_ptr);
        }
    }
}


void brawl_score_kill_and_collect_bounty(struct brawl_player_data* victim, struct brawl_player_data* killer) {
    ++killer->kill_count;
    killer->money_held += victim->bounty;
    brawl_handle_death(victim, killer->userid);
}


void brawl_handle_death(struct brawl_player_data* victim, const char* name_of_last_killer) {
    int index;

    strcpy(victim->name_of_last_killer, name_of_last_killer);
    prfmsg(BBGOTKIL, &victim->userid, name_of_last_killer);
    brawl_output_message_to_all_brawlers();

    brawl_current_random_message_num = rand() % (DEATHLST - DEATH1) + DEATH1;
    prfmsg(brawl_current_random_message_num, name_of_last_killer);
    brawl_output_message_to_specified_brawler(victim->player_index);
    clrprf();

    victim->player_state = BRAWL_PLAYER_STATE_DEAD;
    victim->aim_update_timer = -1;
    victim->queued_command = -1;
    victim->queued_command_update_countdown_timer = -1;

    if (victim->weapon_in_hand != BRAWL_WEAPON_NONE) {
        ++brawl_weapon_current_lying_on_floor_count[victim->weapon_in_hand];
    }

    index = 0;
    do {
        if (victim->holsters[index].weapon != BRAWL_WEAPON_NONE) {
            ++brawl_weapon_current_lying_on_floor_count[victim->holsters[index].weapon];
        }

        ++index;
    } while (index < BRAWL_HOLSTER_COUNT);

    victim->rank = BRAWL_PLAYER_RANK_GREENHORN;
    victim->banked_money_held = 0;
    victim->banked_kill_count = 0;

    index = 0;
    do {
        victim->banked_holsters[index].weapon = BRAWL_WEAPON_NONE;
        victim->banked_holsters[index].ammo_in_weapon = -1;
        ++index;
    } while (index < BRAWL_HOLSTER_COUNT);

    victim->banked_holsters[0].weapon = BRAWL_STARTING_WEAPON;
    victim->banked_holsters[0].ammo_in_weapon = BRAWL_STARTING_WEAPON_AMMO;

    if (victim->player_index == brawl_dynamite_target_player_index) {
        brawl_dynamite_target_player_index = -1;
    }
}



void brawl_handle_death_killed_by_dynamite(struct brawl_player_data* victim, const char* name_of_last_killer) {
    // There's no need for this wrapper to exist. But it does, and it is used
    brawl_handle_death(victim, name_of_last_killer);
}


int brawl_can_draw_weapon(void) {
    int index;
    BRAWL_WEAPON specified_weapon;

    if (brawl_current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_NONE) {
        prfmsg(BBHANDFL);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else if (margc < 2) {
        prfmsg(BBDRWFMT);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else {
        specified_weapon = brawl_find_weapon_index_by_name(margv[1]);

        if (specified_weapon == BRAWL_WEAPON_NONE) {
            // type name specified for weapon wasn't valid
            prfmsg(BBDRWFMT, margv[1]);
            brawl_output_message_to_specified_brawler(usrnum);
            return 0;
        } else {
            index = 0;
            do {
                if (brawl_current_player_data_ptr->holsters[index].weapon == specified_weapon) {
                    break;
                }
                ++index;
            } while (index < BRAWL_HOLSTER_COUNT);

            if (index == BRAWL_HOLSTER_COUNT) {
                // don't have a weapon of the specified type holstered
                prfmsg(BBDRWNIH, margv[1]);
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            }

            return 1;
        }
    }
}


int brawl_can_grab_weapon(void) {
    BRAWL_WEAPON weapon_to_grab;

    if (brawl_current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_NONE) {
        prfmsg(BBGRBFUL);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else if (margc < 2) {
        prfmsg(BBGRBFMT);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else {
        weapon_to_grab = brawl_find_weapon_index_by_name(margv[1]);

        if (weapon_to_grab == BRAWL_WEAPON_NONE) {
            // invalid weapon type specified
            prfmsg(BBGRBNTE, margv[1]);
            brawl_output_message_to_specified_brawler(usrnum);
            return 0;
        } else {
            if (brawl_weapon_current_lying_on_floor_count[weapon_to_grab] == 0) {
                prfmsg(BBGRBNTF, margv[1]);
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            }

            return 1;
        }
    }
}


int brawl_can_drop_weapon(void) {
    if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
        prfmsg(BBDRPNTF);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    }

    return 1;
}


int brawl_can_holster_weapon(void) {
    int index;
    int result;

    result = 0;

    if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
        prf("Nothing to holster.\r");
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else if (brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].time_to_holster == -1) {
        prf("You cain\'t holster a %s.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else {
        index = 0;
        do {
            if (brawl_current_player_data_ptr->holsters[index].weapon == brawl_current_player_data_ptr->weapon_in_hand) {
                prf("You already have a %s holstered.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            }

            if (brawl_current_player_data_ptr->holsters[index].weapon == BRAWL_WEAPON_NONE) {
                result = 1;
            }
            ++index;
        } while (index < BRAWL_HOLSTER_COUNT);

        if (!result) {
            prf("No available holster.\r");
            brawl_output_message_to_specified_brawler(usrnum);
            return 0;
        }

        return 1;
    }
}


int brawl_can_load_weapon(void) {
    if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
        prf("You don\'t have a weapon in hand to load.\r");
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else {
        if (brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].ammo_capacity == 0) {
            prf("You cannot load a %s.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
            brawl_output_message_to_specified_brawler(usrnum);
            return 0;
        } else if (brawl_current_player_data_ptr->weapon_in_hand_ammo == brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].ammo_capacity) {
            prf("Your %s is already fully loaded.\r", brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
            brawl_output_message_to_specified_brawler(usrnum);
            return 0;
        } else {
            return 1;
        }
    }
}


int brawl_handle_view_command(void) {
    if (margc >= 2) {
        if (sameas(margv[1], "brawlers")) {
            brawl_handle_view_brawlers();
            return 1;
        } else if (sameas(margv[1], "items")) {
            brawl_handle_view_items();
            return 1;
        }

        // No match, so will error out
    }

    prfmsg(BBVEWFMT);
    brawl_output_message_to_specified_brawler(usrnum);
    return 0;
}


void brawl_handle_view_brawlers(void) {
    struct brawl_player_data* current_player_data_ptr;
    int user_num;

    for (user_num = 0, current_player_data_ptr = brawl_player_data_array; user_num < nterms; ++user_num, ++current_player_data_ptr) {
        if (user[user_num].state == teleconference_module_index && user[user_num].substt == TELECONFERENCE_USER_SUBSTATE_NORMAL) {
            if (current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING) {
                prf("%s (%s) is ", uacoff(user_num), brawl_player_rank_names[current_player_data_ptr->rank]);
                prf("%s. ", brawl_get_hp_description(current_player_data_ptr->current_hp));

                if (0 < current_player_data_ptr->bounty) {
                    prf("WANTED -- $%d reward!\r", current_player_data_ptr->bounty);
                }

                if (current_player_data_ptr->aim_target_player_index != -1) {
                    prf(" Aiming at %s. ", uacoff(current_player_data_ptr->aim_target_player_index));
                }

                if (current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_NONE) {
                    prf(" Totin\' a %s.", brawl_get_weapon_name(current_player_data_ptr->weapon_in_hand));
                }

                prf("\r\r");
            } else if (current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_DEAD) {
                prf("%s (%s) is pushing up daisies.\r\r", uacoff(user_num)->userid, brawl_player_rank_names[current_player_data_ptr->rank]);
            } else if (current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_ESCAPED) {
                prf("%s (%s) has escaped.\r\r", uacoff(user_num)->userid, brawl_player_rank_names[current_player_data_ptr->rank]);
            }
        }
    }

    brawl_output_message_to_specified_brawler(usrnum);
}


void brawl_handle_view_items(void) {
    int found_item;
    BRAWL_WEAPON weapon;

    found_item = 0;
    weapon = BRAWL_WEAPON_COLT45;

    do {
        if (0 < brawl_weapon_current_lying_on_floor_count[weapon]) {
            found_item = 1;
            prf("%d %s%s\r",
                brawl_weapon_current_lying_on_floor_count[weapon],
                brawl_get_weapon_name(weapon),
                brawl_weapon_current_lying_on_floor_count[weapon] == 1 ? " " : "s");
        }

        ++weapon;
    } while (weapon < BRAWL_WEAPON_COUNT);

    if (!found_item) {
        prf("No available items.");
    }

    brawl_output_message_to_specified_brawler(usrnum);
}


void brawl_handle_drop_weapon(void) {
    prf("%s dropped a %s.\r", usaptr, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
    brawl_output_message_to_all_brawlers();

    ++brawl_weapon_current_lying_on_floor_count[brawl_current_player_data_ptr->weapon_in_hand];
    brawl_current_player_data_ptr->weapon_in_hand = BRAWL_WEAPON_NONE;
    brawl_current_player_data_ptr->weapon_in_hand_ammo = 0;
}


int brawl_can_hit_or_throw(void) {
    int count_of_users_matching_name;

    if (margc < 2) {
        prfmsg(BBHITFMT);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else {
        rstrin();

        if ((count_of_users_matching_name = brawl_get_user_count_matching_name(margv[1])) == 0) {  // Doing inline to match original binary
            // target with name not found
            prfmsg(BBHITNTF, margv[1]);
            brawl_output_message_to_specified_brawler(usrnum);
            return 0;
        } else if (count_of_users_matching_name != 1) {
            // specified target name is ambiguous
            prfmsg(AMBIGUS, margv[1]);
            brawl_output_message_to_specified_brawler(usrnum);
            return 0;
        } else {
            brawl_current_aim_target_ptr = &brawl_player_data_array[othusn];

            if (brawl_current_aim_target_ptr->player_state == BRAWL_PLAYER_STATE_NOT_BRAWLING) {
                prfmsg(BBNOTBWL, othuap->userid);
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            } else if (brawl_current_aim_target_ptr->player_state == BRAWL_PLAYER_STATE_DEAD) {
                prfmsg(BBALRDE3, othuap->userid);
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            } else if (brawl_current_aim_target_ptr->player_state == BRAWL_PLAYER_STATE_ESCAPED) {
                prfmsg(BBHASES2, othuap->userid);
                brawl_output_message_to_specified_brawler(usrnum);
                return 0;
            } else {
                parsin();
                return 1;
            }
        }
    }
}


void brawl_handle_hit_or_throw(void) {
    int is_throw;
    int damage_from_hit_or_throw;
    int random_value;

    is_throw = 0;

    if (sameas(margv[0], "throw")) {
        is_throw = 1;
    }

    // This should check `is_throw` directly, without ` == 1`, but the original binary didn't do that
    if (is_throw == 1 && brawl_current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_NONE) {
        brawl_current_random_message_num = rand() % (BBTHRLST - BBTHRW1) + BBTHRW1;
        prfmsg(brawl_current_random_message_num, usaptr->userid, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand), othuap->userid);
        brawl_output_message_to_all_brawlers();
    }

    random_value = rand() % 100 + 1;

    if (random_value <= BRAWL_CHANCE_TO_LAND_HIT_OR_THROW) {
        damage_from_hit_or_throw = brawl_roll_and_get_damage_for_hit_or_throw();

        if (damage_from_hit_or_throw > 0) {
            brawl_do_damage_for_hit_or_throw(damage_from_hit_or_throw);
        } else {
            prfmsg(BBHITNOD, othuap->userid, usaptr->userid);
            brawl_output_message_to_all_brawlers();
        }

        if (is_throw == 1 && brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_DYNAMITE &&
                brawl_dynamite_state == BRAWL_DYNAMITE_STATE_LIT) {
            brawl_dynamite_target_player_index = othusn;
        }
    } else {
        if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
            prfmsg(BBHITMIS, usaptr->userid, othuap->userid, "bare-hand punch.");
            brawl_output_message_to_all_brawlers();
        } else {
            prfmsg(BBHITMIS, usaptr->userid, othuap->userid, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
            brawl_output_message_to_all_brawlers();
        }
    }

    if (is_throw == 1) {
        brawl_current_player_data_ptr->fatigue_points = brawl_current_player_data_ptr->fatigue_points + brawl_get_fatigue_cost_for_hit_or_throw() + rand() % (BRAWL_THROW_MAX_RANDOM_FATIGUE_PENALTY + 1);
    } else {
        brawl_current_player_data_ptr->fatigue_points = brawl_current_player_data_ptr->fatigue_points + brawl_get_fatigue_cost_for_hit_or_throw();
    }

    if (BRAWL_FATIGUE_THRESHOLD_TO_TRIGGER_RESTING <= brawl_current_player_data_ptr->fatigue_points) {
        brawl_current_player_data_ptr->resting_status_update_countdown_timer = brawl_current_player_data_ptr->resting_status_update_countdown_timer + BRAWL_RESTING_STATUS_TIMER_ADDED_WHEN_FATIGUE_THRESHOLD_HIT;
        brawl_current_player_data_ptr->fatigue_points = brawl_current_player_data_ptr->fatigue_points - BRAWL_FATIGUE_THRESHOLD_TO_TRIGGER_RESTING;
        prfmsg(BBUTIRED);
        brawl_output_message_to_specified_brawler(usrnum);
    }

    // This should check the `is_destroyed_on_hit_or_throw` value directly, without ` == 1`, but the original binary didn't do that
    if (brawl_current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_NONE &&
            brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].is_destroyed_on_hit_or_throw == 1) {
        brawl_current_player_data_ptr->weapon_in_hand = BRAWL_WEAPON_NONE;
    }

    if (brawl_current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_NONE && is_throw == 1) {
        ++brawl_weapon_current_lying_on_floor_count[brawl_current_player_data_ptr->weapon_in_hand];
        brawl_current_player_data_ptr->weapon_in_hand = BRAWL_WEAPON_NONE;
    }
}


void brawl_handle_hit_old_but_undeleted(void) {  // Completely unused function
    int damage_from_hit_or_throw;
    int random_value;

    random_value = rand() % 100 + 1;

    if (random_value <= BRAWL_CHANCE_TO_LAND_HIT_OR_THROW) {
        damage_from_hit_or_throw = brawl_roll_and_get_damage_for_hit_or_throw();

        if (0 < damage_from_hit_or_throw) {
            brawl_do_damage_for_hit_or_throw(damage_from_hit_or_throw);
            return;
        }

        prfmsg(BBHITNOD, othuap, usaptr);
        brawl_output_message_to_all_brawlers();
    } else {
        if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
            prfmsg(BBHITMIS, usaptr, othuap, "bare-hand punch.");
            brawl_output_message_to_all_brawlers();
        } else {
            prfmsg(BBHITMIS, usaptr, othuap, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
            brawl_output_message_to_all_brawlers();
        }
    }
}


int brawl_roll_and_get_damage_for_hit_or_throw(void) {
    int max_damage_exclusive;

    if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
        max_damage_exclusive = BRAWL_BAREHANDED_DAMAGE_FOR_HIT_OR_THROW;
    } else {
        max_damage_exclusive = brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].max_hit_or_throw_damage_inclusive + 1;
    }

    return rand() % max_damage_exclusive;
}


void brawl_do_damage_for_hit_or_throw(int damage_amount) {
    brawl_current_aim_target_ptr->current_hp = brawl_current_aim_target_ptr->current_hp - damage_amount;

    if (brawl_current_aim_target_ptr->current_hp <= 0) {
        brawl_score_kill_and_collect_bounty(brawl_current_aim_target_ptr, brawl_current_player_data_ptr);
    } else {
        brawl_current_random_message_num = rand() % (BBHITLST - BBHIT1) + BBHIT1;

        if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
            prfmsg(brawl_current_random_message_num, usaptr, othuap, "fist in the face");
            brawl_output_message_to_all_brawlers();
        } else {
            prfmsg(brawl_current_random_message_num, usaptr, othuap, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
            brawl_output_message_to_all_brawlers();
        }

        if (BRAWL_DAMAGE_THRESHOLD_TO_TRIGGER_STUN <= damage_amount) {
            brawl_cancel_aim_and_queued_command(brawl_current_aim_target_ptr);
        }
    }
}


int brawl_get_fatigue_cost_for_hit_or_throw(void) {
    if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
        return BRAWL_BAREHANDED_FATIGUE_FOR_HIT_OR_THROW;
    } else {
        return brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].fatigue_cost_for_hit_or_throw;
    }
}


int brawl_can_light_dynamite(void) {
    if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
        prfmsg(BBPUDFST);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else if (brawl_current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_DYNAMITE) {
        prfmsg(BBNODYNA);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else if (brawl_dynamite_state == BRAWL_DYNAMITE_STATE_LIT) {
        prfmsg(BBALRDYL);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else {
        return 1;
    }
}


void brawl_handle_light_dynamite(void) {
    brawl_dynamite_update_countdown_timer = rand() % (BRAWL_DYNAMITE_MAX_RANDOM_TIME_ON_LIGHT - BRAWL_DYNAMITE_MIN_RANDOM_TIME_ON_LIGHT + 1) + BRAWL_DYNAMITE_MIN_RANDOM_TIME_ON_LIGHT;
    brawl_dynamite_target_player_index = usrnum;
    brawl_dynamite_state = BRAWL_DYNAMITE_STATE_LIT;

    brawl_current_random_message_num = rand() % (LIGHTLST - LIGHT1) + LIGHT1;
    prfmsg(brawl_current_random_message_num, &brawl_current_player_data_ptr->userid);
    brawl_output_message_to_all_brawlers();
}


int brawl_can_defuse_dynamite(void) {
    if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_NONE) {
        // warn the player that they picked up the dynamite before defusing it
        prfmsg(BBPUDBFD);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else if (brawl_current_player_data_ptr->weapon_in_hand != BRAWL_WEAPON_DYNAMITE) {
        // tell the player they're trying to defuse something that isn't dynamite
        prfmsg(BBNODYND);
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else {
        return 1;
    }
}


void brawl_handle_defuse_dynamite(void) {
    brawl_current_player_data_ptr->weapon_in_hand = BRAWL_WEAPON_NONE;
    brawl_dynamite_state = BRAWL_DYNAMITE_STATE_NOT_SPAWNED;
    brawl_dynamite_target_player_index = -1;

    brawl_current_random_message_num = rand() % (DFUSELST - DEFUSE1) + DEFUSE1;
    prfmsg(brawl_current_random_message_num, &brawl_current_player_data_ptr->userid);
    brawl_output_message_to_all_brawlers();
}


void brawl_handle_draw_weapon(void) {
    int index;

    brawl_current_player_data_ptr->weapon_in_hand = brawl_current_player_data_ptr->queued_weapon_to_grab_or_draw;

    index = 0;
    do {
        if (brawl_current_player_data_ptr->weapon_in_hand == brawl_current_player_data_ptr->holsters[index].weapon) {
            brawl_current_player_data_ptr->weapon_in_hand_ammo = brawl_current_player_data_ptr->holsters[index].ammo_in_weapon;
            brawl_current_player_data_ptr->holsters[index].weapon = BRAWL_WEAPON_NONE;
            brawl_current_player_data_ptr->holsters[index].ammo_in_weapon = 0;
        }
        ++index;
    } while (index < BRAWL_HOLSTER_COUNT);

    // tell the player they just drew a weapon
    brawl_current_random_message_num = rand() % (DRAWLST - DRAW1) + DRAW1;
    prfmsg(brawl_current_random_message_num, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
    brawl_output_message_to_specified_brawler(usrnum);

    // tell other player(s?) they just drew a weapon
    prfmsg(BBDRWOUT, uacoff(usrnum), brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
    brawl_output_message_to_all_brawlers();
}


void brawl_handle_holster_weapon(void) {
    int index;

    index = 0;
    do {
        if (brawl_current_player_data_ptr->holsters[index].weapon == BRAWL_WEAPON_NONE) {
            brawl_current_player_data_ptr->holsters[index].weapon = brawl_current_player_data_ptr->weapon_in_hand;
            brawl_current_player_data_ptr->holsters[index].ammo_in_weapon = brawl_current_player_data_ptr->weapon_in_hand_ammo;
            break;
        }
        ++index;
    } while (index < BRAWL_HOLSTER_COUNT);

    brawl_current_random_message_num = rand() % (HOLSTLST - HOLSTR1) + HOLSTR1;
    prfmsg(brawl_current_random_message_num, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
    brawl_output_message_to_specified_brawler(usrnum);

    prfmsg(BBHOLSTR, uacoff(usrnum)->userid, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
    brawl_output_message_to_all_brawlers();

    brawl_current_player_data_ptr->weapon_in_hand = BRAWL_WEAPON_NONE;
    brawl_current_player_data_ptr->weapon_in_hand_ammo = 0;
}


int brawl_handle_grab_weapon() {
    if (brawl_weapon_current_lying_on_floor_count[brawl_current_player_data_ptr->queued_weapon_to_grab_or_draw] == 0) {
        prfmsg(BBGRBGON, brawl_get_weapon_name(brawl_current_player_data_ptr->queued_weapon_to_grab_or_draw));
        brawl_output_message_to_specified_brawler(usrnum);
        return 0;
    } else {
        brawl_current_player_data_ptr->weapon_in_hand = brawl_current_player_data_ptr->queued_weapon_to_grab_or_draw;
        brawl_current_player_data_ptr->weapon_in_hand_ammo = 0;
        --brawl_weapon_current_lying_on_floor_count[brawl_current_player_data_ptr->weapon_in_hand];

        if (brawl_current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_DYNAMITE && brawl_dynamite_state == BRAWL_DYNAMITE_STATE_LIT) {
            brawl_dynamite_target_player_index = usrnum;
        }

        prfmsg(BBGRABED, uacoff(usrnum)->userid, brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
        brawl_output_message_to_all_brawlers();
        return 1;
    }
}


int brawl_handle_load_weapon(void) {
    brawl_current_player_data_ptr->weapon_in_hand_ammo = brawl_weapon_table[brawl_current_player_data_ptr->weapon_in_hand].ammo_capacity;
    prf("%s has reloaded the %s.\r", uacoff(usrnum), brawl_get_weapon_name(brawl_current_player_data_ptr->weapon_in_hand));
    brawl_output_message_to_all_brawlers();
    return 1;
}


int brawl_handle_escape(void) {
    brawl_current_player_data_ptr->player_state = BRAWL_PLAYER_STATE_ESCAPED;
    prfmsg(BBESCAPE, uacoff(usrnum)->userid);
    brawl_output_message_to_all_brawlers();
    return 1;
}


const char* brawl_get_weapon_name(BRAWL_WEAPON weapon) {
    const char* result;

    if (weapon < BRAWL_WEAPON_COLT45 || BRAWL_WEAPON_COUNT <= weapon) {
        result = " ";
    } else if (weapon == BRAWL_WEAPON_DYNAMITE) {
        if (brawl_dynamite_state == BRAWL_DYNAMITE_STATE_LIT) {
            result = "stick of lit DYNAMITE";
        } else {
            result = "stick of unlit DYNAMITE";
        }
    } else {
        result = brawl_weapon_table[weapon].weapon_name;  // Seems weapon description is unused in the original code. I'd sort of expect it to be referenced here
    }

    return result;
}


BRAWL_WEAPON brawl_find_weapon_index_by_name(const char* weapon_name) {
    int current_weapon_index;

    current_weapon_index = 0;
    do {
        if (sameas((char*)weapon_name, brawl_weapon_table[current_weapon_index].weapon_name)) {  // Can't change signature of header. sameas doesn't modify the string
            return current_weapon_index;
        }

        ++current_weapon_index;
    } while (current_weapon_index < BRAWL_WEAPON_COUNT);

    return BRAWL_WEAPON_NONE;
}


const char* brawl_get_command_name_unused(BRAWL_COMMAND command) {  // Completely unused function
    const char* result;

    if (command < BRAWL_COMMAND_INV || BRAWL_COMMAND_COUNT <= command) {
        result = " ";
    } else {
        result = brawl_command_table[command].command_name;
    }

    return result;
}


const char* brawl_get_player_state_name(BRAWL_PLAYER_STATE player_state) {
    const char* result;

    switch(player_state) {

    case BRAWL_PLAYER_STATE_NOT_BRAWLING:
        result = "NOT BRAWLING";
        break;

    case BRAWL_PLAYER_STATE_BRAWLING:
        result = "BRAWLING";
        break;

    case BRAWL_PLAYER_STATE_DEAD:
        result = "DEAD";
        break;

    case BRAWL_PLAYER_STATE_ESCAPED:
        result = "ESCAPED";
        break;

    default:
        result = "ERROR";
        break;

    }

    return result;
}


const char* brawl_get_hp_description(int hit_points) {
    char* result;

    if (hit_points == BRAWL_HP_MAX) {
        result = "unscratched";
    } else if (hit_points >= BRAWL_HP_MIN_GOOD_SHAPE) {
        result = "in pretty good shape";
    } else if (hit_points >= BRAWL_HP_MIN_HANGING_IN_THERE) {
        result = "hanging in there";
    } else {
        result = "hanging on by a thread";
    }

    return result;
}


void brawl_output_message_to_all_brawlers(void) {
    int user_index;
    struct user* current_user_ptr;

    prfmsg(TLCPMT);

    for (user_index = 0, current_user_ptr = user; user_index < nterms; ++user_index, ++current_user_ptr) {
        if (current_user_ptr->state == teleconference_module_index && current_user_ptr->substt == TELECONFERENCE_USER_SUBSTATE_NORMAL &&
                brawl_player_data_array[user_index].player_state != BRAWL_PLAYER_STATE_NOT_BRAWLING) {
            outprf(user_index);
        }
    }

    clrprf();
}


void brawl_print_tc_prompt_to_all_other_brawlers(void) {
    int user_index;
    struct user* current_user_ptr;

    prfmsg(TLCPMT);

    for (user_index = 0, current_user_ptr = user; user_index < nterms; ++user_index, ++current_user_ptr) {
        if (usrnum != user_index &&
                current_user_ptr->state == teleconference_module_index && current_user_ptr->substt == TELECONFERENCE_USER_SUBSTATE_NORMAL &&
                brawl_player_data_array[user_index].player_state != BRAWL_PLAYER_STATE_NOT_BRAWLING) {
            outprf(user_index);
        }
    }

    clrprf();
}


void brawl_output_message_to_all_but_current_and_target_brawlers(int target_user_index) {
    int user_index;
    struct user* current_user_ptr;

    prfmsg(TLCPMT);

    for (user_index = 0, current_user_ptr = user; user_index < nterms; ++user_index, ++current_user_ptr) {
        if (usrnum != user_index &&
                user_index != target_user_index &&
                current_user_ptr->state == teleconference_module_index && current_user_ptr->substt == TELECONFERENCE_USER_SUBSTATE_NORMAL &&
                brawl_player_data_array[user_index].player_state != BRAWL_PLAYER_STATE_NOT_BRAWLING) {
            outprf(user_index);
        }
    }

    clrprf();
}


void brawl_output_message_to_specified_brawler(int user_index) {
    if (user[user_index].state == teleconference_module_index && user[user_index].substt == TELECONFERENCE_USER_SUBSTATE_NORMAL &&
            brawl_player_data_array[user_index].player_state != BRAWL_PLAYER_STATE_NOT_BRAWLING) {
        prfmsg(TLCPMT);
        outprf(user_index);
    } else {
        clrprf();
    }
}


void brawl_update(void) {
    int current_active_brawler_count;

    current_active_brawler_count = 0;
    setmbk(brawl_mcv_config_file);

    if (brawl_arena_state.game_state == BRAWL_GAME_STATE_ROUND_IN_PROGRESS) {
        for (usrnum = 0, brawl_current_player_data_ptr = brawl_player_data_array; usrnum < nterms; ++usrnum, ++brawl_current_player_data_ptr) {
            if (brawl_current_player_data_ptr->userid[0] != '\0' &&
                    user[usrnum].state == teleconference_module_index && user[usrnum].substt == TELECONFERENCE_USER_SUBSTATE_NORMAL &&
                    brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING) {
                ++current_active_brawler_count;

                if (brawl_current_player_data_ptr->resting_status_update_countdown_timer > 0) {
                    --brawl_current_player_data_ptr->resting_status_update_countdown_timer;

                    if (brawl_current_player_data_ptr->resting_status_update_countdown_timer == 0) {
                        prf("You\'ve recovered.\r");
                        brawl_output_message_to_specified_brawler(usrnum);
                    }
                } else if (brawl_current_player_data_ptr->queued_command == BRAWL_COMMAND_AIM) {
                    if (brawl_current_player_data_ptr->aim_update_timer < BRAWL_AIM_TABLE_COUNT) {
                        ++brawl_current_player_data_ptr->aim_update_timer;
                    }

                    if (brawl_current_player_data_ptr->aim_update_timer % BRAWL_NOTIFY_STILL_UNDER_AIM_THREAT_EVERY_NTH_UPDATE == 0) {
                        prf("%s is still aiming at you!\r", uacoff(usrnum)->userid);

                        if (brawl_current_player_data_ptr->aim_target_player_index != -1) {
                            brawl_output_message_to_specified_brawler(brawl_current_player_data_ptr->aim_target_player_index);
                        }

                        clrprf();
                    }
                } else if (brawl_current_player_data_ptr->queued_command != BRAWL_COMMAND_NONE && 0 < brawl_current_player_data_ptr->queued_command_update_countdown_timer) {
                    --brawl_current_player_data_ptr->queued_command_update_countdown_timer;

                    if (brawl_current_player_data_ptr->queued_command_update_countdown_timer == 0) {
                        switch(brawl_current_player_data_ptr->queued_command) {

                        case BRAWL_COMMAND_DRAW:
                            brawl_handle_draw_weapon();
                            break;

                        case BRAWL_COMMAND_GRAB:
                            brawl_handle_grab_weapon();
                            break;

                        case BRAWL_COMMAND_HOLSTER:
                            brawl_handle_holster_weapon();
                            break;

                        case BRAWL_COMMAND_LOAD:
                            brawl_handle_load_weapon();
                            break;

                        case BRAWL_COMMAND_ESC:
                            brawl_handle_escape();

                        }

                        brawl_clear_player_queued_command_and_timer(brawl_current_player_data_ptr);
                    }
                }
            }
        }

        if (current_active_brawler_count == 0 && (BRAWL_TIME_TO_SHERIFF_MAX_WHEN_ALL_BRAWLERS_DEAD_OR_ESCAPED - 1) < brawl_arena_state.sheriff_arrive_update_countdown_timer) {
            brawl_arena_state.sheriff_arrive_update_countdown_timer = BRAWL_TIME_TO_SHERIFF_MAX_WHEN_ALL_BRAWLERS_DEAD_OR_ESCAPED;
        }

        brawl_update_weapon_spawns();

        if (brawl_dynamite_state == BRAWL_DYNAMITE_STATE_LIT) {
            if (brawl_dynamite_update_countdown_timer-- == 0) {
                // trigger dynamite explosion
                brawl_dynamite_state = BRAWL_DYNAMITE_STATE_NOT_SPAWNED;
                brawl_dynamite_update_countdown_timer = -1;
                brawl_handle_exploding_dynamite();
            } else if (brawl_dynamite_update_countdown_timer % BRAWL_NOTIFY_DYNAMITE_STILL_LIT_EVERY_NTH_UPDATE == 0) {
                // printing dynamite ignited picture
                prfmsg(BBDYNPIC, brawl_dynamite_target_player_index == -1
                    ? "DYNAMITE"
                    : brawl_player_data_array[brawl_dynamite_target_player_index].userid);
                brawl_output_message_to_all_brawlers();
            }
        }
    }

    if (--brawl_arena_state.sheriff_arrive_update_countdown_timer < 0) {
        switch (brawl_arena_state.game_state) {

        case BRAWL_GAME_STATE_ROUND_IN_PROGRESS:
            // the sheriff arrived. halting the brawl round and setting a "sheriff stay" timer
            brawl_arena_state.game_state = BRAWL_GAME_STATE_SHERIFF_IS_HERE;
            brawl_arena_state.sheriff_arrive_update_countdown_timer = brawl_config_time_sheriff_stay;
            prfmsg(BBSHFHER);
            brawl_print_round_result();
            brawl_output_message_to_all_brawlers();
            brawl_handle_sheriff_arrives();
            break;

        case BRAWL_GAME_STATE_SHERIFF_IS_HERE:
            // sheriff just left
            brawl_arena_state.game_state = BRAWL_GAME_STATE_SALOON_IS_OPEN;
            brawl_arena_state.sheriff_arrive_update_countdown_timer = brawl_config_time_to_sheriff_arrival;
            prfmsg(BBSHFGON);
            brawl_output_message_to_all_brawlers();
            break;

        }
    }

    if (brawl_arena_state.game_state == BRAWL_GAME_STATE_ROUND_IN_PROGRESS &&
            brawl_arena_state.sheriff_arrive_update_countdown_timer == BRAWL_NOTIFY_SHERIFF_ARRIVING_AT_TIMER_VALUE) {
        // print warning that the sheriff is about to arrive
        prfmsg(BBSHFWRN);
        brawl_output_message_to_all_brawlers();
    }

    rtkick(BRAWL_UPDATE_FREQUENCY_IN_SECONDS, brawl_update);
}


void brawl_update_weapon_spawns(void) {
    int existing_total_weapon_count;
    int index;

    existing_total_weapon_count = 0;

    index = 0;
    do {
        existing_total_weapon_count = existing_total_weapon_count + brawl_weapon_current_lying_on_floor_count[index];
        ++index;
    } while (index < BRAWL_WEAPON_COUNT);

    if ((BRAWL_WEAPON_MID_ROUND_DESPAWN_COUNT_THRESHOLD - 1) < existing_total_weapon_count) {
        if (rand() % 100 + 1 <= BRAWL_WEAPON_MID_ROUND_CHANCE_DESPAWN) {
            brawl_despawn_most_common_weapon();
        }
    }

    if (existing_total_weapon_count < (BRAWL_WEAPON_MID_ROUND_SPAWN_COUNT_LIMIT + 1)) {
        if (rand() % 100 + 1 <= BRAWL_WEAPON_MID_ROUND_CHANCE_SPAWN) {
            brawl_spawn_random_weapon();
        }
    }

    return;
}


void brawl_spawn_random_weapon(void) {
    int retry_counter;
    BRAWL_WEAPON random_weapon;

    retry_counter = 0;

    for (retry_counter = 0; retry_counter < BRAWL_WEAPON_SPAWN_REROLL_COUNT_MAX; ++retry_counter) {
        random_weapon = brawl_pick_random_weapon_to_spawn();

        if ((random_weapon != BRAWL_WEAPON_DYNAMITE || brawl_dynamite_state == BRAWL_DYNAMITE_STATE_NOT_SPAWNED) &&
            brawl_weapon_current_lying_on_floor_count[random_weapon] < brawl_weapon_table[random_weapon].max_spawn_count)
        {
            ++brawl_weapon_current_lying_on_floor_count[random_weapon];

            if (random_weapon == BRAWL_WEAPON_DYNAMITE) {
                brawl_dynamite_state = BRAWL_DYNAMITE_STATE_SPAWNED;
            }

            return;
        }
    }
}


BRAWL_WEAPON brawl_pick_random_weapon_to_spawn(void) {
    int random_value;
    int current_weapon_index;

    random_value = rand() % 100 + 1;

    current_weapon_index = 0;
    do {
        if (random_value <= brawl_weapon_table[current_weapon_index].spawn_chance) {
            return current_weapon_index;
        }
        ++current_weapon_index;
    } while (current_weapon_index < BRAWL_WEAPON_COUNT);

    return BRAWL_WEAPON_COLT45;
}


void brawl_despawn_most_common_weapon(void) {
    BRAWL_WEAPON chosen_weapon;
    int highest_seen_on_floor_count;
    int weapon_index;

    chosen_weapon = BRAWL_WEAPON_NONE;
    highest_seen_on_floor_count = 0;

    weapon_index = 0;
    do {
        if (highest_seen_on_floor_count < brawl_weapon_current_lying_on_floor_count[weapon_index]) {
            chosen_weapon = weapon_index;
            highest_seen_on_floor_count = brawl_weapon_current_lying_on_floor_count[weapon_index];
        }
        ++weapon_index;
    } while (weapon_index < BRAWL_WEAPON_COUNT);

    if (chosen_weapon != BRAWL_WEAPON_NONE && chosen_weapon != BRAWL_WEAPON_DYNAMITE) {
        --brawl_weapon_current_lying_on_floor_count[chosen_weapon];
    }
}


void brawl_handle_exploding_dynamite(void) {
    struct brawl_player_data* current_player_data_ptr;
    int damage_to_direct_target;
    int damage_to_splash_target;
    int user_index;

    // print message that the dynamite is exploding
    prfmsg(BBDYNBLS);
    brawl_output_message_to_all_brawlers();

    damage_to_direct_target = rand() % (BRAWL_DYNAMITE_DIRECT_DAMAGE_MAX - BRAWL_DYNAMITE_DIRECT_DAMAGE_MIN + 1) + BRAWL_DYNAMITE_DIRECT_DAMAGE_MIN;
    damage_to_splash_target = damage_to_direct_target / BRAWL_DYNAMITE_SPLASH_DAMAGE_RATIO;

    for (user_index = 0, current_player_data_ptr = brawl_player_data_array; user_index < nterms; ++user_index, ++current_player_data_ptr) {
        if (current_player_data_ptr->userid[0] != '\0' &&
                user[user_index].state == teleconference_module_index && user[user_index].substt == TELECONFERENCE_USER_SUBSTATE_NORMAL &&
                current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING) {

            if (user_index == brawl_dynamite_target_player_index) {
                current_player_data_ptr->current_hp = current_player_data_ptr->current_hp - damage_to_direct_target;

                if (current_player_data_ptr->weapon_in_hand == BRAWL_WEAPON_DYNAMITE) {
                    current_player_data_ptr->weapon_in_hand = BRAWL_WEAPON_NONE;
                }

                if (damage_to_direct_target >= BRAWL_DAMAGE_THRESHOLD_TO_TRIGGER_STUN) {
                    brawl_cancel_aim_and_queued_command(current_player_data_ptr);
                }
            } else {
                current_player_data_ptr->current_hp = current_player_data_ptr->current_hp - damage_to_splash_target;

                if (damage_to_splash_target >= BRAWL_DAMAGE_THRESHOLD_TO_TRIGGER_STUN) {
                    brawl_cancel_aim_and_queued_command(current_player_data_ptr);
                }
            }

            if (current_player_data_ptr->current_hp <= 0) {
                brawl_handle_death_killed_by_dynamite(current_player_data_ptr, "dynamite");
            } else {
                brawl_current_random_message_num = rand() % (BLASTLST - BLAST1) + BLAST1;
                prfmsg(brawl_current_random_message_num);
                brawl_output_message_to_specified_brawler(user_index);
            }
        }
    }

    brawl_weapon_current_lying_on_floor_count[BRAWL_WEAPON_DYNAMITE] = 0;
    brawl_dynamite_state = BRAWL_DYNAMITE_STATE_NOT_SPAWNED;
    brawl_dynamite_target_player_index = -1;
}


int brawl_get_user_count_matching_name(char* name_to_match) {
    int count_matching_name;
    int last_user_index_matching_name;

    count_matching_name = 0;

    for (othusn = 0, othusp = user; othusn < nterms; ++othusn, ++othusp) {
        othuap = uacoff(othusn);

        if (SUPLON <= othusp->class &&
                othusp->state == teleconference_module_index && user[othusn].substt == TELECONFERENCE_USER_SUBSTATE_NORMAL &&
                sameto(name_to_match, othuap->userid)) {
            last_user_index_matching_name = othusn;

            if (sameas(name_to_match, othuap->userid)) {
                return 1;
            }

            ++count_matching_name;
        }
    }

    if (count_matching_name == 1) {
        othusn = last_user_index_matching_name;
        othusp = &user[last_user_index_matching_name];
        othuap = uacoff(othusn);
    }

    return count_matching_name;
}


int brawl_check_do_action_and_maybe_start_brawl(BRAWL_COMMAND command) {
    int current_user_index;

    if (!hasmkey(BRAWLKEY)) {
        // "haven't paid your tab"
        prfmsg(BBYERTAB);
        // Note that outprf isn't called here. That may be a bug? But the original binary doesn't do it
        return 0;
    } else if (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_NOT_BRAWLING) {
        prf("You aren\'t brawling, you can\'t do brawl actions.\r");
        prf("If you\'d like to brawl type BRAWL ON\r");
        outprf(usrnum);
        return 0;
    } else if (brawl_arena_state.game_state == BRAWL_GAME_STATE_ROUND_IN_PROGRESS) {
        if (brawl_current_player_data_ptr->player_state != BRAWL_PLAYER_STATE_BRAWLING && !brawl_command_table[command].can_do_when_dead_or_escaped_or_resting) {
            prf("You are %s, you can\'t do that brawl action.\r", brawl_get_player_state_name(brawl_current_player_data_ptr->player_state));
            outprf(usrnum);
            return 0;
        } else if (brawl_current_player_data_ptr->resting_status_update_countdown_timer > 0 && !brawl_command_table[command].can_do_when_dead_or_escaped_or_resting) {
            prf("You can\'t do that brawl action until you recover.\r");
            brawl_output_message_to_specified_brawler(usrnum);
            return 0;
        } else {
            return 1;
        }
    } else if (brawl_arena_state.game_state == BRAWL_GAME_STATE_SALOON_IS_OPEN) {
        // doing a brawl action when the saloon is open. start the brawl round!
        brawl_arena_state.game_state = BRAWL_GAME_STATE_ROUND_IN_PROGRESS;
        brawl_arena_state.sheriff_arrive_update_countdown_timer = brawl_config_time_to_sheriff_arrival + rand() % (BRAWL_TIME_TO_SHERIFF_MAX_RANDOM_TIME_TO_APPEND + 1);

        for (current_user_index = 0; current_user_index < nterms; current_user_index = current_user_index + 1) {
            brawl_reset_player_data_for_round(current_user_index);
        }

        prfmsg(BBBEGIN);
        brawl_output_message_to_all_brawlers();
        brawl_spawn_weapons_and_set_bounty();
        return 1;
    } else if (brawl_arena_state.game_state == BRAWL_GAME_STATE_SHERIFF_IS_HERE) {
        // sheriff is here, so cannot do brawl commands. print error
        prfmsg(BBNOTNOW);
        outprf(usrnum);
        return 0;
    } else if (brawl_arena_state.game_state == BRAWL_GAME_STATE_SALOON_IS_CLOSED) {
        // saloon is closed, so no brawls are allowed
        prfmsg(BBSALCLS);
        outprf(usrnum);
        return 0;
    } else {
        return 0;
    }
}


void brawl_callout_player(void) {
    int user_count_matching_name;
    int do_callout;

    do_callout = 0;

    if (margc >= 2) {
        rstrin();

        if ((user_count_matching_name = brawl_get_user_count_matching_name(margv[1])) == 1) {  // Doing inline to match original binary
            if (brawl_player_data_array[othusn].callout_update_countdown_timer != 0) {
                #define TIME_SINCE_LAST_BRAWL_CALLOUT_IN_MINUTES ((brawl_config_time_between_pages_to_draw + 30) / 60)
                prfmsg(BBCALL5, othuap->userid, TIME_SINCE_LAST_BRAWL_CALLOUT_IN_MINUTES);
                #undef TIME_SINCE_LAST_BRAWL_CALLOUT_IN_MINUTES
            } else {
                prfmsg(BBCALL1, usaptr);

                if (injoth()) {
                    #define UPDATES_UNTIL_NEXT_ALLOWED_BRAWL_CALLOUT ((brawl_config_time_between_pages_to_draw + 7) / 5)
                    brawl_player_data_array[othusn].callout_update_countdown_timer = UPDATES_UNTIL_NEXT_ALLOWED_BRAWL_CALLOUT;
                    #undef UPDATES_UNTIL_NEXT_ALLOWED_BRAWL_CALLOUT

                    prfmsg(BBCALL2, othuap->userid);
                    do_callout = 1;
                } else {
                    prfmsg(BBCALL3, othuap->userid);
                }
            }
        } else if (user_count_matching_name == 0) {
            prfmsg(BBCALL4, othuap->userid);
        } else {
            prfmsg(AMBIGUS, othuap->userid);
        }
    } else {
        prfmsg(HLPCALL);
    }

    outprf(usrnum);

    if (do_callout) {
        prf("\r%s IS CALLING %s OUT!\r", usaptr->userid, othuap->userid);
        brawl_print_tc_prompt_to_all_other_brawlers();
    }
}


void brawl_print_round_result(void) {
    int current_meanest_player_index;
    int current_richest_player_index;
    int current_deadest_player_index;
    int current_meanest_player_kill_count;
    int current_richest_player_money_held;
    int current_deadest_player_hp;

    current_meanest_player_index = -1;
    current_richest_player_index = -1;
    current_deadest_player_index = -1;
    current_meanest_player_kill_count = (BRAWL_KILLS_UNTIL_MEANEST_THRESHOLD - 1);
    current_richest_player_money_held = (BRAWL_MONEY_UNTIL_RICHEST_THRESHOLD - 1);
    current_deadest_player_hp = (BRAWL_HP_DEADEST_THRESHOLD + 1);

    for (usrnum = 0, brawl_current_player_data_ptr = brawl_player_data_array; usrnum < nterms; ++usrnum, ++brawl_current_player_data_ptr) {
        if (brawl_current_player_data_ptr->userid[0] != '\0') {
            if (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_DEAD) {
                if (brawl_current_player_data_ptr->current_hp < current_deadest_player_hp) {
                    current_deadest_player_hp = brawl_current_player_data_ptr->current_hp;
                    current_deadest_player_index = usrnum;
                }
            } else {
                if (user[usrnum].state == teleconference_module_index && user[usrnum].substt == TELECONFERENCE_USER_SUBSTATE_NORMAL) {
                    if (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_ESCAPED ||
                            (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING && brawl_current_player_data_ptr->kill_count < BRAWL_KILLS_UNTIL_OUTLAW_THRESHOLD)) {
                        if (current_meanest_player_kill_count < brawl_current_player_data_ptr->kill_count) {
                            current_meanest_player_kill_count = brawl_current_player_data_ptr->kill_count;
                            current_meanest_player_index = usrnum;
                        }

                        if (current_richest_player_money_held < brawl_current_player_data_ptr->money_held) {
                            current_richest_player_money_held = brawl_current_player_data_ptr->money_held;
                            current_richest_player_index = usrnum;
                        }
                    } else if (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING &&
                            brawl_current_player_data_ptr->kill_count >= BRAWL_KILLS_UNTIL_OUTLAW_THRESHOLD &&
                            current_meanest_player_kill_count < brawl_current_player_data_ptr->kill_count) {
                        current_meanest_player_kill_count = brawl_current_player_data_ptr->kill_count;
                        current_meanest_player_index = usrnum;
                    }
                }
            }
        }
    }

    // print players that escaped
    prfmsg(BBENDESC);

    for (usrnum = 0, brawl_current_player_data_ptr = brawl_player_data_array; usrnum < nterms; ++usrnum, ++brawl_current_player_data_ptr) {
        if (brawl_current_player_data_ptr->userid[0] != '\0' &&
                user[usrnum].state == teleconference_module_index && user[usrnum].substt == TELECONFERENCE_USER_SUBSTATE_NORMAL &&
                (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_ESCAPED ||
                    (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING && brawl_current_player_data_ptr->kill_count < BRAWL_KILLS_UNTIL_OUTLAW_THRESHOLD))) {
            prfmsg(
                BBENDUSR,
                &brawl_current_player_data_ptr->userid,
                brawl_current_player_data_ptr->shots_fired_this_round,
                brawl_current_player_data_ptr->shots_landed_this_round,
                usrnum == current_meanest_player_index ? " - meanest" : "",
                usrnum == current_richest_player_index ? " - richest" : "");
        }
    }

    // print players that got caught
    prfmsg(BBENDPOK);

    for (usrnum = 0, brawl_current_player_data_ptr = brawl_player_data_array; usrnum < nterms; ++usrnum, ++brawl_current_player_data_ptr) {
        if (brawl_current_player_data_ptr->userid[0] != '\0' &&
                user[usrnum].state == teleconference_module_index && user[usrnum].substt == TELECONFERENCE_USER_SUBSTATE_NORMAL &&
                brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING &&
                brawl_current_player_data_ptr->kill_count >= BRAWL_KILLS_UNTIL_OUTLAW_THRESHOLD) {
            prfmsg(
                BBENDUSR,
                &brawl_current_player_data_ptr->userid,
                brawl_current_player_data_ptr->shots_fired_this_round,
                brawl_current_player_data_ptr->shots_landed_this_round,
                usrnum == current_meanest_player_index ? " - meanest" : "",
                usrnum == current_richest_player_index ? " - richest" : "");
        }
    }

    // print players that died
    prfmsg(BBENDDED);

    for (usrnum = 0, brawl_current_player_data_ptr = brawl_player_data_array; usrnum < nterms; ++usrnum, ++brawl_current_player_data_ptr) {
        if (brawl_current_player_data_ptr->userid[0] != '\0' && brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_DEAD) {
            // Includes users who aren't in teleconference, in case they left after dying
            prfmsg(
                BBKILLBY,
                &brawl_current_player_data_ptr->userid,
                brawl_current_player_data_ptr->shots_fired_this_round,
                brawl_current_player_data_ptr->shots_landed_this_round,
                &brawl_current_player_data_ptr->name_of_last_killer,
                usrnum == current_deadest_player_index ? " - deadest" : "");
        }
    }
}


void brawl_handle_sheriff_arrives(void) {
    int current_top_gun_player_index;
    int current_top_gun_player_score;
    int index;
    int temp_current_user_score;

    current_top_gun_player_index = -1;
    current_top_gun_player_score = 0;

    for (usrnum = 0, brawl_current_player_data_ptr = brawl_player_data_array; usrnum < nterms; ++usrnum, ++brawl_current_player_data_ptr) {
        if (brawl_current_player_data_ptr->userid[0] != '\0' &&
                user[usrnum].state == teleconference_module_index && user[usrnum].substt == TELECONFERENCE_USER_SUBSTATE_NORMAL &&
                (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING || brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_ESCAPED)) {

            if (brawl_current_player_data_ptr->player_state == BRAWL_PLAYER_STATE_BRAWLING && brawl_current_player_data_ptr->kill_count >= BRAWL_KILLS_UNTIL_OUTLAW_THRESHOLD) {
                index = 0;
                do {
                    brawl_current_player_data_ptr->holsters[index].weapon = BRAWL_WEAPON_NONE;
                    brawl_current_player_data_ptr->holsters[index].ammo_in_weapon = -1;
                    ++index;
                } while (index < BRAWL_HOLSTER_COUNT);

                brawl_current_player_data_ptr->holsters[0].weapon = BRAWL_STARTING_WEAPON;
                brawl_current_player_data_ptr->holsters[0].ammo_in_weapon = BRAWL_STARTING_WEAPON_AMMO;
                brawl_current_player_data_ptr->money_held = 0;
            } else if (brawl_current_player_data_ptr->rank < BRAWL_PLAYER_RANK_GUNSLINGER && brawl_current_player_data_ptr->kill_count > brawl_current_player_data_ptr->banked_kill_count) {
                if (++brawl_current_player_data_ptr->rank == BRAWL_PLAYER_RANK_BORDER_RIDER) {
                    prfmsg(BBARIDER);
                } else if (brawl_current_player_data_ptr->rank == BRAWL_PLAYER_RANK_SHARPSHOOTER) {
                    prfmsg(BBASHARP);
                } else if (brawl_current_player_data_ptr->rank == BRAWL_PLAYER_RANK_GUNSLINGER) {
                    prfmsg(BBAGUNSL);
                }

                brawl_output_message_to_specified_brawler(usrnum);
            }

            temp_current_user_score = brawl_current_player_data_ptr->rank + (brawl_current_player_data_ptr->kill_count - brawl_current_player_data_ptr->banked_kill_count);

            if (brawl_current_player_data_ptr->money_held - brawl_current_player_data_ptr->banked_money_held > 0) {
                temp_current_user_score = temp_current_user_score + (brawl_current_player_data_ptr->money_held - brawl_current_player_data_ptr->banked_money_held) / BRAWL_BOUNTY_PER_KILL_AS_OUTLAW;
            }

            if (temp_current_user_score > current_top_gun_player_score) {
                current_top_gun_player_score = temp_current_user_score;
                current_top_gun_player_index = usrnum;
            }

            index = 0;
            do {
                brawl_current_player_data_ptr->banked_holsters[index].weapon = brawl_current_player_data_ptr->holsters[index].weapon;
                brawl_current_player_data_ptr->banked_holsters[index].ammo_in_weapon = brawl_current_player_data_ptr->holsters[index].ammo_in_weapon;
                ++index;
            } while (index < BRAWL_HOLSTER_COUNT);

            brawl_current_player_data_ptr->banked_kill_count = brawl_current_player_data_ptr->kill_count;
            brawl_current_player_data_ptr->banked_money_held = brawl_current_player_data_ptr->money_held;
        }
    }

    if (-1 < current_top_gun_player_index) {
        prfmsg(BBTOPGUN, &brawl_player_data_array[current_top_gun_player_index].userid);
        brawl_output_message_to_all_brawlers();
    }
}


void brawl_handle_user_disconnect(void) {
    brawl_current_player_data_ptr = &brawl_player_data_array[usrnum];

    setbtv(brawl_btrieve_db_player_userid_file);
    movmem(usaptr->userid, &brawl_current_player_data_ptr->userid, UIDSIZ);

    if (qeqbtv(usaptr, 0)) {        // Check if player userid record 0 exists for this user
        gabbtv(NULL, absbtv(), 0);  //   if so, then "get" it
        strcpy(brawl_current_player_data_ptr->userid, usaptr->userid);
        updbtv(brawl_current_player_data_ptr->userid);  // And update it with the current user id
    }
}


void brawl_handle_user_delete_account(char* user_id) {
    setbtv(brawl_btrieve_db_player_userid_file);

    if (acqbtv(NULL, user_id, 0)) {  // Check if the player userid file exists for this user id
        delbtv();                    //   if so, then delete it
    }
}


void brawl_handle_system_shutdown(void) {
    clsmsg(brawl_mcv_config_file);
    clsbtv(brawl_btrieve_db_player_userid_file);
}


void brawl_handle_midnight_cleanup(void) {
    // The way this was implemented was a bit hacky. The value is stored in space reserved by the MBBS-API for future use.
    // This is somewhat fine, since the code could just be upgraded when the BBS is versioned, though no version checks are done in this module.
    // But it could also potentially conflict with other modules, if they happen to use the same trick with the same offset.
    ++sv.spare[BRAWL_HACK_OFFSET_TO_STASH_DAYS_MODULE_ACTIVE];
}
