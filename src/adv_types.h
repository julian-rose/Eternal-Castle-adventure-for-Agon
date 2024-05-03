/*
 * Module: adv_types.h
 *
 * Purpose: Data types for adventure
 *
 * Author: Copyright (c) julian rose, jhr
 *
 * Version:	who	when	what
 *		      ---	------	----------------------------------------
 *		      jhr	150417	wrote initial version on cygwin
 *				               test data structure ideas
 *                16xxxx   ported to Android (native backend to a Java frontend)
 *                240429   port to Agon light (eZ80) with 24-bit ints and 32-bit longs
 */

#include <sys/types.h>
#include "adv_obfus.h"

#if !defined( __CYGWIN__ )&& !defined( __clang__ )
# define VARARGS 1
# define DEBUG 1
#endif

#if defined( DEBUG )
# if defined( __CYGWIN__ )
#   define DEBUGF if( __trace )\
                        ( void )printf( "  %s:%d\t", __FILE__, __LINE__ ),\
                        ( void )printf
# elif defined( __clang__ )
#   define DEBUGF if( __trace )\
                        ( void )printf( "  %s:%d\t", __FILE__, __LINE__ ),\
                        ( void )printf
# else
#   include <android/log.h>
#   define  LOG_TAG    "DEBUGF"
#   define  DEBUGF(...) if( __trace )\
                        __android_log_print( ANDROID_LOG_DEBUG, LOG_TAG, "  %s:%d\t", __FILE__, __LINE__ ),\
                        __android_log_print( ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__ )
# endif
#else
#   define DEBUGF ( void )
#endif


/*
 ******************* Definitions
 */
#define MAX_CELL_DESC_LEN 16
#define MAX_CELLS 128 /* sizeof(char): coU32d arrange as 8*8*2 grid */
#define MAX_CELL_OBJECTS 8

#define MAX_CONTAINERS 16
#define MAX_SOURCES 4

#define MAX_AVATAR_NAME_LEN 16
#define MAX_AVATAR_OBJECTS 8

#define MAX_OBJECT_ACTIONS 4

#define MAX_WORLD_NAME_LEN 16
#define MAX_WAYS 6

#define INBUF_LEN  32


/*
 ******************* Enumerations
 */
enum _GAME_OVER
{
   GAME_OVER_AVATAR_WON = 1,
   GAME_OVER_AVATAR_RESOURCE = 2,
   GAME_OVER_LIGHTBEAM_BLOWOUT = 3,
   GAME_OVER_SUN_BLOWOUT = 4,
   GAME_OVER_TRAPPED = 5,
   GAME_OVER_BOARD_BLOWOUT = 6
};

enum _ERR
{
   ERR_none = 0,
   ERR_file = 1,
   ERR_game = 2,
   ERR_game_won = 3
};

enum _TRISTATE
{
   _FALSE = 0,
   _TRUE = 1,
   _UNDECIDED = 2
};
typedef enum _TRISTATE TRISTATE;


/*
 *********************** Types
 */
typedef unsigned char UC;
typedef unsigned char const UCC;
typedef unsigned short int USI;
typedef unsigned int UI;
typedef unsigned int const UIC;
#if defined( __CYGWIN__ )
typedef unsigned int U32;
typedef unsigned int const U32C;
#elif defined( __clang__ )
typedef unsigned long int U32;
typedef unsigned long int const U32C;
#endif
typedef enum _ERR ERR;

typedef struct _data_t
{
   void * data;
   size_t data_sz;
   TRISTATE do_read;
} data_t;

/* Support for 256 disinct handlers (actions);
 * each verb has a unique encoded 32-bit value, e.g. "walk".
 * each action may have several verbs, that mean the same;
 * each verb is assigned to one action function;
 */
typedef UC index_t;

enum _name_t
{
   NAME_NULL = 0,
   NAME_NORTH = 1,   /* for ways, must be 1... */
   NAME_EAST = 2,
   NAME_SOUTH = 3,
   NAME_WEST = 4,
   NAME_UP = 5,
   NAME_DOWN = 6,    /* ...must be 6, for ways */
   NAME_LEFT = 7,
   NAME_RIGHT = 8,
   NAME_AROUND = 9,  /* as in, look around */
   NAME_CELL = 10,
   NAME_WORLD = 11,
   NAME_AVATAR = 12,
   NAME_INVENTORY = 13,  /* as in, show inventory */
   NAME_EVERYTHING = 14, /* as in, drop everything */

   NAME_ENDS = 255 /* a limit of 256 builtin names applies */
} /* indexes to names[] */;

enum _dish
{
      /* Will move telescope 30-degrees each increment,
       * else it appears nothing is happening.
       * So set significant objects on 30-degree boundaries. */
   DISH_FOREST_1 = 8,
     DISH_MAST = 9,
   DISH_FOREST_2 = 17,
     DISH_MOON = 18,
   DISH_MOUNTAINS = 26,
     DISH_SUN = 27,
   DISH_HORIZON = 36
};

enum _action_t
{
   ACTION_NULL = 0,

   ACTION_ASK = 1,
   ACTION_BREAK = 2,
   ACTION_BUILTIN = 3,
   ACTION_CLOSE = 4,
   ACTION_DRINK = 5,
   ACTION_EAT = 6,
   ACTION_EXCHANGE = 7,
   ACTION_FILL = 8,
   ACTION_FIND = 9,
   ACTION_FIX = 10,
   ACTION_FOLLOW = 11,
   ACTION_GIVE = 12,
   ACTION_GO = 13,
   ACTION_HIDE = 14,
   ACTION_INVENTORY = 15,
   ACTION_LISTEN = 16,
   ACTION_LOCK = 17,
   ACTION_LOOK = 18,
   ACTION_MAKE = 19,
   ACTION_MOVE = 20,
   ACTION_NAVIGATION = 21,
   ACTION_OPEN = 22,
   ACTION_PLAY = 23,
   ACTION_PRESS = 24,
   ACTION_PUSH = 25,
   ACTION_READ= 26,
   ACTION_SAY = 27,
   ACTION_SET = 28,
   ACTION_SHOW = 29,
   ACTION_SIT = 30,
   ACTION_SMELL = 31,
   ACTION_STAND = 32,
   ACTION_SWIM = 33,
   ACTION_TAKE = 34,
   ACTION_TOUCH = 35,
   ACTION_TURN = 36,
   ACTION_UNLOCK = 37,
   ACTION_USE = 38,
   ACTION_WEAR = 39,
   ACTION_WRITE = 40,

   ACTION_ENDS = 256  /* a limit of 256 actions applies */
} /* indexes to actions[] */;

typedef struct _code
{
   U32 value;      /* encoded name e.g. "walk" */
   index_t idx;   /* index to code table */
} code_t;


/**************** Types for objects, cells and avatars *****************/

enum _sources
{
   SOURCE_SUN = 0,
   SOURCE_MOON = 1,
   SOURCE_MAGNOTRON = 2,
   SOURCE_ENERGY = 3,

   /* the same meaning, but a different index */
   SOURCE_SUN_TEST = 1,
   SOURCE_MOON_TEST = 2,
   SOURCE_MAGNOTRON_TEST = 4
};

enum _lightbeam_descriptor
{
   L_BLINDING_DIAMOND = 1,
   L_TRIANGLE_DIAMOND = 2,
   L_ENGAGE_DIAMOND = 3,
   L_STANDBY_DIAMOND = 4,
   L_BLINDING_GOLD = 5,
   L_TRIANGLE_GOLD = 6,
   L_ENGAGE_GOLD = 7,
   L_STANDBY_GOLD = 8,
   L_DISENGAGE = 9,
   L_READY_DIAMOND = 10,
   L_READY_GOLD = 11
};
typedef enum _lightbeam_descriptor lb_state;

enum _lightbeam_level
{
   L_SOLID = 100,
   L_FALTERING = 1,
   L_EMPTY = 0
};
typedef enum _lightbeam_level lb_level;

enum _rotary
{
   ROTARY_OFF = 0,
   ROTARY_DISH = 1,
   ROTARY_LIGHTBEAM = 2,
   ROTARY_DISH_MIRROR = 3,
   ROTARY_SPIRE_MIRROR = 4,

   ROTARY_0 = 0,
   ROTARY_1 = 1,
   ROTARY_2 = 2,
   ROTARY_3 = 3,
   ROTARY_4 = 4,
   ROTARY_5 = 5,
   ROTARY_6 = 6
};

/* Support for 256 state descriptors
 */
typedef struct _state_descriptor
{
   char val1_description[ 16 ];   /* the '1' value case */
   char val0_description[ 16 ];   /* the '0' value case */
} state_descriptor_t;

typedef UC state_value_t;

enum _state_descriptor_index
{
      /* ensure STATE_* and CONTAINER_* don't overlap */
   STATE_NULL = 0,

   STATE_MASK__ = 0x40,

   STATE_FIXED  =( STATE_MASK__ | 0x20 ),

   STATE_ON =( STATE_MASK__ | 0x1 ),
   STATE_OPEN =( STATE_MASK__ | 0x2 ),
   STATE_FULL =( STATE_MASK__ | 0x3 ),
   STATE_LOCKED =( STATE_MASK__ | 0x4 ),
   STATE_LIGHT =( STATE_MASK__ | 0x5 ),
   STATE_NUMBER =( STATE_MASK__ | 0x6 ),  /* state number is a counter */
   STATE_PERCENTAGE =( STATE_MASK__ | 0x7 ),  /* state %age is a volume */
   STATE_SPECIAL =( STATE_MASK__ | 0x8 )  /* state special is hard-coded */
};

/* Each object has a single state variable.
 * A state variable is described by a state descriptor, e.g. "open";
 * A state variable has a state or value, e.g. 1=open, 0=closed.
 */
typedef struct _state
{
   UC descriptor;       /* _state_descriptor_index */
   state_value_t value; /* enum _state_value */
} state_t;

/* A condition is an equality test (==) rather than an assignment (=) 
 * on the object current_state and descriptor, e.g. 
 *    current_state.state_descriptor == 0 &&
 *    current_state.state_value == 0
 *  which might translate to "current_state is on".
 *
 * This also allows a test for endgame: if all the object conditions are such,
 *  then game over.
 */
typedef UC cond_test_t;
enum _condition_tests
{
   TEST_NU32L = 0,
   TEST_PRECOND = 1,     /* normal state_descriptors/state_value test */
   TEST_CONTAINER = 2,   /* check inventory has a container */
   TEST_CONTAINER_SPECIAL = 3 /* check inventory has a specific container */
};

typedef struct _condition
{
   cond_test_t type;    /* type of condition test */
   UC descriptor;       /* _state_descriptor_index */
   state_value_t value; /* enum _state_value before applying action */
} condition_t;

/*
 * A direction connects a cell to others;
 */
typedef signed char direction_t;

/*
 * A description encodes adjectives (e.g. dark, blue)
 * together with a landscape (e.g. forest).
 */
typedef struct _longdesc
{
   U32 adjective;
   U32 name;
} description_t;

enum _adjective_t
{
   BLACK =   ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<0)),
   BROWN =   ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<1)),
   RED =     ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<2)),
   ORANGE =  ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<3)),
   YELLOW =  ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<4)),
   GREEN =   ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<5)),
   BLUE =    ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<6)),
   VIOLET =  ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<7)),
   GREY =    ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<8)),
   WHITE =   ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<9)),
   GOLD =    ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<10)),
   SILVER =  ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<11)),
   BRONZE =  ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<12)),
   DIAMOND = ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<13)),
   GLASS =   ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<14)),
   METAL =   ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<15)),
   CERAMIC = ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<16)), 
   PINK =    ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<17)),
   BLONDE =  ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<18)),
   DARK =    ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<19)),
   LEAD =    ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<20)),
   CONTROL = ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<21)),   /* CONTROL needs to be with colours */

   DEEP =         ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<0)),
   SHALLOW =      ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<1)),
   TALL =         ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<2)),
   SHORT =        ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<3)),
   LARGE =        ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<4)),
   SMALL =        ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<5)),
   WARM =         ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<6)),
   COLD =         ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<7)),
   BARRED =       ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<8)),
   BROKEN =       ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<9)),
   FAST =         ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<10)),
   HEAVY =        ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<11)),
   STRAW =        ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<12)),
   HARD =         ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<13)),
   OAK =          ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<14)),
   BEECH =        ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<15)),
   BIRCH =        ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<16)),
   ASH =          ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<17)),
   WORN =         ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<18)),
   OLD =          ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<19)),

   LONG_HAIRED =  ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<1)),
   LAB =          ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<2)),
   ENERGY =       ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<3)),
   COLLECTOR =    ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<4)),
   DINING =       ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<5)),
   SHEET =        ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<6)),
   BAKING =       ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<7)),
   RING =         ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<8)),
   ELECTRIC =     ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<9)),
   ROTARY =       ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<10)),
   WELLINGTON =   ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<11)),
   REFLECTIVE =   ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<12)),
   SAFETY =       ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<13)),
   MINERS =       ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<14)),
   CRIB =         ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<15))
};

enum _landscape_t
{
   /* forest landscapes */
   FOREST =       ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<0)),
   POOL =         ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<1)),
   FLOOR =        ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<2)),
   GLADE =        ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<3)),
   CANOPY =       ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<4)),
   SKY =          ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<5)),
   EDGE =         ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<6)),
   HILLTOP =      ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<7)),
   HILLTOP_DISH = ((( U32 )( 1 )<<31)+(( U32 )( 1 )<<8)),


   /* castle landscapes */
   CASTLE =     ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<0)),
   DRAWBRIDGE = ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<1)),
   MOAT =       ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<2)),
   ROOM =       ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<3)),
   GUARD =      ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<4)),
   COMMON =     ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<5)),
   C_MAP =      ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<6)),
   C_THRONE =   ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<7)),
   C_CONTROL =  ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<8)),
   STAIRWAY =   ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<9)),
   DUNGEON =    ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<10)),
   COURTYARD =  ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<11)),
   KEEP =       ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<12)),
   TOWER =      ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<13)),
   SPIRE =      ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<14)),
   ROOF =       ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<15)),
   KITCHEN =    ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<16)),
   CELLAR =     ((( U32 )( 1 )<<30)+(( U32 )( 1 )<<17)),

   /* mountain landscapes */
   MOUNTAIN =    ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<0)), 
   PASS =        ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<1)), 
   PEAK =        ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<2)), 
   OBSERVATORY = ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<3)), 
   DOME =        ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<4)), 
   TUNNEL =      ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<5)), 
   STOREROOM =   ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<6)), 
   FUNICULAR =   ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<7)), 
   CAVERN =      ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<8)), 
   LABORATORY =  ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<9)), 
   LAB_STORES =  ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<10)), 
   MANUFACTORY = ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<11)), 
   LAKE =        ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<12)),
   BOTTOM =      ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<13)),

   MINE =        ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<14)),
   HEAD =        ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<15)),
   CAGE =        ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<16)),
   SHAFT =       ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<17)),
   ENTRANCE =    ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<18)),
   FACE =        ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<19)),
   OBSERVATION = ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<20)),
   DECK =        ((( U32 )( 1 )<<29)+(( U32 )( 1 )<<21)),

   MAGNOTRON =  ((( U32 )( 1 )<<28)+(( U32 )( 1 )<<0)), 
   COMPLEX =    ((( U32 )( 1 )<<28)+(( U32 )( 1 )<<1)), 
   OPERATION =  ((( U32 )( 1 )<<28)+(( U32 )( 1 )<<2)), 
   ELEMENT_0 =  ((( U32 )( 1 )<<28)+(( U32 )( 1 )<<3)), 
   ELEMENT_1 =  ((( U32 )( 1 )<<28)+(( U32 )( 1 )<<4)), 
   ELEMENT_2 =  ((( U32 )( 1 )<<28)+(( U32 )( 1 )<<5)), 
   ELEMENT_3 =  ((( U32 )( 1 )<<28)+(( U32 )( 1 )<<6)), 
   ELEMENT_4 =  ((( U32 )( 1 )<<28)+(( U32 )( 1 )<<7)), 
   ELEMENT_5 =  ((( U32 )( 1 )<<28)+(( U32 )( 1 )<<8)), 
   ELEMENT_6 =  ((( U32 )( 1 )<<28)+(( U32 )( 1 )<<9)), 

   LANDSCAPE_END = ((( U32 )( 1 )<<27))
};

enum _objectname_t
{
   OBJECT_NULL = 0,

   TREE =           1,
   MUSHROOM =       2,
   FISH =           3,
   WATER =          4,
   FLASK =          5,
   BOOK =           6,
   KEY =            7,
   CLOAK =          8,
   JAR =            9,
   NEST =          10,
   EGGS =          11,
   CROW =          12,
   MONUMENT =      13,
   SPECTACLES =    14,
   PORTCULLIS =    15,
   BROTH =         16,
   BREAD =         17,
   SIGN =          18,
   DOOR =          19,
   WINDOW =        20,
   VEGETATION =    21,
   O_STRAW =       22,
   RAT =           23,
   LIGHTBEAM =     24,
   SWITCH =        25,
   CHAIN =         26,
   O_MAP =         27,
   O_THRONE =      28,
   HEADBAND =      29,
   GNOME =         30,
   SNOW =          31,
   GOGGLES =       32,
   TELESCOPE =     33,
   SEAT =          34,
   JOYSTICK =      35,
   BUTTON =        36,
   O_FUNICULAR =   37,
   O_MOUNTAIN =    38,
   LEVER =         39,
   MONKEY =        40,
   WIZARD =        41,
   BURET =         42,
   BURNER =        43,
   TESTTUBE =      44,
   CRUCIBLE =      45,
   STAND =         46,
   SOURCE =        47,
   HOIST =         48,
   SHAFT_HEAD =    49,
   O_CAGE =        50,
   BAT =           51,
   DIVINGSUIT =    52,
   CAP =           53,
   HAT =           54,
   SHADES =        55,
   DISH =          56,
   O_CASTLE =      57,
   O_RING =        58,
   TABLE =         59,
   CHAIRS =        60,
   PICK =          61,
   SHOVEL =        62,
   SCREEN =        63,
   PAINTINGS =     64,
   REMOTE =        65,
   PANEL =         66,
   PAPER =         67,
   WILBURY =       68,
   SAND =          69,
   MIRROR =        70,
   POT =           71,
   TRAY =          72,
   LADDER =        73,
   BRACKET =       74,
   LENS =          75,
   FILTER =        76,
   SUN =           77,
   MOON =          78,
   DUST =          79,
   O_MAGNOTRON =   80,
   TRAVELATOR =    81,
   MISSION =       82,
   BOARD =         83,
   MAGNET =        84,
   CART =          85,
   CAMP_FIRE =     86,
   SLIDER_SWITCH = 87,
   BOOTS =         88,
   JACKET =        89,
   MOLE =          90,
   LAMP =          91,
   TAP =           92,
   SINK =          93,
   METER =         94,
   RACK =          95,
   PINION =        96,
   WHEEL =         97,
   NOTE =          98,
   TRAIL =         99,
   PLOUGH =       100

     /* can be any 32-bit upper bound */
};

/*
 * A property assigns attributes to an object.
 */
typedef USI properties_t;

enum _properties_t
{
   PROPERTY_MOBILE = 1<<0,    /* 1=can be taken, 0=cannot be taken */
   PROPERTY_PERPETUAL = 1<<1, /* 1=persists after taking, 0=not */
   PROPERTY_SELECTED = 1<<2,  /* 1=is selected (held in hand), 0=not */
   PROPERTY_CONTAINER = 1<<3, /* 1=can carry an object, 0=not */
   PROPERTY_HIDDEN = 1<<4,    /* 1=is invisible, 0=not */
   PROPERTY_WEARABLE = 1<<5,  /* 1=can be worn, 0=not */
   PROPERTY_WORN = 1<<6,      /* 1=is worn, 0=not */
   PROPERTY_NOFREE = 1<<7,    /* 1=is not free-standing, 0=is */
   PROPERTY_FIXPOINT = 1<<8   /* 1=can be fixed to, 0=not */
} /* bit masks for properties_t */ ;

/* Each object has a single encoded noun, e.g. "tree";
 * Each object may have several adjectives, e.g. "blue", which let several
 *  similar objects be distinguished;
 * An object has properties, e.g. mobile if it can be moved.
 * Each object has a number of actions that can be applied to it, 
 *  by avatar e.g. "climb" tree.
 * Each object may allow the avatar to traverse cells (e.g. climb tree).
 *  directional actions or nouns (e.g. Go up) apply.
 */
typedef struct _object
{
   U32 o_magic;
   description_t output; /* enumerated desc/adj for output, e.g. BLUE | DOOR */
   description_t input;  /* encoded name/adj for input test, e.g. "blue door" */
   properties_t properties;  /* object properties */

   state_t current_state;    /* state_descriptors for the object, e.g. "open";
                                or in the case of containers, the output name */

      /* permitted actions, in addition to directional verbs */
   index_t actions[ MAX_OBJECT_ACTIONS ];  
      /* state conditions on actions */
   condition_t pre_cond[ MAX_OBJECT_ACTIONS ];
      /* action-resU32ting state change */
   state_t post_state[ MAX_OBJECT_ACTIONS ];

   direction_t ways[ MAX_WAYS ];   /* ways (N,E,S,W,U,D) to adjacent cells */
} object_t;

/* static actions and states hard-coded into adventure games */
typedef void( *action_fcn_t )( U32C, U32C, object_t * const, UCC );

/* Containers can hold objects:
 * some objects, like water, can only be picked up if the avatar has a 
 * suitable empty container;
 * when a container is picked up or put down, its contents are likewise
 * relocated;
 * when a certain object is picked up, the container must be filled;
 * when a contained object is consumed, e.g. water is drunk, the container 
 * must be emptied. */
enum   /* container 'names' enumerations (index into adv_world.containers[]) */
{
   CONTAINER_0 = 0,
   CONTAINER_1 = 1,
   CONTAINER_2 = 2,
   CONTAINER_3 = 3,
   CONTAINER_4 = 4,
   CONTAINER_5 = 5,
   CONTAINER_6 = 6,
   CONTAINER_7 = 7,
   CONTAINER_8 = 8,
   CONTAINER_9 = 9,
   CONTAINER_10 = 10,
   CONTAINER_11 = 11,
   CONTAINER_12 = 12,
   CONTAINER_13 = 13,
   CONTAINER_14 = 14,
   CONTAINER_15 =( MAX_CONTAINERS - 1 )
};

/* The world is arranged into cells;
 * each cell has a number of objects, some of which can be taken and new ones
 * can be given;
 * each avatar exists in a cell. */
enum _cell_name  /* cell 'names' enumerations (index into adv_world.cells[]) */
{
   C_NULL =               0,
   C_TRASH =              1,

   C_FOREST_GLADE =       2,
   C_FOREST =             3,
   C_FOREST_CANOPY =      4,
   C_FOREST_POOL =        5,
   C_FOREST_POOL_FLOOR =  6,
   C_DEEP_FOREST =        7,
   C_FOREST_EDGE =        8,
   C_HILLTOP =            9,
   C_HILLTOP_DISH =      10,

   C_CASTLE_DRAWBRIDGE =   11,
   C_CASTLE_MOAT =         12,
   C_CASTLE_GUARDSROOM =   13,
   C_CASTLE_COMMONROOM =   14,
   C_CASTLE_TOWER =        15,
   C_CASTLE_MAPROOM =      16,
   C_CASTLE_DUNGEON =      17,
   C_CASTLE_THRONEROOM =   18,
   C_CASTLE_CONTROLROOM =  19,
   C_CASTLE_COURTYARD =    20,
   C_CASTLE_KEEP =         21,
   C_CASTLE_SPIRE =        22,
   C_CASTLE_SPIRE_ROOF =   23,
   C_CASTLE_KITCHEN =      24,
   C_CASTLE_CELLAR =       25,

   C_MOUNTAIN_PASS =             26,
   C_MOUNTAIN_PEAK =             27,
   C_MOUNTAIN_OBSERVATORY =      28,
   C_MOUNTAIN_OBSERVATORY_DOME = 29,
   C_MOUNTAIN_TUNNEL =           30,
   C_MOUNTAIN_STORES =           31,
   C_MOUNTAIN_FUNICULAR =        32,
   C_MOUNTAIN_CAVERN =           33,
   C_MOUNTAIN_LAB =              34,
   C_MOUNTAIN_LAB_STORES =       35,
   C_MOUNTAIN_MANUFACTORY =      36,

   C_MOUNTAIN_LAKE =        37,
   C_MOUNTAIN_LAKE_MID =    38,
   C_MOUNTAIN_LAKE_BOTTOM = 39,

   C_MINE_HEAD =     40,
   C_MINE_OBSERVATION_DECK = 41,
   C_MINE_CAGE =     42,
   C_MINE_SHAFT =    43,
   C_MINE_ENTRANCE = 44,
   C_MINE_TUNNEL =   45,
   C_MINE_FACE =     46,

   C_MAGNOTRON_COMPLEX =    47,
   C_MAGNOTRON_OPERATION =  48,
   C_MAGNOTRON_ELEMENT_0 =  49,
   C_MAGNOTRON_ELEMENT_1 =  50,
   C_MAGNOTRON_ELEMENT_2 =  51,
   C_MAGNOTRON_ELEMENT_3 =  52,
   C_MAGNOTRON_ELEMENT_4 =  53,
   C_MAGNOTRON_ELEMENT_5 =  54,
   C_MAGNOTRON_ELEMENT_6 =  55,

   C_NAMED_CELLS,

   C_MAX_CELL =( MAX_CELLS - 1 )  /* direction is 'signed char' to allow -C_xxx */
};

/*
 * A cell is at a fixed location in the world.
 * Each cell can be described, e.g. "a forest";
 * Objects can be placed or removed from a cell.
 * An avatar (or mobile objects) can navigate between cells, provided there is
 * a way: North=0, East=1, South=2, West=3, Up=4, Down=5.
 * No actions are associated with cells, rather specific actions or nouns (as
 *  in Go North) can apply: North, South, West, East, Up, Down.
 */
typedef struct _cell
{
   U32 c_magic;  /* const 0xce11 */
   description_t output; /* enumerated desc/adj for output, e.g. BLUE | DOOR */
   description_t input;  /* encoded name/adj for input test, e.g. "blue door" */
   object_t objects[ MAX_CELL_OBJECTS ];
   direction_t ways[ MAX_WAYS ];   /* possible directions to adjacent cells */
} cell_t;

/*
 * A location is a cell coordinate;
 * cells are fixed in the world, but an avatar can move around.
 */
typedef UC location_t;

/*
 * An avatar has a name, and can carry several objects.
 * An avatar is at a location can move around the world cells.
 * An avatar can drop and pick up (mobile) objects.
 * An avatar needs food and drink for sustenance.
 */

enum _avatar_name
{
   A_WILBURY = 0,   /* the main character, undefined species */
   A_LUCY = 1,      /* a crow, owns the skies */
   A_GARY = 2,      /* the resident dungeon rat */
   A_NELLY = 3,     /* the castle gnome, runs the castle */
   A_BUSTER = 4,    /* a mole, works the magnotron */
   A_HARRY = 5,     /* the bat, hangs out in the mines */
   A_LOFTY = 6,     /* the black wizard, mysterious dude */
   A_OTTO = 7,      /* the lab rat, a wizard maker works the lab */
   A_CHARLIE = 8,   /* long-haired wizard, works the observatory */

   A_NUM_AVATARS,   /* for assigning arrays */

   A_MASK = 128
};

typedef struct _avatar
{
   U32 a_magic;
   char name[ MAX_AVATAR_NAME_LEN + 1 ];
   description_t output; /* enumerated desc/adj for output, e.g. BLUE | DOOR */
   description_t input;  /* encoded name/adj for input test, e.g. "blue door" */
   location_t location;
   state_t current_state;  /* mU32ti-purpose avatar state */
   object_t inventory[ MAX_AVATAR_OBJECTS ];
   U32 help;
   UC hunger;   /* decrement each move; top up by eating something */
   UC thirst;   /* ditto; top up by drinking something */
} avatar_t;

/*
 * The world has a name, and simply consists of cells (3D-grid).
 * It is static.
 */
typedef struct _world
{
   U32 w_magic;
   char name[ MAX_WORLD_NAME_LEN + 1 ];
   cell_t cells[ MAX_CELLS ];
   object_t containers[ MAX_CONTAINERS ];
   object_t sources[ MAX_SOURCES ];
   ERR errnum;

} world_t;



/*
 ************************* Prototypes
 */
void throw( int const l, ERR const e );

ERR adv__welcome( void );
ERR adv__goodbye( TRISTATE const );
void adv__print_welcome( void );
void adv__print_goodbye( void );
void adv__game_over( enum _GAME_OVER const );

void adv__new( void );
void adv__load( void );
void adv__save( void );
ERR adv__read( void );
ERR adv__write( void );
void adv__print( char const * const );
void adv__print_error( ERR const );

ERR adv__get_input( void );
void display_initial_help( void );
void set_inbuf( UC const * const ins, size_t const );
ERR adv__perform_input( void );
UC adv__get_location( void );

ERR adv__test_input( TRISTATE* );
ERR adv__test_input_verb( UC** );
ERR adv__test_input_adverb( UC** );
ERR adv__test_input_noun( UC**, UC** );

void adv__on_exit( TRISTATE const );

