/*
 * Module: adv_run.c
 *
 * Purpose: Runtime for adventure game
 *
 * Author: Copyright (c) julian rose, jhr
 *
 * Version:	who	when	what
 *		      ---	------	----------------------------------------
 *		      jhr	150417	wrote initial version on cygwin
 *	               			test basic command line
 *                16xxxx   ported to Android (native backend to a Java frontend)
 *                240429   port to Agon light (eZ80) with 24-bit ints and 32-bit longs
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "adv_types.h"
#include "adv_dict.h"


/*
 ************************* Global Data
 */
extern world_t  adv_world;
extern avatar_t adv_avatar[ A_NUM_AVATARS ];
extern TRISTATE fullGameEnabled;
extern TRISTATE fullGameLimitHit;


int __trace = 0;
char outbuf[ OUTBUF_LEN ];   /* output buffer */


/*
 ************************* Private Data
 */
static unsigned long int inputs = 0;

static object_t empty_object ={ 0 };

static UC inbuf[ INBUF_LEN ];     /* input buffer */
static UC *verb = 0;       /* pointer into inbuf */
static UC *adverb = 0;     /* pointer into inbuf */
static UC *noun = 0;       /* pointer into inbuf */
static UC *nadj= 0;        /* pointer into inbuf */

static avatar_t *cavatar = &adv_avatar[ A_WILBURY ];  /* current avatar */
static object_t *containers = &adv_world.containers[ 0 ];


/*
 ***************** Macro declarations
 */
#define IS_VOWEL(x) /* write 'a tree', 'an apple' */  \
   (('a'==(x))||('e'==(x))||('i'==(x))||('o'==(x))||('u'==(x)))?"an":"a"

#define SET_AVATAR_LOCATION( a, x ) \
   { ( a )->location =( x ); \
     if( C_MAGNOTRON_COMPLEX <=( x ) && C_MAGNOTRON_ELEMENT_6 >=( x )) \
        adv_avatar[ A_BUSTER ].location =( x ); /* pink mole comes along */ }
#define SET_AVATAR_THIRST( a, x ) { (a)->thirst =( x ); }
#define SET_AVATAR_HUNGER( a, x ) { (a)->hunger=( x ); }
#define CLEAR_AVATAR_INVENTORY( a, cobj, aobj ) \
   {( cobj )= *( aobj ); *( aobj )= empty_object; }
#define SET_AVATAR_INVENTORY( a, aobj, cobj ) \
   { *( aobj )=( cobj ); ( aobj )->properties &= ~PROPERTY_PERPETUAL; \
     if( 0 ==( PROPERTY_PERPETUAL &( cobj ).properties ))\
        ( cobj )= empty_object; }


/*
 ***************** Private function declarations
 */
static void get_words( void );
static TRISTATE strip_word( UC*[ ], UCC );
static void get_verb_and_noun( UCC, UC*[ ]);
static void get_verb_and_string( void );
static char get_vowel_of_object( description_t const * );
static index_t find_code( 
      U32 const, code_t const *, size_t const, U32, U32, UCC );
static index_t find_action( U32C );
static U32 adv__encode_name( UCC * const );

static int find_space_in_cell( UCC );
static object_t *find_object_in_cell( U32C, U32C, UCC );
static object_t *find_object_in_world( U32C, U32C, UC* );
static object_t *find_object_with_avatar( U32C, U32C );
static direction_t find_noun_as_cell_in_current( U32C, U32C );
static UC find_noun_as_cell_in_world( U32C );
static object_t const *find_noun_in_cell( U32C, U32C, UCC );
static object_t const *find_noun_as_object_in_current( U32C, U32C );
static object_t const *find_noun_as_object_in_world( U32C, U32C );
static object_t const *find_noun_as_object_with_avatars( U32C, U32C, UCC );
static index_t const find_noun_as_avatar( U32C, U32C, TRISTATE );
static index_t const find_noun_as_avatar_rtol( void );
static index_t const find_noun_as_builtin( U32C );
static TRISTATE test_and_set_verb( 
      object_t * const, enum _action_t const, TRISTATE const );

static void find_selected( object_t ** );
static int test_inventory( avatar_t*, U32C, U32C, int* );
static void test_inventory_selected( avatar_t*, int* );
static void set_inventory_selected( avatar_t*, object_t* );
static void test_inventory_worn( avatar_t*, int* );  /* initialise 2nd parm */
static void test_inventory_free_space( avatar_t*, int* );
static TRISTATE give_object( object_t* );
static TRISTATE take_object( object_t* );

static void test_cell_selected( int* );
static void set_cell_selected( object_t * const );

static TRISTATE container_test( object_t const * const, int* );

static void make_fix( object_t*, object_t* );
static void break_fix( object_t* );
static TRISTATE test_fix( object_t const*, object_t const* );

static void adv__print_status( TRISTATE const );
static void adv__look_around( UC, UC, int );
static void do_periodic_checks( void );
static void do_avatar_mobile_checks( TRISTATE const );

static ERR check_avatars( void );
static void swap_avatar( U32C );
static int distance( location_t const, location_t const );
static void print_inventory( avatar_t const * const );
static void print_wearing( avatar_t * const );

static void print_object_state( state_t const * const );
static void print_object_desc( description_t const * const );
static void print_adjective( U32C );
static void print_cell_desc( description_t const * const );
static void print_cell_objects( UC const );
static UC print_cell_avatars( UC const, TRISTATE const );

static lb_state get_lightbeam_state( void );
static enum _adjective_t get_lightbeam_colour( void );
static void lightbeam_check( void );
static void print_lightbeam_state( TRISTATE const );
static void lightbeam_event( object_t * const );
static void lightbeam_test( void );
static void energy_test( object_t* );
static void mirror_test( void );
static void sun_test( void );
static void magnotron_test( void );
static void print_meter_state( void );

static enum _rotary get_rotary_switch_state( enum _cell_name const );
static void print_rotary_switch_state( enum _cell_name const );
static void set_rotary_switch( enum _cell_name const, UCC );
static void set_slider_switch( object_t *, UCC );
static void set_element_state( UCC );
static void print_element_state( UCC );

static void make_stuff( U32C, U32C );
static void make_board( enum _avatar_name const );
static int get_count_of_boards_in_world( void );


/*
 ************************** Action Related
 */
static void a_ask( U32C, U32C, object_t *, UCC );
static void a_break( U32C, U32C, object_t *, UCC );
static void a_builtin( U32C, U32C, object_t *, UCC );
static void a_close( U32C, U32C, object_t *, UCC );
static void a_drink( U32C, U32C, object_t *, UCC );
static void a_eat( U32C, U32C, object_t *, UCC );
static void a_exchange( U32C, U32C, object_t *, UCC );
static void a_find( U32C, U32C, object_t *, UCC );
static void a_fill( U32C, U32C, object_t *, UCC );
static void a_fix( U32C, U32C, object_t *, UCC );
static void a_follow( U32C, U32C, object_t *, UCC );
static void a_give( U32C, U32C, object_t *, UCC );
static void a_go( U32C, U32C, object_t *, UCC );
static void a_hide( U32C, U32C, object_t *, UCC );
static void a_inventory( U32C, U32C, object_t *, UCC );
static void a_listen( U32C, U32C, object_t *, UCC );
static void a_lock( U32C, U32C, object_t *, UCC );
static void a_look( U32C, U32C, object_t *, UCC );
static void a_make( U32C, U32C, object_t *, UCC );
static void a_move( U32C, U32C, object_t *, UCC );
static void a_navigation( U32C, U32C, object_t *, UCC );
static void a_open( U32C, U32C, object_t *, UCC );
static void a_play( U32C, U32C, object_t *, UCC );
static void a_press( U32C, U32C, object_t *, UCC );
static void a_push( U32C, U32C, object_t *, UCC );
static void a_read( U32C, U32C, object_t *, UCC );
static void a_say( U32C, U32C, object_t *, UCC );
static void a_set( U32C, U32C, object_t *, UCC );
static void a_show( U32C, U32C, object_t *, UCC );
static void a_sit( U32C, U32C, object_t *, UCC );
static void a_smell( U32C, U32C, object_t *, UCC );
static void a_stand( U32C, U32C, object_t *, UCC );
static void a_swim( U32C, U32C, object_t *, UCC );
static void a_take( U32C, U32C, object_t *, UCC );
static void a_touch( U32C, U32C, object_t *, UCC );
static void a_turn( U32C, U32C, object_t *, UCC );
static void a_unlock( U32C, U32C, object_t *, UCC );
static void a_use( U32C, U32C, object_t *, UCC );
static void a_wear( U32C, U32C, object_t *, UCC );
static void a_write( U32C, U32C, object_t *, UCC );

action_fcn_t const actions[ ]= /* max size 256 limited by action_t */
{
   0,            /* ACTION_NULL */
   a_ask,        /* ACTION_ASK (ask) */
   a_break,      /* ACTION_BREAK (Break, Smash, Hit) */
   a_builtin,    /* ACTION_BUILTIN (save, load, quit, new, help, what, goto, 
                                    why, how, when, __tr, __go) */
   a_close,      /* ACTION_CLOSE (Close) */
   a_drink,      /* ACTION_DRINK (Drink) */
   a_eat,        /* ACTION_EAT (Eat, Taste) */
   a_exchange,   /* ACTION_EXCHANGE (Exchange, Swap) */
   a_fill,       /* ACTION_FILL (Fill) */
   a_find,       /* ACTION_FIND (Find, Search, Seek) */
   a_fix,        /* ACTION_FIX (Fix, Attach, Fit) */
   a_follow,     /* ACTION_FOLLOW (Follow) */
   a_give,       /* ACTION_GIVE (Put, Give, Throw, Drop, Pay) */
   a_go,         /* ACTION_GO (Go, Walk, Climb, Run) */
   a_hide,       /* ACTION_HIDE (Hide, Secret, Cover) */
   a_inventory,  /* ACTION_INVENTORY (Inv, Inventory, i) */
   a_listen,     /* ACTION_LISTEN (Listen, Hear) */
   a_lock,       /* ACTION_LOCK (Lock) */
   a_look,       /* ACTION_LOOK (Look, See, Watch, Where, Examine) */
   a_make,       /* ACTION_MAKE (Make, Do, Create, Build) */
   a_move,       /* ACTION_MOVE (Move, Adjust) */
   a_navigation, /* ACTION_NAVIGATION (North, South, West, East, Up, Down) */
   a_open,       /* ACTION_OPEN (Open) */
   a_play,       /* ACTION_PLAY (Play) */
   a_press,      /* ACTION_PLAY (Press) */
   a_push,       /* ACTION_PUSH (Push, Pull) */
   a_read,       /* ACTION_READ (Read) */
   a_say,        /* ACTION_SAY (Say, Tell, Speak) */
   a_set,        /* ACTION_SET (Set, Switch, Light) */
   a_show,       /* ACTION_SHOW (Show, Offer) */
   a_sit,        /* ACTION_SIT (Sit) */
   a_smell,      /* ACTION_SMELL (Smell) */
   a_stand,      /* ACTION_STAND (Stand) */
   a_swim,       /* ACTION_SWIM (Swim) */
   a_take,       /* ACTION_TAKE (Take, Get, Pick, Carry, Bring, Buy, Catch) */
   a_touch,      /* ACTION_TOUCH(Touch, Feel) */
   a_turn,       /* ACTION_TURN (Turn, Rotate) */
   a_unlock,     /* ACTION_USE (Unlock) */
   a_use,        /* ACTION_USE (Use, Select, Choose) */
   a_wear,       /* ACTION_WEAR (Wear) */
   a_write,      /* ACTION_WRITE (Write, Draw) */
};


/**************** Verbs *******************/

code_t const verbs[ ]=  /* sort ascii-betical on value */
{
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 0 )<< 0 )), ACTION_NULL },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'd' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'i' )<< 0 )), ACTION_INVENTORY },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'n' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 's' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'u' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'w' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 'd' )<< 8 )+(( U32 )( 'o' )<< 0 )), ACTION_MAKE },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'o' )<< 0 )), ACTION_GO },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'p' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 'k' )<< 0 )), ACTION_ASK },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'b' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'y' )<< 0 )), ACTION_TAKE },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_EAT },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'f' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_FIX },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'f' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 'x' )<< 0 )), ACTION_FIX },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'g' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_TAKE },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_BREAK },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'w' )<< 0 )), ACTION_BUILTIN },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'v' )<< 0 )), ACTION_INVENTORY },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'n' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'w' )<< 0 )), ACTION_BUILTIN },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'p' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'y' )<< 0 )), ACTION_GIVE },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'p' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_GIVE },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'n' )<< 0 )), ACTION_GO },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 's' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'y' )<< 0 )), ACTION_SAY },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 's' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_LOOK },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 's' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_SET },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 's' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_SIT },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_USE },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'h' )<< 8 )+(( U32 )( 'y' )<< 0 )), ACTION_BUILTIN },
#  if defined( DEBUG )
   { (( '_'<<24 )+( '_'<<16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'o' )<< 0 )), ACTION_BUILTIN },
   { (( '_'<<24 )+( '_'<<16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'r' )<< 0 )), ACTION_BUILTIN },
#  endif
   { ((( U32 )( 'a' )<< 24 )+(( U32 )( 'd' )<< 16 )+(( U32 )( 'j' )<< 8 )+(( U32 )( 'u' )<< 0 )), ACTION_MOVE },
   { ((( U32 )( 'a' )<< 24 )+(( U32 )( 't' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'a' )<< 0 )), ACTION_FIX },
   { ((( U32 )( 'b' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'a' )<< 0 )), ACTION_BREAK },
   { ((( U32 )( 'b' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 'n' )<< 0 )), ACTION_TAKE },
   { ((( U32 )( 'b' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 'l' )<< 0 )), ACTION_MAKE },
   { ((( U32 )( 'c' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'r' )<< 0 )), ACTION_TAKE },
   { ((( U32 )( 'c' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'c' )<< 0 )), ACTION_TAKE },
   { ((( U32 )( 'c' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'o' )<< 0 )), ACTION_USE },
   { ((( U32 )( 'c' )<< 24 )+(( U32 )( 'l' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 'm' )<< 0 )), ACTION_GO },
   { ((( U32 )( 'c' )<< 24 )+(( U32 )( 'l' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 's' )<< 0 )), ACTION_CLOSE },
   { ((( U32 )( 'c' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'v' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_HIDE },
   { ((( U32 )( 'c' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'a' )<< 0 )), ACTION_MAKE },
   { ((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'w' )<< 8 )+(( U32 )( 'n' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 'd' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'w' )<< 0 )), ACTION_WRITE },
   { ((( U32 )( 'd' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 'n' )<< 0 )), ACTION_DRINK },
   { ((( U32 )( 'd' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'p' )<< 0 )), ACTION_GIVE },
   { ((( U32 )( 'e' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 'e' )<< 24 )+(( U32 )( 'x' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'm' )<< 0 )), ACTION_LOOK },
   { ((( U32 )( 'e' )<< 24 )+(( U32 )( 'x' )<< 16 )+(( U32 )( 'c' )<< 8 )+(( U32 )( 'h' )<< 0 )), ACTION_EXCHANGE },
   { ((( U32 )( 'f' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'l' )<< 0 )), ACTION_TOUCH },
   { ((( U32 )( 'f' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 )), ACTION_FILL },
   { ((( U32 )( 'f' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'd' )<< 0 )), ACTION_FIND },
   { ((( U32 )( 'f' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 )), ACTION_FOLLOW },
   { ((( U32 )( 'g' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'v' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_GIVE },
   { ((( U32 )( 'h' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'r' )<< 0 )), ACTION_LISTEN },
   { ((( U32 )( 'h' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'p' )<< 0 )), ACTION_BUILTIN },
   { ((( U32 )( 'h' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'd' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_HIDE },
   { ((( U32 )( 'h' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'd' )<< 0 )), ACTION_USE },
   { ((( U32 )( 'i' )<< 24 )+(( U32 )( 'n' )<< 16 )+(( U32 )( 'v' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_INVENTORY },
   { ((( U32 )( 'l' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'h' )<< 0 )), ACTION_SET },
   { ((( U32 )( 'l' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_LISTEN },
   { ((( U32 )( 'l' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'd' )<< 0 )), ACTION_BUILTIN },
   { ((( U32 )( 'l' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'c' )<< 8 )+(( U32 )( 'k' )<< 0 )), ACTION_LOCK },
   { ((( U32 )( 'l' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'k' )<< 0 )), ACTION_LOOK },
   { ((( U32 )( 'm' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'k' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_MAKE },
   { ((( U32 )( 'm' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'v' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_MOVE },
   { ((( U32 )( 'n' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 'o' )<< 24 )+(( U32 )( 'f' )<< 16 )+(( U32 )( 'f' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_SHOW },
   { ((( U32 )( 'o' )<< 24 )+(( U32 )( 'p' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'n' )<< 0 )), ACTION_OPEN },
   { ((( U32 )( 'p' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'c' )<< 8 )+(( U32 )( 'k' )<< 0 )), ACTION_TAKE },
   { ((( U32 )( 'p' )<< 24 )+(( U32 )( 'l' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'y' )<< 0 )), ACTION_PLAY },
   { ((( U32 )( 'p' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 's' )<< 0 )), ACTION_PRESS },
   { ((( U32 )( 'p' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 )), ACTION_PUSH },
   { ((( U32 )( 'p' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 'h' )<< 0 )), ACTION_PUSH },
   { ((( U32 )( 'q' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_BUILTIN },
   { ((( U32 )( 'r' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'd' )<< 0 )), ACTION_READ },
   { ((( U32 )( 'r' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'a' )<< 0 )), ACTION_TURN },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'v' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_BUILTIN },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'r' )<< 0 )), ACTION_FIND },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'c' )<< 8 )+(( U32 )( 'r' )<< 0 )), ACTION_HIDE },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'k' )<< 0 )), ACTION_FIND },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_USE },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'w' )<< 0 )), ACTION_SHOW },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'm' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 's' )<< 0 )), ACTION_BREAK },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'm' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'l' )<< 0 )), ACTION_SMELL },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'p' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'a' )<< 0 )), ACTION_SAY },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 't' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'n' )<< 0 )), ACTION_STAND },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'p' )<< 0 )), ACTION_EXCHANGE },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 'm' )<< 0 )), ACTION_SWIM },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_SET },
   { ((( U32 )( 't' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'k' )<< 8 )+(( U32 )( 'e' )<< 0 )), ACTION_TAKE },
   { ((( U32 )( 't' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_EAT },
   { ((( U32 )( 't' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 )), ACTION_SAY },
   { ((( U32 )( 't' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'o' )<< 0 )), ACTION_GIVE },
   { ((( U32 )( 't' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'c' )<< 0 )), ACTION_TOUCH },
   { ((( U32 )( 't' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'n' )<< 0 )), ACTION_TURN },
   { ((( U32 )( 'u' )<< 24 )+(( U32 )( 'n' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'o' )<< 0 )), ACTION_UNLOCK },
   { ((( U32 )( 'w' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'k' )<< 0 )), ACTION_GO },
   { ((( U32 )( 'w' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'c' )<< 0 )), ACTION_LOOK },
   { ((( U32 )( 'w' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'r' )<< 0 )), ACTION_WEAR },
   { ((( U32 )( 'w' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_NAVIGATION },
   { ((( U32 )( 'w' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_BUILTIN },
   { ((( U32 )( 'w' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'n' )<< 0 )), ACTION_BUILTIN },
   { ((( U32 )( 'w' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'r' )<< 0 )), ACTION_LOOK },
   { ((( U32 )( 'w' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 )), ACTION_WRITE }
};


static code_t const names[ ]=  /* sort ascii-betical on value */
{
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 0 )<< 0 )), NAME_NULL },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'e' )<< 0 )), NAME_EAST },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'n' )<< 0 )), NAME_NORTH },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 's' )<< 0 )), NAME_SOUTH },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'w' )<< 0 )), NAME_WEST },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'p' )<< 0 )), NAME_UP },
   { ((( U32 )( 0 )<< 24 )+(( U32 )( 's' )<< 16 )+(( U32 )( 'k' )<< 8 )+(( U32 )( 'y' )<< 0 )), NAME_UP },
   { ((( U32 )( 'a' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'u' )<< 0 )), NAME_AROUND },
   { ((( U32 )( 'a' )<< 24 )+(( U32 )( 'v' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 't' )<< 0 )), NAME_AVATAR },
   { ((( U32 )( 'c' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 )), NAME_CELL },
   { ((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'w' )<< 8 )+(( U32 )( 'n' )<< 0 )), NAME_DOWN },
   { ((( U32 )( 'e' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 't' )<< 0 )), NAME_EAST },
   { ((( U32 )( 'e' )<< 24 )+(( U32 )( 'v' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'r' )<< 0 )), NAME_EVERYTHING },
   { ((( U32 )( 'i' )<< 24 )+(( U32 )( 'n' )<< 16 )+(( U32 )( 'v' )<< 8 )+(( U32 )( 'e' )<< 0 )), NAME_INVENTORY },
   { ((( U32 )( 'l' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'f' )<< 8 )+(( U32 )( 't' )<< 0 )), NAME_LEFT },
   { ((( U32 )( 'm' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'n' )<< 0 )), NAME_UP },
   { ((( U32 )( 'n' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 't' )<< 0 )), NAME_NORTH },
   { ((( U32 )( 'r' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'h' )<< 0 )), NAME_RIGHT },
   { ((( U32 )( 's' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 't' )<< 0 )), NAME_SOUTH },
   { ((( U32 )( 'w' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 't' )<< 0 )), NAME_WEST },
   { ((( U32 )( 'w' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'l' )<< 0 )), NAME_WORLD }
};


static char const * discards[ ]=
{
   /* don't put the following in discards:
    * "i"  inventory;
    * "on", "off" - needed for lamps etc. */
   "the", "is", "am", "a", "at", "in", "to", "with", "do",
   "you", "we", "can", "know", "of", "for", "does", "some"
};


/*
 ******************************** State Descriptors
 *  are words to display the state of an object.
 *  They are linked to state_t and condition_t types.
 */
state_descriptor_t const state_descriptors[ 
     /* max 256 limited by state_value_t */ ]=
{
   { "", ""  /* STATE_NULL */ },
   { "on"  , "off"         /* STATE_ON */ },
   { "open" , "closed"     /* STATE_OPEN */ },
   { "full" , "empty"      /* STATE_FULL */ },
   { "locked" , "unlocked" /* STATE_LOCKED */ },
   { "lit" , "unlit"       /* STATE_LIGHT */ }
};

/*
 ************************* Action Functions
 */

static void a_ask( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   UC n;
   object_t *p = 0;
   cell_t *c;
   TRISTATE spoken = _FALSE;
   int i = -1;
   int j;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( wcode & A_MASK )
   {
      DEBUGF( "Testing for avatar\n" );
      n = wcode & ~A_MASK;

      if( &( adv_avatar[ n ]) == cavatar )
      {
         actions[ ACTION_BUILTIN ]( 0x68656c70 /* help */, NAME_NULL, 0, 0 );
      }
      else
      if( 0 == distance( cavatar->location, adv_avatar[ n ].location ))
      {
         get_verb_and_string( );

         /* general questions, to any avatar */
            /* see if an avatar name/type can be found in recovered string,
             * pointed to by noun, right-to-left to recover the last name */
         j = find_noun_as_avatar_rtol( );
         if( NAME_ENDS != j )
         {
            /* asking about any other avatar, give their location */
            PRINTS _THE END_PRINT
            print_object_desc( &( adv_avatar[ n ].output ));
            PRINTS _SAYS, _COMMA, _YOULL, _FIND, _THE END_PRINT
            print_object_desc( &( adv_avatar[ j & ~A_MASK ].output ));
            PRINTS _IN, _THE END_PRINT
            c = &( adv_world.cells[ adv_avatar[ j & ~A_MASK ].location ]);
            print_cell_desc( &( c->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
            spoken = _TRUE;
         }

         /* apply verb <ncode> to avatar */
         if( MONKEY == adv_avatar[ n ].output.name )
         {
            PRINTS _THE END_PRINT
            if(( _TRUE == adv_strstr( noun, ( UCC* )"energy", INBUF_LEN ))||
               ( _TRUE == adv_strstr( noun, ( UCC* )"energio", INBUF_LEN )))
            {
               /* make energy from raw matrials */
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _SAYS, _QUOTE_LEFT, _RIGHT, _COMMA, _LETS, _GIVE, _THIS,
                        _STRANGE, _MAGIC, _A, _TRY, _ELIPSIS, _QUOTE_RIGHT, 
                        _NEWLINE END_PRINT
               make_stuff( ENERGY, SOURCE );
            }
            else
            if(( _FALSE != adv_strstr( noun, ( UCC* )"mirror", INBUF_LEN ))||
               ( _FALSE != adv_strstr( noun, ( UCC* )"spegulon", INBUF_LEN )))
            {
                  /* make mirrors from raw materials;
                   * the colour depends on the colour of the lightbeam:
                   * silver/diamond or gold */
               if( SILVER == get_lightbeam_colour( ))
               {
                  print_object_desc( &( adv_avatar[n].output ));
                  PRINTS _SAYS, _QUOTE_LEFT, _COMING, _UP, _COMMA,
                           _ONE, _SILVERED, _TICKET, _TO, _THE, _MOON,
                           _QUOTE_RIGHT, _ELIPSIS, _NEWLINE END_PRINT
                  make_stuff( SILVER, MIRROR );
               }
               else
               if( GOLD == get_lightbeam_colour( ))
               {
                  print_object_desc( &( adv_avatar[n].output ));
                  PRINTS _SAYS, _QUOTE_LEFT, _COMING, _UP, _COMMA,
                           _ONE, _GOLDEN, _TICKET, _TO, _THE, _MOON,
                           _QUOTE_RIGHT, _ELIPSIS, _NEWLINE END_PRINT
                  make_stuff( GOLD, MIRROR );
               }
               else
               {
                  print_object_desc( &( adv_avatar[n].output ));
                  PRINTS _SAYS, _QUOTE_LEFT, _I, _DONT, _KNOW, _WHAT, _COLOUR,
                           _THE, _LIGHTBEAM, _IS, _QUOTE_RIGHT,
                           _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            else
            if(( _TRUE == adv_strstr( noun, ( UCC* )"board", INBUF_LEN ))||
               ( _FALSE != adv_strstr( noun, ( UCC* )"control", INBUF_LEN ))||
               ( _FALSE != adv_strstr( noun, ( UCC* )"estraro", INBUF_LEN )))
            {
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _SAYS, _QUOTE_LEFT, _ANOTHER, _OF, _THE, _DEADLY, 
                         _SINS, _QUOTE_RIGHT, _FULLSTOP, _NEWLINE END_PRINT
               make_stuff( CONTROL, BOARD );
            }
            else
            if(( _TRUE == adv_strstr( noun, ( UCC* )"filter", INBUF_LEN ))||
               ( _TRUE == adv_strstr( noun, ( UCC* )"filtrilon", INBUF_LEN )))
            {
               /* get a lens filter, before pointing the telescope at 
                * the sun. */
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _SAYS, _QUOTE_LEFT, _ONE, _SUN, _RAY, _LENS,
                        _COMING, _UP, _QUOTE_RIGHT, _FULLSTOP, _NEWLINE 
                        END_PRINT
               make_stuff( DARK, FILTER );
            }
            else
            if( _TRUE == adv_strstr( noun, ( UCC* )"lenso", INBUF_LEN ))
            {
               /* lens */
            }
            else
            if( _TRUE == adv_strstr( noun, ( UCC* )"saltilo", INBUF_LEN ))
            {
               /* switch */
            }
            else
            if( _TRUE == adv_strstr( noun, ( UCC* )"slosilo", INBUF_LEN ))
            {
               /* key */
            }
            else
            if( _TRUE == adv_strstr( noun, ( UCC* )"teo", INBUF_LEN ))
            {
               /* tea */
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _SAYS, _QUOTE_LEFT, _THE, _KETTLE, _IS, _BROKEN, _COMMA, 
                      _YOULL, _HAVE, _TO, _MAKE, _DO, _WITH, _LAKE, _WATER, 
                      _OR_, _SNOW, _FULLSTOP, _QUOTE_RIGHT, _NEWLINE END_PRINT
            }
            else
            {
               if( _FALSE == spoken )
               {
                  print_object_desc( &( adv_avatar[n].output ));
                  PRINTS _EYES, _ME, _EXPECTANTLY, _ELIPSIS, _NEWLINE END_PRINT
               }
            }
         }
         else
         if(( WIZARD == adv_avatar[ n ].output.name )&&
            ( LONG_HAIRED == adv_avatar[ n ].output.adjective ))
         {
            PRINTS _THE END_PRINT
            print_object_desc( &( adv_avatar[n].output ));

            if(( _TRUE == adv_strstr( noun, ( UCC* )"filter", INBUF_LEN ))||
               ( _TRUE == adv_strstr( noun, ( UCC* )"filtrilon", INBUF_LEN )))
            {
               PRINTS _SAYS, _QUOTE_LEFT, _ESC_U, _THERE, _ARE, _NO, _SPARE, 
                      _FILTERS, _FULLSTOP, _YOULL, _HAVE, _TO, _ASK END_PRINT
               SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
               PRINTS _TO, _MAKE, _ONE, _FULLSTOP, _QUOTE_RIGHT, 
                      _NEWLINE END_PRINT
            }
            else
            {
               if( _FALSE == spoken )
               {
                  PRINTS _EYES, _ME, _SPECULATIVELY, _ELIPSIS, 
                         _NEWLINE END_PRINT
               }
            }
         }
         else
         if(( WIZARD == adv_avatar[ n ].output.name )&&
            ( BLACK == adv_avatar[ n ].output.adjective ))
         {
            PRINTS _THE END_PRINT
            print_object_desc( &( adv_avatar[n].output ));

            p = find_object_in_cell(
                  ((( U32 )( 'l' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'h' )<< 0 )), 0, 
                  C_CASTLE_SPIRE_ROOF );
            if( 2 > p->current_state.value )
            {
               PRINTS _ASKS, _QUOTE_LEFT, _DID, _YOU, _BRING, _A, _TUBE, 
                        _OF, _ENERGY, _QUESTION, _QUOTE_RIGHT,
                        _NEWLINE END_PRINT
            }
            else
            {
               if( _FALSE == spoken )
               {
                  PRINTS _IGNORES, _ME, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
         }
         else
         if( MOLE == adv_avatar[ n ].output.name )
         {
            if(( _TRUE == adv_strstr( noun, ( UCC* )"tunnel", INBUF_LEN ))||
               ( _FALSE != adv_strstr( noun, ( UCC* )"magnet", INBUF_LEN )))
            {
               /* make energy from raw matrials */
               PRINTS _THE END_PRINT
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _SAYS, _QUOTE_LEFT, _ITS, _ALREADY, _DONE, _MISTER,
                        _FULLSTOP, _QUOTE_RIGHT, _NEWLINE END_PRINT
            }
            else
            if(( _TRUE == adv_strstr( noun, ( UCC* )"board", INBUF_LEN ))||
               ( _FALSE != adv_strstr( noun, ( UCC* )"control", INBUF_LEN )))
            {
               make_board( A_BUSTER );
            }
            else
            if( _TRUE == adv_strstr( noun, ( UCC* )"ring", INBUF_LEN ))
            {
               i=-1, test_inventory_worn( &adv_avatar[ A_BUSTER ], &i );
               test_inventory_free_space( cavatar, &j );
               if(( 0 <= i )&&( 0 <= j ))
               {
                  p = &adv_avatar[ A_BUSTER ].inventory[ i ];
                  if(( GOLD == p->output.adjective )&&
                     ( O_RING == p->output.name ))
                  {
                     PRINTS _THE END_PRINT
                     print_object_desc( &( adv_avatar[ A_BUSTER ].output ));
                     PRINTS _HANDS, _ME, _THE END_PRINT
                     print_object_desc( &( p->output ));
                     PRINTS _COMMA, _AND_, _SAYS END_PRINT
                     /* Buster's ring is the key to the magnotron complex */
                     PRINTS _QUOTE_LEFT, _ESC_U, _DESCEND, _INTO, _THE,
                              _MINES, _AND_, _DISEMBARK, _THE, _CAGE, _AT, 
                              _LEVEL, _1, _TO, _ENTER, _THE, _RING, _COMPLEX, 
                              _QUOTE_RIGHT, _FULLSTOP, _NEWLINE END_PRINT

                     p->properties &= ~PROPERTY_WORN;
                     cavatar->inventory[ j ]= *p;
                     *p = empty_object;
                  }
               }
               else
               {
                  SPRINT_( outbuf, "%s", adv_avatar[ A_BUSTER ].name );
                  PRINTS _SAYS, _COLON, _I, _CANNOT, _GIVE, _YOU, _THE 
                         END_PRINT
                  SPRINT_( outbuf, "%s", noun );
                  PRINTS _FULLSTOP, _YOU, _ARE, _ALREADY, _CARRYING, _ALL, _YOU,
                         _CAN, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            else
            if( _TRUE == adv_strstr( noun, ( UCC* )"math", INBUF_LEN ))
            {
               ( void )test_inventory( &adv_avatar[ A_BUSTER ], 
                  (( U32 )( 'n' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'e' )<< 0 ), 
                  (( U32 )( 'c' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 'b' )<< 0 ), &i );
               if( 0 <= i )
               {
                  p = &adv_avatar[ A_BUSTER ].inventory[ i ];
                  test_inventory_free_space( cavatar, &j );
                  if( 0 <= j )
                  {
                     SPRINT_( outbuf, "%s", adv_avatar[ A_BUSTER ].name );
                     PRINTS _HANDS, _ME, _A END_PRINT
                     print_object_desc( &( p->output ));
                     PRINTS _FULLSTOP, _NEWLINE END_PRINT

                     p->properties &= ~PROPERTY_HIDDEN;
                     cavatar->inventory[ j ]= *p;
                     *p = empty_object;
                  }
                  else
                  {
                     SPRINT_( outbuf, "%s", adv_avatar[ A_BUSTER ].name );
                     PRINTS _SAYS, _COLON, _I, _CANNOT, _GIVE, _YOU, _THE 
                            END_PRINT
                     print_object_desc( &( p->output ));
                     PRINTS _FULLSTOP, _YOU, _ARE, _ALREADY, _CARRYING, _ALL, 
                           _YOU, _CAN, _FULLSTOP, _NEWLINE END_PRINT
                  }
               }
               else
               {
                  SPRINT_( outbuf, "%s", adv_avatar[ A_BUSTER ].name );
                  PRINTS _SAYS, _COLON, _I, _CANNOT, _GIVE, _YOU, _MY, _CRIB,
                         _NOTE, _SEMICOLON, _I, _DONT, _KNOW, _WHERE, _IT, _IS,
                         _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            else
            {
               if( _FALSE == spoken )
               {
                  print_object_desc( &( adv_avatar[n].output ));
                  PRINTS _EYES, _ME, _SPECULATIVELY, _ELIPSIS, 
                         _NEWLINE END_PRINT
               }
            }
         }
         else
         if( BAT == adv_avatar[ n ].output.name )
         {
            /* check it's dark */
            p =( object_t * )find_noun_as_object_in_current(
                  ((( U32 )( 'l' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'm' )<< 8 )+(( U32 )( 'p' )<< 0 )), 0 );
            i =( p )
               ? p->current_state.value
               : 0;
            if( 0 == i )
            {
               if(( _TRUE == adv_strstr( noun, ( UCC* )"way", INBUF_LEN ))||
                  ( _FALSE != adv_strstr( noun, ( UCC* )"where", INBUF_LEN ))||
                  ( _FALSE != adv_strstr( noun, ( UCC* )"direction", INBUF_LEN )))
               {
                  PRINTS _THE END_PRINT
                  print_object_desc( &( adv_avatar[ n ].output ));
                  PRINTS _SAYS, _FOLLOW, _ME, _FULLSTOP END_PRINT
                  a_follow( 0, NAME_NULL, 0,( A_HARRY | A_MASK ));
               }
               else
               {
                  if( _FALSE == spoken )
                  {
                     PRINTS _THE END_PRINT
                     print_object_desc( &( adv_avatar[n].output ));
                     PRINTS _EYES, _ME, _SPECULATIVELY, _ELIPSIS, _NEWLINE 
                        END_PRINT
                  }
               }
            }
            else
            {
               PRINTS _THE END_PRINT
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _IS, _ELUSIVE, _IN, _THE, _LIGHT, _FULLSTOP,
                      _NEWLINE END_PRINT
            }
         }
         else
         if( RAT == adv_avatar[ n ].output.name )
         {
            if(( C_CASTLE_MAPROOM == adv_avatar[ n ].location )&&
               ( _FALSE != adv_strstr( noun, ( UCC* )"bronze", INBUF_LEN ))&&
               ( _TRUE == adv_strstr( noun, ( UCC* )"key", INBUF_LEN )))
            {
               p = find_object_in_cell(
                    ((( U32 )( 0 )<< 24 )+(( U32 )( 'k' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'y' )<< 0 )),
                    ((( U32 )( 'b' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'n' )<< 0 )),
                    C_CASTLE_MAPROOM );
               if( 0 == p )
               {
                  p = find_object_in_cell(
                    ((( U32 )( 0 )<< 24 )+(( U32 )( 'k' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'y' )<< 0 )),
                    ((( U32 )( 'b' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'n' )<< 0 )),
                    C_CASTLE_MOAT );
                  if( 0 == p )
                  {
                     p = find_object_in_cell(
                       ((( U32 )( 0 )<< 24 )+(( U32 )( 'k' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'y' )<< 0 )),
                       ((( U32 )( 'b' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'n' )<< 0 )),
                       C_CASTLE_DUNGEON );
                  }

                  PRINTS _THE END_PRINT
                  print_object_desc( &( adv_avatar[n].output ));
                  PRINTS _DISAPPEARS, _THROUGH, _A, _CRACK, _IN, _THE,
                         _WALL, _ELIPSIS, _QUICKLY, _RETURNING END_PRINT
                  /* move bronze key from moat to map room */
                  if( p )
                  {
                     i = find_space_in_cell( C_CASTLE_MAPROOM );
                     if( 0 <= i )
                     {
                        PRINTS _WITH, _THE END_PRINT
                        print_object_desc( &( p->output ));
                        PRINTS _FULLSTOP END_PRINT

                        /* do the swap */
                        adv_world.cells[ C_CASTLE_MAPROOM ].objects[ i ]= *p;
                        *p = empty_object;
                     }
                  }

                  p = find_object_in_cell(
                       ((( U32 )( 0 )<< 24 )+(( U32 )( 'k' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'y' )<< 0 )),
                       ((( U32 )( 'b' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'n' )<< 0 )),
                       C_CASTLE_MAPROOM );
                  if( 0 == p )
                  {
                     PRINTS _AND_, _SAYS, _QUOTE_LEFT, _I, _DONT, _KNOW, _WHERE,
                            _THE, _BRONZE, _KEY, _IS, _MISTER, _FULLSTOP,
                            _NEWLINE END_PRINT
                     adv__game_over( GAME_OVER_TRAPPED );
                     adv_world.errnum = ERR_game;
                  }

                  PRINTS _NEWLINE END_PRINT
                  spoken = _TRUE;
               }
            }

            if( _FALSE == spoken )
            {
               PRINTS _THE END_PRINT
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS END_PRINT
            }
         }
         else
         if( GNOME == adv_avatar[ n ].output.name )
         {
            if(( C_CASTLE_THRONEROOM == adv_avatar[ n ].location )&&
               ( _FALSE != adv_strstr( noun, ( UCC* )"remote", INBUF_LEN )))
            {
               PRINTS _THE END_PRINT
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _NODS, _TOWARDS, _THE, _THRONE, _FULLSTOP,
                      _NEWLINE END_PRINT
               spoken = _TRUE;
            }

            if( _FALSE == spoken )
            {
               PRINTS _THE END_PRINT
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _EYES, _ME, _SPECULATIVELY, _ELIPSIS, _NEWLINE
                  END_PRINT
            }
         }
         else
         if( WILBURY == adv_avatar[ n ].output.name )
         {
            if( _FALSE == spoken )
            {
               PRINTS _THE END_PRINT
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _EYES, _ME, _SPECULATIVELY, _ELIPSIS, _NEWLINE END_PRINT
            }
         }
         else
         {
            if( _FALSE == spoken )
            {
               PRINTS _THE END_PRINT
               print_object_desc( &( adv_avatar[n].output ));
               PRINTS _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS, _NEWLINE END_PRINT
            }
         }
      }
      else
      {
         PRINTS _I, _DONT, _SEE, _IT, _HERE, _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif

      PRINTS _IT, _DOESNT, _RESPOND, _FULLSTOP, _NEWLINE END_PRINT
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I, _ASK, _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_break( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_BREAK, _TRUE );
      if( _TRUE == allowed )
      {
         /* specific post-cond actions for ACTION_BREAK */
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _IS, _NOW END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I, _BREAK, _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_builtin( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   /* recognised verbs:
    * save, load, quit, new, help */
   DEBUGF( "builtin\n" );

   get_verb_and_string( );

   switch( vcode )
   {
      case 0x68656c70 /* help */ :
      case 0x77686174 /* what */ :
      {
         if( 0 == cavatar->help )
         {
            PRINTS _DISCOVER, _WHAT, _I, _CAN, _DO, _AND_, _WHAT, _YOUR, 
                     _MISSION, _IS, _FULLSTOP END_PRINT
            PRINTS _DIRECT, _ME, _USING, _SHORT, _COMMANDS, _COMMA,
                     _MOSTLY, _OF, _THE, _FORM, _QUOTE_LEFT, _LESS_, _VERB, 
                     _MORE_, _LESS_, _NOUN, _MORE_, _QUOTE_RIGHT, _COLON 
                     END_PRINT
            PRINTS _FOR, _EXAMPLE, 
                     _QUOTE_LEFT, _LOOK, _AROUND, _QUOTE_RIGHT, _COMMA, 
                     _QUOTE_LEFT, _GO, _NORTH, _QUOTE_RIGHT, _COMMA, 
                     _QUOTE_LEFT, _CLIMB, _TREE, _QUOTE_RIGHT, _COMMA, 
                     _QUOTE_LEFT, _TAKE, _WATER, _QUOTE_RIGHT, _FULLSTOP
                     END_PRINT
            PRINTS _I, _ALSO, _UNDERSTAND, _A, _FEW, _SHORTCUTS, _INCLUDING, 
                     _QUOTE_LEFT, _W_, _QUOTE_RIGHT, _FOR, _GO, _WEST, _COMMA, 
                     _QUOTE_LEFT, _i, _QUOTE_RIGHT, _FOR, _SHOW, _INVENTORY, 
                     _FULLSTOP END_PRINT
            PRINTS _AT, _ANY, _TIME, _YOU, _CAN, _QUOTE_LEFT, _QUIT, 
                     _QUOTE_RIGHT, _AND_, _ON, _EACH, _PLAY, _WE,
                     _RESUME, _WHERE, _YOU, _LEFT, _OFF, _FULLSTOP,
                     _NEWLINE END_PRINT
            cavatar->help++;
         }
         else
         if( 1 == cavatar->help )
         {
            PRINTS _SORRY, _THERE, _IS, _NO, _CONTEXT, _MINUS, _SENSITIVE,
                   _HELP, _COMMA END_PRINT
            PRINTS _BUT, _I, _KNOW, _A, _FEW, _1, _MINUS, _WORD, _GAME, 
                     _COMMANDS, _COMMA, _INCLUDING,
                     _QUOTE_LEFT, _NEW, _QUOTE_RIGHT, _TO, _START, _OVER, 
                     _COMMA, 
                     _QUOTE_LEFT, _SAVE, _QUOTE_RIGHT, _AND_, 
                     _QUOTE_LEFT, _LOAD, _QUOTE_RIGHT, 
                     _TO, _WORK, _WITH, _SAVE, _POINTS, _FULLSTOP 
                     END_PRINT
            PRINTS _OH, _AND_, _I, _NEED, _A, _CONTAINER, _COMMA, _LIKE, _A, 
                     _JAR, _COMMA, _TO, _CARRY, _CERTAIN, _ITEMS, _LIKE, 
                     _WATER, _FULLSTOP END_PRINT
            PRINTS _AND_, _I, _NEED, _CERTAIN, _ITEMS, _AS, _TOOLS, _COMMA, 
                     _LIKE, _KEYS, _TO, 
                     _QUOTE_LEFT, _UNLOCK, _DOOR, _QUOTE_RIGHT, 
                     _FULLSTOP, _NEWLINE END_PRINT
            cavatar->help++;
         }
         else
         if( 2 == cavatar->help )
         {
            PRINTS _I, _CAN, _INTERACT, _WITH, _OTHER, _AVATARS, _SEMICOLON,
                     _SOME, _RESPOND, _TO, _SPECIFIC, _PHRASES, _YOU, _SAY,
                     _FULLSTOP END_PRINT
            PRINTS _FOR, _EXAMPLE, _COMMA, _YOU, _CAN, _QUOTE_LEFT, _ASK,
                   _WIZARD, _ABOUT, _FILTER, _QUOTE_RIGHT, _FULLSTOP
                   END_PRINT
            PRINTS _I, _ALSO, _UNDERSTAND, _A, _FEW, _SHORT, _PHRASES,
                     _COMMA, _SUCH, _AS, 
                     _QUOTE_LEFT, _OPEN, _BLUE, _DOOR, _QUOTE_RIGHT, _COMMA, 
                     _OR_, _QUOTE_LEFT, _FIX, _MIRROR, _TO, _BRACKET, 
                     _QUOTE_RIGHT, _FULLSTOP, _NEWLINE END_PRINT
            cavatar->help++;
         }
         else
         if( 3 == cavatar->help )
         {
            /* Part of the game is learning to operate Wilbury and interact.
             * So we don't want to offer comprehensive help. 
             * We could think about some context-help, e.g. help 'fix' which
             * would say something about the fix verb.
             * Moreover, adding help e.g. 'list the words you know', would 
             * make the executable much larger. */
            PRINTS _FINDING, _OUT, _ABOUT, _THIS, _WORLD, _COMMA,
                     _YOUR, _MISSION, _IN, _IT, _AND_, _HOW, _TO, _COMMAND,
                     _ME, _AND_, _USE, _ITEMS, _COMMA, _IS, _ALL, _PART,
                     _OF, _THE, _GAME, _FULLSTOP, _NEWLINE END_PRINT
            cavatar->help = 0;
         }
      }
      break;

      case 0x00776879 /* why */ :
      {
         PRINTS _I, _DONT, _KNOW, _FULLSTOP, _NEWLINE END_PRINT
      }
      break;

      case 0x00686f77 /* how */ :
      case 0x7768656e /* when */ :
      {
         PRINTS _I, _CANNOT, _SAY, _FULLSTOP, _NEWLINE END_PRINT
      }
      break;

      case 0x6c6f6164 /* load */ :
      {
         /* load saved versions of world and avatar files */
         adv__load( );
      }
      break;

      case 0x006e6577 /* new */ :
      {
         /* load defaults for world and avatar structures */
         adv__new( );
         ( void )adv__welcome( );
         inputs = 0;
      }
      break;

      case 0x73617665 /* save */ :
      {
         /* save current world and avatar structures to file */
         adv__save( );
      }
      break;

      case 0x71756974 /* quit */ :
      {
         /* save any changes, then end */
         adv__on_exit( _TRUE );
      }
      break;

#     if defined( DEBUG )
      case 0x5f5f676f /* __goto */ :
      {
         /* goto: relocate avatar 
          * input is 'goto n cell': <verb>goto\0nnn\0<noun>cell\0 */
         DEBUGF( "Goto: %s %s\n", noun, nadj );
         SET_AVATAR_LOCATION( cavatar, atoi(( char* )nadj ));
      }
      break;

      case 0x5f5f7472 /* __trace */ :
      {
         /* __trace: enable debug print
          * input is '__tr': <verb>__tr\0 */
         __trace = 1 - __trace;
      }
      break;
#     endif

      default :
      {
         /* how did this happen? */
      }
      break;
   }
}

/* Close is intended for objects like: door, window, chest */
static void a_close( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   object_t *p;
   int can;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_CLOSE, _TRUE );
      if( _TRUE == allowed )
      {
            /* specific post-cond actions for ACTION_CLOSE */
         if((( BLUE == obj->output.adjective )||
             ( YELLOW == obj->output.adjective ))&&
            ( SWITCH & obj->output.name ))
         {
            lightbeam_event( obj );
         }
         else
         if((((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 ))==
               obj->input.name )&&
            (((( U32 )( 't' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'w' )<< 8 )+(( U32 )( 'e' )<< 0 ))==
               obj->input.adjective))
         {
            p = find_object_in_cell( obj->input.name, obj->input.adjective, 
                      C_CASTLE_TOWER );
            p->current_state = obj->current_state;
         }
         else
         if( CAP == obj->output.name )
         {
            /* check the laser is disengaged */
            switch( get_lightbeam_state( ))
            {
               case L_DISENGAGE :
               case L_READY_DIAMOND :
               case L_READY_GOLD :
               {
                  /* the energy test-tube in the cap should be made hidden */
                     /* find the test-tube stored in the energy cap */
                  can = obj->current_state.descriptor;
                  p = &containers[ can ];
                  if( TESTTUBE == p->output.name )
                  {
                     p->properties |= PROPERTY_HIDDEN;
                  }
               }
               break;

               default :
               {
                  /* blow out, and game over */
                  adv__game_over( GAME_OVER_LIGHTBEAM_BLOWOUT );
                  adv_world.errnum = ERR_game;
               }
               break;
            }
         }
         else
         if( TAP == obj->output.name )
         {
            /* the sink doesn't get filled anymore */
            p = find_object_in_cell(
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'k' )<< 0 ), 0,
                  C_CASTLE_KITCHEN );
            can = p->current_state.descriptor;
            if( OBJECT_NULL != containers[ can ].output.name )
            {
               containers[ can ].properties &= ~PROPERTY_PERPETUAL;
            }
         }

         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _IS, _NOW END_PRINT
         print_object_state( &( obj->current_state ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I, _CLOSE, _QUESTION, _NEWLINE END_PRINT
   }
}

/* a_give, a_take, a_drink & a_eat work with containers. */
/* avatar can drink an object from the inventory or from the current cell */
static void a_drink( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   int can;
   int n;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_DRINK, _TRUE );
      if( _TRUE == allowed )
      {
         /* if taken from the avatar inventory, the object should not
          * be persistent, so LASTLY delete it;
          * it will be persistent if e.g. drinking from pool */
         if( 0 ==( PROPERTY_PERPETUAL & obj->properties ))
         {
              /* save a copy of the object in trash, so a noun search
               * still works. */
            n = find_space_in_cell( C_TRASH );
            if( 0 <= n )
            {
               adv_world.cells[ C_TRASH ].objects[ n ]= *obj;
            }

            /* must empty a container */
            if( _TRUE == container_test( obj, &n ))
            {
               can = cavatar->inventory[ n ].current_state.descriptor;
               containers[ can ]= empty_object;
            }

            *obj = empty_object;
         }

            /* a_drink is a special case, in that it recharges the avatar */
         if(( 255 - 35 )>= cavatar->thirst )
         {
            cavatar->thirst += 35;
         }

         PRINTS _THATS, _REFRESHING, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I, _DRINK, _QUESTION, _NEWLINE END_PRINT
   }
}

/* a_give, a_take, a_drink & a_eat work with containers. */
static void a_eat( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   int n;
   int can;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_EAT, _TRUE );
      if( _TRUE == allowed )
      {
         /* if taken from the avatar inventory, the object should not
          * be persistent, so LASTLY delete it */
         if( 0 ==( PROPERTY_PERPETUAL & obj->properties ))
         {
              /* save a copy of the object in trash, so a noun search
               * still works. */
            n = find_space_in_cell( C_TRASH );
            if( 0 <= n )
            {
               adv_world.cells[ C_TRASH ].objects[ n ]= *obj;
            }

            /* must empty a container */
            if( _TRUE == container_test( obj, &n ))
            {
               can = cavatar->inventory[ n ].current_state.descriptor;
               containers[ can ]= empty_object;
            }
            else
            {
               *obj = empty_object;
            }
         }

         /* a_eat is a special case, in that it recharges the avatar */
         if(( 255 - 35 )>= cavatar->hunger )
         {
            cavatar->hunger += 35;
         }

         PRINTS _THATS, _SUSTAINING, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
      DEBUGF( "Testing for avatar\n" );
      n = wcode & ~A_MASK;

      /* apply verb <ncode> to avatar */
      if( CROW == adv_avatar[ n ].output.name )
      {
         PRINTS _THE, _CROW, _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS, _THEN,
                  _FLIES, _AWAY, _FULLSTOP, _NEWLINE END_PRINT
         /* temporarily store the crow in cell C_NULL */
         adv_avatar[ n ].location = C_NULL;
         /* the crow is restored to the treetop when the avatar is
          * hungry again */
      }
      else
      if( RAT == adv_avatar[ n ].output.name )
      {
         PRINTS _THE, _RAT, _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS, _THEN,
                  _SCURRIES, _OUT, _OF, _SIGHT, _FULLSTOP, _NEWLINE END_PRINT
         /* temporarily store the rat in cell C_NULL */
         adv_avatar[ n ].location = C_NULL;
         /* the rat is restored to the dungeon when the avatar is
          * hungry again */
      }
   }
   else
   {
      DEBUGF( "Testing for builtin to avatar\n" );
      PRINTS _WHAT, _SHOULD, _I, _EAT, _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_exchange( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   int i;
   object_t *p = 0;
   object_t *q = 0;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* test for specific object behaviours */
      {
         /* search object to see if verb is an allowed action */
         allowed = test_and_set_verb( obj, ACTION_EXCHANGE, _TRUE );
         if( _TRUE == allowed )
         {
            /* check there is a non-HIDDEN object of type in the inventory and 
             * another in the current cell, then swap them */
            q = find_object_with_avatar(
                   obj->input.name, obj->input.adjective );
            p =( object_t* )find_noun_in_cell( 
                  obj->input.name, obj->input.adjective, cavatar->location );
            if(( p )&&( q ))
            {
               /* find a temporary swap space in C_NULL */
               i = find_space_in_cell( C_NULL );
               if( 0 <= i )
               {
                  /* do the swap */
                  adv_world.cells[ C_NULL ].objects[ i ]= *p;
                  *p = *q;
                  *q = adv_world.cells[ C_NULL ].objects[ i ];
                  adv_world.cells[ C_NULL ].objects[ i ]= empty_object;
               }
            }
            else
            {
               allowed = _FALSE;
            }

            if( _FALSE == allowed )
            {
               PRINTS _I, _MUST, _BE, _CARRYING, _A END_PRINT
               print_object_desc( &( obj->output ));
               PRINTS _AND_, _THERE, _MUST, _BE, _ANOTHER END_PRINT
               print_object_desc( &( obj->output ));
               PRINTS _IN, _THE, _CURRENT, _CELL, _TO, _SWAP, _WITH,
                        _FULLSTOP, _NEWLINE END_PRINT
            }
            else
            {
               PRINTS _THE END_PRINT
               print_object_state( &( q->current_state ));
               print_object_desc( &( q->output ));
               PRINTS _AND_ END_PRINT
               print_object_state( &( p->current_state ));
               print_object_desc( &( p->output ));
               PRINTS _ARE, _SWAPPED, _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         else
         {
            /* action is not allowed on this object */
            PRINTS _I, _CANNOT END_PRINT
            SPRINT_( outbuf, "%s", verb );
            PRINTS _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }
   }
   else
   if( wcode & A_MASK )
   {
      DEBUGF( "apply verb 0x%lx to avatar %d\n", vcode, wcode );
      swap_avatar( wcode & ~A_MASK );
   }
   else
   {
      DEBUGF( "Testing for builtin to avatar\n" );
      PRINTS _WHAT, _SHOULD, _I, _EXCHANGE, _QUESTION, _NEWLINE END_PRINT
   }
}

/* a_give, a_fill, a_take, a_drink & a_eat work with containers. */
static void a_fill( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   /* was meant to be used with containers, but is really just another
    * form of take? */
   actions[ ACTION_TAKE ]( vcode, ncode, obj, wcode );
}

static void a_fix( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   object_t *p = 0;
   object_t *q = 0;
   cell_t *c = 0;
   description_t d ={ 0 };
   U32 acode;
   TRISTATE allowed = _FALSE;
   int n;
   int i;
   int can;


   /* Attach or fix an object to someplace;
    * Input of form 'fix x to y' becomes verb, noun=x, adverb=y */
   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif

      /* find fixable target (adverb) in current cell */
      acode = adv__encode_name( adverb );
      p = find_object_in_cell( acode, 0, cavatar->location );
      if(( p )&&( PROPERTY_FIXPOINT & p->properties ))
      {
            /* test if source (object) is held in inventory */
         q = find_object_with_avatar( obj->input.name, obj->input.adjective );
         if( q )
         {
            n = find_space_in_cell( cavatar->location );
            if( 0 <= n )
            {
               d = q->output;

                  /* apply fix relationship */
               make_fix( q, p );

                  /* special case tests */
               if( MIRROR == obj->output.name )
               {
                  lightbeam_event( obj );
               }
               else
               if( BOARD == obj->output.name )
               {
                  /* test the lightbeam is off, otherwise some electrical
                   * badness happens */
                  switch( get_lightbeam_state( ))
                  {
                     case L_DISENGAGE :
                     case L_READY_DIAMOND :
                     case L_READY_GOLD :
                     {
                        /* the lightbeam is off, all is well */
                     }
                     break;

                     default :
                     {
                        PRINTS _OOPS, _THE, _LIGHTBEAM, _IS, _ON,
                           _SEMICOLON, _THE, _BOARD, _SHORTS, _OUT, _AND_, _IS,
                           _DESTROYED, _AS, _I, _TRY, _TO, _FIX, _IT, 
                           _FULLSTOP END_PRINT
                        *obj = empty_object;
                        p = find_object_in_cell(
                               (( U32 )( 'b' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'r' )<< 0 ), 
                               0, C_MOUNTAIN_MANUFACTORY );
                        if( 8 <= p->current_state.value )
                        {
                           PRINTS _NEWLINE END_PRINT

                           /* blow out, and game over */
                           adv__game_over( GAME_OVER_BOARD_BLOWOUT );
                           adv_world.errnum = ERR_game;
                        }
                        else
                        {
                           PRINTS _LUCKILY, _WE, _CAN, _MAKE, _ONE, _MORE,
                                  _FULLSTOP, _NEWLINE END_PRINT
                        }
                     }
                     break;
                  }
               }

               c = &( adv_world.cells[ cavatar->location ]);
               if( _TRUE == container_test( obj, &i ))
               {
                  /* must empty a container into cell */
                  can = cavatar->inventory[ i ].current_state.descriptor;
                  c->objects[ n ]= containers[ can ];
                  containers[ can ]= empty_object;
               }
               else
               {
                  /* must place inventory object into cell */
                  CLEAR_AVATAR_INVENTORY( cavatar, c->objects[ n ], obj );
                     /* obj = EMPTY */
               }

               allowed = _TRUE;
            }
            else
            {
               /* insufficient space to put object */
            }
         }
         else
         {
               /* test if object resides in cell */
            q = find_object_in_cell(
                  obj->input.name, obj->input.adjective, cavatar->location );
            if( 0 == q )
            {
               q =( object_t* )find_noun_as_object_with_avatars( 
                     obj->input.name, obj->input.adjective, cavatar->location );
            }
            if( q )
            {
               d = q->output;

                  /* apply fix relationship */
               make_fix( q, p );

               allowed = _TRUE;
            }
         }
      }

      if( _FALSE == allowed )
      {
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         if( adverb )
         {
            PRINTS _TO, _THE END_PRINT
            SPRINT_( outbuf, "%s", adverb );
         }
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         PRINTS _THE END_PRINT
         print_object_desc( &d );
         PRINTS _IS, _NOW, _FIXED, _TO, _THE END_PRINT
         print_object_desc( &p->output );
         PRINTS _FULLSTOP, _NEWLINE END_PRINT

         /* special cases */
         if( BOARD == d.name )
         {
            /* need to initialise the value of each element board
             * (see magnotron_test) */
            set_element_state( cavatar->location - C_MAGNOTRON_ELEMENT_0 );
         }
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
   }
}

static void a_find( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   UC cn;
   UC n;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* see if object is in inventory */
      if( 0 != find_object_with_avatar( obj->input.name, obj->input.adjective ))
      {
         PRINTS _I, _AM, _ALREADY, _CARRYING, _IT, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      if( find_object_in_cell(
               obj->input.name, obj->input.adjective, cavatar->location ))
      {
         PRINTS _I, _CAN, _SEE, _IT, _HERE, _FULLSTOP, _NEWLINE END_PRINT
         actions[ ACTION_LOOK ]( vcode, ncode, obj, wcode );
      }
      else
      if( find_object_in_world( obj->input.name, obj->input.adjective, &cn ))
      {
         if( C_FOREST_GLADE > cn )
         {
            PRINTS _I, _CANNOT, _FIND, _IT, _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         if( 1 >= distance( cavatar->location, cn ))
         {
            PRINTS _IT, _IS, _CLOSE, _BY, _IN, _THE END_PRINT
            print_cell_desc( &( adv_world.cells[ cn ].output ));
            PRINTS _NEWLINE END_PRINT
         }
         else
         {
            PRINTS _IT, _IS, _FAR, _AWAY, _IN, _THE END_PRINT
            print_cell_desc( &( adv_world.cells[ cn ].output ));
            PRINTS _NEWLINE END_PRINT
         }
      }
   }
   else
   if( wcode & A_MASK )
   {
      DEBUGF( "Testing for avatar\n" );
      n = wcode & ~A_MASK;

      if( &( adv_avatar[ n ]) == cavatar )
      {
         PRINTS _I, _LOOK, _AT, _MYSELF, _SUSPICIOUSLY, _FULLSTOP, 
                _NEWLINE END_PRINT
      }
      else
      {
         cn = adv_avatar[ n ].location;

         SPRINT_( outbuf, "%s", adv_avatar[ n ].name );
         if( 0 == distance( cavatar->location, cn ))
         {
            PRINTS _IS, _HERE, _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         if( 1 >= distance( cavatar->location, cn ))
         {
            PRINTS _IS, _CLOSE, _BY, _IN, _THE END_PRINT
            print_cell_desc( &( adv_world.cells[ cn ].output ));
            PRINTS _NEWLINE END_PRINT
         }
         else
         {
            PRINTS _IS, _FAR, _AWAY, _IN, _THE END_PRINT
            print_cell_desc( &( adv_world.cells[ cn ].output ));
            PRINTS _NEWLINE END_PRINT
         }
      }
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_follow( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif

      if( TRAIL == obj->output.name )
      {
         actions[ ACTION_GO ]( vcode, ncode, obj, wcode );
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
      if( _TRUE == fullGameEnabled )
      {
         switch( wcode & ~A_MASK )
         {
            case A_HARRY :
            {
               if(( cavatar->location == adv_avatar[ A_HARRY ].location )&&
                  ( &adv_avatar[ A_HARRY ]!= cavatar ))
               {
                  SPRINT_( outbuf, "%s", adv_avatar[ A_HARRY ].name );
                  PRINTS _FLIES, _OFF, _AT, _A, _PACE, _COMMA,
                        _BUT, _I, _MANAGE, _TO, _KEEP, _UP, _FULLSTOP
                        END_PRINT
                  PRINTS _WE, _MAKE, _MANY, _TWISTS, _AND_, _TURNS, _COMMA,
                        _BUT, _IN, _THE, _DARKNESS, _I, _LOSE, _MY,
                        _BEARINGS, _FULLSTOP END_PRINT

                  switch( cavatar->location )
                  {
                     case C_MINE_TUNNEL :
                     {
                        PRINTS _AT, _LAST END_PRINT
                        SPRINT_( outbuf, "%s", adv_avatar[ A_HARRY ].name );
                        PRINTS _BRINGS, _ME, _TO, _THE END_PRINT
                        print_cell_desc(
                           &( adv_world.cells[ C_MINE_FACE ].output ));
                        PRINTS _NEWLINE END_PRINT
                        cavatar->location =
                           adv_avatar[ A_HARRY ].location = C_MINE_FACE;
                     }
                     break;

                     case C_MINE_FACE :
                     {
                        PRINTS _FINALLY END_PRINT
                        SPRINT_( outbuf, "%s", adv_avatar[ A_HARRY ].name );
                        PRINTS _GETS, _ME, _BACK, _TO, _THE END_PRINT
                        print_cell_desc(
                           &( adv_world.cells[ C_MINE_SHAFT ].output ));
                        PRINTS _NEWLINE END_PRINT
                        cavatar->location = C_MINE_SHAFT;
                        adv_avatar[ A_HARRY ].location = C_MINE_TUNNEL;
                     }
                     break;

                     default:
                     {
                        PRINTS _NEWLINE END_PRINT
                     }
                     break;
                  }
               }
            }
            break;

            case A_NELLY :
            {
               if(( cavatar != &adv_avatar[ A_NELLY ] )&&
                  ( 1 >=
                    distance( cavatar->location, adv_avatar[ A_NELLY ].location )))
               {
                  do_avatar_mobile_checks( _TRUE );
                  cavatar->location = adv_avatar[ A_NELLY ].location;
                  PRINTS _I, _FOLLOW END_PRINT
                  SPRINT_( outbuf, "%s", adv_avatar[ A_NELLY ].name );
                  PRINTS _TO, _THE END_PRINT
                  print_cell_desc(
                     &( adv_world.cells[ cavatar->location ].output ));
                  PRINTS _FULLSTOP, _NEWLINE END_PRINT
               }
               else
               {
                  PRINTS _I, _DONT, _SEE END_PRINT
                  SPRINT_( outbuf, "%s", adv_avatar[ A_NELLY ].name );
                  PRINTS _HERE, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            break;

            default:
            break;
         }
      }
      else
      {
         /* game limited, cannot move location */
         fullGameLimitHit = _TRUE;
      }
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

/* a_give, a_take, a_drink & a_eat work with containers. */
static void a_give( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   object_t *p = 0;
   int n = -1;
   int m;
   int can;
   U32 acode;


#  if defined( DEBUG )
   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );
   if(( __trace )&&( obj ))
   {
      print_object_desc( &( obj->output ));
      PRINTS _NEWLINE END_PRINT
   }
#  endif

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* if a matching object already exists in the current cell that is
       * PERPETUAL, then destroy the avatars object. */
      p = find_object_in_cell( obj->input.name, obj->input.adjective, 
                               cavatar->location );
      if(( p )&&( PROPERTY_PERPETUAL & p->properties ))
      {
         DEBUGF( "Destroying duplicate object\n" );
         *obj = empty_object;
         allowed = _TRUE;
      }
      else
      if(( 0 == p )&&( PROPERTY_NOFREE & obj->properties ))
      {
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _DISPERSES END_PRINT

         /* must also empty a container */
         if( _TRUE == container_test( obj, &n ))
         {
            PRINTS _FROM, _THE END_PRINT
            print_object_desc( &( cavatar->inventory[ n ].output ));

            can = cavatar->inventory[ n ].current_state.descriptor;
            containers[ can ]= empty_object;
         }
         else
         {
            DEBUGF( "Unexpected null container" );
            *obj = empty_object;
         }
         PRINTS _FULLSTOP, _NEWLINE END_PRINT

         allowed = _TRUE;
      }
      else
      {
         allowed = give_object( obj );
      }
   }
   else
   if( wcode & A_MASK )
   {
      /* likely input is give <avatar> <object> */
      acode = adv__encode_name( adverb );
      p = find_object_with_avatar( acode, 0 );
      if( p )
      {
         /* find free space in avatar inventory */
         test_inventory_free_space( &adv_avatar[ wcode & ~A_MASK ], &m );
         if( 0 <= m )
         {
            adv_avatar[ wcode & ~A_MASK ].inventory[ m ]= *p;
            *p = empty_object;
            SPRINT_( outbuf, "%s", adv_avatar[ wcode & ~A_MASK ].name);
            PRINTS _IS, _NOW, _CARRYING, _THE END_PRINT
            SPRINT_( outbuf, "%s", adverb );
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         {
            SPRINT_( outbuf, "%s", adv_avatar[ wcode & ~A_MASK ].name);
            PRINTS _CANNOT, _CARRY, _ANY, _MORE, _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      {
         PRINTS _IM, _NOT, _CARRYING END_PRINT
         if( adverb )
         {
            SPRINT_( outbuf, "%s", adverb);
         }
         else
         {
            SPRINT_( outbuf, "%s", "it" );
         }
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode )
   {
      /* test builtin */
      if( NAME_EVERYTHING == wcode )
      {
         /* foreach inventory object */
         for( m=0; m < MAX_AVATAR_OBJECTS; m++ )
         {
            if( cavatar->inventory[ m ].input.name )
            {
               if( _TRUE == give_object( &( cavatar->inventory[ m ])))
               {
                  allowed = _TRUE;
               }
            }
         }
      }
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }

   if( _TRUE == allowed )
   {
      a_inventory( vcode, ncode, obj, wcode );
   }
}

static void a_go( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   UC direction = NAME_NULL;
   UC c = C_NULL;
   TRISTATE allowed = _TRUE;
   TRISTATE found = _FALSE;
   TRISTATE printed = _FALSE;
   object_t *p = 0;
   int i = -1;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if( obj )
   {
      /* test special cases: any door has to be open */
      if(((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 ))== obj->input.name )
      {
         /* check door is open */
         if( ! (( STATE_OPEN == obj->current_state.descriptor )&&
                ( 1 == obj->current_state.value )))
         {
            allowed = _FALSE;
         }
      }
      else
      {
         /* test pre-condition */
         for( i=0;( i < MAX_OBJECT_ACTIONS )&&( _FALSE == found ); i++ )
         {
            if( obj->actions[ i ]== ACTION_GO )
            {
               DEBUGF( "ACTION_GO requires a pre-condition\n" );
               found = _TRUE;

               /* check condition on action */
               DEBUGF( "checking pre-condition\n" );
               switch( obj->pre_cond[ i ].type )
               {
                  case TEST_PRECOND :
                  {
                     if(( obj->pre_cond[ i ].value == 
                          obj->current_state.value )&&
                        ( obj->pre_cond[ i ].descriptor == 
                          obj->current_state.descriptor ))
                     {
                        /* action permitted in this state */
                     }
                     else
                     {
                        allowed = _FALSE;
                     }
                  }
                  break;

                  default :
                  break;
               }
            }
            else
            {
               /* no pre-condition on action */
            }
         }
      }

      if( _TRUE == allowed )
      {
         if( C_MOUNTAIN_FUNICULAR == cavatar->location )
         {
            p = find_object_in_cell(
                  (( U32 )( 'l' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'v' )<< 8 )+(( U32 )( 'e' )<< 0 ), 0,
                  C_MOUNTAIN_FUNICULAR );
            if( 0 == p->current_state.value )
            {
               c = C_MOUNTAIN_CAVERN;
            }
            else
            if( 5 == p->current_state.value )
            {
               c = C_MOUNTAIN_TUNNEL;
            }
            else
            {
               c = C_MOUNTAIN_FUNICULAR;
            }
         }
         else
         if( C_MINE_CAGE == cavatar->location )
         {
            p = find_object_in_cell(
                  (( U32 )( 'l' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'v' )<< 8 )+( 'e'<< 0 ), 0, 
                  C_MINE_CAGE );
            if( 6 == p->current_state.value )
            {
               c = C_MINE_OBSERVATION_DECK;
            }
            else
            if( 5 == p->current_state.value )
            {
               c = C_MINE_HEAD;
            }
            else
            if( 1 == p->current_state.value )
            {
               c = C_MAGNOTRON_COMPLEX;
            }
            else
            if( 0 == p->current_state.value )
            {
               c = C_MINE_SHAFT;
            }
         }
         else
         {
            /* describe the journey by this object */
            if( TRAVELATOR == obj->output.name )
            {
               PRINTS _WE, _GLIDE, _SMOOTHLY, _AND_, _EFFORTLESSLY, _FOR, _A, 
                      _WHILE, _COMMA, _BEFORE, _REACHING, _OUR, _DESTINATION,
                      _FULLSTOP, _NEWLINE END_PRINT
            }

            /* apply verb to object<noun> */
            /* search object directions and follow the first non-0 entry */
            for( direction = NAME_NORTH; direction <= NAME_DOWN; direction++ )
            {
               if( obj->ways[ direction - 1 ])
               {
                  c = obj->ways[ direction - 1 ];
                  break;
               }
            }

            /* special cases: object moves with avatar */
            if(( ELECTRIC== obj->output.adjective )&&
               ( CART == obj->output.name ))
            {
               if( 0 > c )
               {
                  c = -c;
               }

               i = find_space_in_cell( c );
               if( 0 <= i )
               {
                  if( C_MAGNOTRON_ELEMENT_6 == obj->ways[ NAME_WEST - 1 ])
                  {
                     /* end of line, back to operations room */
                     obj->ways[ NAME_WEST - 1 ]= C_MAGNOTRON_OPERATION;
                  }
                  else
                  {
                     /* update cart ways to point at next magnotron ring */
                     obj->ways[ NAME_WEST - 1 ]=( c + 1 );
                  }
                  /* move cart */
                  adv_world.cells[ c ].objects[ i ]= *obj;
                  *obj = empty_object;

                  PRINTS _THE, _ELECTRIC, _CART, _FOLLOWS, _GUIDES, _IN, _THE,
                      _FLOOR, _AS, _WE END_PRINT
                  if( C_MAGNOTRON_ELEMENT_0 == c )
                  {
                     PRINTS _START END_PRINT
                  }
                  else
                  {
                     PRINTS _CONTINUE END_PRINT
                  }
                  PRINTS _OUR, _CIRCUMNAVIGATION, _OF, _THE, _UNDERGROUND, 
                         _MAGNOTRON, _FACILITY, _FULLSTOP END_PRINT
                  if( C_MAGNOTRON_ELEMENT_3 == c )
                  {
                     PRINTS _THE, _MOLE, _SEEMS, _PARTICULARLY, _APPRECIATIVE,
                        _OF, _THE, _DIGSMANSHIP, _ON, _THIS, _STRETCH, _OF,
                        _TUNNEL, _FULLSTOP END_PRINT
                  }
                  PRINTS _NEWLINE END_PRINT
               }
               else
               {
                  /* no space in next ring cell, cart is stuck */
                  c = C_NULL;
                  allowed = _FALSE;
               }
            }
         }
      }
      else
      {
         PRINTS _I, _CANNOT, _GO, _THERE, _YET, _FULLSTOP, _NEWLINE END_PRINT
         printed = _TRUE;
      }
   }
   else
   {
      /* apply verb <ncode> to avatar */
      a_navigation( 0, 0, 0, wcode );
      printed = _TRUE;
   }

   if( NAME_NULL != c )
   {
      if( 0 > c )
      {
         c = -c;
      }

      /* unselect anything in current cell */
      set_cell_selected( 0 );

      if( _FALSE == fullGameEnabled )
      {
         if( C_HILLTOP_DISH >= c )
         {
            SET_AVATAR_LOCATION( cavatar, c );
         }
         else
         {
            /* game limited, cannot move location */
            fullGameLimitHit = _TRUE;
         }
      }
      else
      {
         SET_AVATAR_LOCATION( cavatar, c );
      }

      adv__print_status( _FALSE );
   }
   else
   if( _FALSE == printed )
   {
      PRINTS _I, _CANNOT, _GO, _THERE, _FULLSTOP, _NEWLINE END_PRINT
   }
}

static void a_hide( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_HIDE, _TRUE );
      if( _TRUE == allowed )
      {
         /* specific post-cond actions for ACTION_HIDE */
         obj->properties |= PROPERTY_HIDDEN;
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _IS, _NOW, _HIDDEN, _OUT, _OF, _SIGHT, _FULLSTOP,
                  _NEWLINE END_PRINT
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_inventory( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   int n;
   avatar_t *av;


   if( wcode & A_MASK )
   {
      av = &( adv_avatar[ wcode & ~A_MASK ]);
   }
   else
   {
      av = cavatar;
   }

   print_inventory( av );

   n = -1, test_inventory_worn( av, &n );
   if( 0 <= n )
   {
      print_wearing( av );
   }
}

static void a_listen( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   UC cn;
   UC n;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif

      PRINTS _I, _HEAR, _NOTHING, _SPECIAL, _FULLSTOP, _NEWLINE END_PRINT
   }
   else
   if( wcode & A_MASK )
   {
      DEBUGF( "Testing for avatar\n" );
      n = wcode & ~A_MASK;

      if( &( adv_avatar[ n ]) == cavatar )
      {
         PRINTS _I, _LOOK, _AT, _MYSELF, _SUSPICIOUSLY, _FULLSTOP, 
                _NEWLINE END_PRINT
      }
      else
      {
         cn = adv_avatar[ n ].location;

         if( 1 >= distance( cavatar->location, cn ))
         {
            PRINTS _I, _HEAR, _NOTHING, _UNUSUAL, _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         {
            SPRINT_( outbuf, "%s", adv_avatar[ n ].name );
            PRINTS _IS, _TOO, _FAR, _AWAY, _TO, _HEAR, _FULLSTOP, 
                   _NEWLINE END_PRINT
         }
      }
   }
   else
   {
      /* apply verb <ncode> to avatar */
      if( C_CASTLE_DUNGEON == cavatar->location )
      {
         PRINTS _I, _HEAR, _WATER, _LAPPING, _OUTSIDE, _THE, _WINDOW, _BARS,
                  _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         PRINTS _I, _HEAR, _NOTHING, _SPECIAL, _FULLSTOP, _NEWLINE END_PRINT
      }
   }
}

static void a_lock( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   int n;  /* if object in inventory */
   object_t *p;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
         /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_LOCK, _FALSE );
      if( _TRUE == allowed )
      {
         /* check lock action */
         if((( STATE_LOCKED == obj->current_state.descriptor )||
             ( STATE_OPEN == obj->current_state.descriptor ))&&
            ( 0 == obj->current_state.value ))
         {
            /* is unlocked, so can lock */

            /* test for the castle tower door */
            if((((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 ))==
               obj->input.name )&&
               (((( U32 )( 't' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'w' )<< 8 )+(( U32 )( 'e' )<< 0 ))==
               obj->input.adjective)&&
               ( C_CASTLE_MAPROOM == cavatar->location ))
            {
               /* object is the castle tower door.
                * test if the avatar has the bronze key selected. */
               test_inventory_selected( cavatar, &n );
               if(( 0 <= n )&&
                  ( BRONZE == cavatar->inventory[ n ].output.adjective )&&
                  ( KEY == cavatar->inventory[ n ].output.name ))
               {
                  obj->current_state.value = 1;
                  PRINTS _THE END_PRINT
                  print_object_desc( &( obj->output ));
                  PRINTS _IS, _NOW, _LOCKED, _FULLSTOP, _NEWLINE END_PRINT

                  /* copy the new state to the "other side" */
                  p = find_object_in_cell( obj->input.adjective, 
                                           obj->input.name, C_CASTLE_TOWER );
                  p->current_state = obj->current_state;
               }
               else
               {
                  PRINTS _I, _CANNOT END_PRINT
                  SPRINT_( outbuf, "%s", verb );
                  PRINTS _THE END_PRINT
                  print_object_desc( &( obj->output ));
                  PRINTS _YET, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            else
            if((((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 ))==
               obj->input.name )&&
               (((( U32 )( 'k' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'p' )<< 0 ))==
               obj->input.adjective))
            {
               /* object is the castle tower door.
                * test if the avatar has the silver key selected. */
               test_inventory_selected( cavatar, &n );
               if(( 0 <= n )&&
                  ( SILVER == cavatar->inventory[ n ].output.adjective )&&
                  ( KEY == cavatar->inventory[ n ].output.name ))
               {
                  obj->current_state.value = 1;
                  PRINTS _THE END_PRINT
                  print_object_desc( &( obj->output ));
                  PRINTS _IS, _NOW, _LOCKED, _FULLSTOP, _NEWLINE END_PRINT
               }
               else
               {
                  PRINTS _I, _CANNOT END_PRINT
                  SPRINT_( outbuf, "%s", verb );
                  PRINTS _THE END_PRINT
                  print_object_desc( &( obj->output ));
                  PRINTS _YET, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
         }
         else
         if( STATE_LOCKED == obj->current_state.descriptor )
         {
            PRINTS _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _IS, _LOCKED, _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_look( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   UC start = 1;
   UC end = 0;
   direction_t way;
   int i = 0;
   int j = 0;
   object_t *p;
   object_t *q;
   object_t *rack;
   TRISTATE allowed = _TRUE;


#  if defined( DEBUG )
   DEBUGF( "Action 0x%lx for %s ", vcode,( 0 == obj )? "avatar" :"object" );
   if(( __trace )&&( obj ))
   {
      print_object_desc( &( obj->output ));
      PRINTS _NEWLINE END_PRINT
   }
#  endif

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      if( MONUMENT == obj->output.name )
      {
         PRINTS _IT, _SHOWS, _DIRECTIONS, _TO, _PLACES, _OF, _INTEREST,
                  _FULLSTOP, _STRANGELY, _COMMA, _DIRECTIONS, _ARE,
                  _QUOTE_LEFT, _AS, _THE, _CROW, _FLIES, _QUOTE_RIGHT,
                  _COMMA END_PRINT

         /* check whether avatar is wearing spectacles */
         i=-1, test_inventory_worn( cavatar, &i );
         if(( 0 <= i )&&( SPECTACLES == cavatar->inventory[ i ].output.name ))
         {
            PRINTS _AND_, _THEY, _ARE, _COLON, _NEWLINE END_PRINT
            PRINTS _TAB, _ESC_U, _TO, _THE, _CASTLE, 
                   _QUOTE_LEFT, _ESC_U, _AL, _LA, _KASTELO, _QUOTE_RIGHT, 
                   _FULLSTOP, _NEWLINE END_PRINT
            PRINTS _TAB, _ESC_U, _TO, _THE, _MOUNTAINS, 
                  _QUOTE_LEFT, _ESC_U, _AL, _LA, _MONTOJ, _QUOTE_RIGHT, 
                  _FULLSTOP, _NEWLINE END_PRINT
            PRINTS _TAB, _ESC_U, _TO, _THE, _FOREST, 
                   _QUOTE_LEFT, _ESC_U, _AL, _LA, _ARBARO, _QUOTE_RIGHT, 
                   _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         {
            PRINTS _BUT, _I, _CANNOT, _QUITE, _MAKE, _THEM, _OUT, _FULLSTOP,
                     _NEWLINE END_PRINT
         }
      }
      else
      if( WINDOW == obj->output.name )
      {
         if( C_CASTLE_DUNGEON == cavatar->location )
         {
            PRINTS _I, _CAN, _JUST, _MAKE, _OUT, _THE, _CASTLE, _MOAT,
                     _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         if( C_CASTLE_MAPROOM == cavatar->location )
         {
            PRINTS _IN, _THE, _DISTANCE, _I, _CAN, _SEE, _A, _MOUNTAIN,
                     _RANGE, _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         {
            DEBUGF( "UNKNOWN window\n" );
         }
      }
      else
      if( TELESCOPE == obj->output.name )
      {
         switch( get_rotary_switch_state( C_MOUNTAIN_OBSERVATORY ))
         {
            case ROTARY_DISH :
            {
               p = find_object_in_cell( 
                      ((( U32 )( 'd' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 'h' )<< 0 )), 0,
                      C_HILLTOP );

               PRINTS _THROUGH, _THE, _TELESCOPE, _I, _CAN, _SEE, _THE,
                        _DISH, _POINTS, _AT END_PRINT
               if( DISH_MAST == p->current_state.value )
               {
                  /* 90 degrees, points to the magnatron mast */
                  PRINTS _A, _MAST, _FULLSTOP, _NEWLINE END_PRINT
               }
               else
               if( DISH_MOON == p->current_state.value )
               {
                  /* 180 degrees, points to the spire */
                  PRINTS _THE, _MOON, _FULLSTOP, _NEWLINE END_PRINT
               }
               else
               if( DISH_SUN == p->current_state.value )
               {
                  /* 270 degrees, points to the sun */
                  PRINTS _THE, _SUN, _FULLSTOP, _NEWLINE END_PRINT
               }
               else
               if( DISH_FOREST_2 >= p->current_state.value )
               {
                  /* DISH_FOREST_1 is implied by order of if tests */
                  PRINTS _THE, _FOREST, _FULLSTOP, _NEWLINE END_PRINT
               }
               else
               if( DISH_MOUNTAINS >= p->current_state.value )
               {
                  PRINTS _THE, _MOUNTAINS, _FULLSTOP, _NEWLINE END_PRINT
               }
               else
               {
                  PRINTS _THE, _HORIZON, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            break;

            case ROTARY_LIGHTBEAM:
            {
               rack = find_object_in_cell( 
                        ((( U32 )( 'r' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'c' )<< 8 )+(( U32 )( 'k' )<< 0 )), 
                        0, C_CASTLE_SPIRE_ROOF );
               if( SOURCE_SUN == rack->current_state.value )
               {
                  /* check the filter is in place,
                   * otherwise we cannot look through the telescope */
                  p = find_object_in_cell(
                              (( U32 )( 'f' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 't' )<< 0 ), 0, 
                              C_MOUNTAIN_OBSERVATORY_DOME );
                  q = find_object_in_cell(
                            (( U32 )( 'l' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 's' )<< 0 ), 0, 
                            C_MOUNTAIN_OBSERVATORY_DOME );
                  allowed = test_fix( p, q );
               }
               if( _TRUE == allowed )
               {
                  p = obj;  /* The value of the lightbeam for alignment is 
                               stored in the telescope */
                  switch( p->current_state.value )
                  {
                     case NAME_LEFT :
                     case NAME_RIGHT :
                     case NAME_UP :
                     case NAME_DOWN :
                     {
                        PRINTS _THROUGH, _THE, _TELESCOPE, _I, _CAN, _SEE,
                                 _DISTANT, _STARS, _FULLSTOP, _NEWLINE END_PRINT
                        PRINTS _THERE, _IS, _A, _HALO END_PRINT
                        SPRINT_( outbuf, "%s", 
                                    ( NAME_LEFT == p->current_state.value )
                                    ? "to the right"
                                    : ( NAME_RIGHT == p->current_state.value )
                                    ? "to the left"
                                    : ( NAME_UP == p->current_state.value )
                                    ? "below"
                                    : "above" );
                        PRINTS _SEMICOLON, _IT, _MIGHT, _BE, _THE END_PRINT
                        SPRINT_( outbuf, "%s",
                                  ( SOURCE_SUN == rack->current_state.value )
                                  ? "sun"
                                  : "moon" );
                        PRINTS  _FULLSTOP, _NEWLINE END_PRINT
                     }
                     break;

                     case NAME_AROUND :
                     {
                        if( SOURCE_SUN == rack->current_state.value )
                        {
                           PRINTS _THE, _SUN, _ACTS, _AS, _A, _GRAVITATIONAL,
                                  _LENS, _AND_, _THROUGH, _THE, _TELESCOPE, _I,
                                  _CAN, _SEE, _THE, _MILKY, _WAY END_PRINT
                        }
                        else
                        {
                           PRINTS _THROUGH, _THE, _TELESCOPE, _I, _CAN, _SEE, 
                                  _THE, _MOON END_PRINT
                        }
                        PRINTS  _FULLSTOP, _NEWLINE END_PRINT
                     }
                     break;

                     default:
                     break;
                  }
               }
               else
               {
                  PRINTS _FULLSTOP, _I, _CANNOT, _LOOK, _AT, _THE, _SUN, _WHILE,
                         _THERE, _IS, _NO, _FILTER, _FIXED, _TO, _THE, _LENS,
                         _NEWLINE END_PRINT
               }

               print_lightbeam_state( _TRUE );
            }
            break;

            case ROTARY_DISH_MIRROR :
            {
               p = find_object_in_cell( 
                      ((( U32 )( 'm' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'r' )<< 0 )), 0,
                      C_HILLTOP_DISH );   /* could be gold or silver */
               q = find_object_in_cell( 
                      ((( U32 )( 'b' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'c' )<< 0 )), 0,
                      C_HILLTOP_DISH );
                  /* test fix relationship */
               if(( p )&&( q->current_state.value == p->output.name )&&
                  ( STATE_FIXED ==( STATE_FIXED & p->current_state.descriptor )))
               {
                  switch( p->current_state.value )
                  {
                     case NAME_LEFT :
                     case NAME_RIGHT :
                     case NAME_UP :
                     case NAME_DOWN :
                     {
                        PRINTS _THROUGH, _THE, _TELESCOPE, _I, _CAN, _SEE,
                                 _A, _MIRROR, _FIXED, _TO, _A, _RECEIVER,
                                 _DISH, _FULLSTOP, _NEWLINE END_PRINT
                        PRINTS _THE, _MIRROR, _REFLECTS, _THE, _SKY END_PRINT
                        SPRINT_( outbuf, "%s", 
                                     (( NAME_LEFT == p->current_state.value )
                                      ? "to the right of"
                                      : ( NAME_RIGHT == p->current_state.value )
                                      ? "to the left of"
                                      : ( NAME_UP == p->current_state.value )
                                      ? "below"
                                      : "above" ));
                        PRINTS _THE, _CASTLE, _SPIRE, _FULLSTOP, _NEWLINE 
                               END_PRINT
                     }
                     break;

                     case NAME_AROUND :
                     {
                        PRINTS _THROUGH, _THE, _TELESCOPE, _I, _CAN, _SEE,
                                 _THE, _MIRROR, _REFLECTING, _THE, _CASTLE,
                                 _SPIRE, _FULLSTOP, _NEWLINE END_PRINT
                     }
                     break;

                     default:
                     break;
                  }
               }
               else
               {
                  /* mirror is not fixed to dish */
                  PRINTS _THROUGH, _THE, _TELESCOPE, _I, _CAN, _SEE, _A,
                           _RECEIVER, _DISH, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            break;

            case ROTARY_SPIRE_MIRROR :
            {
               p = find_object_in_cell( 
                      ((( U32 )( 'm' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'r' )<< 0 )), 0,
                      C_CASTLE_SPIRE_ROOF );/* could be gold or silver mirror */
               q = find_object_in_cell( 
                      ((( U32 )( 'b' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'c' )<< 0 )), 0,
                      C_CASTLE_SPIRE_ROOF );
                  /* test fix relationship */
               if(( p )&&( q->current_state.value == p->output.name )&&
                  ( STATE_FIXED ==( STATE_FIXED & p->current_state.descriptor )))
               {
                  switch( p->current_state.value )
                  {
                     case NAME_LEFT :
                     case NAME_RIGHT :
                     case NAME_UP :
                     case NAME_DOWN :
                     {
                        PRINTS _THROUGH, _THE, _TELESCOPE, _I, _CAN, _SEE, _A,
                                 _MIRROR, _FIXED, _TO, _THE, _CASTLE, _SPIRE,
                                 _FULLSTOP, _NEWLINE END_PRINT
                        PRINTS _THE, _MIRROR, _REFLECTS, _A, _HILLTOP 
                               END_PRINT
                        SPRINT_( outbuf, "%s",
                                   ( NAME_LEFT == p->current_state.value )
                                   ? "to the right of"
                                   : ( NAME_RIGHT == p->current_state.value )
                                   ? "to the left of"
                                   : ( NAME_UP == p->current_state.value )
                                   ? "below"
                                   : "above" );
                        PRINTS _A, _RECEIVER, _DISH, _FULLSTOP, _NEWLINE 
                               END_PRINT
                     }
                     break;

                     case NAME_AROUND :
                     {
                        PRINTS _THROUGH, _THE, _TELESCOPE, _I, _CAN, _SEE,
                                 _THE, _MIRROR, _REFLECTING, _THE, _RECEIVER,
                                 _DISH, _FULLSTOP, _NEWLINE END_PRINT
                     }
                     break;

                     default:
                     break;
                  }
               }
               else
               {
                  /* mirror is not fixed to dish */
                  PRINTS _THROUGH, _THE, _TELESCOPE, _I, _CAN, _SEE, _THE,
                           _CASTLE, _SPIRE, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            break;

            case ROTARY_OFF :
            default :
            {
               PRINTS _I, _CANNOT, _SEE, _ANYTHING, _THROUGH, _THE,
                         _TELESCOPE, _FULLSTOP, _NEWLINE END_PRINT
            }
            break;
         }
      }
      else
      if( LIGHTBEAM == obj->output.name )
      {
         print_lightbeam_state( _TRUE );
      }
      else
      if( WHEEL == obj->output.name )
      {
         PRINTS _THE, _WHEEL, _CAN, _BE, _TURNED, _SO, _THAT, _THE,
                _LIGHTBEAM, _FACES, _DIFFERENT, _DIRECTIONS, _FULLSTOP,
                _NEWLINE END_PRINT
      }
      else
      if(( PINION == obj->output.name )||
         ( RACK == obj->output.name ))
      {
         PRINTS _THE, _WHEEL, _IS, _CONNECTED, _TO, _THE, _PINION, _WHICH, 
                _DRIVES, _THE, _RACK, _UPON, _WHICH, _THE, _LIGHTBEAM, _STANDS,
                _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      if( COLLECTOR == obj->output.adjective )
      {
         /* The value of the lightbeam alignment is stored in the telescope */
         p = find_object_in_cell( 
                  ((( U32 )( 't' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'e' )<< 0 )), 
                  0, C_MOUNTAIN_OBSERVATORY );
         i = 0;
         if( NAME_AROUND == p->current_state.value )
         {
            switch( get_lightbeam_state( ))
            {
               case L_TRIANGLE_DIAMOND :
               {
                  i |= 8;
               }
               /* break; fall through */
               case L_ENGAGE_DIAMOND :
               case L_STANDBY_DIAMOND :
               {
                  i |= 4;
               }
               break;

               case L_TRIANGLE_GOLD :
               {
                  i |= 2;
               }
               /* break; fall through */
               case L_ENGAGE_GOLD :
               case L_STANDBY_GOLD :
               {
                  i |= 1;
               }
               break;

               case L_DISENGAGE :
               case L_READY_DIAMOND :
               case L_READY_GOLD :
               default :
               break;
            }
         }

         if(( i & 0x4 )||( i & 0x1 ))
         {
            PRINTS _THE, _REFLECTED END_PRINT
            SPRINT_( outbuf, "%s", ( i & 0x4 )?"diamond" :"gold" );
            PRINTS _LIGHTBEAM, _IS, _FOCUSED, _ON, _THE, _RECEIVER END_PRINT
         }
         if(( i & 0x8 )||( i & 0x2 ))
         {
            PRINTS _AND_, _FLOODS, _THE, _DISH, _WITH, _ENERGY END_PRINT
         }
         if( 0 == i )
         {
            PRINTS _I, _SEE, _NOTHING, _SPECIAL END_PRINT
         }
         PRINTS _NEWLINE END_PRINT
      }
      else
      if( PAINTINGS == obj->output.name )
      {
         PRINTS _THEY, _APPEAR, _TO, _BE, _ANCESTRAL, _PORTRAITS, _FULLSTOP,
                  _NEWLINE END_PRINT
         p = find_object_in_cell( 
            ((( U32 )( 'h' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'd' )<< 0 )), 
            ((( U32 )( 'y' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 )), 
            C_CASTLE_THRONEROOM );
         if( 0 == p )
         {
            ( void )test_inventory(
                  cavatar, (( U32 )( 'h' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'd' )<< 0 ), 
                  ((( U32 )( 'y' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 )), &i );
            if( 0<= i )
            {
               p = &cavatar->inventory[ i ];
            }
         }
         if(( p )&&( 1 == p->current_state.value ))
         {
            PRINTS _THE, _LATEST, _RESEMBLES, _THE, _AGED, _MAN, _IN, _THE,
                     _HEAD, _MINUS, _BAND, _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      if(( CONTROL == obj->output.adjective )&&( PANEL == obj->output.name ))
      {
         /* display all buttons, switches, joysticks in this cell */
         PRINTS _THE, _PANEL, _HAS END_PRINT
         for( i=0, j=0; i < MAX_CELL_OBJECTS; i++ )
         {
            p = &( adv_world.cells[ cavatar->location ].objects[ i ]);
            if(( BUTTON == p->output.name )||
               ( SWITCH == p->output.name )||
               ( SLIDER_SWITCH == p->output.name )||
               ( JOYSTICK == p->output.name ))
            {
               j++;
               PRINTS _A END_PRINT
               print_object_desc( &( p->output ));
               PRINTS _COMMA END_PRINT
            }
         }
         if( j )
         {
            PRINTS _BS END_PRINT  /* delete last comma */
         }
         else
         {
            PRINTS _NO, _CONTROLS END_PRINT
         }
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      if(( ROTARY == obj->output.adjective )&&( SWITCH == obj->output.name ))
      {
         print_rotary_switch_state( cavatar->location );
      }
      else
      if( SLIDER_SWITCH == obj->output.name )
      {
         p = find_object_in_cell( obj->input.name, obj->input.adjective,
                                  C_MAGNOTRON_OPERATION );
         PRINTS _THE END_PRINT
         print_object_desc( &( p->output ));
         PRINTS _IS, _SET, _TO, _LEVEL END_PRINT
         SPRINT_( outbuf, "%d", p->current_state.value );
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      if(( GOLD == obj->output.adjective )&&( O_RING == obj->output.name ))
      {
         if( test_inventory_worn( &adv_avatar[ A_BUSTER ], &i ),( 0 > i ))
         {
            /* Buster's ring is the key to the magnotron complex */
            SPRINT_( outbuf, "%s", adv_avatar[ A_BUSTER ].name );
            PRINTS _TELLS, _ME END_PRINT 
         }
         PRINTS _IT, _IS, _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _OF, _ADMITTANCE, _TO, _THE, _MAGNOTRON, _COMPLEX, _FULLSTOP,
                  _NEWLINE END_PRINT
      }
      else
      if(( O_MAP == obj->output.name )||
         ( METER == obj->output.name ))
      {
         a_read( 0, ncode, obj, wcode );
      }
      else
      {
         /* apply verb to object<noun> */
         if( 0 ==( obj->properties & PROPERTY_HIDDEN ))
         {
            if( PROPERTY_CONTAINER & obj->properties )
            {
               PRINTS _THE END_PRINT
               i = obj->current_state.descriptor;
               if( containers[ i ].output.name )
               {
                  print_object_desc( &( obj->output ));
                  PRINTS _CONTAINS END_PRINT
                  print_object_state( &( containers[ i ].current_state ));
                  print_object_desc( &( containers[ i ].output ));
                  PRINTS _OTHERWISE END_PRINT
               }
               else
               {
                  print_object_desc( &( obj->output ));
                  PRINTS _IS, _EMPTY, _COMMA, _OTHERWISE END_PRINT
               }
            }
            else
            if(( STATE_NULL < obj->current_state.descriptor )&&
               ( STATE_NUMBER > obj->current_state.descriptor ))
            {
               PRINTS _THE END_PRINT
               print_object_desc( &( obj->output ));
               PRINTS _IS END_PRINT
               print_object_state( &( obj->current_state ));
               PRINTS _OTHERWISE END_PRINT
            }
         }
         PRINTS _I, _SEE, _NOTHING, _SPECIAL, _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
      DEBUGF( "Testing for avatar\n" );
      j = wcode & ~A_MASK;

      if( cavatar == &( adv_avatar[ j ]))
      {
         a_inventory( 0, 0, 0, wcode );
      }
      else
      if( cavatar->location == adv_avatar[ j ].location )
      {
         /* apply verb <ncode> to avatar */
         if(( CROW == adv_avatar[ j ].output.name )||
            ( RAT == adv_avatar[ j ].output.name )||
            ( BAT == adv_avatar[ j ].output.name ))
         {
            PRINTS _THE END_PRINT
            print_object_desc( &( adv_avatar[j].output ));
            PRINTS _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS, _NEWLINE END_PRINT
         }
         else
         if(( GNOME == adv_avatar[ j ].output.name )||
            ( MOLE == adv_avatar[ j ].output.name )||
            ( MONKEY == adv_avatar[ j ].output.name ))
         {
            PRINTS _THE END_PRINT
            print_object_desc( &( adv_avatar[j].output ));
            PRINTS _EYES, _ME, _SPECULATIVELY, _ELIPSIS, _NEWLINE END_PRINT
         }
         else
         if( WIZARD == adv_avatar[ j ].output.name )
         {
            PRINTS _THE, _WIZARD, _GLANCES, _IN, _MY, _GENERAL, _DIRECTION,
                     _THEN, _LOOKS, _AWAY, _ELIPSIS, _NEWLINE END_PRINT
         }

         a_inventory( 0, 0, 0, wcode );
      }
      else
      {
         PRINTS _I, _DONT, _SEE, _THE END_PRINT
         print_object_desc( &( adv_avatar[ j ].output ));
         PRINTS _HERE, _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   {
      /* apply verb <ncode> to avatar */
      /* apply builtin to avatar */
      if(( NAME_NORTH <= wcode )&&( NAME_DOWN >= wcode ))
      {
         start = end =( wcode - 1 );
            /* -1 for hardcoded enum _name_t */
      }
      else
      if( NAME_LEFT == wcode )
      {
         start = end = NAME_WEST - 1;
      }
      else
      if( NAME_RIGHT == wcode )
      {
         start = end = NAME_EAST - 1;
      }
      else
      if(( NAME_AROUND == wcode )||
         ( NAME_NULL == wcode ) /* just 'look' is treated as 'look around' */ )
      {
         /* special case: we don't want to list the objects (i.e. the cage)
          * in the mine shaft head if we've climbed up there. */
         allowed = _TRUE;
         if( C_MINE_OBSERVATION_DECK == cavatar->location )
         {
            p = find_object_in_cell( 
                          ((( U32 )( 'l' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'v' )<< 8 )+(( U32 )( 'e' )<< 0 )), 
                          0, C_MINE_CAGE );
            if( 6 > p->current_state.value )
            {
               allowed = _FALSE;
            }
         }
         adv__print_status( allowed );

         start = NAME_NORTH - 1;
         end = NAME_DOWN - 1;
      }

      /* count the number of possible directions */
      for( j=0, i=start; i <= end; i++ )
      {
         way = adv_world.cells[ cavatar->location ].ways[ i ];
         if( 0 < way )
         {
            j++;
         }
      }
      if( 0 < j )
      {
         adv__look_around( start, end, j );
      }
      else
      if( NAME_NULL != wcode )
      {
         /* we look in a direction, but there is nothing there */
         PRINTS _I, _SEE, _NOTHING, _SPECIAL, _FULLSTOP, _NEWLINE END_PRINT
      }

      /* special location views */
      switch( cavatar->location )
      {
         static int o = 0;

         case C_FOREST_CANOPY :
         case C_MOUNTAIN_PASS :
         case C_HILLTOP :
         case C_HILLTOP_DISH :
         case C_MOUNTAIN_PEAK :
         case C_MOUNTAIN_OBSERVATORY_DOME :
         {
            print_lightbeam_state( _FALSE );
         }
         break;

         case C_MINE_OBSERVATION_DECK :
         {
            PRINTS _BELOW, _I, _CAN, _SEE, _THE END_PRINT
            print_cell_desc( &( adv_world.cells[ C_MOUNTAIN_LAB ].output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT

            if( 0 == o++ )
            {
               PRINTS _SUDDENLY, _THERE, _IS, _AN, _EXPLOSION, _FROM, _BELOW,
                      _ELIPSIS END_PRINT
               SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
               PRINTS _APPEARS, _BRIEFLY, _ABOVE, _THE, _BALCONY, _COMMA,
                      _BEFORE, _DROPPING, _BACK, _DOWN, _FULLSTOP, 
                      _NEWLINE END_PRINT
            }
         }
         break;

         case C_MOUNTAIN_LAB :
         {
            if( o )
            {
               PRINTS _THERE, _ARE, _SOME, _SCORCH, _MARKS, _ON, _THE, _WALL,
                      _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         break;

         default:
         break;
      }
   }
}

static void a_make( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
}

static void a_move( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   object_t *p = 0;
   object_t *q = 0;
   TRISTATE selected = _FALSE;
   U32 acode;
   signed char c = 0;
   UC direction;
   UC granularity;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
      selected = _TRUE;
   }

   if( obj )
   {
         /* apply verb to object<noun> */
         /* search object directions and follow the first non-0 entry if any */
      if( _FALSE == selected )
      {
         acode = adv__encode_name( adverb );
         direction = find_noun_as_builtin( acode );
      }
      else
      {
         direction = wcode;
      }

      /* test special cases: telescope joystick */
      if(((( U32 )( 'j' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'y' )<< 8 )+(( U32 )( 's' )<< 0 ))== obj->input.name )
      {
         /* get the granularity from the orange button setting */
         granularity = find_object_in_cell( 
                          ((( U32 )( 'b' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 't' )<< 0 )), 
                          ((( U32 )( 'o' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'n' )<< 0 )), 
                          C_MOUNTAIN_OBSERVATORY )-> current_state.value;

         switch( get_rotary_switch_state( C_MOUNTAIN_OBSERVATORY ))
         {
            case ROTARY_OFF :
            default :
            break;

            case ROTARY_DISH :
            {
               q = find_object_in_cell( 
                  ((( U32 )( 'd' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 'h' )<< 0 )), 0, C_HILLTOP );
            }
            break;

            case ROTARY_LIGHTBEAM :
            {
               p = find_object_in_cell( 
                  ((( U32 )( 't' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'e' )<< 0 )), 
                  0, C_MOUNTAIN_OBSERVATORY );
            }
            break;

            case ROTARY_DISH_MIRROR :
            {
               p = find_object_in_cell( 
                  ((( U32 )( 'm' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'r' )<< 0 )), 0, C_HILLTOP_DISH );
            }
            break;

            case ROTARY_SPIRE_MIRROR :
            {
               p = find_object_in_cell( 
                  ((( U32 )( 'm' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'r' )<< 0 )), 
                  0, C_CASTLE_SPIRE_ROOF );
            }
            break;
         }

         if( p )
         {
            /* the lightbeam rotation, or the mirror (mounts) rotation */
            switch( direction )
            {
               case NAME_UP :
               case NAME_DOWN :
               case NAME_LEFT :
               case NAME_RIGHT :
               {
                  if( 0 == granularity )
                  {
                     /* coarse granularity */
                     p->current_state.value = direction;
                  }
                  else
                  {
                     /* fine granularity */
                     if(( 15 ==( direction + p->current_state.value ))||
                        ( 11 ==( direction + p->current_state.value )))
                     {
                        /* NAME_AROUND means pointing at the desired target */
                        p->current_state.value = NAME_AROUND;
                     }
                     else
                     {
                        p->current_state.value = direction;
                     }
                  }

                  PRINTS _THE END_PRINT
                  print_object_desc( &( p->output ));
                  PRINTS _MOVES, _FULLSTOP, _NEWLINE END_PRINT
               }
               break;

               default :
               {
                  PRINTS _I, _CANNOT, _MOVE, _THE END_PRINT
                  print_object_desc( &( p->output ));
                  PRINTS _THAT, _WAY, _FULLSTOP, _NEWLINE END_PRINT
               }
               break;
            }

            lightbeam_event( p );
         }
         else
         if( q )
         {
            /* the dish rotation;
             * moves in 30-degree steps */
            switch( direction )
            {
               case NAME_LEFT :
               {
                  if( 2 < q->current_state.value )
                  {
                     q->current_state.value -= 3;  /* move 30deg each time */
                  }
                  else
                  {
                     q->current_state.value = 35;
                  }

                  PRINTS _THE END_PRINT
                  print_object_desc( &( q->output ));
                  PRINTS _MOVES, _ANOTHER, _3, _BS, _0, _DEGREES, _FULLSTOP, 
                         _NEWLINE END_PRINT
               }
               break;

               case NAME_RIGHT :
               {
                  if( 33 > q->current_state.value )
                  {
                     q->current_state.value += 3;  /* 30-deg at a time */
                  }
                  else
                  {
                     q->current_state.value = 0;
                  }

                  PRINTS _THE END_PRINT
                  print_object_desc( &( q->output ));
                  PRINTS _MOVES, _ANOTHER, _3, _BS, _0, _DEGREES, _FULLSTOP, 
                         _NEWLINE END_PRINT
               }
               break;

               default :
               {
                  PRINTS _I, _CANNOT, _MOVE, _THE END_PRINT
                  print_object_desc( &( q->output ));
                  PRINTS _THAT, _WAY, _FULLSTOP, _NEWLINE END_PRINT
               }
               break;
            }

            lightbeam_event( q );
         }
         else
         {
            /* neither p nor q targets */
            PRINTS _MOVING, _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _HAS, _NO, _EFFECT, _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      if(( SWITCH == obj->output.name )&&( ROTARY == obj->output.adjective ))
      {
         set_rotary_switch( cavatar->location, direction );

         print_rotary_switch_state( cavatar->location );
      }
      else
      if( SLIDER_SWITCH == obj->output.name )
      {
         set_slider_switch( obj, direction );
         print_element_state( get_rotary_switch_state( cavatar->location ));
      }
      else
      if(((( U32 )( 'w' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'e' )<< 0 ))== obj->input.name )
      {
         /* test special cases: wheel (laserbeam rack & pinion) */
         actions[ ACTION_TURN ]( vcode, ncode, obj, wcode );
      }
      else
      {
         PRINTS _I, _CANNOT, _MOVE, _THE END_PRINT
         print_object_desc( &obj->output);
         PRINTS _NEWLINE END_PRINT
      }
   }
   else
   {
      /* apply verb <ncode> to avatar */
      if( NAME_NULL < wcode )
      {
         direction = NAME_ENDS;

         /* apply builtin to avatar */
         if(( NAME_NORTH <= wcode )&&( NAME_DOWN >= wcode ))
         {
            direction =( wcode - 1 ); /* -1 for hardcoded enum _name_t */
         }
         else
         if( NAME_LEFT == wcode )
         {
            direction = NAME_WEST - 1;
         }
         else
         if( NAME_RIGHT == wcode )
         {
            direction = NAME_EAST - 1;
         }

         if( NAME_DOWN > direction )
         {
            c = adv_world.cells[ cavatar->location ].ways[ direction ];
         }
         else
         {
            c = 0;
         }

         if( 0 == c )
         {
            PRINTS _I, _CANNOT, _MOVE, _IN, _THAT, _DIRECTION, _FULLSTOP,
                   _NEWLINE END_PRINT
         }
      }
   }

   if( c )
   {
      if( 0 > c )
      {
         c = -c;
      }

      /* unselect anything in current cell */
      set_cell_selected( 0 );

      if( _FALSE == fullGameEnabled )
      {
         if( C_HILLTOP_DISH >= c )
         {
            SET_AVATAR_LOCATION( cavatar, c );
         }
         else
         {
            /* game limited, cannot move location */
            fullGameLimitHit = _TRUE;
         }
      }
      else
      {
         SET_AVATAR_LOCATION( cavatar, c );
      }

      adv__print_status( _FALSE );
   }
}

static void a_navigation( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   unsigned char direction = NAME_NULL;
   signed char c;
   int m = -1;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if( obj )
   {
      /* apply verb to object<noun> */
   }
   else
   {
      if( vcode )
      {
         /* apply verb <ncode> to avatar */
         switch( vcode )
         {
            case((( U32 )( 'n' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 't' )<< 0 )):
            case((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'n' )<< 0 )):
            {
               direction = NAME_NORTH;
            }
            break;

            case((( U32 )( 'e' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 't' )<< 0 )):
            case((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'e' )<< 0 )):
            case((( U32 )( 'r' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'h' )<< 0 )):
            case((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'r' )<< 0 )):
            {
               direction = NAME_EAST;
            }
            break;

            case((( U32 )( 's' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 't' )<< 0 )):
            case((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 's' )<< 0 )):
            {
               direction = NAME_SOUTH;
            } 
            break;

            case((( U32 )( 'w' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 't' )<< 0 )):
            case((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'w' )<< 0 )):
            case((( U32 )( 'l' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'f' )<< 8 )+(( U32 )( 't' )<< 0 )):
            case((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'l' )<< 0 )):
            {
               direction = NAME_WEST;
            }
            break;

            case((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'p' )<< 0 )):
            case((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'u' )<< 0 )):
            {
               direction = NAME_UP;
            }
            break;

            case((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'w' )<< 8 )+(( U32 )( 'n' )<< 0 )):
            case((( U32 )( 0 )<< 24 )+(( U32 )( 0 )<< 16 )+(( U32 )( 0 )<< 8 )+(( U32 )( 'd' )<< 0 )):
            {
               direction = NAME_DOWN;
            }
            break;

            default :
            break;
         }
      }
      else
      {
         direction = wcode;
      }

         /* special cases */
      if(( C_MOUNTAIN_LAKE_MID == cavatar->location )&&
         ( NAME_DOWN == direction ))
      {
         m=-1, test_inventory_worn( cavatar, &m );
         if( DIVINGSUIT != cavatar->inventory[ m ].output.name )
         {
            PRINTS _I, _CANNOT, _GO, _THERE, _YET, _FULLSTOP,
                     _NEWLINE END_PRINT
            direction = NAME_NULL;
         }
      }

      if(( NAME_NULL != direction )&&( MAX_WAYS >= direction ))
      {
         c = adv_world.cells[ cavatar->location ].ways[ direction  - 1 ];
         if( c )
         {
            if( 0 > c )
            {
               c = -c;
            }

            /* unselect anything in current cell */
            set_cell_selected( 0 );

            if( _FALSE == fullGameEnabled )
            {
               if( C_HILLTOP_DISH >= c )
               {
                  SET_AVATAR_LOCATION( cavatar, c );
               }
               else
               {
                  /* game limited, cannot move location */
                  fullGameLimitHit = _TRUE;
               }
            }
            else
            {
               SET_AVATAR_LOCATION( cavatar, c );
            }

            adv__print_status( _FALSE );
         }
         else
         {
            PRINTS _I, _CANNOT, _GO, _IN, _THAT, _DIRECTION, _FULLSTOP, 
                     _NEWLINE END_PRINT
         }
      }
      else
      {
         /* unknown direction */
      }
   }
}

/* Open is intended for objects like: door, window, chest */
static void a_open( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   object_t *p = 0;
   int i;
   int can;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
         /* check special cases */
      if(( C_MOUNTAIN_FUNICULAR == cavatar->location )&&
         ( DOOR == obj->output.name ))
      {
         /* can only open and close the funicular door at the start or end
          * of the line */
         p = find_object_in_cell(
               (( U32 )( 'l' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'v' )<< 8 )+( 'e'<< 0 ), 0, 
               C_MOUNTAIN_FUNICULAR );
         if(( 0 == p->current_state.value )||( 5 == p->current_state.value ))
         {
            allowed = test_and_set_verb( obj, ACTION_OPEN, _TRUE );
         }
      }
      else
      if(( C_MINE_CAGE == cavatar->location )&&( DOOR == obj->output.name ))
      {
         /* can only open and close the cage door at the top (5) or 
          * floor (0) of the lift;
          * a special case, at level (1) is the magnatron complex 
          * and at level (6) the observation deck */
         p = find_object_in_cell(
               (( U32 )( 'l' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'v' )<< 8 )+( 'e'<< 0 ), 0, 
               C_MINE_CAGE );
         if(( 0 == p->current_state.value )||( 5 == p->current_state.value )||
            ( 6 == p->current_state.value ))
         {
            allowed = test_and_set_verb( obj, ACTION_OPEN, _TRUE );
         }
         else
         if( 1 == p->current_state.value )
         {
            /* allowed into the magnotron complex if wearing Buster's gold
             * ring */
            i=-1, test_inventory_worn( cavatar, &i );
            if( O_RING == cavatar->inventory[ i ].output.name )
            {
               allowed = test_and_set_verb( obj, ACTION_OPEN, _TRUE );
            }
            else
            {
               PRINTS _I, _MUST, _BE, _WEARING, _THE, _RING, _TO, _GO,
                        _THERE, _FULLSTOP, _NEWLINE END_PRINT
            }
         }
      }
      else
      {
         /* search object to see if verb is an allowed action */
         allowed = test_and_set_verb( obj, ACTION_OPEN, _TRUE );
      }

      if( _TRUE == allowed )
      {
            /* specific post-cond actions for ACTION_OPEN */
         if((( BLUE == obj->output.adjective )||
             ( YELLOW == obj->output.adjective ))&&
            ( SWITCH == obj->output.name ))
         {
            lightbeam_event( obj );
         }
         else
         if( CAP == obj->output.name )
         {
            /* check the laser is disengaged */
            switch( get_lightbeam_state( ))
            {
               case L_DISENGAGE :
               case L_READY_DIAMOND :
               case L_READY_GOLD :
               {
                  /* the energy test-tube in the cap should be made un-hidden */
                     /* find the test-tube stored in the energy cap */
                  can = obj->current_state.descriptor;
                  p = &containers[ can ];
                  if( TESTTUBE == p->output.name )
                  {
                     p->properties &= ~PROPERTY_HIDDEN;
                  }
               }
               break;

               default :
               {
                  /* blow out, and game over */
                  adv__game_over( GAME_OVER_LIGHTBEAM_BLOWOUT );
                  adv_world.errnum = ERR_game;
               }
               break;
            }
         }
         else
         if((((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 ))==
               obj->input.name )&&
            (((( U32 )( 't' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'w' )<< 8 )+(( U32 )( 'e' )<< 0 ))==
               obj->input.adjective ))
         {
            p = find_object_in_cell( obj->input.name,
                                     obj->input.adjective, C_CASTLE_TOWER );
            p->current_state = obj->current_state;
         }
         else
         if( TAP == obj->output.name )
         {
            PRINTS _THE, _SINK, _IS, _FILLED, _WITH, _WATER, _FULLSTOP,
                   _NEWLINE END_PRINT

            /* fill the sink */
            p = find_object_in_cell(
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'k' )<< 0 ), 0,
                  C_CASTLE_KITCHEN );
            can = p->current_state.descriptor;
            if( OBJECT_NULL == containers[ can ].output.name )
            {
               p = find_object_in_cell(
                  (( U32 )( 'w' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'e' )<< 0 ), 0,
                  C_CASTLE_MOAT );
         
               /* fill the sink with water;
                * water properties remains perpetual while the tap is open */
               containers[ can ]= *p;
            }
         }

         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _IS, _NOW END_PRINT
         print_object_state( &( obj->current_state ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_play( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_PLAY, _TRUE );
      if( _FALSE == allowed )
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      if( _UNDECIDED == allowed )
      {
         /* action is allowed on this object, but its state is wrong */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _YET, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         PRINTS _ELIPSIS, _NOTHING, _SEEMS, _TO, _HAPPEN, _FULLSTOP,
                  _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_press( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   object_t *p = 0;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_PRESS, _FALSE );
      if( _TRUE == allowed )
      {
            /* specific post-cond actions for ACTION_PRESS */
         if(( ORANGE == obj->output.adjective )&&( BUTTON & obj->output.name ))
         {
            /* observatory telescope granularity */
            obj->current_state.value =( 1 - obj->current_state.value );

            PRINTS _THE, _SWITCH, _CLICKS, _AS, _I END_PRINT
            SPRINT_( outbuf, "%s", verb );
            PRINTS _IT, _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         if(( RED == obj->output.adjective )&&( BUTTON & obj->output.name ))
         {
            /* funicular power */
            obj->current_state.value =( 1 - obj->current_state.value );
            if( 1 == obj->current_state.value )
            {
               PRINTS _A, _LOUD, _BUZZER, _SOUNDS, _ONCE, _FULLSTOP,
                        _NEWLINE END_PRINT
            }
            else
            {
               PRINTS _IT, _CLICKS, _WHEN, _PRESSED, _FULLSTOP, 
                        _NEWLINE END_PRINT
            }

            p =( object_t* )find_object_in_cell(
                 ((( U32 )( 'f' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'i' )<< 0 )), 0, 
                 C_MOUNTAIN_TUNNEL );
            if( 0 != p )
            {
               /* open/close funicular */
               p->current_state.value = obj->current_state.value;
            }
         }
         else
         if((( BLUE == obj->output.adjective )||
             ( YELLOW == obj->output.adjective ))&&
            ( SWITCH & obj->output.name ))
         {
            PRINTS _THE, _SWITCH, _CLICKS, _AS, _I END_PRINT
            SPRINT_( outbuf, "%s", verb );
            PRINTS _IT, _FULLSTOP, _NEWLINE END_PRINT

            obj->current_state.value =( 1 - obj->current_state.value );
            lightbeam_event( obj );
         }
         else
         if( REMOTE == obj->output.name )
         {
            /* if wilbury is sat on the throne, then descend/ascend
             * between the C_CASTLE_THRONEROOM and C_CASTLE_CONTROLROOM.
             * Nelly comes too. */
            p =( object_t* )find_object_in_cell(
                 ((( U32 )( 't' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'o' )<< 0 )), 0, 
                 cavatar->location );
            if( p )
            {
               if( 1 == p->current_state.value )
               {
                  PRINTS _THE, _THRONE, _STARTS, _TO END_PRINT
                  if( C_CASTLE_THRONEROOM == cavatar->location )
                  {
                     adv_avatar[ A_NELLY ].location =
                        cavatar->location = C_CASTLE_CONTROLROOM;
                     PRINTS _DESCEND END_PRINT
                  }
                  else
                  {
                     adv_avatar[ A_NELLY ].location =
                        cavatar->location = C_CASTLE_THRONEROOM;
                     PRINTS _ASCEND END_PRINT
                  }
                  PRINTS _COMMA, _AND_ END_PRINT
                  SPRINT_( outbuf, "%s", adv_avatar[ A_NELLY ].name );
                  PRINTS _COMMA, _THE END_PRINT
                  print_object_desc( &adv_avatar[ A_NELLY ].output );
                  PRINTS _COMMA, _JUMPS, _ON, _FULLSTOP,_NEWLINE END_PRINT
               }
               else
               {
                  /* wilbury need to sit on the throne first */
                  PRINTS _I, _HEAR, _A, _CLICK, _COMMA, _BUT, _NOTHING,
                         _HAPPENS, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            else
            {
               /* if wilbury is not in the throne room */
               PRINTS _NOTHING, _SEEMS, _TO, _HAPPEN, _FULLSTOP, 
                      _NEWLINE END_PRINT
            }

            p = find_object_in_cell(
                  (( U32 )( 'b' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 't' )<< 0 ),
                  (( U32 )( 'w' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 ),
                  C_CASTLE_CONTROLROOM );
            p->current_state.value = 0;
         }
         else
         if(( WHITE == obj->output.adjective )&&
            ( C_CASTLE_CONTROLROOM == cavatar->location ))
         {
            if( 0 == obj->current_state.value )
            {
               PRINTS _A, _PROJECTION, _APPEARS, _ON, _THE, _SCREEN,
                        _SEMICOLON, _IT, _IS, _THE, _AGED, _MAN, _IN, _THE,
                        _PORTRAIT, _COLON, _NEWLINE END_PRINT
               PRINTS _HELLO, _AGAIN, _COMMA, _YOUNG END_PRINT
               SPRINT_( outbuf, "%s", cavatar->name );
               PRINTS _FULLSTOP END_PRINT
               SPRINT_( outbuf, "%s", adv_avatar[ A_HARRY ].name );
               PRINTS _TELLS, _ME, _COMMA, _AND_, _I, _MUST, _SAY, _I, 
                        _CONCUR, _COMMA, _DIAMONDS, _ARE, _IN, _SHORT, _SUPPLY,
                        _IN, _THE, _MINES, _FULLSTOP, _SO, _WE, _MUST, 
                        _INNOVATE, _TO, _SURVIVE, _COMMA, _WHAT, _QUESTION 
                      END_PRINT
               PRINTS _IVE, _RIGGED, _THE, _LIGHTBEAM, _TO, _MANUFACTURE,
                        _NEW, _DIAMONDS, _COMMA, _BY, _FOCUSING, _ENERGY, 
                        _FROM, _THE, _DISH, _ONTO, _CARBON, _DEPOSITS, 
                        _FULLSTOP END_PRINT
               PRINTS _THE, _PROBLEM, _COMMA, _FORMULA_, _RIGHT, _COMMA, _IS, 
                        _THAT, _IT, _TAKES, _TOO, _LONG, _FOR, _LIGHTBEAM, 
                        _ENERGY, _TO, _MASS, _ITSELF, _INTO, _A, _NEW, _DIAMOND,
                        _FULLSTOP END_PRINT
               PRINTS _IVE, _GOT, _THREE, _IDEAS, _ON, _THE, _GO, _COMMA,
                        _AND_, _NEED, _YOUR, _HELP, _FULLSTOP END_PRINT
               PRINTS _FIRST, _YOU, _CAN, _TRY, _MULTIPLE, _REFLECTIONS, _TO,
                        _THE, _MOON, _COMMA, _TO, _INCREASE, _THE, _SUM, _OF,
                        _ITS, _ENERGY, _FULLSTOP END_PRINT
               PRINTS _SECOND, _YOU, _CAN, _SET, _THE, _DISH, _TO, _POINT,
                        _AT, _THE, _SUN, _INSTEAD, _OF, _THE, _MOON, _FULLSTOP
                        END_PRINT
               PRINTS _REMEMBER, _COMMA, _DONT, _LOOK, _DIRECTLY, _AT, _THE, 
                        _SUN, _FULLSTOP END_PRINT
               PRINTS _YOULL, _NEED, _TO, _ASK END_PRINT
               SPRINT_( outbuf, "%s", adv_avatar[ A_CHARLIE ].name );
               PRINTS _OR_ END_PRINT
               SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
               PRINTS _FOR, _A, _LENS, _FILTER, _OF, _COURSE, _FULLSTOP 
                        END_PRINT
               PRINTS _THIRD, _YOU, _CAN, _TRY, _OUT, _MY, _NEWLY, _INVENTED,
                        _PHOTON, _LUMINOUS, _MAGNOTRON, _FULLSTOP, _ACTUALLY 
                        END_PRINT
               SPRINT_( outbuf, "%s", adv_avatar[ A_BUSTER ].name );
               PRINTS _COMMA, _THE END_PRINT
               print_object_desc( &adv_avatar[ A_BUSTER ].output );
               PRINTS _COMMA, _SHOULD, _JUST, _ABOUT, _HAVE, _FINISHED, 
                        _DIGGING, _AND_, _LINING, _A, _2, _BS, _0, _KM, _TORUS,
                        _UNDER, _THE, _MOUNTAIN, _FULLSTOP,
                        _THIS, _MAY, _BE, _A, _BIT, _RISKY, _COMMA, _BUT,
                        _WHAT, _THE, _HECK, _EH, _QUESTION END_PRINT
               PRINTS _SO, _WHERE, _DO, _YOU, _WANT, _TO, _BEGIN, _SP,
                        _MINUS, _SP, _WHAT, _DO, _YOU, _SAY, _SP, _MINUS,
                        _SP, _FIRST, _COMMA, _SECOND, _OR_, _THIRD, 
                        _QUESTION, _NEWLINE END_PRINT

               obj->current_state.value = 1;
            }
            else
            {
               PRINTS _THERE, _IS, _A, _MUTED, _CLICK, _FULLSTOP, _NEWLINE
                      END_PRINT
            }

            a_ask( vcode, 0, 0,  A_NELLY | A_MASK );
         }
         else
         {
            PRINTS _ELIPSIS, _NOTHING, _SEEMS, _TO, _HAPPEN, _FULLSTOP,
                     _NEWLINE END_PRINT
         }
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
      PRINTS _ELIPSIS, _NOTHING, _SEEMS, _TO, _HAPPEN, _FULLSTOP,
                     _NEWLINE END_PRINT
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_push( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   object_t *p = 0;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_PUSH, _TRUE );
      if( _FALSE == allowed )
      {
         if( BUTTON & obj->output.name )
         {
            /* try pressing a button instead */
            a_press( vcode, ncode, obj, wcode );
         }
         else
         {
            /* action is not allowed on this object */
            PRINTS _I, _CANNOT END_PRINT
            SPRINT_( outbuf, "%s", verb );
            PRINTS _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      if( _TRUE == allowed )
      {
         if(( CHAIN == obj->output.name )&&( BROKEN == obj->output .adjective))
         {
            PRINTS _ITS, _NO, _LONGER, _ATTACHED, _TO, _THE, _PORTCULLIS,
                     _AND_, _FALLS, _TO, _THE, _FLOOR, _FULLSTOP, _NEWLINE
                   END_PRINT
         }
         else
         if(( LEVER == obj->output.name )&&
            ( C_MOUNTAIN_FUNICULAR == cavatar->location ))
         {
               /* the funicular door must be closed */
            p = find_object_in_cell(
                  (( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 ), 0, 
                  C_MOUNTAIN_FUNICULAR );
            if( 0 == p->current_state.value )
            {
               /* To mimick a train journey into the heart of a mountain,
                * we will pull the funicular lever several times to mark
                * a 'change of tracks' */
               /* push the lever = descend,
                * pull the lever = ascend */
               if((( U32 )( 'p' )<< 24 )+('u'<<16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 'h' )<< 0 )== vcode )
               {
                  if( 0 < obj->current_state.value )
                  {
                     if( 5 == obj->current_state.value )
                     {
                        PRINTS _A, _CHIME, _SOUNDS, _AS, _OUR, _DESCENT,
                               _BEGINS END_PRINT
                     }
                     else
                     if( 3 == obj->current_state.value )
                     {
                        PRINTS _WE, _PASS, _THE, _OTHER, _ASCENDING,
                                 _FUNICULAR, _CARRIAGE END_PRINT
                     }
                     else
                     if( 4 == obj->current_state.value )
                     {
                        PRINTS _THE, _CLANK, _OF, _COGS, _AND_, _GEARS,
                               _MARKS, _OUR, _DESCENT END_PRINT
                     }
                     else
                     {
                        PRINTS _OUR, _DESCENT, _CONTINUES, _AS, _WE, _PASS,
                               _ROUGH, _HEWN, _ROCK, _WALLS END_PRINT
                     }
                     PRINTS _FULLSTOP, _NEWLINE END_PRINT

                     obj->current_state.value--;
                  }

                  if( 0 == obj->current_state.value )
                  {
                     PRINTS _A, _LOUD, _BUZZER, _SOUNDS, _ONCE, _FULLSTOP,
                           _WERE, _ALREADY, _AT, _THE, _FINISH, _FULLSTOP,
                           _NEWLINE END_PRINT
                  }
               }
               else
               {
                  if( 5 > obj->current_state.value )
                  {
                     if( 0 == obj->current_state.value )
                     {
                        PRINTS _A, _CHIME, _SOUNDS, _AS, _OUR, _ASCENT,
                               _BEGINS END_PRINT
                     }
                     else
                     if( 2 == obj->current_state.value )
                     {
                        PRINTS _WE, _PASS, _THE, _OTHER, _DESCENDING,
                                 _FUNICULAR, _CARRIAGE END_PRINT
                     }
                     else
                     if( 1 == obj->current_state.value )
                     {
                        PRINTS _THE, _CLANK, _OF, _COGS, _AND_, _GEARS,
                               _MARKS, _OUR, _ASCENT END_PRINT
                     }
                     else
                     {
                        PRINTS _OUR, _ASCENT, _CONTINUES, _AS, _WE, _PASS,
                               _ROUGH, _HEWN, _ROCK, _WALLS END_PRINT
                     }
                     PRINTS _FULLSTOP, _NEWLINE END_PRINT

                     obj->current_state.value++;
                  }

                  if( 5 == obj->current_state.value )
                  {
                     PRINTS _A, _LOUD, _BUZZER, _SOUNDS, _ONCE, _FULLSTOP,
                              _WERE, _ALREADY, _AT, _THE, _START, _FULLSTOP,
                              _NEWLINE END_PRINT
                  }
               }
            }
            else
            {
               PRINTS _I,_CANNOT, _DO, _THAT, _YET, _FULLSTOP,
                        _TWO, _CHIMES, _SOUND, _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         else
         if(( LEVER == obj->output.name )&&
            ( C_MINE_CAGE == cavatar->location ))
         {
               /* the cage door must be closed */
            p = find_object_in_cell(
                  (( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 ), 0, 
                  C_MINE_CAGE );
            if( 0 == p->current_state.value )
            {
               /* To mimick a descent down a mine shaft,
                * we will pull the cage lever several times to mark
                * a 'change of tracks' */
               /* push the lever = descend,
                * pull the lever = ascend */
                  /* the minehead hoist must be running */
               p = find_object_in_cell(
                  (( U32 )( 'h' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 's' )<< 0 ), 0, 
                  C_MINE_HEAD );
               if( 1 == p->current_state.value )
               {
                  if((( U32 )( 'p' )<< 24 )+('u'<<16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 'h' )<< 0 )== vcode )
                  {
                     if( 0 >= obj->current_state.value )
                     {
                        PRINTS _A, _LOUD, _BUZZER, _SOUNDS, _ONCE, _FULLSTOP,
                                 _WERE, _ALREADY, _AT, _THE, _FINISH,
                                 _FULLSTOP, _NEWLINE END_PRINT
                     }
                     else
                     {
                        if( 5 <= obj->current_state.value )
                        {
                           PRINTS _A, _CHIME, _SOUNDS, _AS, _OUR, _DESCENT,
                                  _BEGINS END_PRINT
                        }
                        else
                        {
                           PRINTS _THE, _WHINE, _OF, _CABLE, _OVER, _BEARINGS,
                                  _MARKS, _OUR, _DESCENT END_PRINT
                        }
                        obj->current_state.value--;
                        PRINTS _FULLSTOP, _THE, _CAGE, _ARRIVES, _AT, _LEVEL
                               END_PRINT
                        SPRINT_( outbuf, "%d", obj->current_state.value );
                        PRINTS _FULLSTOP, _NEWLINE END_PRINT
                     }
                  }
                  else
                  {
                     if( 6 <= obj->current_state.value )
                     {
                        PRINTS _A, _LOUD, _BUZZER, _SOUNDS, _ONCE, _FULLSTOP,
                                 _WERE, _ALREADY, _AT, _THE, _START,
                                 _FULLSTOP, _NEWLINE END_PRINT
                     }
                     else
                     {
                        if( 0 >= obj->current_state.value )
                        {
                           PRINTS _A, _CHIME, _SOUNDS, _AS, _OUR, _DESCENT,
                                  _BEGINS END_PRINT
                        }
                        else
                        {
                           PRINTS _THE, _WHINE, _OF, _CABLE, _OVER, _BEARINGS,
                                  _MARKS, _OUR, _DESCENT END_PRINT
                        }
                        obj->current_state.value++;
                        PRINTS _FULLSTOP, _THE, _CAGE, _ARRIVES, _AT, _LEVEL
                               END_PRINT
                        SPRINT_( outbuf, "%d", obj->current_state.value );
                        PRINTS _FULLSTOP, _NEWLINE END_PRINT
                     }
                  }
               }
               else
               {
                  PRINTS _NOTHING, _HAPPENS, _FULLSTOP,
                         _THERE, _IS, _NO, _POWER, _TO, _THE, _HOIST,
                            _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            else
            {
               PRINTS _I, _CANNOT, _DO, _THAT, _YET, _FULLSTOP,
                        _TWO, _CHIMES, _SOUND, _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         else
         {
            PRINTS _ELIPSIS, _NOTHING, _SEEMS, _TO, _HAPPEN, _FULLSTOP,
                     _NEWLINE END_PRINT
         }
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      if( 0x70756c6c == vcode )
      {
         PRINTS _PULL END_PRINT
      }
      else
      {
         PRINTS _PULL END_PRINT
      }
      PRINTS _FULLSTOP, _NEWLINE END_PRINT
   }
}

static void a_read( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   int n;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_READ, _TRUE );
      if( _TRUE == allowed )
      {
         /* specific post-cond actions for ACTION_READ */

         /* case of castle tower sign */
         if((((( U32 )( 's' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'n' )<< 0 ))==
             obj->input.name )&&
            (((( U32 )( 'd' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'g' )<< 0 ))==
             obj->input.adjective))
         {
            PRINTS _THERE, _IS, _A, _ONE, _MINUS, _WAY, _SIGN, _COMMA,
                     _POINTING, _DOWNWARDS, _FULLSTOP END_PRINT
            PRINTS _AND_, _A, _NO, _MINUS, _ENTRY, _SIGN, _COMMA,
                     _POINTING, _UPWARDS, _FULLSTOP, _AND_,
                     _THERE, _IS, _SOMETHING, _ELSE, _COMMA END_PRINT

            /* check whether avatar is wearing spectacles */
            n=-1, test_inventory_worn( cavatar, &n );
            if(( 0 <= n )&&
               ( SPECTACLES == cavatar->inventory[ n ].output.name ))
            {
               PRINTS _WHICH, _SAYS, _COLON, _NEWLINE END_PRINT
               PRINTS _TAB, _ESC_U, _TO, _THE, _MAP, _ROOM, _COMMA, _QUOTE_LEFT,
                      _ESC_U, _AL, _LA, _MAPO, _QUOTE_RIGHT, _NEWLINE END_PRINT
            }
            else
            {
               PRINTS _BUT, _I, _CANNOT, _QUITE, _MAKE, _IT, _OUT, _FULLSTOP,
                        _NEWLINE END_PRINT
            }
         }
         else
         if((((( U32 )( 's' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'n' )<< 0 ))==
             obj->input.name )&&
            (((( U32 )( 't' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'n' )<< 0 ))==
             obj->input.adjective))
         {
            PRINTS _IT, _SAYS, _COMMA, _WEAR, _PROTECTIVE, _GEAR, _BEYOND,
                     _THIS, _POINT, _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         if((((( U32 )( 's' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'n' )<< 0 ))==
             obj->input.name )&&
            (((( U32 )( 'm' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'e' )<< 0 ))==
             obj->input.adjective))
         {
            PRINTS _THERE, _IS, _A, _ONE, _MINUS, _WAY, _SIGN, _COMMA,
                     _POINTING, _IN, _ALL, _DIRECTIONS, _FULLSTOP END_PRINT
            PRINTS _THERE, _IS, _SOMETHING, _ELSE, _COMMA END_PRINT

            /* check whether avatar is wearing spectacles */
            n=-1, test_inventory_worn( cavatar, &n );
            if(( 0 <= n )&&
               ( SPECTACLES == cavatar->inventory[ n ].output.name ))
            {
               PRINTS _WHICH, _SAYS, _COLON, _NEWLINE END_PRINT
               PRINTS _TAB, _IF, _YOU, _NEED, _TO, _KNOW, _WHICH, _WAY, _TO, 
                      _GO, _DOFF, _YOUR, _HAT, _AND_, _ASK, _A, _BAT, _FULLSTOP,
                      _NEWLINE END_PRINT
            }
            else
            {
               PRINTS _BUT, _I, _CANNOT, _QUITE, _MAKE, _IT, _OUT, _FULLSTOP,
                        _NEWLINE END_PRINT
            }
         }
         else
         if((((( U32 )( 'b' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'k' )<< 0 ))==
             obj->input.name )&&
            (((( U32 )( 'k' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'p' )<< 0 ))==
             obj->input.adjective))
         {
            /* case of castle keep book */
            PRINTS _ITS, _A, _USER, _MANUAL, _FOR, _THE, _LIGHTBEAM, _COLON
                   END_PRINT

            /* check whether avatar is wearing spectacles */
            n=-1, test_inventory_worn( cavatar, &n );
            if(( 0 <= n )&&
               ( SPECTACLES == cavatar->inventory[ n ].output.name ))
            {
               PRINTS _NEWLINE, _WARNING, _COLON, _DO, _NOT, _OPEN, _FILLER,
                        _CAP, _WHILE, _BEAM, _IS, _OPERATING, _FULLSTOP,
                        _NEWLINE END_PRINT
               PRINTS _TO, _DISENGAGE, _COMMA, _CLOSE, _THE, _BLUE, _SWITCH,
                        _BEFORE, _THE, _YELLOW, _SWITCH, _FULLSTOP END_PRINT
               PRINTS _TO, _TOP, _UP, _COMMA, _OPEN, _THE, _FILLER, _CAP,
                        _COMMA, _INSERT, _A, _NEW, _ENERGY, _TUBE, _COMMA,
                        _THEN, _CLOSE, _THE, _CAP, _FULLSTOP END_PRINT
               PRINTS _TO, _ENGAGE, _COMMA, _INSERT, _THE, _MASTER, _KEY,
                        _COMMA, _THEN, _OPEN, _THE, _YELLOW, _SWITCH, _BEFORE,
                        _THE, _BLUE, _FULLSTOP, _NEWLINE END_PRINT
            }
            else
            {
               PRINTS _ELIPSIS, _BUT, _I, _CANNOT, _QUITE, _MAKE, _IT, _OUT,
                        _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         else
         if((((( U32 )( 0 )<< 24 )+(( U32 )( 'm' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'p' )<< 0 ))==
             obj->input.name )&&
            (((( U32 )( 'w' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'l' )<< 0 ))==
             obj->input.adjective))
         {
            /* is the castle maproom world map */
            PRINTS _NEWLINE END_PRINT
            PRINTS __MAP0, _NEWLINE,
                     __MAP1, _NEWLINE,
                     __MAP2, _NEWLINE,
                     __MAP3, _NEWLINE,
                     __MAP4, _NEWLINE,
                     __MAP5, _NEWLINE,
                     __MAP6, _NEWLINE,
                     __MAP7, _NEWLINE,
                     __MAP8, _NEWLINE,
                     __MAP9, _NEWLINE,
                     __MAP10, _NEWLINE,
                     __MAP11, _NEWLINE,
                     __MAP12, _NEWLINE END_PRINT
         }
         else
         if(( LAB == obj->output.adjective )&&( BOOK == obj->output.name ))
         {
            PRINTS _IT, _IS, _TITLED, _QUOTE_LEFT, _EXPERIMENTS, _IN,
                     _LIGHTBEAM, _ENERGY, _QUOTE_RIGHT, _FULLSTOP END_PRINT

            /* check whether avatar is wearing spectacles */
            n=-1, test_inventory_worn( cavatar, &n );
            if(( 0 <= n )&&
               ( SPECTACLES == cavatar->inventory[ n ].output.name ))
            {
               PRINTS _THERE, _ARE, _MANY, _PAGES, _OF, _EXPERIMENTS, _COMMA,
                        _NOT, _ALL, _COMPLETE, _ELIPSIS END_PRINT
               /* The lab book can be read several times, like reading a page
                * at a time. It circles back after the last page. */
               if( 0 == obj->current_state.value )
               {
                  PRINTS _THERE, _IS, _A, _WAY, _HERE, _TO, _MAKE, _ENERGY,
                           _REQUIRING, _A, _DIAMOND, _SOURCE, _AND_, _HEAVY,
                           _WATER, _FULLSTOP END_PRINT
                  PRINTS _A, _SKETCH, _SHOWS, _THE, _DIAMOND, _SOURCE, _IN, _A,
                           _CRUCIBLE, _COMMA, _HEAVY, _WATER, _IN, _A, _JAR,
                           _COMMA, _A, _BURET, _COMMA, _A, _BURNER, _AND_, 
                           _STAND, _COMMA, _AND_, _A, _TEST, _TUBE, _TO, _HOLD,
                           _THE, _RESULTING, _ENERGY, _FULLSTOP  END_PRINT
                  PRINTS _AH, _COMMA, _THE, _LAST, _ENTRY, _SAYS, _ASK 
                         END_PRINT
                  SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
                  PRINTS _TO, _MAKE, _ENERGY, _COMMA, _QUOTE_LEFT, _FARI,
                           _ENERGIO, _QUOTE_RIGHT, _FULLSTOP END_PRINT
                  PRINTS _THERES, _MORE, _ELIPSIS, _NEWLINE END_PRINT
               }
               else
               if( 1 == obj->current_state.value )
               {
                  PRINTS _ANOTHER, _PAGE, _SHOWS, _A, _METHOD, _FOR, _MAKING, 
                           _A, _SUN, _FILTER, _COMMA, _FROM, _SILVER,
                           _SAND, _AND_, _WATER, _FULLSTOP END_PRINT
                  PRINTS _IT, _SAYS, _TO, _HEAT, _THE, _SAND, _TO, _1,
                           _BS, _0, _BS, _0,
                           _DEGREES, _COMMA, _THEN, _PASS, _WATER, _THROUGH,
                           _IT, _TO, _DRAW, _OUT, _IMPURITIES, _FULLSTOP,
                           _HEAT, _UP, _TO, _1, _BS, _5, _BS,
                           _0, _BS, _0, _DEGREES, _THEN, _POUR,
                           _ONTO, _A, _BAKING, _DISH, _TO, _COOL, _BEFORE,
                           _CUTTING, _TO, _SIZE, _FULLSTOP END_PRINT
                  PRINTS _OR_, _IF END_PRINT
                  SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
                  PRINTS _IS, _AROUND, _COMMA, _FETCH, _SAND, _AND_, _WATER,
                           _THEN, _ASK, _HIM, _TO, _MAKE, _A, _FILTER,
                           _COMMA, _QUOTE_LEFT, _FARI, _FILTRILON, _QUOTE_RIGHT,
                           _FULLSTOP END_PRINT
                  PRINTS _THERES, _MORE, _ELIPSIS, _NEWLINE END_PRINT
               }
               else
               if( 2 == obj->current_state.value )
               {
                  PRINTS _THERE, _IS, _A, _METHOD, _FOR, _MAKING, _AND_,
                           _POLISHING, _MIRRORS, _COMMA, _FROM, _SILVER,
                           _SAND, _AND_, _WATER, _FULLSTOP END_PRINT

                  PRINTS _IT, _LOOKS, _LIKE, _THERES, _A, _THEME, _RUNNING,
                           _HERE, _COMMA, _BUT, _IT, _SAYS, _TO, _HEAT, _THE, 
                           _SAND, _TO, _1, _BS, _0, _BS, _0,
                           _DEGREES, _COMMA, _THEN, _PASS, _WATER, _THROUGH,
                           _IT, _TO, _DRAW, _OUT, _IMPURITIES, _FULLSTOP,
                           _HEAT, _UP, _TO, _1, _BS, _0, _BS,
                           _0, _BS, _0, _DEGREES, _THEN, _POUR,
                           _ONTO, _A, _BAKING, _DISH, _TO, _COOL, _FULLSTOP
                           END_PRINT

                  PRINTS _OR_, _IF END_PRINT
                  SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
                  PRINTS _IS, _AROUND, _COMMA, _FETCH, _SAND, _AND_, _WATER,
                           _THEN, _ASK, _HIM, _TO, _MAKE, _A, _MIRROR,
                           _COMMA, _QUOTE_LEFT, _FARI, _SPEGULON, _QUOTE_RIGHT,
                           _FULLSTOP END_PRINT
                  PRINTS _THERES, _MORE, _ELIPSIS, _NEWLINE END_PRINT
               }
               else
               if( 3 == obj->current_state.value )
               {
                  PRINTS _OTHER, _ENTRIES, _ARE, _TORN, _OR_, _CROSSED, _OUT,
                            _AND_, _ARE, _DIFFICULT, _TO, _READ, _FULLSTOP,
                            _BUT, _I, _CAN, _MAKE, _OUT, _SOME, _MAKING, _WORDS,
                            _IN, _THE, _MARGIN, _FULLSTOP
                         END_PRINT
                  PRINTS _IT, _MAY, _BE, _RISKY, _TO, _TRY, _THEM, _COMMA,
                            _BUT, _THEY, _ARE, _COLON END_PRINT
                  PRINTS _TEO, _AND_, _WHAT, _LOOKS, _LIKE, _A, _ROSTER, _OF,
                            _SORTS, _SEMICOLON END_PRINT
                  PRINTS _LENSO, _AND_, _SOMETHING, _RED, _MINUS, _INKED, _OUT,
                            _SEMICOLON END_PRINT
                  PRINTS  _ESTRARO, _FOLLOWED, _BY, _VERSION, _4, _FULLSTOP, 
                            _BS, _9, _FULLSTOP, _BS, _2, _SEMICOLON END_PRINT
                  PRINTS _SLOSILO, _THEN, _SOMETHING, _LIKE, _LOSE, _IT,
                            _AGAIN, _ELIPSIS, _SEMICOLON END_PRINT
                  PRINTS _SALTILO, _COMMA, _BUT, _THE, _REST, 
                            _IS, _ILLEGIBLE, _FULLSTOP, _NEWLINE END_PRINT
               }

               obj->current_state.value++;
               obj->current_state.value %= 4;
            }
            else
            {
               PRINTS _BUT, _I, _CANNOT, _QUITE, _MAKE, _THE, _ENTRIES, _OUT,
                        _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         else
         if(( PAPER == obj->output.name )&&( SHEET == obj->output.adjective ))
         {
            switch( obj->current_state.value )
            {
               case SOURCE_MOON_TEST :
               {
                  /* mirrors & the moon:
                   * light speed: (3*10^8)m/s, or 3*10^5 km/s;
                   * moon distance: ~ 4*10^5 km, *2 there and back;
                   * light takes ~ 2.5s to the moon and back.
                   * M=E/(C^2) ~= 2.5 / 3*10^10 , or approx M = 1 / 1*10^10. 
                   * Secs/day = 86400, or approx 100,000 = 1*10^5.
                   * so M ~= 10^5 days or 273 years.
                   * A 1000-fold increase -> ~10^2 days or 3-4 months. */
                   PRINTS _IT, _SAYS, _COMMA, _A, _METHOD, _TO, _REDUCE,
                            _TIME, _FOR, _THE, _MANUFACTURE, _OF, _DIAMONDS,
                            _FROM, _2, _BS, _7, _BS, _3, _YEARS,
                            _DOWN, _TO, _3, _MINUS, _4, _MONTHS, _COMMA,
                            _BY, _REFLECTING, _A, _LIGHTBEAM, _OFF, _THE,
                            _MOON, _FULLSTOP, _NEWLINE END_PRINT
                   PRINTS _STEP, _1, _COLON, _ASK END_PRINT
                   SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
                   PRINTS _TO, _BUILD, _A, _PAIR, _OF, _POLISHED, _MIRRORS,
                            _FULLSTOP END_PRINT
                   PRINTS _STEP, _2, _COLON, _FIX, _ONE, _MIRROR, _ON,
                            _THE, _COLLECTOR, _AND_, _ANOTHER, _ON, _THE, 
                            _SPIRE, _FULLSTOP END_PRINT
                   PRINTS _STEP, _3, _COLON, _ADJUST, _THE, _MIRRORS, _TO,
                            _FORM, _A, _LIGHTBEAM, _TRIANGLE, _COMMA, _FROM,
                            _THE, _SPIRE, _ROUND, _THE, _DARK, _SIDE, _OF, _THE,
                            _MOON, _COMMA, _TO, _THE, _DISH, _AND_, _BACK, _TO,
                            _THE, _SPIRE, _COMMA, _THAT, _REFLECTS, 
                            _1, _BS, _0, _BS, _0, _BS, _0, _TIMES, _BEFORE, 
                            _COLLECTING, _THE, _BEAM, _AT, _THE, _DISH, 
                            _FULLSTOP, _NEWLINE END_PRINT
               }
               break;

               case SOURCE_SUN_TEST :
               {
                  /* sun: 
                   * appears about 400,000 times brighter than the fullmoon.
                   * light speed: (3*10^8)m/s, or 3*10^5 km/s;
                   * sun distance: ~ 2000*10^5 km, *2 there and back;
                   * light takes ~ 1250s to the sun and back.
                   * M=E/(C^2) ~= 1250 / 3*10^10 , or approx M = 415 / 1*10^10. 
                   * Secs/day = 86400, or approx 100,000 = 1*10^5.
                   * so M ~= 10^5/415 days or 241 days.  */
                   PRINTS _IT, _SAYS, _COMMA, _A, _METHOD, _TO, _REDUCE,
                            _TIME, _FOR, _THE, _MANUFACTURE, _OF, _DIAMONDS,
                            _TO, _8, _MONTHS, _COMMA, _BY, _BOUNCING, _A,
                            _LIGHTBEAM, _OFF, _THE, _SUN, _FULLSTOP,
                            _DONT, _LOOK, _DIRECTLY, _AT, _THE, _SUN, _AND_,
                            _WEAR, _EYE, _PROTECTION, _AT, _ALL, _TIMES, 
                            _FULLSTOP END_PRINT
                   PRINTS _STEP, _1, _COLON, _TURN, _THE, _LIGHTBEAM, _TO, 
                            _POINT, _AT, _THE, _SUN, _FULLSTOP END_PRINT
                   PRINTS _STEP, _2, _COLON, 
                            _FIT, _A, _FILTER, _TO, _THE, _TELESCOPE, _LENS, 
                            _AND_, _ADJUST, _THE, _COLLECTOR, _DISH, _TO,
                            _RECEIVE, _REFLECTED, _LIGHTBEAM, _FULLSTOP
                            END_PRINT
                   PRINTS _STEP, _3, _COLON, _AS, _THE, _SUN, _IS, _4, _BS,
                            _0, _BS, _0, _BS, _0, _BS, _0, _BS, _0, _TIMES,
                            _BRIGHTER, _THAN, _THE, _MOON, _COMMA, _STAND,
                            _WELL, _BACK, _FULLSTOP, _NEWLINE END_PRINT
               }
               break;

               case SOURCE_MAGNOTRON_TEST :
               {
                  /* Phlux Magnotron:
                   * Similar to the moon, extend the distance the lightbeam
                   * travels to multiply the energy.
                   * light speed: (3*10^8)m/s, or 3*10^5 km/s;
                   * Ring distance: 20km (circumference) * 10000 = 2*10^9m
                   * light takes ~0.67s for 10000 trips round the ring;
                   * M=E/(C^2) ~= 0.67 / 3*10^10 , or approx M = 1 / 4*10^10. 
                   * Secs/day = 86400, or approx 100,000 = 1*10^5.
                   * M ~= 1/4*10^5 days or 400000 days, or about 1095 years. */
                  PRINTS _IT, _SAYS, _COMMA, _AN, _EXPERIMENT, _TO, _REDUCE,
                           _TIME, _FOR, _THE, _MANUFACTURE, _OF, _DIAMONDS,
                           _USING, _MAGNETS, _TO, _DIRECT, _PHOTON, _ENERGY,
                           _IN, _A, _SUBTERRANEAN, _RING, _MAGNOTRON, _FULLSTOP
                           END_PRINT
                  PRINTS _STEP, _1, _COLON, _LOCATE END_PRINT
                  SPRINT_( outbuf, "%s", adv_avatar[ A_BUSTER ].name );
                  PRINTS _AND_, _ASK, _FOR, _THE, _RING, _OF, _ADMITTANCE, 
                         _FULLSTOP END_PRINT
                  PRINTS _STEP, _2, _COLON, _GAIN, _ENTRY, _TO, _THE,
                           _MAGNOTRON, _COMPLEX, _FULLSTOP END_PRINT
                  PRINTS _STEP, _3, _COLON, _FOLLOW, _INSTRUCTIONS, _IN, _THE,
                           _MAGNOTRON, _OPERATION, _MANUAL, _TO, _INSTALL, _THE,
                           _CONTROLLER, _BOARDS, _AND_, _SWITCH, _ON,
                           _FULLSTOP, _NEWLINE END_PRINT
               }
               break;

               default :
               break;
            }
         }
         else
         if(( CONTROL == obj->output.adjective )&&( BOOK == obj->output.name ))
         {
            PRINTS _IT, _IS, _THE, _MANUAL, _FOR, _A, _PHOTON, _LUMINOUS,
                     _MAGNETIC, _CONTROLLER, _WITH, _KNOBS, _ON,
                     _FULLSTOP END_PRINT
            /* check whether avatar is wearing spectacles */
            n=-1, test_inventory_worn( cavatar, &n );
            if(( 0 <= n )&&
               ( SPECTACLES == cavatar->inventory[ n ].output.name ))
            {
               /* The lab book can be read several times, like reading a page
                * at a time. It circles back after the last page. */
               if( 0 == obj->current_state.value )
               {
                  PRINTS _THE, _FIRST, _PAGE, _IS, _THE, _ASSEMBLY, 
                         _INSTRUCTIONS, _FULLSTOP END_PRINT
                  PRINTS _STEP, _1, _COLON, _MAKE, _7, _CONTROLLER, _BOARDS,
                         _SEMICOLON, _ASK END_PRINT
                  SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
                  PRINTS _COMMA, _QUOTE_LEFT, _FARI, _ESTRARO, _QUOTE_RIGHT,
                         _FULLSTOP END_PRINT
                  PRINTS _STEP, _2, _COLON, _INSTALL, _A, _CONTROLLER, _BOARD, 
                         _AT, _EACH, _OF, _THE, _7, _RING, _LOCATIONS, 
                         _FULLSTOP END_PRINT
                  PRINTS _REMEMBER, _TO, _SWITCH, _OFF, _THE, _LIGHTBEAM,
                         _FIRST, _FULLSTOP END_PRINT
                  PRINTS _STEP, _3, _COLON, _SWITCH, _ON, _THE, _LIGHTBEAM, 
                         _AND_, _ADJUST, _EACH, _BOARD, _OUTPUT, _BY, _FIRST,
                         _TURNING, _THE, _ROTARY, _SWITCH, _TO, _SELECT, _IT, 
                         _AND_, _THEN, _BY, _MOVING, _THE, _CONTROL, _PANEL, 
                         _SLIDER, _SWITCHES, _UP, _OR_, _DOWN, _FULLSTOP
                         END_PRINT
                  PRINTS _FIND, _THE, _OPTIMUM, _EFFICIENCY, _SETTING, _FOR, 
                         _EACH, _BOARD, _FULLSTOP, _NEWLINE END_PRINT
                  PRINTS _THERES, _MORE, _ELIPSIS, _NEWLINE END_PRINT
               }
               else
               if( 1 == obj->current_state.value )
               {
                  PRINTS _THE, _NEXT, _PAGE, _SHOWS, _SEVERAL, _EQUATIONS, 
                         _AND_, _DIAGRAMS, _FULLSTOP END_PRINT
                  PRINTS _AT, _THE, _TOP, _OF, _THE, _PAGE, _IT, _GIVES, 
                         _THE, _FUNCTION, _BRACKET_LEFT, _BRACKET_LEFT,
                         _BLUE, _PLUS, _YELLOW, _BRACKET_RIGHT, _DIVIDE,
                         _RED, _BRACKET_RIGHT, _FULLSTOP END_PRINT
                  PRINTS _UNDERNEATH, _THIS, _A, _STRAIGHT, _LINE, _GRAPH, 
                         _SHOWS, _THE, _FUNCTION, _COLON, 
                         _EFFICIENCY, _SP, _EQUALS, _BRACKET_LEFT, 
                         _BRACKET_LEFT, _2, _BS, _5, _TIMES, _ELEMENT, 
                         _BRACKET_RIGHT, _PLUS, _2, _BS, _5, _BRACKET_RIGHT,
                         _FULLSTOP END_PRINT
                  PRINTS _UNDERNEATH, _THIS, _IS, _WRITTEN, _COLON, _BLUE, 
                         _HUNDREDS, _COMMA, _YELLOW, _TENS, _COMMA, _RED, 
                         _UNITS, _FULLSTOP END_PRINT
                  PRINTS _A, _FOOTNOTE, _SAYS, _IF, _YOU, _NEED, _MORE, _HELP,
                         _THEN, _ASK END_PRINT
                  SPRINT_( outbuf, "%s", adv_avatar[ A_BUSTER ].name );
                  PRINTS _THE END_PRINT
                  print_object_desc( &( adv_avatar[ A_BUSTER ].output ));
                  PRINTS _QUOTE_LEFT, _HELP, _MATH_, _QUOTE_RIGHT, _FULLSTOP, 
                         _NEWLINE END_PRINT
               }

               obj->current_state.value++;
               obj->current_state.value %= 2;
            }
            else
            {
               PRINTS _BUT, _I, _CANNOT, _QUITE, _MAKE, _THE, _ENTRIES, _OUT,
                        _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         else
         if(( CRIB == obj->output.adjective )&&( NOTE == obj->output.name ))
         {
            PRINTS _IT, _SAYS, _COLON, _BLUE, _HUNDREDS, _COMMA, _YELLOW, _TENS,
                   _COMMA, _RED, _UNITS, _FULLSTOP, _SO, _NEWLINE END_PRINT
            PRINTS _TAB, _BLUE, _COLON, _0, _COMMA, _1, _BS, _0, _BS, _0, _COMMA,
                   _2, _BS, _0, _BS, _0, _COMMA, _3, _BS, _0, _BS, _0, _COMMA,
                   _4, _BS, _0, _BS, _0, _COMMA, _5, _BS, _0, _BS, _0, _COMMA,
                   _6, _BS, _0, _BS, _0, _COMMA, _7, _BS, _0, _BS, _0, _COMMA,
                   _8, _BS, _0, _BS, _0, _COMMA, _9, _BS, _0, _BS, _0, 
                   _NEWLINE END_PRINT
            PRINTS _TAB, _YELLOW, _COLON, _0, _COMMA, _1, _BS, _0, _COMMA,
                   _2, _BS, _0, _COMMA, _3, _BS, _0, _COMMA,
                   _4, _BS, _0, _COMMA, _5, _BS, _0, _COMMA,
                   _6, _BS, _0, _COMMA, _7, _BS, _0, _COMMA,
                   _8, _BS, _0, _COMMA, _9, _BS, _0, _NEWLINE END_PRINT
            PRINTS _TAB, _RED, _COLON, _0, _COMMA, _1, _COMMA, _2, _COMMA, _3, _COMMA,
                   _4, _COMMA, _5, _COMMA, _6, _COMMA, _7, _COMMA,
                   _8, _COMMA, _9, _NEWLINE END_PRINT
            PRINTS _IT, _SAYS, _COLON, _EFFICIENCY, _SP, _EQUALS, _BRACKET_LEFT,
                   _BRACKET_LEFT, _2, _BS, _5, _TIMES, _ELEMENT, 
                   _BRACKET_RIGHT, _PLUS, _2, _BS, _5, _BRACKET_RIGHT,
                   _FULLSTOP, _SO, _NEWLINE END_PRINT
            PRINTS _TAB, _ELEMENT, _0, _COLON, _BRACKET_LEFT, _BRACKET_LEFT, 
                   _2, _BS, _5, _TIMES, _0, _BRACKET_RIGHT, _PLUS, _2, _BS, _5, 
                   _BRACKET_RIGHT, _SP, _EQUALS,  _2, _BS, _5, _FULLSTOP, 
                   _NEWLINE END_PRINT
            PRINTS _TAB, _ELEMENT, _1, _COLON, _BRACKET_LEFT, _BRACKET_LEFT, 
                   _2, _BS, _5, _TIMES, _1, _BRACKET_RIGHT, _PLUS, _2, _BS, _5, 
                   _BRACKET_RIGHT, _SP, _EQUALS,  _5, _BS, _0, _FULLSTOP, 
                   _NEWLINE END_PRINT
            PRINTS _AND_, _SO, _ON, _SUCH, _THAT,
                   _ELEMENT, _2, _SP, _EQUALS, _7, _BS, _5, _COMMA,
                   _ELEMENT, _3, _SP, _EQUALS, _1, _BS, _0, _BS, _0, _COMMA,
                   _ELEMENT, _4, _SP, _EQUALS, _1, _BS, _2, _BS, _5, _COMMA,
                   _ELEMENT, _5, _SP, _EQUALS, _1, _BS, _5, _BS, _0, _COMMA,
                   _AND_, _ELEMENT, _6, _SP, _EQUALS, _1, _BS, _7, _BS, _5,
                   _FULLSTOP, _NEWLINE, _NEWLINE END_PRINT
            PRINTS _YOU, _CAN, _SET, _THE, _SLIDER, _SWITCHES, _IN, _SEVERAL,
                   _COMBINATIONS, _FULLSTOP, _FOR, _EXAMPLE, _COMMA, _FOR,
                   _ELEMENT, _0, _YOU, _COULD, _SET, _BLUE, _TO, _0, _COMMA,
                   _YELLOW, _TO, _5, _COMMA, _AND_, _RED, _TO, _2, _THAT, _IS,
                   _COLON, _BRACKET_LEFT, _0, _PLUS, _5, _BS, _0, 
                   _BRACKET_RIGHT, _DIVIDE, _2, _FULLSTOP END_PRINT
            PRINTS _SIMILARLY, _FOR, _ELEMENT, _1, _YOU, _COULD, _SET,
                   _BLUE, _TO, _0, _COMMA, _YELLOW, _TO, _5, _COMMA, _AND_, 
                   _RED, _TO, _1, _THAT, _IS, _COLON, _BRACKET_LEFT, _0, _PLUS,
                   _5, _BS, _0, _BRACKET_RIGHT, _DIVIDE, _1, _FULLSTOP END_PRINT
            PRINTS _AND_, _FOR, _ELEMENT, _6, _YOU, _COULD, _SET,
                   _BLUE, _TO, _3, _COMMA, _YELLOW, _TO, _5, _COMMA, _AND_, 
                   _RED, _TO, _2, _THAT, _IS, _COLON, _BRACKET_LEFT, _3, _BS, 
                   _0, _BS, _0, _PLUS, _5, _BS, _0, _BRACKET_RIGHT, _DIVIDE, 
                   _2, _FULLSTOP END_PRINT
            PRINTS _IT, _MAY, _HELP, _IF, _YOU, _WORK, _OUT, _SLIDER, _SWITCH, 
                   _SETTINGS, _FOR, _THE, _REMAINING, _ELEMENTS, _BEFORE, 
                   _ENTERING, _THEM, _ON, _THE, _CONTROL, _PANEL, _FULLSTOP, 
                   _NEWLINE END_PRINT
         }
         else
         if( METER == obj->output.name )
         {
            print_meter_state( );
         }
         else
         {
            PRINTS _I, _DONT, _KNOW, _WHAT END_PRINT
            SPRINT_( outbuf, "%s", 
                     IS_VOWEL( get_vowel_of_object( &( obj->input ))));
            PRINTS _LESS_ END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _MORE_, _IS, _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

/* a_say is a special case of input, of the form <verb> <string> */
static void a_say( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE spoken = _FALSE;
   int n = -1;
   int m = -1;
   int k = -1;
   int j = 0;


   if( noun )
   {
      DEBUGF("Action 0x%lx for %s\n", vcode, noun );

      /* test for specific spoken phrases and game states */
      get_verb_and_string( );

      if(_FALSE != adv_strcmpi((UC *) &("hello"), noun ))
      {
         if( 0 != print_cell_avatars(cavatar->location, _TRUE ))
         {
            spoken = _TRUE;
         }
      }
      else
      if( _TRUE == fullGameEnabled )
      {
         /* test 1: crow is in cell and say "" */
         DEBUGF( "Checking current cell %d\n", cavatar->location );
         if(( cavatar->location == adv_avatar[ A_LUCY ].location )&&
            ( &adv_avatar[ A_LUCY ]!= cavatar ))
         {
            DEBUGF( "CROW is in the cell\n" );

            if( _TRUE == adv_strcmpi(( UC* )&( "Al La Kastelo" ), noun ))
            {
               DEBUGF( "To the castle..." );
               SET_AVATAR_LOCATION( cavatar, C_CASTLE_DRAWBRIDGE );
               spoken = _TRUE;
               adv_avatar[ A_LUCY ].location = C_CASTLE_DRAWBRIDGE;
            }
            else
            if( _TRUE == adv_strcmpi(( UC* )&( "Al La Arbaro" ), noun ))
            {
               DEBUGF( "To the forest..." );
               SET_AVATAR_LOCATION( cavatar, C_FOREST_CANOPY );
               spoken = _TRUE;
               adv_avatar[ A_LUCY ].location = C_FOREST_CANOPY;
            }
            else
            if( _TRUE == adv_strcmpi(( UC* )&( "Al La Montoj" ), noun ))
            {
               DEBUGF( "To the mountains..." );
               SET_AVATAR_LOCATION( cavatar, C_MOUNTAIN_PASS );
               spoken = _TRUE;
               adv_avatar[ A_LUCY ].location = C_MOUNTAIN_PASS;
            }

            if( _TRUE == spoken )
            {
               PRINTS _THE, _CROW, _CARRIES, _ME, _ALOFT, _FULLSTOP,
                      _NEWLINE END_PRINT
            }
            else
            {
               a_ask( vcode, ncode, obj, A_LUCY | A_MASK );
               spoken = _TRUE;
            }
         }

         /* test 2: rat is in cell and say "" */
         if(( cavatar->location == adv_avatar[ A_GARY ].location )&&
            ( &adv_avatar[A_GARY] != cavatar ))
         {
            PRINTS _THE END_PRINT
            print_object_desc( &( adv_avatar[ A_GARY ].output ));
            PRINTS _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS END_PRINT

            if(( C_CASTLE_DUNGEON == cavatar->location )&&
               ( _TRUE == adv_strcmpi(( UC* )&( "al la mapo" ), noun )))
            {
               PRINTS _BS, _ESC_L, _AND_, _LEADS, _ME, _THROUGH, _A, _CRACK,
                      _IN, _THE, _WALL, _FULLSTOP, _NEWLINE END_PRINT
               SET_AVATAR_LOCATION( cavatar, C_CASTLE_MAPROOM );
               spoken = _TRUE;
               adv_avatar[ A_GARY ].location = C_CASTLE_MAPROOM;
            }
            else
            {
               a_ask( vcode, ncode, obj, A_GARY | A_MASK );
               spoken = _TRUE;
            }
         }

         /* test 3: monkey is in cell and say "" */
         if(( cavatar->location == adv_avatar[ A_OTTO ].location )&&
            ( &adv_avatar[ A_OTTO ] != cavatar ))
         {
            a_ask( vcode, ncode, obj, A_OTTO | A_MASK );
            spoken = _TRUE;
         }

         /* test 4: bat is in cell and say "" */
         if(( cavatar->location == adv_avatar[ A_HARRY ].location )&&
            ( &adv_avatar[ A_HARRY ]!= cavatar ))
         {
            a_ask( vcode, ncode, obj, A_HARRY | A_MASK );
            spoken = _TRUE;
         }

         /* test 5: mole is in cell and say "" */
         if(( cavatar->location == adv_avatar[ A_BUSTER ].location )&&
            ( &adv_avatar[ A_BUSTER ]!= cavatar ))
         {
            a_ask( vcode, ncode, obj, A_BUSTER | A_MASK );
            spoken = _TRUE;
         }

         /* test 6: wizard is in cell and say "" */
         if(( cavatar->location == adv_avatar[ A_CHARLIE ].location )&&
            ( &adv_avatar[ A_CHARLIE ]!= cavatar ))
         {
            a_ask( vcode, ncode, obj, A_CHARLIE | A_MASK );
            spoken = _TRUE;
         }

         /* test 7: wizard is in cell and say "" */
         if(( cavatar->location == adv_avatar[ A_LOFTY ].location )&&
            ( &adv_avatar[ A_LOFTY ]!= cavatar ))
         {
            a_ask( vcode, ncode, obj, A_LOFTY | A_MASK );
            spoken = _TRUE;
         }

         /* test 8: specific location tests, gnome in cell */
         if( cavatar->location == C_CASTLE_CONTROLROOM )
         {
            if( _FALSE != adv_strcmpi(( UC* )&( "first" ), noun ))
            {
               if( SOURCE_MOON_TEST &
                   adv_world.sources[ SOURCE_ENERGY ].current_state.value )
               {
                  PRINTS _THE END_PRINT
                  print_object_desc(&( adv_avatar[ A_NELLY ].output ));
                  PRINTS _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS END_PRINT
                  PRINTS _ESC_L, _THAT, _TASK, _IS, _ALREADY, _COMPLETE,
                         _FULLSTOP,
                         _NEWLINE END_PRINT
               }
               else
               {
                  /* mirrors and the moon */
                  m = find_space_in_cell( C_NULL );
                  (void) test_inventory(
                          &adv_avatar[ A_NELLY ],
                          ((( U32 )( 'p' )<< 24 ) + (( U32 )( 'a' )<< 16 ) + (( U32 )( 'p' )<< 8 ) + (( U32 )( 'e' )<< 0 )),
                          ((( U32 )( 's' )<< 24 ) + (( U32 )( 'h' )<< 16 ) + (( U32 )( 'e' )<< 8 ) + (( U32 )( 'e' )<< 0 )),
                          &n );
                  test_inventory_free_space( cavatar, &k );
                  j = SOURCE_MOON_TEST;  /* for energy_test() */
               }
            }
            else
            if( _FALSE != adv_strcmpi(( UC* )&( "second" ), noun ))
            {
               if( SOURCE_SUN_TEST &
                   adv_world.sources[ SOURCE_ENERGY ].current_state.value )
               {
                  PRINTS _THE END_PRINT
                  print_object_desc( &( adv_avatar[ A_NELLY ].output ));
                  PRINTS _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS END_PRINT
                  PRINTS _THAT, _TASK, _IS, _ALREADY, _COMPLETE, _FULLSTOP,
                         _NEWLINE END_PRINT
               }
               else
               {
                  /* sun */
                  m = find_space_in_cell( C_NULL );
                  (void) test_inventory(
                          &adv_avatar[ A_NELLY ],
                          ((( U32 )( 'p' )<< 24 ) + (( U32 )( 'a' )<< 16 ) + (( U32 )( 'p' )<< 8 ) + (( U32 )( 'e' )<< 0 )),
                          ((( U32 )( 's' )<< 24 ) + (( U32 )( 'h' )<< 16 ) + (( U32 )( 'e' )<< 8 ) + (( U32 )( 'e' )<< 0 )),
                          &n );
                  test_inventory_free_space( cavatar, &k );
                  j = SOURCE_SUN_TEST;  /* for energy_test() */
               }
            }
            else
            if( _FALSE != adv_strcmpi(( UC * )&( "third" ), noun ))
            {
               if( SOURCE_MAGNOTRON_TEST &
                   adv_world.sources[ SOURCE_ENERGY ].current_state.value )
               {
                  PRINTS _THE END_PRINT
                  print_object_desc( &( adv_avatar[A_NELLY].output ));
                  PRINTS _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS END_PRINT
                  PRINTS _THAT, _TASK, _IS, _ALREADY, _COMPLETE, _FULLSTOP,
                         _NEWLINE END_PRINT
               }
               else
               {
                  /* Phlux Magnotron */
                  m = find_space_in_cell( C_NULL );
                  (void) test_inventory(
                          &adv_avatar[ A_NELLY ],
                          ((( U32 )( 'p' )<< 24 ) + (( U32 )( 'a' )<< 16 ) + (( U32 )( 'p' )<< 8 ) + (( U32 )( 'e' )<< 0 )),
                          ((( U32 )( 's' )<< 24 ) + (( U32 )( 'h' )<< 16 ) + (( U32 )( 'e' )<< 8 ) + (( U32 )( 'e' )<< 0 )),
                          &n );
                  test_inventory_free_space( cavatar, &k );
                  j = SOURCE_MAGNOTRON_TEST;  /* for energy_test() */
               }
            }


            if(( 0 <= n )&&
               ( 0 == adv_avatar[ A_NELLY ].inventory[ n ].current_state.value )&&
               ( &adv_avatar[ A_NELLY ]!= cavatar ))
            {
               if(( 0 <= m )&&( 0 <= k )&&( 0 < j ))
               {
                  SPRINT_( outbuf, "%s", adv_avatar[ A_NELLY ].name );
                  PRINTS _HANDS, _ME, _A, _SHEET, _OF, _PAPER, _FULLSTOP,
                         _NEWLINE END_PRINT
                  /* arm the sheet of paper with the new task number;
                   * the sheet of paper is cleared in energy_test on task
                   * completion */
                  adv_avatar[ A_NELLY ].inventory[ n ].current_state.value = j;
                  CLEAR_AVATAR_INVENTORY(
                          &adv_avatar[ A_NELLY ],
                          adv_world.cells[ C_NULL ].objects[ m ],
                          &adv_avatar[ A_NELLY ].inventory[ n ]);
                  /* obj = EMPTY */
                  SET_AVATAR_INVENTORY(
                          cavatar,
                          &( cavatar->inventory[ k ]),
                          adv_world.cells[ C_NULL ].objects[ m ]);

                  if( SOURCE_MAGNOTRON_TEST == j )
                  {
                     SPRINT_( outbuf, "%s", adv_avatar[ A_NELLY ].name );
                     PRINTS _SAYS, _QUOTE_LEFT, _LOOKS, _LIKE, _YOU, _NEED, _TO,
                            _BE, _LOOKING, _FOR, _A, _MOLE, _FULLSTOP,
                            _QUOTE_RIGHT, _NEWLINE END_PRINT

                     /* set Buster roaming around the forest */
                     adv_avatar[ A_BUSTER ].location = C_FOREST;
                     adv_avatar[ A_BUSTER ].current_state.descriptor = 1;
                  }

                  spoken = _TRUE;
               }
            }
         }
      }
      else
      {
         /* game limited, cannot move location */
         fullGameLimitHit = _TRUE;
      }
   }

   if( _FALSE == spoken )
   {
      PRINTS _NOTHING, _SEEMS, _TO, _HAPPEN, _FULLSTOP, _NEWLINE END_PRINT
   }
}

static void a_show( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
   }
   else
   if( wcode & A_MASK )
   {
      /* apply verb to named avatar */
      a_look( 0, 0, 0, wcode );
   }
   else
   if( wcode )
   {
      /* apply verb to builtin */
      if( NAME_INVENTORY == wcode )
      {
         a_inventory( 0, 0, 0, wcode );
      }
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_set( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   TRISTATE selected = _FALSE;
   TRISTATE printed = _FALSE;
   U32 acode = 0;
   UC direction;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
      selected = _TRUE;
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      /* search object to see if verb is an allowed action,
       * and apply post-condition if so */
      allowed = test_and_set_verb( obj, ACTION_SET, _TRUE );
      if( _FALSE == allowed )
      {
         if( BUTTON & obj->output.name )
         {
            /* try pressing a button instead */
            a_press( vcode, ncode, obj, wcode );
         }
         else
         {
            /* action is not allowed on this object */
            PRINTS _I, _CANNOT END_PRINT
            SPRINT_( outbuf, "%s", verb );
            PRINTS _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      if( _UNDECIDED == allowed )
      {
         /* action is not allowed on this object in its current state */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _YET, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         if( _FALSE == selected )
         {
            acode = adv__encode_name( adverb );
            direction = find_noun_as_builtin( acode );
         }
         else
         {
            direction = wcode;
         }

         if( NAME_ENDS == direction )
         {
            if(((( U32 )( 'o' )<< 16 )+(( U32 )( 'f' )<< 8 )+(( U32 )( 'f' )<< 0 ))== acode )
            {
               direction = 0;
            }
            else
            if((((( U32 )( 'o' )<< 8 )+(( U32 )( 'n' )<< 0 ))== acode )||( 0 == acode ))
            {
               direction = 1;
            }

            if( NAME_ENDS != direction )
            {
               if(( LAMP == obj->output.name )||
                  ( HOIST == obj->output.name ))
               {
                  obj->current_state.value = direction;
               }
            }
         }
         else
         {
            /* specific post-cond actions for ACTION_SET */
            if( ROTARY == obj->output.adjective )
            {
               set_rotary_switch( cavatar->location, direction );

               print_rotary_switch_state( cavatar->location );
               printed = _TRUE;
            }
            else
            if( SLIDER_SWITCH == obj->output.name )
            {
               set_slider_switch( obj, direction );
               print_element_state( 
                     get_rotary_switch_state( cavatar->location ));
               printed = _TRUE;
            }
         }

         if( _FALSE == printed )
         {
            PRINTS _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _IS END_PRINT
            print_object_state( &( obj->current_state ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_sit( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   object_t *p = 0;
   TRISTATE allowed;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
#     if defined( DEBUG )
      DEBUGF( "apply verb to object 0x%lx ", ncode );
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif

      allowed = test_and_set_verb( obj, ACTION_SIT, _TRUE );
      if( _TRUE == allowed )
      {
         /* test special cases */
         if(( O_THRONE == obj->output.name )&&
            ( C_CASTLE_THRONEROOM == cavatar->location ))
         {
            PRINTS _I, _AM, _SAT, _ON, _THE, _THRONE, _FULLSTOP, _NEWLINE
                   END_PRINT

            p =( object_t* )find_object_in_cell(
                 ((( U32 )( 'h' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'd' )<< 0 )), 
                 ((( U32 )( 'y' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 )), 
                 C_CASTLE_THRONEROOM );
            if( 0 != p )
            {
               /* make headband visible */
               p->properties &= ~PROPERTY_HIDDEN;
            }
         }
         else
         if(( SEAT == obj->output.name )&&
            ( C_MOUNTAIN_OBSERVATORY == cavatar->location ))
         {
            PRINTS _I, _AM, _SAT, _ON, _THE, _TELESCOPE, _CHAIR, _FULLSTOP,
                     _NEWLINE END_PRINT

            p =( object_t* )find_object_in_cell(
                 ((( U32 )( 'j' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'y' )<< 8 )+(( U32 )( 's' )<< 0 )), 
                 0, C_MOUNTAIN_OBSERVATORY );
            if( 0 != p )
            {
               /* make control panel visible */
               p->properties &= ~PROPERTY_HIDDEN;
            }

            /* orange button controls granularity of target adjustment */
            p =( object_t* )find_object_in_cell(
                 ((( U32 )( 'b' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 't' )<< 0 )),
                 ((( U32 )( 'o' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'n' )<< 0 )),
                 C_MOUNTAIN_OBSERVATORY );
            if( 0 != p )
            {
               /* make control panel button visible */
               p->properties &= ~PROPERTY_HIDDEN;
            }

            /* rotary switch selects target for adjustment */
            p =( object_t* )find_object_in_cell(
                 ((( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 )),
                 ((( U32 )( 'r' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'a' )<< 0 )),
                 C_MOUNTAIN_OBSERVATORY );
            if( 0 != p )
            {
               /* make control panel button visible */
               p->properties &= ~PROPERTY_HIDDEN;
            }
         }
         else
         {
            PRINTS _I, _AM, _SAT, _ON, _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _ON, _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      {
         PRINTS _I, _AM, _SAT, _ON, _THE, _GROUND, _FULLSTOP, _NEWLINE
                END_PRINT
      }
   }
}

static void a_smell( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif

      allowed = test_and_set_verb( obj, ACTION_SMELL, _TRUE );
      if( _TRUE == allowed )
      {
         PRINTS _I, _SMELL, _NOTHING, _SPECIAL, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      if( C_CASTLE_DUNGEON == cavatar->location )
      {
         PRINTS _THE, _DUNGEON, _SMELLS, _FAINTLY, _OF, _ROTTING, 
                _VEGETATION, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         PRINTS _I, _SMELL, _NOTHING, _SPECIAL, _FULLSTOP, _NEWLINE END_PRINT
      }
   }
}

static void a_stand( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed;
   object_t *p = 0;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif

      allowed = test_and_set_verb( obj, ACTION_STAND, _TRUE );
      if( _TRUE == allowed )
      {
         PRINTS _I, _AM, _NOW, _STANDING END_PRINT
         PRINTS _ON, _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _ON, _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      {
         PRINTS _I, _AM, _NOW, _STANDING, _FULLSTOP, _NEWLINE END_PRINT
      }
   }

   if( C_MOUNTAIN_OBSERVATORY == cavatar->location )
   {
      p =( object_t* )find_object_in_cell(
           ((( U32 )( 'j' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'y' )<< 8 )+(( U32 )( 's' )<< 0 )), 0, C_MOUNTAIN_OBSERVATORY );
      if( 0 != p )
      {
         /* make control panel hidden */
         p->properties |= PROPERTY_HIDDEN;
      }

      p =( object_t* )find_object_in_cell(
           ((( U32 )( 'b' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 't' )<< 0 )),
           ((( U32 )( 'o' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'n' )<< 0 )),
           C_MOUNTAIN_OBSERVATORY );
      if( 0 != p )
      {
         /* make control panel button hidden */
         p->properties |= PROPERTY_HIDDEN;
      }

      /* rotary switch selects target for adjustment */
      p =( object_t* )find_object_in_cell(
           ((( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 )),
           ((( U32 )( 'r' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'a' )<< 0 )),
           C_MOUNTAIN_OBSERVATORY );
      if( 0 != p )
      {
         /* make control panel rotary switch hidden */
         p->properties |= PROPERTY_HIDDEN;
      }
   }

   if(( C_CASTLE_THRONEROOM == cavatar->location )||
      ( C_CASTLE_CONTROLROOM == cavatar->location ))
   {
      p =( object_t* )find_object_in_cell(
                 ((( U32 )( 't' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'o' )<< 0 )), 
                 0, 
                 cavatar->location );
      if( 0 != p )
      {
         /* record throne not sat in */
         p->current_state.value = 0;
      }
   }
}

static void a_swim( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   switch( cavatar->location )
   {
      case C_FOREST_POOL :
      case C_FOREST_POOL_FLOOR :
      case C_CASTLE_MOAT :
      case C_MOUNTAIN_LAKE :
      case C_MOUNTAIN_LAKE_MID :
      case C_MOUNTAIN_LAKE_BOTTOM :
      {
         actions[ ACTION_GO ]( vcode, ncode, obj, wcode );
      }
      break;

      default :
      {
         PRINTS _I, _GO, _THROUGH, _THE, _MOTIONS, _BUT, _THERE, _IS, _NO,
                _EFFECT, _FULLSTOP, _NEWLINE END_PRINT
      }
      break;
   }
}

/* a_give, a_take, a_drink & a_eat work with containers. */
static void a_take( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   cell_t *c;
   int i;
   int n = -1;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      if( !( PROPERTY_MOBILE & obj->properties ))
      {
         /* this object is not mobile and cannot be taken */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         allowed = take_object( obj );

         if( _FALSE == allowed )
         {
            PRINTS _I, _CANNOT, _CARRY, _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _FULLSTOP, _TRY, _DROPPING, _SOMETHING, _FIRST, _FULLSTOP,
                   _NEWLINE END_PRINT
         }
      }
   }
   else
   if( wcode & A_MASK )
   {
      DEBUGF( "Testing for avatar\n" );
      n = wcode & ~A_MASK;

      /* apply verb <ncode> to avatar */
      if( CROW == adv_avatar[ n ].output.name )
      {
         PRINTS _THE, _CROW, _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS, _THEN,
                  _FLIES, _AWAY, _FULLSTOP, _NEWLINE END_PRINT
         /* temporarily store the crow in cell C_NULL */
         adv_avatar[ n ].location = C_NULL;
         /* the crow is restored to the treetop when the avatar is
          * hungry again */
      }
      else
      if( RAT == adv_avatar[ n ].output.name )
      {
         PRINTS _THE, _RAT, _EYES, _ME, _SUSPICIOUSLY, _ELIPSIS, _THEN,
                  _SCURRIES, _OUT, _OF, _SIGHT, _FULLSTOP, _NEWLINE END_PRINT
         /* temporarily store the rat in cell C_NULL */
         adv_avatar[ n ].location = C_NULL;
         /* the rat is restored to the dungeon when the avatar is
          * hungry again */
      }
   }
   else
   if( wcode )
   {
      /* test builtin */
      if( NAME_EVERYTHING == wcode )
      {
         c = &( adv_world.cells[ cavatar->location ]);

         /* foreach cell object */
         for( i=0; i < MAX_CELL_OBJECTS; i++ )
         {
            if(( c->objects[ i ].input.name )&&
               ( PROPERTY_MOBILE & c->objects[ i ].properties ))
            {
               if( _TRUE == take_object( &( c->objects[ i ])))
               {
                  allowed = _TRUE;
               }
               else
               {
                  PRINTS _I, _CANNOT, _CARRY, _THE END_PRINT
                  print_object_desc( &( c->objects[ i ].output ));
                  PRINTS _FULLSTOP, _TRY, _DROPPING, _SOMETHING, _FIRST, 
                         _FULLSTOP, _NEWLINE END_PRINT
               }
            }
         }
      }
   }
   else
   {
      switch( ncode )
      {
         case((( U32 )( 'u' )<< 8 )+( 'p'<< 0 )):
         {
            a_stand( vcode, ncode, obj, wcode );
         }
         break;

         case((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'w' )<< 8 )+( 'n'<< 0 )):
         {
            a_sit( vcode, ncode, obj, wcode );
         }
         break;

         case 0 :
         {
            DEBUGF( "Testing for builtin to avatar\n" );
            PRINTS _WHAT, _SHOULD, _I END_PRINT
            SPRINT_( outbuf, "%s", verb );
            PRINTS _QUESTION, _NEWLINE END_PRINT
         }
         break;

         default :
         {
            DEBUGF( "Testing for builtin to avatar\n" );
            PRINTS _I, _DONT, _KNOW, _HOW, _TO, _DO, _THAT, _FULLSTOP,
                   _NEWLINE END_PRINT
         }
         break;
      }
   }

   if( _TRUE == allowed )
   {
      /* print the new inventory */
      a_inventory( vcode, ncode, obj, wcode );
   }
}

static void a_turn( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   object_t *p = 0;
   TRISTATE selected = _FALSE;
   TRISTATE allowed = _FALSE;
   U32 acode;
   UC direction = NAME_NULL;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
      selected = _TRUE;
   }

   if( obj )
   {
         /* apply verb to object<noun> */
         /* search object directions and follow the first non-0 entry if any */
      if( _FALSE == selected )
      {
         acode = adv__encode_name( adverb );
         direction = find_noun_as_builtin( acode );
      }
      else
      {
         direction = wcode;
      }

      /* test special cases: wheel (laserbeam rack & pinion) */
      if(((( U32 )( 'w' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'e' )<< 0 ))== obj->input.name )
      {
         p = find_object_in_cell( 
                        ((( U32 )( 'r' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'c' )<< 8 )+(( U32 )( 'k' )<< 0 )), 
                        0, C_CASTLE_SPIRE_ROOF );

         if(( NAME_LEFT == direction )&&
            ( SOURCE_MOON == p->current_state.value ))
         {
            /* turn to the sun */
            p->current_state.value = SOURCE_SUN;
            allowed = _TRUE;
         }
         else
         if(( NAME_RIGHT == direction )&&
            ( SOURCE_SUN == p->current_state.value ))
         {
            /* turn to the moon */
            p->current_state.value = SOURCE_MOON;
            allowed = _TRUE;
         }

         if( _TRUE == allowed )
         {
            PRINTS _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _NEEDS, _OILING, _AND_, _IS, _DIFFICULT, _TO, _TURN, _COMMA,
                   _BUT, _THE, _RACK, _AND_, _PINION, _MOVE, _FULLSTOP,
                   _NEWLINE END_PRINT

            lightbeam_event( p );
         }
         else
         {
            PRINTS _I, _CANNOT, _TURN, _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _THAT, _WAY, _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      if(( SWITCH == obj->output.name )&&( ROTARY == obj->output.adjective ))
      {
         set_rotary_switch( cavatar->location, direction );

         print_rotary_switch_state( cavatar->location );
      }
      else
      if( HOIST == obj->output.name )
      {
         actions[ ACTION_SET ]( vcode, ncode, obj, wcode );
      }
      else
      {
         PRINTS _I, _CANNOT, _TURN, _THE END_PRINT
         print_object_desc( &obj->output);
         PRINTS _NEWLINE END_PRINT
      }
   }
   else
   {
      /* apply verb <ncode> to avatar */
      if( NAME_NULL < wcode )
      {
         /* apply builtin to avatar */
         if(( NAME_NORTH <= wcode )&&( NAME_UP > wcode ))
         {
            direction =( wcode - 1 ); /* -1 for hardcoded enum _name_t */
         }
         else
         if( NAME_LEFT == wcode )
         {
            direction = NAME_WEST - 1;
         }
         else
         if( NAME_RIGHT == wcode )
         {
            direction = NAME_EAST - 1;
         }

         if( NAME_NULL == direction )
         {
            PRINTS _I, _CANNOT, _TURN, _IN, _THAT, _DIRECTION, _FULLSTOP,
                   _NEWLINE END_PRINT
         }
         else
         {
            PRINTS _I, _TURN, _TO, _FACE, _THE END_PRINT
            SPRINT_( outbuf, "%s",
                  (( NAME_NORTH - 1 )== direction )
                  ? "north" :
                  (( NAME_EAST - 1 )== direction )
                  ? "east" :
                  (( NAME_SOUTH - 1 )== direction )
                  ? "south" :
                    "west" );
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }
   }
}

static void a_touch( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif

      /* If the object is in the cavatar inventory, or in the current cell,
       * then it can be touched. */
      if( 0 != find_object_with_avatar( obj->input.name, obj->input.adjective ))
      {
         allowed = _TRUE;
      }
      else
      if( find_object_in_cell(
               obj->input.name, obj->input.adjective, cavatar->location ))
      {
         allowed = _TRUE;
      }
      
      if( _TRUE == allowed )
      {
         PRINTS _I, _FEEL, _NOTHING, _SPECIAL, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

static void a_unlock( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   TRISTATE allowed = _FALSE;
   int n;  /* if object in inventory */
   object_t *p;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
         /* search object to see if verb is an allowed action */
      allowed = test_and_set_verb( obj, ACTION_UNLOCK, _FALSE );
      if( _TRUE == allowed )
      {
         /* specific post-cond actions for ACTION_LOCK */
         if(( STATE_LOCKED == obj->current_state.descriptor )&&
            ( 1 == obj->current_state.value ))
         {
            /* is locked, so can unlock */

            /* test for the castle tower door */
            if((((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 ))==
               obj->input.name )&&
               (((( U32 )( 't' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'w' )<< 8 )+(( U32 )( 'e' )<< 0 ))==
               obj->input.adjective)&&
               ( C_CASTLE_MAPROOM == cavatar->location ))
            {
               /* object is the castle tower door.
                * test if the avatar has the bronze key selected. */
               test_inventory_selected( cavatar, &n );
               if(( 0 <= n )&&
                  ( BRONZE == cavatar->inventory[ n ].output.adjective )&&
                  ( KEY == cavatar->inventory[ n ].output.name ))
               {
                  obj->current_state.value = 0;
                  PRINTS _THE END_PRINT
                  print_object_desc( &( obj->output ));
                  PRINTS _IS, _NOW, _UNLOCKED, _FULLSTOP, _NEWLINE END_PRINT

                  /* copy the new state to the "other side" */
                  p = find_object_in_cell( obj->input.adjective, 
                                           obj->input.name, C_CASTLE_TOWER );
                  p->current_state = obj->current_state;
               }
               else
               {
                  PRINTS _I, _CANNOT, _UNLOCK, _THE END_PRINT
                  print_object_desc( &( obj->output ));
                  PRINTS _YET, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
            else
            if((((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 ))==
               obj->input.name )&&
	       ((((( U32 )( 'k' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'p' )<< 0 ))==
                  obj->input.adjective)||
                (((( U32 )( 's' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'v' )<< 0 ))==
                  obj->input.adjective)))
            {
               /* object is the castle keep door.
                * test if the avatar has the silver key selected. */
               test_inventory_selected( cavatar, &n );
               if(( 0 <= n )&&
                  ( SILVER == cavatar->inventory[ n ].output.adjective )&&
                  ( KEY == cavatar->inventory[ n ].output.name ))
               {
                  obj->current_state.value = 0;
                  PRINTS _THE END_PRINT
                  print_object_desc( &( obj->output ));
                  PRINTS _IS, _NOW, _UNLOCKED, _FULLSTOP, _NEWLINE END_PRINT
               }
               else
               {
                  PRINTS _I, _CANNOT, _UNLOCK, _THE END_PRINT
                  print_object_desc( &( obj->output ));
                  PRINTS _YET, _FULLSTOP, _NEWLINE END_PRINT
               }
            }
         }
         else
         {
            PRINTS _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _IS, _UNLOCKED, _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      {
         /* action is not allowed on this object */
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT

         /* specific cases */
         if(( BRONZE == obj->output.adjective )&&
            ( DOOR == obj->output.name )&&
            ( C_CASTLE_TOWER == cavatar->location ))
         {
            /* the bronze door from the tower-side */
            PRINTS _IT, _APPEARS, _TO, _BE,_LOCKED, _FROM, _THE, _INSIDE,
                   _FULLSTOP, _NEWLINE END_PRINT
         }
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      PRINTS _WHAT, _SHOULD, _I END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _QUESTION, _NEWLINE END_PRINT
   }
}

/* If you want to achieve something that needs a tool, like unlocking a door
 * using a key, the avatar musy first 'hold' or or 'use' the key
 * before 'unlock door' can work.
 */
static void a_use( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   object_t *p;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif
      
      p = find_object_with_avatar( ncode, obj->input.adjective );

      if((( U32 )( 's' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'e' )<< 0 )== vcode )
      {
         /* select, you don't have to hold the object first, unlike use */
         /* first un-select any other object in the cell and inventory
          * (including if none is currently selected) */
         set_cell_selected( obj );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _IS, _SELECTED, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      if( 0 == p )
      {
         PRINTS _I, _CANNOT, _USE, _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _TRY, _TAKING, _IT, _FIRST, _FULLSTOP, _NEWLINE
                END_PRINT
      }
      else
      {
         /* un-select any other object (including if none is selected);
          * then select this object */
         set_inventory_selected( cavatar, p );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _IS, _READY, _FOR, _USE, _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
      DEBUGF( "apply verb 0x%lx to avatar %d\n", vcode, wcode );
      swap_avatar( wcode & ~A_MASK );
   }
   else
   {
      /* apply verb <ncode> to avatar */
      DEBUGF( "apply verb 0x%lx to avatar\n", vcode );

      if((( U32 )( 's' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'e' )<< 0 )== vcode )
      {
         /* treat command as 'select nothing' */
         set_cell_selected( 0 );
         PRINTS _NOTHING, _IS, _SELECTED, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         set_inventory_selected( cavatar, 0 );
         PRINTS _NOTHING, _IS, _READY, _FOR, _USE, _FULLSTOP, _NEWLINE 
                END_PRINT
      }
   }
}

static void a_wear( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
   object_t *p = 0;
   int n;
   int m = -1;


   DEBUGF( "Action 0x%lx for %s\n", vcode,( 0 == obj )? "avatar" :"object" );

   if(( 0 == obj )&&( 0 ==( A_MASK & wcode )))
   {
      find_selected( &obj );
   }

   if( obj )
   {
      /* apply verb to object<noun> */
      DEBUGF( "apply verb to object 0x%lx ", ncode );
#     if defined( DEBUG )
      if( __trace )
      {
         print_object_desc( &( obj->output ));
         PRINTS _NEWLINE END_PRINT
      }
#     endif

      if( PROPERTY_WEARABLE & obj->properties )
      {
         ( void )test_inventory( cavatar, ncode, obj->input.adjective, &n );
         if( 0 > n )
         {
            PRINTS _I, _CANNOT END_PRINT
            SPRINT_( outbuf, "%s", verb );
            PRINTS _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _FULLSTOP, _TRY, _TAKING, _IT, _FIRST, _FULLSTOP,
                     _NEWLINE END_PRINT
         }
         else
         {
            /* first un-wear any other object  */
            m=-1, test_inventory_worn( cavatar, &m );
            if( 0 <= m )
            {
               cavatar->inventory[ m ].properties &= ~PROPERTY_WORN;

               if( GOGGLES == cavatar->inventory[ m ].output.name )
               {
                  /* wilbury is no longer wearing the goggles,
                   * so hide mountain peak door */
                  p = find_object_in_cell(
                       ((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 )), 
                       0, C_MOUNTAIN_PEAK );
                  p->properties |= PROPERTY_HIDDEN;
               }
            }

            /* now wear the requested object */
            cavatar->inventory[ n ].properties |= PROPERTY_WORN;

            PRINTS _I, _AM, _NOW, _WEARING, _THE END_PRINT
            print_object_desc( &( obj->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT

            /* test specific cases */
            DEBUGF( "Test for wearing headband\n" );
            if( C_CASTLE_THRONEROOM == cavatar->location )
            {
               if(( HEADBAND == obj->output.name )&&
                  ( YELLOW == obj->output.adjective ))
               {
                  if( 0 == obj->current_state.value )
                  {
                     PRINTS _AN, _IMAGE, _FILLS, _MY, _MIND, _ELIPSIS, _BS,
                            _ESC_L, _AN, _AGED, _MAN, _SPEAKS, _COLON, 
                            _NEWLINE END_PRINT
                     PRINTS _AH, _HELLO, _THERE, _COMMA, _YOUNG END_PRINT
                     SPRINT_( outbuf, "%s", cavatar->name );
                     PRINTS _FULLSTOP, _GOOD, _TO, _SEE, _YOU, _FULLSTOP
                            END_PRINT
                     PRINTS _AS, _YOU, _NO, _DOUBT, _KNOW, _COMMA, _I,
                              _CONSTRUCTED, _LUCY, _COMMA, _MY, _LASER,
                              _UTILISING, _CHRONOMETRIC, _YELLOWING, _COMMA,
                              _TO, _COLOUR, _THE, _PASSAGE, _OF, _TIME, _COMMA,
                              _WHAT, _QUESTION END_PRINT
                     PRINTS _AS, _SHE, _TRAVELS, _THROUGH, _SPACE, 
                              _COMMA, _LUCY, _IS, _GOING, _FORWARDS, _IN, _TIME,
                              _COMMA, _YOU, _SEE, _FULLSTOP END_PRINT
                     PRINTS _IT, _WAS END_PRINT
                     SPRINT_( outbuf, "%s", adv_avatar[ A_CHARLIE ].name );
                     PRINTS _WHO, _HAD, _THE, _IDEA, _REALLY, _FULLSTOP 
                            END_PRINT
                     PRINTS _RIGHT, _COMMA, _SO, _WHEN, _SHE, _BOUNCES, _OFF,
                              _THE, _DARK, _SIDE, _OF, _THE, _MOON, _BACK, _TO,
                              _THE, _COLLECTOR, _DISH, _COMMA, 
                              _WE, _MEASURE, _THE, _COLOUR, _CHANGE, _TO, _SEE,
                              _HOW, _FAR, _INTO, _THE, _FUTURE, _LUCY, _HAS, 
                              _GONE, _FULLSTOP END_PRINT
                     PRINTS _IF, _WE, _GET, _IT, _RIGHT, _COMMA, _WE, _CAN,
                              _USE, _LUCY, _TO, _AFFECT, _FUTURE, _ENERGY,
                              _SUPPLY, _FULLSTOP END_PRINT
                     PRINTS _MARVELLOUS, _FULLSTOP, _ANYWAY, _MY,
                              _WORK, _IS, _NOT, _YET, _FINISHED, _COMMA, _BUT,
                              _THE, _SUPPLY, _OF, _ENERGY, _RUNS, _LOW,
                              _FULLSTOP END_PRINT
                     PRINTS _SO, _I, _MUST, _JOURNEY, _INTO, _THE, _MOUNTAINS,
                              _FOR, _A, _FRESH, _SOURCE, _FULLSTOP END_PRINT
                     PRINTS _I, _LEFT END_PRINT
                     SPRINT_( outbuf, "%s", adv_avatar[ A_NELLY ].name );
                     PRINTS _IN, _CHARGE, _COMMA, _HES, _AROUND, _SOMEPLACE,
                              _FULLSTOP, _DONT, _PRESS, _ANY, _BUTTONS, _COMMA,
                              _THERES, _A, _GOOD, _FELLOW, _FULLSTOP,
                              _NEWLINE END_PRINT
                  }

                  obj->current_state.value = 1;
               }
            }

            /* test if wilbury is wearing the snow goggles */
            DEBUGF( "Test for wearing goggles\n" );
            if( GOGGLES == obj->output.name )
            {
               /* wilbury is wearing the goggles */
               p = find_object_in_cell(
                    ((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 )), 0, 
                    C_MOUNTAIN_PEAK );
               p->properties &= ~PROPERTY_HIDDEN;
            }
         }
      }
      else
      {
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( wcode & A_MASK )
   {
   }
   else
   {
      /* apply verb <ncode> to avatar */
      DEBUGF( "apply verb 0x%lx to avatar\n", vcode );
   }
}

static void a_write( U32C vcode, U32C ncode, object_t * obj, UCC wcode )
{
}



/******* general private functions *******/

static void print_inventory( avatar_t const * const a )
{
   int i;
   int n = 0;
   int j;


   if( cavatar == a )
   {
      PRINTS _I, _AM, _CARRYING, _COLON END_PRINT
   }
   else
   {
      PRINTS _THE END_PRINT
      print_object_desc( &( a->output ));
      PRINTS _IS, _CARRYING, _COLON END_PRINT
   }

   for( i=0; i < MAX_AVATAR_OBJECTS; i++ )
   {
      if(( a->inventory[ i ].input.name )&&
         ( 0 ==( PROPERTY_WORN & a->inventory[ i ].properties )))
      {
         n++;

         if( PROPERTY_CONTAINER & a->inventory[ i ].properties )
         {
            j = a->inventory[ i ].current_state.descriptor;
            if( containers[ j ].output.name )
            {
               print_object_desc( &( a->inventory[ i ].output ));
               PRINTS _CONTAINING END_PRINT
               print_object_state( &( containers[ j ].current_state ));
               print_object_desc( &( containers[ j ].output ));
            }
            else
            {
               PRINTS _EMPTY END_PRINT
               print_object_desc( &( a->inventory[ i ].output ));
            }
         }
         else
         {
            print_object_state( &( a->inventory[ i ].current_state ));
            print_object_desc( &( a->inventory[ i ].output ));
         }

         PRINTS _COMMA END_PRINT
      }
   }
   if( 0 == n )
   {
      PRINTS _NOTHING END_PRINT
   }
   else
   {
      PRINTS _BS END_PRINT   /* undo last comma */
   }
   PRINTS _FULLSTOP, _NEWLINE END_PRINT
}

static void print_wearing( avatar_t * const a )
{
   int n = -1;
   int s = 0;


   if( cavatar == a )
   {
      PRINTS _I, _AM, _WEARING END_PRINT
   }
   else
   {
      PRINTS _THE END_PRINT
      print_object_desc( &( a->output ));
      PRINTS _IS, _WEARING END_PRINT
   }

   do
   {
      /* within while loop, do not re-initialise n in call to 
       * test_inventory_worn */
      test_inventory_worn( a, &n );
      if( 0 <= n )
      {
         print_object_desc( &( a->inventory[ n ].output ));
         PRINTS _COMMA END_PRINT
         s++;
      }
   }
   while( 0 <= n );
   if( 0 == s )
   {
      PRINTS _NOTHING END_PRINT
   }
   else
   {
      PRINTS _BS END_PRINT   /* undo last comma */
   }
   PRINTS _FULLSTOP, _NEWLINE END_PRINT
}

static void print_object_state( state_t const * const s )
{
   char const *p;
   int idx = s->descriptor & ~STATE_MASK__;


   /* objects may be fixed to a fixpoint */
   if( STATE_FIXED ==( STATE_FIXED & s->descriptor ))
   {
      PRINTS _FIXED END_PRINT
   }

   /* certain objects have a status that can be reported,
    * for example doors can be open or shut */
   switch( s->descriptor )
   {
      case STATE_OPEN :
      case STATE_LOCKED :
      case STATE_FULL :
      case STATE_ON :
      case STATE_LIGHT :
      {
         if( s->value )
         {
            p = state_descriptors[ idx ].val1_description;
         }
         else
         {
            p = state_descriptors[ idx ].val0_description;
         }
         SPRINT_( outbuf, "%s", p );
      }
      break;

      case STATE_NUMBER :
      {
      }
      break;

      case STATE_PERCENTAGE :
      {
         if( 0 == s->value )
         {
            PRINTS _EMPTY END_PRINT
         }
         else
         {
            SPRINT_( outbuf, "%d%%",(( int )( s->value * 100 )/ 255 ));
            PRINTS _FULL END_PRINT
         }
      }
      break;

      case STATE_SPECIAL :
      {
      }
      break;

      case CONTAINER_9 :
      {
         /* special case, CONTAINER_9 is ENERGY CAP
          * its state is STATE_OPEN */
         idx = STATE_OPEN & ~STATE_MASK__;
         if( s->value )
         {
            p = state_descriptors[ idx ].val1_description;
         }
         else
         {
            p = state_descriptors[ idx ].val0_description;
         }
         SPRINT_( outbuf, "%s", p );
      }
      break;

      case STATE_NULL :
      default:
      break;
   }
}

static void print_adjective( U32C adj )
{
   if((( U32 )( 1 )<<31 )& adj )
   {
      /* print object adjective */
      if( BLACK ==( BLACK & adj )){ PRINTS _BLACK END_PRINT }
      if( BROWN ==( BROWN & adj )){ PRINTS _BROWN END_PRINT }
      if( RED ==( RED & adj )){ PRINTS _RED END_PRINT }
      if( ORANGE ==( ORANGE & adj )){ PRINTS _ORANGE END_PRINT }
      if( YELLOW ==( YELLOW & adj )){ PRINTS _YELLOW END_PRINT }
      if( GREEN ==( GREEN & adj )){ PRINTS _GREEN END_PRINT }
      if( BLUE ==( BLUE & adj )){ PRINTS _BLUE END_PRINT }
      if( VIOLET ==( VIOLET & adj )){ PRINTS _VIOLET END_PRINT }
      if( GREY ==( GREY & adj )){ PRINTS _GREY END_PRINT }
      if( WHITE ==( WHITE & adj )){ PRINTS _WHITE END_PRINT }
      if( GOLD ==( GOLD & adj )){ PRINTS _GOLD END_PRINT }
      if( SILVER ==( SILVER & adj )){ PRINTS _SILVER END_PRINT }
      if( BRONZE ==( BRONZE & adj )){ PRINTS _BRONZE END_PRINT }
      if( DIAMOND ==( DIAMOND & adj )){ PRINTS _DIAMOND END_PRINT }
      if( GLASS ==( GLASS & adj )){ PRINTS _GLASS END_PRINT }
      if( METAL ==( METAL & adj )){ PRINTS _METAL END_PRINT }
      if( CERAMIC ==( CERAMIC & adj )){ PRINTS _CERAMIC END_PRINT }
      if( PINK ==( PINK & adj )){ PRINTS _PINK END_PRINT }
      if( BLONDE ==( BLONDE & adj )){ PRINTS _BLONDE END_PRINT }
      if( DARK ==( DARK & adj )){ PRINTS _DARK END_PRINT }
      if( LEAD ==( LEAD & adj )){ PRINTS _LEAD END_PRINT }
      if( CONTROL ==( CONTROL & adj )){ PRINTS _CONTROL END_PRINT }
   }

   if((( U32 )( 1 )<<30 )& adj )
   {
      if( DEEP ==( DEEP & adj )){ PRINTS _DEEP END_PRINT }
      if( SHALLOW ==( SHALLOW & adj )){ PRINTS _SHALLOW END_PRINT }
      if( TALL ==( TALL & adj )){ PRINTS _TALL END_PRINT }
      if( SHORT ==( SHORT & adj )){ PRINTS _SHORT END_PRINT }
      if( LARGE ==( LARGE & adj )){ PRINTS _LARGE END_PRINT }
      if( SMALL ==( SMALL & adj )){ PRINTS _SMALL END_PRINT }
      if( WARM ==( WARM & adj )){ PRINTS _WARM END_PRINT }
      if( COLD ==( COLD & adj )){ PRINTS _COLD END_PRINT }
      if( BARRED ==( BARRED & adj )){ PRINTS _BARRED END_PRINT }
      if( BROKEN ==( BROKEN & adj )){ PRINTS _BROKEN END_PRINT }
      if( FAST ==( FAST & adj )){ PRINTS _FAST END_PRINT }
      if( HEAVY ==( HEAVY & adj )){ PRINTS _HEAVY END_PRINT }
      if( STRAW ==( STRAW & adj )){ PRINTS _STRAW END_PRINT }
      if( HARD ==( HARD & adj )){ PRINTS _HARD END_PRINT }
      if( OAK ==( OAK & adj )){ PRINTS _OAK END_PRINT }
      if( BEECH ==( BEECH & adj )){ PRINTS _BEECH END_PRINT }
      if( BIRCH ==( BIRCH & adj )){ PRINTS _BIRCH END_PRINT }
      if( ASH ==( ASH & adj )){ PRINTS _ASH END_PRINT }
      if( WORN ==( WORN & adj )){ PRINTS _WORN END_PRINT }
      if( OLD ==( OLD & adj )){ PRINTS _OLD END_PRINT }
   }

   if((( U32 )( 1 )<<29 )& adj )
   {
      if( LONG_HAIRED ==( LONG_HAIRED & adj )){ PRINTS _LONG_HAIRED END_PRINT }
      if( LAB ==( LAB & adj )){ PRINTS _LAB END_PRINT }
      if( ENERGY ==( ENERGY & adj )){ PRINTS _ENERGY END_PRINT }
      if( COLLECTOR ==( COLLECTOR & adj )){ PRINTS _COLLECTOR END_PRINT }
      if( DINING ==( DINING & adj )){ PRINTS _DINING END_PRINT }
      if( SHEET ==( SHEET & adj )){ PRINTS _SHEET END_PRINT }
      if( BAKING ==( BAKING & adj )){ PRINTS _BAKING END_PRINT }
      if( ELECTRIC ==( ELECTRIC & adj )){ PRINTS _ELECTRIC END_PRINT }
      if( ROTARY ==( ROTARY & adj )){ PRINTS _ROTARY END_PRINT }
      if( WELLINGTON ==( WELLINGTON & adj )){ PRINTS _WELLINGTON END_PRINT }
      if( REFLECTIVE ==( REFLECTIVE & adj )){ PRINTS _REFLECTIVE END_PRINT }
      if( SAFETY ==( SAFETY & adj )){ PRINTS _SAFETY END_PRINT }
      if( MINERS ==( MINERS & adj )){ PRINTS _MINERS END_PRINT }
      if( CRIB ==( CRIB & adj )){ PRINTS _CRIB END_PRINT }
   }
}

static void print_object_desc( description_t const * const d )
{
   if( d->adjective )
   {
      print_adjective( d->adjective );
   }

   /* print object name */
   switch( d->name )
   {
      case TREE :{ PRINTS _TREE END_PRINT } break;
      case MUSHROOM :{ PRINTS _MUSHROOMS END_PRINT } break;
      case FISH :{ PRINTS _FISH END_PRINT } break;
      case WATER :{ PRINTS _WATER END_PRINT } break;
      case FLASK :{ PRINTS _FLASK END_PRINT } break;
      case BOOK :{ PRINTS _BOOK END_PRINT } break;
      case KEY :{ PRINTS _KEY END_PRINT } break;
      case CLOAK :{ PRINTS _CLOAK END_PRINT } break;
      case JAR :{ PRINTS _JAR END_PRINT } break;
      case NEST :{ PRINTS _NEST END_PRINT } break;
      case EGGS :{ PRINTS _EGGS END_PRINT } break;
      case CROW :{ PRINTS _CROW END_PRINT } break;
      case MONUMENT :{ PRINTS _MONUMENT END_PRINT } break;
      case SPECTACLES :{ PRINTS _SPECTACLES END_PRINT } break;
      case PORTCULLIS :{ PRINTS _PORTCULLIS END_PRINT } break;
      case BROTH :{ PRINTS _BROTH END_PRINT } break;
      case BREAD :{ PRINTS _BREAD END_PRINT } break;
      case SIGN :{ PRINTS _SIGN END_PRINT } break;
      case DOOR :{ PRINTS _DOOR END_PRINT } break;
      case WINDOW :{ PRINTS _WINDOW END_PRINT } break;
      case VEGETATION :{ PRINTS _VEGETATION END_PRINT } break;
      case O_STRAW :{ PRINTS _STRAW END_PRINT } break;
      case RAT :{ PRINTS _RAT END_PRINT } break;
      case LIGHTBEAM :{ PRINTS _LIGHTBEAM END_PRINT } break;
      case SWITCH :{ PRINTS _SWITCH END_PRINT } break;
      case CHAIN :{ PRINTS _CHAIN END_PRINT } break;
      case O_MAP :{ PRINTS _MAP END_PRINT } break;
      case O_THRONE :{ PRINTS _THRONE END_PRINT } break;
      case HEADBAND :{ PRINTS _HEAD, _MINUS, _BAND END_PRINT } break;
      case GNOME :{ PRINTS _GNOME END_PRINT } break;
      case SNOW :{ PRINTS _SNOW END_PRINT } break;
      case GOGGLES :{ PRINTS _GOGGLES END_PRINT } break;
      case TELESCOPE :{ PRINTS _TELESCOPE END_PRINT } break;
      case SEAT :{ PRINTS _SEAT END_PRINT } break;
      case JOYSTICK :{ PRINTS _JOYSTICK END_PRINT } break;
      case BUTTON :{ PRINTS _BUTTON END_PRINT } break;
      case O_FUNICULAR :{ PRINTS _FUNICULAR END_PRINT } break;
      case O_MOUNTAIN :{ PRINTS _MOUNTAIN END_PRINT } break;
      case LEVER :{ PRINTS _LEVER END_PRINT } break;
      case MONKEY :{ PRINTS _MONKEY END_PRINT } break;
      case WIZARD :{ PRINTS _WIZARD END_PRINT } break;
      case BURET :{ PRINTS _BURET END_PRINT } break;
      case BURNER :{ PRINTS _BURNER END_PRINT } break;
      case TESTTUBE :{ PRINTS _TEST, _TUBE END_PRINT } break;
      case CRUCIBLE :{ PRINTS _CRUCIBLE END_PRINT } break;
      case STAND :{ PRINTS _STAND END_PRINT } break;
      case SOURCE :{ PRINTS _SOURCE END_PRINT } break;
      case HOIST :{ PRINTS _HOIST END_PRINT } break;
      case SHAFT_HEAD :{ PRINTS _SHAFT, _HEAD END_PRINT } break;
      case O_CAGE :{ PRINTS _CAGE END_PRINT } break;
      case BAT :{ PRINTS _BAT END_PRINT } break;
      case DIVINGSUIT :{ PRINTS _DIVINGSUIT END_PRINT } break;
      case CAP :{ PRINTS _CAP END_PRINT } break;
      case HAT :{ PRINTS _HAT END_PRINT } break;
      case SHADES :{ PRINTS _SHADES END_PRINT } break;
      case DISH :{ PRINTS _DISH END_PRINT } break;
      case O_CASTLE :{ PRINTS _CASTLE END_PRINT } break;
      case O_RING :{ PRINTS _RING END_PRINT } break;
      case TABLE :{ PRINTS _TABLE END_PRINT } break;
      case CHAIRS :{ PRINTS _CHAIRS END_PRINT } break;
      case PICK :{ PRINTS _PICK END_PRINT } break;
      case SHOVEL :{ PRINTS _SHOVEL END_PRINT } break;
      case SCREEN :{ PRINTS _SCREEN END_PRINT } break;
      case PAINTINGS :{ PRINTS _PAINTINGS END_PRINT } break;
      case REMOTE :{ PRINTS _REMOTE END_PRINT } break;
      case PANEL :{ PRINTS _PANEL END_PRINT } break;
      case PAPER :{ PRINTS _PAPER END_PRINT } break;
      case WILBURY :{ PRINTS _WILBURY END_PRINT } break;
      case SAND :{ PRINTS _SAND END_PRINT } break;
      case MIRROR :{ PRINTS _MIRROR END_PRINT } break;
      case POT :{ PRINTS _POT END_PRINT } break;
      case TRAY :{ PRINTS _TRAY END_PRINT } break;
      case LADDER :{ PRINTS _LADDER END_PRINT } break;
      case BRACKET :{ PRINTS _BRACKET END_PRINT } break;
      case LENS :{ PRINTS _LENS END_PRINT } break;
      case FILTER :{ PRINTS _FILTER END_PRINT } break;
      case SUN :{ PRINTS _SUN END_PRINT } break;
      case MOON :{ PRINTS _MOON END_PRINT } break;
      case DUST :{ PRINTS _DUST END_PRINT } break;
      case O_MAGNOTRON :{ PRINTS _MAGNOTRON END_PRINT } break;
      case TRAVELATOR :{ PRINTS _TRAVELATOR END_PRINT } break;
      case BOARD :{ PRINTS _BOARD END_PRINT } break;
      case MAGNET :{ PRINTS _MAGNET END_PRINT } break;
      case CART :{ PRINTS _CART END_PRINT } break;
      case CAMP_FIRE :{ PRINTS _CAMPFIRE END_PRINT } break;
      case SLIDER_SWITCH :{ PRINTS _SLIDER, _SWITCH END_PRINT } break;
      case BOOTS :{ PRINTS _BOOTS END_PRINT } break;
      case JACKET :{ PRINTS _JACKET END_PRINT } break;
      case MOLE :{ PRINTS _MOLE END_PRINT } break;
      case LAMP :{ PRINTS _LAMP END_PRINT } break;
      case TAP :{ PRINTS _TAP END_PRINT } break;
      case SINK :{ PRINTS _SINK END_PRINT } break;
      case METER :{ PRINTS _METER END_PRINT } break;
      case RACK :{ PRINTS _RACK END_PRINT } break;
      case PINION :{ PRINTS _PINION END_PRINT } break;
      case WHEEL :{ PRINTS _WHEEL END_PRINT } break;
      case NOTE :{ PRINTS _NOTE END_PRINT } break;
      case TRAIL :{ PRINTS _TRAIL END_PRINT } break;
      case PLOUGH :{ PRINTS _PLOUGH END_PRINT } break;

      default :{ PRINTS _UNKNOWN, _OBJECT END_PRINT } break;
   }
}

static void print_cell_objects( UC const cn )
{
   int i;
   object_t const *p = 0;
   int n = 0;
   int h = 0;
   int j;


   /* get the number of visible objects */
   for( i=0; i < MAX_CELL_OBJECTS; i++ )
   {
      p = &( adv_world.cells[ cn ].objects[ i ]);
      if( p->output.name )
      {
         if( 0 ==( p->properties & PROPERTY_HIDDEN ))
         {
            n++;
         }
         else
         {
            h++;
         }
      }
   }

   if( n )
   {
      PRINTS _I, _SEE END_PRINT
      for( i=0;( i < MAX_CELL_OBJECTS )&&( n ); i++ )
      {
         p = &( adv_world.cells[ cn ].objects[ i ]);
         if(( p->output.name )&&( 0 ==( p->properties & PROPERTY_HIDDEN )))
         {
            if( PROPERTY_CONTAINER & p->properties )
            {
               j = p->current_state.descriptor;
               if(( containers[ j ].output.name )&&
                  ( 0 ==( containers[ j ].properties & PROPERTY_HIDDEN )))
               {
                  print_object_state( &( containers[ j ].current_state ));
                  print_object_desc( &( containers[ j ].output ));
                  PRINTS _IN END_PRINT
               }
            }

            print_object_state( &( p->current_state ));
            print_object_desc( &( p->output ));

            n--;
            if( n ){ PRINTS _COMMA END_PRINT }
         }
      }
      PRINTS _HERE, _FULLSTOP, _NEWLINE END_PRINT
   }

   if( h )
   {
      PRINTS _I, _SENSE, _SOMETHING, _ELSE, _IS, _HERE, _COMMA, _BUT, _I,
               _CANNOT, _SEE, _IT, _FULLSTOP, _NEWLINE END_PRINT
   }
}

static void print_cell_desc( description_t const * const d )
{
   if( d->adjective )
   {
      print_adjective( d->adjective );
   }

   if( d->name &(( U32 )( 1 )<<31 ))
   {
      /* forest landscapes */
      if( FOREST ==( FOREST & d->name )){ PRINTS _FOREST END_PRINT }
      if( POOL ==( POOL & d->name )){ PRINTS _POOL END_PRINT }
      if( FLOOR ==( FLOOR & d->name )){ PRINTS _FLOOR END_PRINT }
      if( GLADE ==( GLADE & d->name )){ PRINTS _GLADE END_PRINT }
      if( CANOPY ==( CANOPY & d->name )){ PRINTS _CANOPY END_PRINT }
      if( SKY ==( SKY & d->name )){ PRINTS _SKY END_PRINT }
      if( EDGE ==( EDGE & d->name )){ PRINTS _EDGE END_PRINT }
      if( HILLTOP ==( HILLTOP & d->name )){ PRINTS _HILLTOP END_PRINT }
      if( HILLTOP_DISH ==( HILLTOP_DISH & d->name ))
         { PRINTS _RECEIVER, _DISH END_PRINT }
   }

   if( d->name &(( U32 )( 1 )<<30 ))
   {
      /* castle landscapes */
      if( CASTLE ==( CASTLE & d->name )) PRINTS _CASTLE END_PRINT
      if( DRAWBRIDGE ==( DRAWBRIDGE & d->name )) PRINTS _DRAWBRIDGE END_PRINT
      if( MOAT ==( MOAT & d->name )) PRINTS _MOAT END_PRINT
      if( GUARD ==( GUARD & d->name )) PRINTS _GUARD END_PRINT
      if( COMMON ==( COMMON & d->name )) PRINTS _COMMON END_PRINT
      if( C_MAP ==( C_MAP & d->name )) PRINTS _MAP END_PRINT
      if( C_THRONE ==( C_THRONE & d->name )) PRINTS _THRONE END_PRINT
      if( C_CONTROL ==( C_CONTROL & d->name )) PRINTS _CONTROL END_PRINT
      if( ROOM ==( ROOM & d->name )) PRINTS _ROOM END_PRINT
      if( STAIRWAY ==( STAIRWAY & d->name )) PRINTS _STAIRWAY END_PRINT
      if( DUNGEON ==( DUNGEON & d->name )) PRINTS _DUNGEON END_PRINT
      if( COURTYARD ==( COURTYARD & d->name )) PRINTS _COURTYARD END_PRINT
      if( KEEP ==( KEEP & d->name )) PRINTS _KEEP END_PRINT
      if( TOWER ==( TOWER & d->name )) PRINTS _TOWER END_PRINT
      if( SPIRE ==( SPIRE & d->name )) PRINTS _SPIRE END_PRINT
      if( ROOF ==( ROOF & d->name )) PRINTS _ROOF END_PRINT
      if( KITCHEN ==( KITCHEN & d->name )) PRINTS _KITCHEN END_PRINT
      if( CELLAR ==( CELLAR & d->name )) PRINTS _CELLAR END_PRINT
   }

   if( d->name &(( U32 )( 1 )<<29 ))
   {
      /* mountain landscapes */
      if( MOUNTAIN ==( MOUNTAIN & d->name )) PRINTS _MOUNTAIN END_PRINT
      if( PASS ==( PASS & d->name )) PRINTS _PASS END_PRINT
      if( PEAK ==( PEAK & d->name )) PRINTS _PEAK END_PRINT
      if( OBSERVATORY ==( OBSERVATORY & d->name ))
         PRINTS _OBSERVATORY END_PRINT
      if( DOME ==( DOME & d->name )) PRINTS _DOME END_PRINT
      if( STOREROOM ==( STOREROOM & d->name ))
         PRINTS _STORE, _ROOM END_PRINT
      if( FUNICULAR ==( FUNICULAR & d->name )) PRINTS _FUNICULAR END_PRINT
      if( CAVERN ==( CAVERN & d->name )) PRINTS _CAVERN END_PRINT
      if( LABORATORY ==( LABORATORY & d->name ))
         PRINTS _LABORATORY END_PRINT
      if( LAB_STORES ==( LAB_STORES & d->name ))
         PRINTS _LABORATORY, _STORES END_PRINT
      if( MANUFACTORY ==( MANUFACTORY & d->name )) PRINTS _FACTORY END_PRINT
      if( LAKE ==( LAKE & d->name )) PRINTS _LAKE END_PRINT
      if( BOTTOM ==( BOTTOM & d->name )) PRINTS _BOTTOM END_PRINT
      
      if( MINE ==( MINE & d->name )) PRINTS _MINE END_PRINT
      if( HEAD ==( HEAD & d->name )) PRINTS _HEAD END_PRINT
      if( CAGE ==( CAGE & d->name )) PRINTS _CAGE END_PRINT
      if( SHAFT ==( SHAFT & d->name )) PRINTS _SHAFT END_PRINT
      if( ENTRANCE ==( ENTRANCE & d->name )) PRINTS _ENTRANCE END_PRINT
      if( FACE ==( FACE & d->name )) PRINTS _FACE END_PRINT
      if( OBSERVATION ==( OBSERVATION & d->name )) PRINTS _OBSERVATION END_PRINT
      if( DECK ==( DECK & d->name )) PRINTS _DECK END_PRINT

      if( TUNNEL ==( TUNNEL & d->name )) PRINTS _TUNNEL END_PRINT
   }

   if( d->name &(( U32 )( 1 )<<28 ))
   {
      if( MAGNOTRON ==( MAGNOTRON & d->name )){ PRINTS _MAGNOTRON END_PRINT }
      if( COMPLEX ==( COMPLEX & d->name )){ PRINTS _COMPLEX END_PRINT }
      if( OPERATION ==( OPERATION & d->name )){ PRINTS _OPERATION END_PRINT }
      if( ELEMENT_0 ==( ELEMENT_0 & d->name ))
         { PRINTS _ELEMENT, _BS, _0 END_PRINT }
      if( ELEMENT_1 ==( ELEMENT_1 & d->name ))
         { PRINTS _ELEMENT, _BS, _1 END_PRINT }
      if( ELEMENT_2 ==( ELEMENT_2 & d->name ))
         { PRINTS _ELEMENT, _BS, _2 END_PRINT }
      if( ELEMENT_3 ==( ELEMENT_3 & d->name ))
         { PRINTS _ELEMENT, _BS, _3 END_PRINT }
      if( ELEMENT_4 ==( ELEMENT_4 & d->name ))
         { PRINTS _ELEMENT, _BS, _4 END_PRINT }
      if( ELEMENT_5 ==( ELEMENT_5 & d->name ))
         { PRINTS _ELEMENT, _BS, _5 END_PRINT }
      if( ELEMENT_6 ==( ELEMENT_6 & d->name ))
         { PRINTS _ELEMENT, _BS, _6 END_PRINT }
   }
}

static UC print_cell_avatars( UC const cn, TRISTATE const hello )
{
   UC num = 0;
   int i;


   DEBUGF( "print_cell_avatars %d\n", cn );

   for( i = A_WILBURY; i <= A_CHARLIE; i++ )
   {
      if(( cn == adv_avatar[ i ].location )&&( &adv_avatar[ i ]!= cavatar ))
      {
         num++;

         if( _TRUE == hello )
         {
            PRINTS _THE END_PRINT
            print_object_desc( &adv_avatar[ i ].output );
            PRINTS _REPLIES, _COMMA, _QUOTE_LEFT, _HELLO, _COMMA, _MY, _NAME,
                     _IS END_PRINT
            SPRINT_( outbuf, "%s", adv_avatar[ i ].name );
            PRINTS _QUOTE_RIGHT, _FULLSTOP, _NEWLINE END_PRINT

            if( A_CHARLIE == i )
            {
               PRINTS _IF, _YOU, _CAN, _SEE, _A, _DIAMOND, _LIGHTBEAM, _IN,
                        _THE, _SKY, _COMMA, _THEN, _THERE, _IS, _STILL, _A,
                        _POWER, _SOURCE, _AND_, _HOPE, _FULLSTOP END_PRINT
               PRINTS _MAKE, _SURE, _TO, _FOCUS, _THE, _LOOKING, _GLASS,
                        _COMMA, _SO, _THE, _BEAM, _REFLECTS, _ONTO, _THE,
                        _COLLECTOR, _DISH, _FULLSTOP END_PRINT
               PRINTS _OH, _COMMA, _AND_, _BEWARE END_PRINT
               SPRINT_( outbuf, "%s", adv_avatar[ A_LOFTY ].name );
               PRINTS _THE END_PRINT
               print_object_desc( &adv_avatar[ A_LOFTY ].output);
               PRINTS _COMMA, _HE,_SEEKS, _A, _DIFFERENT, _PATH, _FULLSTOP,
                        _NEWLINE END_PRINT
            }
            else
            if( A_LOFTY == i )
            {
               PRINTS _IVE, _GOT, _AN, _IDEA, _ON, _THE, _LIGHTBEAM, _COMMA,
                        _AND_, _IT, _GOES, _LIKE, _THIS, _COLON, _DECREASE,
                        _THE, _YELLOW, _AND_, _INCREASE, _THE, _BLUE,
                        _FULLSTOP END_PRINT
               PRINTS _SO, _COMMA, _YOU, _MUST, _REVERSE, _THE, 
                        _ORDER, _OF, _SWITCHES, _WHEN, _ENGAGING, _THE, _BEAM,
                        _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         else
         if( A_HARRY == i )
         {
            /* let wilbury know bat is in the room */
            PRINTS _I, _HEAR, _SOME, _FLUTTERING, _ELIPSIS, 
                   _ESC_L, _IT, _SOUNDS, _LIKE, _A END_PRINT
            print_object_desc( &adv_avatar[ i ].output );
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         {
            PRINTS _I, _SEE, _A END_PRINT
            print_object_desc( &adv_avatar[ i ].output );
            PRINTS _HERE, _FULLSTOP, _NEWLINE END_PRINT
         }
      }
   }

   return( num );
}

static void lightbeam_event( object_t * const obj )
{
   static int prev_lightbeam = L_DISENGAGE;
   object_t *yellow;
   object_t *blue;
   object_t *lbeam;
   object_t *dish;
   object_t *rack;
   object_t *energy;
   int can;
   object_t *dish_mirror;
   object_t *spire_mirror;
   int n;
   TRISTATE change = _FALSE;


   /* we have just changed the state of obj [yellow] or [blue],
    * so check the combination/order of button presses */
   /* disengage[diamond]: blue -> yellow -> open;
    * disengage[gold]: yellow -> blue -> open;
    * engage[charlie/diamond]: close -> yellow -> blue 
    * engage[roy/gold]: close -> blue -> yellow */

   lbeam = find_object_in_cell(
                        (( U32 )( 'l' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'h' )<< 0 ),
                        0, C_CASTLE_SPIRE_ROOF );
   dish = find_object_in_cell( 
                        ((( U32 )( 'd' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 'h' )<< 0 )), 
                        0, C_HILLTOP );
   rack = find_object_in_cell( 
                        ((( U32 )( 'r' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'c' )<< 8 )+(( U32 )( 'k' )<< 0 )), 
                        0, C_CASTLE_SPIRE_ROOF );

   /* find the test-tube stored in the energy cap */
   energy = find_object_in_cell((( U32 )( 'c' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'p' )<< 0 ), 0, 
                                C_CASTLE_SPIRE );
   can = energy->current_state.descriptor;
   energy = &containers[ can ];

   if(( TESTTUBE != energy->output.name )||
      ( 0 == energy->current_state.value ))
   {
         /* run out of petrol - need new energy and a system reset */
      lbeam->current_state.value = L_DISENGAGE;
   }
   else
   if(( DISH_MOON != dish->current_state.value )&&
      ( DISH_SUN != dish->current_state.value ))
   {
      if( L_DISENGAGE == prev_lightbeam )
      {
         /* remember the previous state so it can be re-instated once the
          * dish points at a source (sun/moon).
          * Otherwise, the avatar would have to close/re-open the blue &
          * yellow switches to re-instate the lightbeam state. */
         prev_lightbeam = lbeam->current_state.value;
         change = _TRUE;
      }

      lbeam->current_state.value = L_DISENGAGE;
   }
   else
   {
      yellow = find_object_in_cell(
                        (( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 ),
                        (( U32 )( 'y' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 ),
                        C_CASTLE_SPIRE );
      blue = find_object_in_cell(
                        (( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 ),
                        (( U32 )( 'b' )<< 24 )+(( U32 )( 'l' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'e' )<< 0 ),
                        C_CASTLE_SPIRE );
      dish_mirror = find_object_in_cell( 
                        ((( U32 )( 'm' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'r' )<< 0 )), 
                        0, C_HILLTOP_DISH );
      spire_mirror = find_object_in_cell( 
                        ((( U32 )( 'm' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'r' )<< 0 )), 
                        0, C_CASTLE_SPIRE_ROOF );
   }

   switch( lbeam->current_state.value )
   {
      case L_ENGAGE_DIAMOND :
      {
         if(( DISH_MOON == dish->current_state.value )&&
            ( SOURCE_MOON == rack->current_state.value )&&
            (( obj == dish_mirror )||( obj == spire_mirror )))
         {
            if((( dish_mirror )&&
                ( NAME_AROUND == dish_mirror->current_state.value ))&&
               (( spire_mirror )&&
                ( NAME_AROUND == spire_mirror->current_state.value )))
            {
               lbeam->current_state.value = L_TRIANGLE_DIAMOND;
               change = _TRUE;
            }
         }
         else
         if(( DISH_SUN == dish->current_state.value )&&
            ( SOURCE_SUN == rack->current_state.value ))
         {
            lbeam->current_state.value = L_BLINDING_DIAMOND;
            /* if there is a filter on the telescope, all is well,
             * otherwise game over */
            change = _TRUE;
         }
      }
      /* break; */
      case L_TRIANGLE_DIAMOND :
      {
         if( obj == blue )
         {
            lbeam->current_state.value = L_STANDBY_DIAMOND;
            change = _TRUE;
         }
         else
         if(( obj == rack )&&( SOURCE_MOON != rack->current_state.value ))
         {
            lbeam->current_state.value = L_STANDBY_DIAMOND;
            change = _TRUE;
         }
      }
      break;

      case L_ENGAGE_GOLD :
      {
         if(( DISH_MOON == dish->current_state.value )&&
            ( SOURCE_MOON == rack->current_state.value )&&
            (( obj == dish_mirror )||( obj == spire_mirror )))
         {
            if((( dish_mirror )&&
                ( NAME_AROUND == dish_mirror->current_state.value ))&&
               (( spire_mirror )&&
                ( NAME_AROUND == spire_mirror->current_state.value )))
            {
               lbeam->current_state.value = L_TRIANGLE_GOLD;
               change = _TRUE;
            }
         }
         else
         if(( DISH_SUN == dish->current_state.value )&&
            ( SOURCE_SUN == rack->current_state.value ))
         {
            lbeam->current_state.value = L_BLINDING_GOLD;
            /* if there is a filter on the telescope, all is well,
             * otherwise game over */
            change = _TRUE;
         }
      }
      /* break; */
      case L_TRIANGLE_GOLD :
      {
         if( obj == yellow )
         {
            lbeam->current_state.value = L_STANDBY_GOLD;
            change = _TRUE;
         }
         else
         if(( obj == rack )&&( SOURCE_MOON != rack->current_state.value ))
         {
            lbeam->current_state.value = L_STANDBY_DIAMOND;
            change = _TRUE;
         }
      }
      break;

      case L_STANDBY_DIAMOND :
      {
         if( obj == yellow )
         {
            lbeam->current_state.value = L_DISENGAGE;
            change = _TRUE;
         }
      }
      break;

      case L_STANDBY_GOLD :
      {
         if( obj == blue )
         {
            lbeam->current_state.value = L_DISENGAGE;
            change = _TRUE;
         }
      }
      break;

      case L_DISENGAGE :
      {
         if( obj == yellow )
         {
            lbeam->current_state.value = L_READY_DIAMOND;
            change = _TRUE;
         }
         else
         if( obj == blue )
         {
            lbeam->current_state.value = L_READY_GOLD;
            change = _TRUE;
         }
         else
         if( obj == dish )
         {
            if(( DISH_MOON == dish->current_state.value )||
               ( DISH_SUN == dish->current_state.value ))
            {
               /* the dish is moved (by observatory rotary/joystick) and now
                * points at the sun or the moon. So re-instate the previous
                * lbeam->current_state.value */
               lbeam->current_state.value = prev_lightbeam;
               prev_lightbeam = L_DISENGAGE;
               change = _TRUE;

               /* check if there is a new state */
               if(( DISH_SUN == dish->current_state.value )&&
                  ( SOURCE_SUN == rack->current_state.value ))
               {
                  if( L_ENGAGE_GOLD == lbeam->current_state.value )
                  {
                     lbeam->current_state.value = L_BLINDING_GOLD;
                  }
                  else
                  if( L_ENGAGE_DIAMOND == lbeam->current_state.value )
                  {
                     lbeam->current_state.value = L_BLINDING_DIAMOND;
                  }
               }
            }
         }
      }
      break;

      case L_READY_DIAMOND :
      {
         if( obj == blue )
         {
            lbeam->current_state.value = L_ENGAGE_DIAMOND;
            change = _TRUE;

            /* Lofty should go hide & sulk; he didn't get his gold beam */
            if( C_CASTLE_SPIRE == adv_avatar[ A_LOFTY ].location )
            {
               SPRINT_( outbuf, "%s", adv_avatar[ A_LOFTY ].name );
               PRINTS _THE END_PRINT
               print_object_desc( &adv_avatar[ A_LOFTY ].output);
               PRINTS _DISAPPEARS, _WITH, _A, _FLOURISH, _FULLSTOP, 
                      _NEWLINE END_PRINT
               adv_avatar[ A_LOFTY ].location = C_CASTLE_CELLAR;
            }

            /* nelson can now locate wilbury and give the remote control */
            ( void )test_inventory( &adv_avatar[ A_NELLY ], 
                     ((( U32 )( 'r' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'm' )<< 8 )+(( U32 )( 'o' )<< 0 )), 0, &n );
            if( 0 <= n )
            {
               adv_avatar[ A_NELLY ].inventory[ n ].current_state.value = 1;
            }
            ( void )test_inventory( &adv_avatar[ A_NELLY ], 
                     ((( U32 )( 'p' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'p' )<< 8 )+(( U32 )( 'e' )<< 0 )), 
                     ((( U32 )( 's' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'e' )<< 0 )), &n );
            if(( 0 <= n )&&
               ( 0 == adv_avatar[ A_NELLY ].inventory[ n ].current_state.value ))
            {
               adv_avatar[ A_NELLY ].current_state.descriptor = 1;
               /* wait for wilbury to come out of the spire */
               adv_avatar[ A_NELLY ].location = C_CASTLE_COURTYARD;
            }

            /* check if the state needs to be upgraded, due to sequence of
             * events. */
            if(( DISH_MOON == dish->current_state.value )&&
               ( SOURCE_MOON == rack->current_state.value ))
            {
               if((( dish_mirror )&&
                   ( NAME_AROUND == dish_mirror->current_state.value ))&&
                  (( spire_mirror )&&
                   ( NAME_AROUND == spire_mirror->current_state.value ))&&
                  ( SOURCE_MOON == rack->current_state.value ))
               {
                  lbeam->current_state.value = L_TRIANGLE_GOLD;
               }
            }
            else
            if(( DISH_SUN == dish->current_state.value )&&
               ( SOURCE_SUN == rack->current_state.value ))
            {
               lbeam->current_state.value = L_BLINDING_GOLD;
            }
         }
      }
      break;

      case L_READY_GOLD :
      {
         if( obj == yellow )
         {
            lbeam->current_state.value = L_ENGAGE_GOLD;
            change = _TRUE;

            /* See if Lofty should return from sulking */
            if( C_CASTLE_CELLAR == adv_avatar[ A_LOFTY ].location )
            {
               SPRINT_( outbuf, "%s", adv_avatar[ A_LOFTY ].name );
               PRINTS _THE END_PRINT
               print_object_desc( &adv_avatar[ A_LOFTY ].output);
               PRINTS _REAPPEARS, _WITH, _A, _WAVE, _FULLSTOP, 
                      _NEWLINE END_PRINT
               adv_avatar[ A_LOFTY ].location = C_CASTLE_CELLAR;
               adv_avatar[ A_LOFTY ].location = C_CASTLE_SPIRE;
            }

            /* nelson can now locate wilbury and give the remote control */
            ( void )test_inventory( 
                     &adv_avatar[ A_NELLY ], 
                     ((( U32 )( 'r' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'm' )<< 8 )+(( U32 )( 'o' )<< 0 )), 0, &n );
            if( 0 <= n )
            {
               adv_avatar[ A_NELLY ].inventory[ n ].current_state.value = 1;
            }
            ( void )test_inventory( &adv_avatar[ A_NELLY ], 
                     ((( U32 )( 'p' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'p' )<< 8 )+(( U32 )( 'e' )<< 0 )), 
                     ((( U32 )( 's' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'e' )<< 0 )), &n );
            if(( 0 <= n )&&
               ( 0 == adv_avatar[ A_NELLY ].inventory[ n ].current_state.value ))
            {
               adv_avatar[ A_NELLY ].current_state.descriptor = 1;
               /* wait for wilbury to come out of the spire */
               adv_avatar[ A_NELLY ].location = C_CASTLE_COURTYARD;
            }

            /* check if the state needs to be upgraded, due to sequence of
             * events. */
            if(( DISH_MOON == dish->current_state.value )&&
               ( SOURCE_MOON == rack->current_state.value ))
            {
               if((( dish_mirror )&&
                   ( NAME_AROUND == dish_mirror->current_state.value ))&&
                  (( spire_mirror )&&
                   ( NAME_AROUND == spire_mirror->current_state.value )))
               {
                  lbeam->current_state.value = L_TRIANGLE_GOLD;
               }
            }
            else
            if(( DISH_SUN == dish->current_state.value )&&
               ( SOURCE_SUN == rack->current_state.value ))
            {
               lbeam->current_state.value = L_BLINDING_GOLD;
            }
         }
      }
      break;

      case L_BLINDING_DIAMOND :
      case L_BLINDING_GOLD :
      default :
      break;
   }

   if( _TRUE == change )
   {
      PRINTS _THERE, _IS, _A, _MUTED, _AUDIBLE, _POP, _FULLSTOP, 
             _NEWLINE END_PRINT
   }
}

static lb_state get_lightbeam_state( void )
{
   object_t *lbeam;

   lbeam = find_object_in_cell((( U32 )( 'l' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'h' )<< 0 ),
                           0, C_CASTLE_SPIRE_ROOF );
   return(( lb_state )lbeam->current_state.value );
}

static enum _adjective_t get_lightbeam_colour( void )
{
   enum _adjective_t c;
   lb_state s;

   s = get_lightbeam_state( );
   switch( s )
   {
      case L_BLINDING_DIAMOND :
      case L_TRIANGLE_DIAMOND :
      case L_ENGAGE_DIAMOND :
      case L_STANDBY_DIAMOND :
      case L_READY_DIAMOND :
      {
         c = SILVER;
      }
      break;

      case L_BLINDING_GOLD :
      case L_TRIANGLE_GOLD :
      case L_ENGAGE_GOLD :
      case L_STANDBY_GOLD :
      case L_READY_GOLD :
      {
         c = GOLD;
      }
      break;

      case L_DISENGAGE :
      default:
      {
         c = BLACK;
      }
      break;
   }

   return( c );
}

static void lightbeam_test( void )
{
   object_t *lbeam;


   lbeam = find_object_in_cell((( U32 )( 'l' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'g' )<< 8 )+(( U32 )( 'h' )<< 0 ),
                           0, C_CASTLE_SPIRE_ROOF );

   switch( lbeam->current_state.value )
   {
      case L_TRIANGLE_DIAMOND :
      case L_TRIANGLE_GOLD :
      {
         mirror_test( );
      }
      break;

      case L_BLINDING_GOLD :
      case L_BLINDING_DIAMOND :
      {
         /* if there is a filter on the telescope, all is well,
          * otherwise game over */
         sun_test( );
      }
      break;

      case L_ENGAGE_DIAMOND :
      case L_ENGAGE_GOLD :
      default :
      break;
   }
}

static void lightbeam_check( )
{
   object_t *energy;
   int can;


   /* check the lightbeam energy consumption */
   switch( get_lightbeam_state( ))
   {
      case L_BLINDING_DIAMOND :
      case L_TRIANGLE_DIAMOND :
      case L_ENGAGE_DIAMOND :
      case L_STANDBY_DIAMOND :
      case L_BLINDING_GOLD :
      case L_TRIANGLE_GOLD :
      case L_ENGAGE_GOLD :
      case L_STANDBY_GOLD :
      {
         /* check and spend an energy unit from the test-tube stored in the
          * energy cap */
         energy = find_object_in_cell((( U32 )( 'c' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'p' )<< 0 ), 0, 
                                      C_CASTLE_SPIRE );
         can = energy->current_state.descriptor;
         energy = &containers[ can ];
         if(( TESTTUBE == energy->output.name )&&
            ( energy->current_state.value ))
         {
            energy->current_state.value--;
            if( 0 == energy->current_state.value )
            {
               lightbeam_event( energy );
            }
         }
      }
      break;

      case L_DISENGAGE :
      case L_READY_DIAMOND :
      case L_READY_GOLD :
      default :
      break;
   }
}

static void print_lightbeam_state( TRISTATE const direction )
{
   object_t *telescope;
   object_t *dish;
   object_t *energy;
   object_t *rack;
   char *p;
   int i = 0;
   int can;


   /* find the test-tube stored in the energy cap */
   energy = find_object_in_cell((( U32 )( 'c' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'p' )<< 0 ), 0, 
                                C_CASTLE_SPIRE );
   can = energy->current_state.descriptor;
   energy = &containers[ can ];

   if(( TESTTUBE == energy->output.name )&&
      ( L_SOLID <= energy->current_state.value ))
   {
      p = "a solid";
   }
   else
   if(( TESTTUBE == energy->output.name )&&
      ( L_FALTERING <= energy->current_state.value ))
   {
      p = "a faltering";
   }
   else
   {
      p = "no";
   }

   rack = find_object_in_cell( 
                        ((( U32 )( 'r' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'c' )<< 8 )+(( U32 )( 'k' )<< 0 )), 
                        0, C_CASTLE_SPIRE_ROOF );
   if( _TRUE == direction )
   {
      PRINTS _THE, _LIGHTBEAM, _IS, _POINTING, _TOWARDS, _THE END_PRINT
      SPRINT_( outbuf, "%s",( SOURCE_SUN == rack->current_state.value )
                         ? "sun"
                         : "moon" );
      PRINTS _AND_ END_PRINT
   }

   switch( get_lightbeam_state( ))
   {
      case L_TRIANGLE_DIAMOND :
      case L_BLINDING_DIAMOND :
      {
         i = 1;
      }
      /* break;  -- fall through */
      case L_ENGAGE_DIAMOND :
      case L_STANDBY_DIAMOND :
      {
         PRINTS _THERE, _IS END_PRINT
         SPRINT_( outbuf, "%s", p );
         PRINTS _BEAM, _OF, _DIAMOND END_PRINT
      }
      break;

      case L_TRIANGLE_GOLD :
      case L_BLINDING_GOLD :
      {
         i = 1;
      }
      /* break;  -- fall through */
      case L_ENGAGE_GOLD :
      case L_STANDBY_GOLD :
      {
         PRINTS _THERE, _IS END_PRINT
         SPRINT_( outbuf, "%s", p );
         PRINTS _BEAM, _OF, _GOLD END_PRINT
      }
      break;

      case L_DISENGAGE :
      case L_READY_DIAMOND :
      case L_READY_GOLD :
      default :
      {
         PRINTS _THERE, _IS END_PRINT
         SPRINT_( outbuf, "%s", p );
         PRINTS _BEAM, _OF END_PRINT
      }
      break;
   }
   PRINTS _LIGHT END_PRINT

   telescope = find_object_in_cell(
                    (( U32 )( 't' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'e' )<< 0 ),
                    0, C_MOUNTAIN_OBSERVATORY );
   if( NAME_AROUND == telescope->current_state.value )
   {
      if( 1 == i )
      {
         if( SOURCE_SUN == rack->current_state.value )
         {
            PRINTS _LINKING, _THE, _SUN, _TO, _THE, _DISH, _FULLSTOP, 
                   _NEWLINE END_PRINT
         }
         else
         {
            PRINTS _LINKING, _THE, _MOON,_COMMA, _SPIRE, _AND_, _DISH, 
                   _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      {
         if( SOURCE_SUN == rack->current_state.value )
         {
            PRINTS _REFLECTING, _OFF, _THE, _SUN, _FULLSTOP, _NEWLINE END_PRINT
         }
         else
         {
            PRINTS _REFLECTING, _OFF, _THE, _MOON, _FULLSTOP, _NEWLINE END_PRINT
         }
      }

      dish = find_object_in_cell((( U32 )( 'd' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 'h' )<< 0 ),
                           0, C_HILLTOP );
      actions[ ACTION_LOOK ]( 0, 0, dish, 0 );
   }
   else
   {
      PRINTS _IN, _THE, _SKY, _FULLSTOP, _NEWLINE END_PRINT
   }
}

   /* another version of print_lightbeam_state */
static void print_meter_state( void )  
{
   object_t *energy;
   int can;


   PRINTS _IT, _INDICATES, _THE, _LIGHTBEAM, _IS END_PRINT

   switch( get_lightbeam_state( ))
   {
      case L_TRIANGLE_DIAMOND :
      {
         PRINTS _MOONLIT, _DIAMOND END_PRINT
      }
      break;

      case L_BLINDING_DIAMOND :
      {
         PRINTS _SUNLIT, _DIAMOND END_PRINT
      }
      break;

      case L_ENGAGE_DIAMOND :
      case L_STANDBY_DIAMOND :
      {
         PRINTS _DIAMOND END_PRINT
      }
      break;

      case L_TRIANGLE_GOLD :
      {
         PRINTS _MOONLIT, _GOLD END_PRINT
      }
      break;

      case L_BLINDING_GOLD :
      {
         PRINTS _SUNLIT, _GOLD END_PRINT
      }
      break;

      case L_ENGAGE_GOLD :
      case L_STANDBY_GOLD :
      {
         PRINTS _GOLD END_PRINT
      }
      break;

      case L_DISENGAGE :
      case L_READY_DIAMOND :
      case L_READY_GOLD :
      default :
      {
         /* find the test-tube stored in the energy cap */
         energy = find_object_in_cell((( U32 )( 'c' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'p' )<< 0 ), 0, 
                                      C_CASTLE_SPIRE );
         can = energy->current_state.descriptor;
         energy = &containers[ can ];

         if(( TESTTUBE == energy->output.name )&&
            ( 0 == energy->current_state.value ))
         {
            PRINTS _EMPTY END_PRINT
         }
         else
         {
            PRINTS _OFF END_PRINT
         }
      }
      break;
   }

   PRINTS _FULLSTOP, _NEWLINE END_PRINT
}

/* make_stuff, like a_give, a_take, a_drink & a_eat, works with containers. */
static void make_stuff( U32C acode, U32C ncode )
{
   object_t *p = 0;
   object_t *q = 0;
   int state = 0;
   int can;
   int n;


   PRINTS _THE END_PRINT
   print_object_desc( &( adv_avatar[A_OTTO].output ));
   PRINTS _SCAMPERS, _AROUND, _COMMA, _ASSEMBLING, _A, _NETWORK, _OF,
            _APPARATUS, _FULLSTOP, _NEWLINE END_PRINT

   if( find_object_in_cell( 
                  ((( U32 )( 'b' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'e' )<< 0 )), 0,
                  C_MOUNTAIN_LAB )&&
       find_object_in_cell( 
                  ((( U32 )( 'b' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'n' )<< 0 )), 0,
                  C_MOUNTAIN_LAB )&&
       find_object_in_cell( 
                  ((( U32 )( 's' )<< 24 )+(( U32 )( 't' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'n' )<< 0 )), 0,
                  C_MOUNTAIN_LAB ))
   {
      /* check wilbury and otis are in the lab;
       * check the lab contains a flask containing heavy water 
       * and a crucible containing either source or sand */
      if( C_MOUNTAIN_LAB != adv_avatar[ A_OTTO ].location )
      {
         SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
         PRINTS _IS, _NOT, _IN, _THE, _LAB, _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      if( SOURCE == ncode )
      {
         p = find_object_in_cell( 
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'r' )<< 0 ),
                  (( U32 )( 'd' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'm' )<< 0 ),
                  cavatar->location );
         if( 0 == p )
         {
            p = find_object_with_avatar(
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'r' )<< 0 ),
                  (( U32 )( 'd' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'm' )<< 0 ));
         }

         if( p )
         {
            state++;
         }
         else
         {
            PRINTS _OOPS, _COMMA, _IT, _APPEARS, _THERE, _IS, _NO END_PRINT
            p = find_object_in_cell(
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'r' )<< 0 ), 0, C_MINE_FACE );
            print_object_desc( &( p->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      if(( MIRROR == ncode )&&( SILVER == acode ))
      {
         p = find_object_in_cell( 
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'd' )<< 0 ),
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'v' )<< 0 ),
                  cavatar->location );
         if( 0 == p )
         {
            p = find_object_with_avatar(
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'd' )<< 0 ),
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'v' )<< 0 ));
         }

         if( p )
         {
            state++;
         }
         else
         {
            PRINTS _OOPS, _COMMA, _IT, _APPEARS, _THERE, _IS, _NO END_PRINT
            p = find_object_in_cell(
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'd' )<< 0 ), 
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'v' )<< 0 ), 
                  C_MOUNTAIN_LAKE_BOTTOM );
            print_object_desc( &( p->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      if(( MIRROR == ncode )&&( GOLD == acode ))
      {
         p = find_object_in_cell( 
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'd' )<< 0 ),
                  (( U32 )( 'y' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 ),
                  cavatar->location );
         if( 0 == p )
         {
            p = find_object_with_avatar(
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'd' )<< 0 ),
                  (( U32 )( 'y' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 ));
         }

         if( p )
         {
            state++;
         }
         else
         {
            PRINTS _OOPS, _COMMA, _IT, _APPEARS, _THERE, _IS, _NO END_PRINT
            p = find_object_in_cell(
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'd' )<< 0 ), 
                  (( U32 )( 'y' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 ), 
                  C_MOUNTAIN_LAKE_BOTTOM );
            print_object_desc( &( p->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }
      else
      if((( BOARD == ncode )&&( CONTROL == acode ))||
         ( FILTER == ncode ))
      {
         p = find_object_in_cell( 
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'd' )<< 0 ), 0,
                  cavatar->location );
         if( 0 == p )
         {
            p = find_object_with_avatar(
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'd' )<< 0 ), 0 );
         }

         if( p )
         {
            state++;
         }
         else
         {
            PRINTS _OOPS, _COMMA, _IT, _APPEARS, _THERE, _IS, _NO END_PRINT
            p = find_object_in_cell(
                  (( U32 )( 's' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 'd' )<< 0 ), 0,  
                  C_MOUNTAIN_LAKE_BOTTOM );
            print_object_desc( &( p->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }

      if( 1 == state )
      {
         /* find heavy water, either in the cell or inventory;
          * hard-coded special case and is the glass flask */
         q = find_object_in_cell( 
                  (( U32 )( 'w' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'e' )<< 0 ), 
                  (( U32 )( 'h' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'v' )<< 0 ),
                  cavatar->location );
         if( 0 == q )
         {
            q = find_object_with_avatar(
                  (( U32 )( 'w' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'e' )<< 0 ), 
                  (( U32 )( 'h' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'v' )<< 0 ));
         }

         if( q )
         {
            state++;
         }
         else
         {
            PRINTS _OOPS, _COMMA, _IT, _APPEARS, _THERE, _IS, _NO END_PRINT
            q = find_object_in_cell(
                  (( U32 )( 'w' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'e' )<< 0 ), 
                  (( U32 )( 'h' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'v' )<< 0 ), 
                  C_MOUNTAIN_LAKE_BOTTOM );
            print_object_desc( &( q->output ));
            PRINTS _FULLSTOP, _NEWLINE END_PRINT
         }
      }

      if( 2 == state )
      {
         /* spend the source/sand and heavy water */
         *p = empty_object;

         if( _TRUE == container_test( q, &n ))
         {
            can = cavatar->inventory[ n ].current_state.descriptor;
            containers[ can ]= empty_object;
         }
         *q = empty_object;

         /* now manufacture the required stuff */
         if( SOURCE == ncode )
         {
            p = find_object_in_cell( 
                  (( U32 )( 't' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'b' )<< 8 )+(( U32 )( 'e' )<< 0 ), 
                  (( U32 )( 't' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 't' )<< 0 ), 
                  cavatar->location );
            if( 0 == p )
            {
               p = find_object_with_avatar(
                     (( U32 )( 't' )<< 24 )+(( U32 )( 'u' )<< 16 )+(( U32 )( 'b' )<< 8 )+(( U32 )( 'e' )<< 0 ), 
                     (( U32 )( 't' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 't' )<< 0 ));
            }

            if( p )
            {
               /* add an amount of fuel,
                * that will 'burn' for a decent while */
               p->current_state.value = L_SOLID + L_SOLID;

               PRINTS _MUCH, _BUBBLING, _AND_, _EFFERVESCING, _HAPPENS,
                        _ELIPSIS END_PRINT
               PRINTS _AND_, _SUCCESS, _EXCLAMATION, _ONE, _TEST, _TUBE, _OF,
                        _ENERGY, _TO, _GO, _FULLSTOP END_PRINT
               SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
               PRINTS _SAYS, _COMMA, _TAKE, _THIS, _TEST, _TUBE, _OF, _ENERGY,
                        _TO, _THE, _LIGHTBEAM, _COMMA, _OPEN, _THE, _FILLER,
                        _CAP, _COMMA, _THEN, _SWAP, _IT, _WITH, _THE, _EMPTY, 
                        _TUBE, _FULLSTOP, _NEWLINE END_PRINT
            }
            else
            {
               PRINTS _OOPS, _COMMA, _THE, _LIQUID, _SPILLS, _TO, _THE,
                        _FLOOR, _FULLSTOP, _THERES, _NO, _EMPTY, _TEST, _TUBE,
                        _HERE, _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         else
         if( MIRROR == ncode )
         {
            p = find_object_in_cell( 
                  (( U32 )( 't' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'y' )<< 0 ), 
                  (( U32 )( 'b' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'k' )<< 8 )+(( U32 )( 'i' )<< 0 ),
                  cavatar->location );
            if( 0 == p )
            {
               p = find_object_with_avatar(
                     (( U32 )( 't' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'y' )<< 0 ), 
                     (( U32 )( 'b' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'k' )<< 8 )+(( U32 )( 'i' )<< 0 ));
            }

            if(( p )&&
               ( 0 == containers[ p->current_state.descriptor ].output.name ))
            {
               p->current_state.value = acode;
               can = p->current_state.descriptor;
               if( SILVER == acode )
               {
                  p = find_object_in_cell(
                           (( U32 )( 'm' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'r' )<< 0 ),
                           (( U32 )( 's' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'v' )<< 0 ),
                           C_MOUNTAIN_MANUFACTORY );
               }
               else
               if( GOLD == acode )
               {
                  p = find_object_in_cell(
                           (( U32 )( 'm' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'r' )<< 0 ),
                           (( U32 )( 'g' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'd' )<< 0 ),
                           C_MOUNTAIN_MANUFACTORY );
               }
               containers[ can ]= *p;

               /* don't need *p = empty_object, as the new mirrors
                  are stored in the castle cavern. */

               PRINTS _MUCH, _MELTING, _AND_, _POURING, _HAPPENS, _ELIPSIS,
                        _HANDLE, _WITH, _CARE, _EXCLAMATION, _ONE END_PRINT
               SPRINT_( outbuf, "%s", 
                        ( SILVER == acode )? "silvered": "golden" );
               PRINTS _MIRROR, _TO, _GO, _FULLSTOP, _NEWLINE END_PRINT
               SPRINT_( outbuf, "%s", adv_avatar[ A_OTTO ].name );
               PRINTS _SAYS, _COMMA, _TAKE, _ONE, _MIRROR, _TO, _THE,
                        _MONUMENT, _COMMA, _AND_, _FIX, _IT, _TO, _THE, _DISH,
                        _FULLSTOP, _TAKE, _ANOTHER, _MIRROR, _AND_, _FIX, _IT,
                        _TO, _THE, _SPIRE, _FULLSTOP, _NEWLINE END_PRINT
            }
            else
            {
               PRINTS _OOPS, _COMMA, _THE, _LIQUID, _GLASS, _DRIPS, _TO,
                        _THE, _FLOOR, _FULLSTOP, _THERES, _NO, _EMPTY, _BAKING,
                        _TRAY, _HERE, _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         else
         if( FILTER == ncode )
         {
            p = find_object_in_cell( 
                  (( U32 )( 't' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'y' )<< 0 ), 
                  (( U32 )( 'b' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'k' )<< 8 )+(( U32 )( 'i' )<< 0 ),
                  cavatar->location );
            if( 0 == p )
            {
               p = find_object_with_avatar(
                      (( U32 )( 't' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'y' )<< 0 ), 
                      (( U32 )( 'b' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'k' )<< 8 )+(( U32 )( 'i' )<< 0 ));
            }

            if(( p )&&
               ( 0 == containers[ p->current_state.descriptor ].output.name ))
            {
               can = p->current_state.descriptor;
               p = find_object_in_cell(
                           (( U32 )( 'f' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 't' )<< 0 ),
                           (( U32 )( 'd' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'k' )<< 0 ), 
                           C_MOUNTAIN_MANUFACTORY );
               containers[ can ]= *p;

               /* don't need *p = empty_object, as the new mirrors
                  are stored in the manufactory. */

               PRINTS _MUCH, _MELTING, _AND_, _POURING, _HAPPENS, _ELIPSIS,
                        _HANDLE, _WITH, _CARE, _EXCLAMATION, _ONE END_PRINT
               PRINTS _FILTER, _TO, _GO, _FULLSTOP, _NEWLINE END_PRINT
            }
            else
            {
               PRINTS _OOPS, _COMMA, _THE, _LIQUID, _GLASS, _DRIPS, _TO,
                        _THE, _FLOOR, _FULLSTOP, _THERES, _NO, _EMPTY, _BAKING,
                        _TRAY, _HERE, _FULLSTOP, _NEWLINE END_PRINT
            }
         }
         else
         if( BOARD == ncode )
         {
            make_board( A_OTTO );
         }
      }
   }
   else
   {
      PRINTS _THERE, _IS, _SOME, _LAB, _EQUIPMENT, _MISSING, _FULLSTOP,
               _NEWLINE END_PRINT
   }
}

static void make_board( enum _avatar_name const who )
{
   object_t *p;
   object_t *q;
   int n;


   p = find_object_in_cell(
                  (( U32 )( 'b' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'r' )<< 0 ), 0,
                  C_MOUNTAIN_MANUFACTORY );
   n = get_count_of_boards_in_world( );
   if(( 7 > n )&&( 8 > p->current_state.value ))
   {
      PRINTS _MUCH, _ETCHING, _AND_, _SOLDERING, _HAPPENS, _ELIPSIS,
               _ESC_L, _AND_, _COMMA, _WITH, _THE, _ADDITION, _OF, _A, _LITTLE,
               _COLOUR, _COMMA, _A, _NEW, _CONTROLLER, _BOARD, _IS,
               _READY, _EXCLAMATION END_PRINT

      SPRINT_( outbuf, "%s", adv_avatar[ who ].name );

      test_inventory_free_space( cavatar, &n );
      if( 0 <= n )
      {
         PRINTS _HANDS, _ME, _THE, _BOARD, _AND_ END_PRINT
            /* actually a copy of the board object */
         cavatar->inventory[ n ]= *p;
         q = &( cavatar->inventory[ n ]);
      }
      else
      {
         n = find_space_in_cell( cavatar->location );
         if( 0 <= n )
         {
            /* actually a copy of the board object */
            adv_world.cells[ cavatar->location ].objects[ n ]= *p;
            q = &( adv_world.cells[ cavatar->location ].objects[ n ]);
         }
      }

      if( 0 > n )
      {
         test_inventory_free_space( &adv_avatar[ A_OTTO ], &n );
         if( 0 <= n )
         {
               /* actually a copy of the board object */
            adv_avatar[ A_OTTO ].inventory[ n ]= *p;
            q = &( adv_avatar[ A_OTTO ].inventory[ n ]);
            PRINTS _HOLDS, _OUT, _THE, _BOARD, _AND_ END_PRINT
         }
      }

      if( 0 > n )
      {
         PRINTS _SAYS, _COMMA, _WE, _NEED, _TO, _MAKE, _SOME, _SPACE, _IN,
                _THE, _LAB, _BEFORE, _ANOTHER, _BOARD, _CAN, _BE, _MADE,
                _FULLSTOP, _NEWLINE END_PRINT
      }
      else
      {
         PRINTS _SAYS, _COMMA, _FIX, _THIS, _BOARD, _TO, _A, _MAGNOTRON,
               _RING, _ELEMENT, _FULLSTOP, _NEWLINE END_PRINT


         /* Update state of manufactory source board;
            increment count of boards made */
         p->current_state.value++;

         /* Update state of instance board, to distinguish them */
         switch( q->current_state.value )
         {
            case 0 :
            {
               q->input.adjective =((( U32 )( 0 )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'd' )<< 0 ));
               q->output.adjective |= RED;
            }
            break;

            case 1 :
            {
               q->input.adjective =((( U32 )( 'o' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'n' )<< 0 ));
               q->output.adjective |= ORANGE;
            }
            break;

            case 2 :
            {
               q->input.adjective =((( U32 )( 'y' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 ));
               q->output.adjective |= YELLOW;
            }
            break;

            case 3 :
            {
               q->input.adjective =((( U32 )( 'g' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'e' )<< 0 ));
               q->output.adjective |= GREEN;
            }
            break;

            case 4 :
            {
               q->input.adjective =((( U32 )( 'b' )<< 24 )+(( U32 )( 'l' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'e' )<< 0 ));
               q->output.adjective |= BLUE;
            }
            break;

            case 5 :
            {
               q->input.adjective =((( U32 )( 'v' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'l' )<< 0 ));
               q->output.adjective |= VIOLET;
            }
            break;

            case 6 :
            {
               q->input.adjective =((( U32 )( 'g' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'y' )<< 0 ));
               q->output.adjective |= GREY;
            }
            break;

            /* more boards should not be needed;
             * but if one get destroyed on fixing to the ring element,
             * (because the lightbeam's left on) then we need more */
            case 7 :
            {
               q->input.adjective =((( U32 )( 'g' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'y' )<< 0 ));
               q->output.adjective |= WHITE;
               PRINTS _I, _CANNOT, _MAKE, _ANY, _MORE, _BOARDS, _AFTER, _THIS,
                      _COMMA, _SO, _TRY, _NOT, _TO, _DESTROY, _IT, _FULLSTOP,
                      _NEWLINE END_PRINT
            }
            break;

            default :
            {
            }
            break;
         }

         q->current_state.value = 0;

      }
   }
   else
   {
      SPRINT_( outbuf, "%s", adv_avatar[ who ].name );
      PRINTS _SAYS, _COMMA, _THERES, _ALREADY, _7, _BOARDS, _MADE,
               _FULLSTOP, _NEWLINE END_PRINT
   }
}

static int get_count_of_boards_in_world( void )
{
   int n = 0;
   U32C ncode =(( U32 )( 'b' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'r' )<< 0 );
   int i;
   int w;
   cell_t const *c;


   /* The boards could be anywhere, so search each of the cells */
   for( w=1;( w < C_NAMED_CELLS ); w++ )
   {
      /* see if object lies in this world cell */
      c = &adv_world.cells[ w ];

      for( i=0;( i < MAX_CELL_OBJECTS ); i++ )
      {
         if( c->objects[ i ].input.name == ncode )
         {
            n++;
         }
      }
   }

   /* subtract 1 for the master board in the manufactory */
   if( n )
   {
      n--;
   }

   return( n );
}

static enum _rotary get_rotary_switch_state( enum _cell_name const c )
{
   enum _rotary i = ROTARY_OFF;
   object_t *p;

   p =( object_t* )find_object_in_cell(
                 ((( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 )),
                 ((( U32 )( 'r' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'a' )<< 0 )),
                 c );
   if( p )
   {
      i = p->current_state.value;
   }

   return( i );
}

static void set_rotary_switch( enum _cell_name const c, UCC direction )
{
   object_t *p;


   p =( object_t* )find_object_in_cell(
                 ((( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 )),
                 ((( U32 )( 'r' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 't' )<< 8 )+(( U32 )( 'a' )<< 0 )),
                 c );

   if( C_MOUNTAIN_OBSERVATORY == c )
   {
      if( NAME_LEFT == direction )
      {
         if( ROTARY_OFF < p->current_state.value )
         {
            p->current_state.value--;
            PRINTS _THERE, _IS, _A, _RUMBLING, _NOISE, _AS, _THE, _TELESCOPE,
                     _MECHANISM, _SLOWLY, _ROTATES, _ANTI, _MINUS, _CLOCKWISE, 
                     _ELIPSIS, _NEWLINE END_PRINT
         }
      }
      else
      if( NAME_RIGHT == direction )
      {
         if( ROTARY_SPIRE_MIRROR > p->current_state.value )
         {
            p->current_state.value++;
            PRINTS _THERE, _IS, _A, _RUMBLING, _NOISE, _AS, _THE, _TELESCOPE,
                     _MECHANISM, _SLOWLY, _ROTATES, _CLOCKWISE, _ELIPSIS,
                     _NEWLINE END_PRINT
         }
      }
      else
      {
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( p->output ));
         PRINTS _IN, _THAT, _DIRECTION, _FULLSTOP, _NEWLINE END_PRINT
      }
   }
   else
   if( C_MAGNOTRON_OPERATION == c )
   {
      if( NAME_LEFT == direction )
      {
         if( ROTARY_0 < p->current_state.value )
         {
            p->current_state.value--;
         }
      }
      else
      if( NAME_RIGHT == direction )
      {
         if( ROTARY_6 > p->current_state.value )
         {
            p->current_state.value++;
         }
      }
      else
      {
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( p->output ));
         PRINTS _IN, _THAT, _DIRECTION, _FULLSTOP, _NEWLINE END_PRINT
      }
   }
}

static void print_rotary_switch_state( enum _cell_name const c )
{
   enum _rotary i;

   i = get_rotary_switch_state( c );
   if( C_MOUNTAIN_OBSERVATORY == c )
   {
      PRINTS _THE, _ROTARY, _SWITCH, _IS, _SET, _TO END_PRINT
      if( ROTARY_DISH == i ){ PRINTS _DISH END_PRINT }
      else if( ROTARY_LIGHTBEAM == i ){ PRINTS _LIGHTBEAM END_PRINT }
      else if( ROTARY_DISH_MIRROR == i ){ PRINTS _DISH, _MIRROR END_PRINT }
      else if( ROTARY_SPIRE_MIRROR == i ){ PRINTS _SPIRE, _MIRROR END_PRINT }
      else { PRINTS _OFF END_PRINT }
      PRINTS _FULLSTOP, _NEWLINE END_PRINT
   }
   else
   if( C_MAGNOTRON_OPERATION == c )
   {
      PRINTS _THE, _ROTARY, _SWITCH, _IS, _SET, _TO, _CONTROLLER END_PRINT
      SPRINT_( outbuf, "%d", i );
      PRINTS _FULLSTOP, _NEWLINE END_PRINT
   }
}

static void set_slider_switch( object_t *p, UCC direction )
{
   if( NAME_DOWN == direction )
   {
      if( 0 < p->current_state.value )
      {
         p->current_state.value--;
      }
   }
   else
   if( NAME_UP == direction )
   {
      if( 7 > p->current_state.value )
      {
         p->current_state.value++;
      }
   }
   else
   {
      PRINTS _I, _CANNOT END_PRINT
      SPRINT_( outbuf, "%s", verb );
      PRINTS _THE END_PRINT
      print_object_desc( &( p->output ));
      PRINTS _IN, _THAT, _DIRECTION, _FULLSTOP, _NEWLINE END_PRINT
   }

   PRINTS _THE END_PRINT
   print_object_desc( &( p->output ));
   PRINTS _IS, _SET, _TO, _LEVEL END_PRINT
   SPRINT_( outbuf, "%d", p->current_state.value );
   PRINTS _FULLSTOP, _NEWLINE END_PRINT

   set_element_state( get_rotary_switch_state( C_MAGNOTRON_OPERATION ));
}

static void set_element_state( UCC ring )
{
   object_t *p;
   object_t *q;
   int v;  /* the value of the 3 operations room slider switches */


   if( 7 > ring )
   {
      /* set the board actual setting */
      q = find_object_in_cell(
                        (( U32 )( 'b' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'r' )<< 0 ),
                        0,( C_MAGNOTRON_ELEMENT_0 + ring ));
         /* if the board is fixed in place */
      if( q )
      {
         switch( get_lightbeam_state( ))
         {
            case L_DISENGAGE :
            case L_READY_DIAMOND :
            case L_READY_GOLD :
            {
               /* lightbeam is disengaged, so 0% efficiency in ring */
               v = 100;  /* set to worst value */
            }
            break;

            default :
            {
               p = find_object_in_cell(
                        (( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 ),
                        (( U32 )( 'b' )<< 24 )+(( U32 )( 'l' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'e' )<< 0 ),
                        C_MAGNOTRON_OPERATION );
               v =( p->current_state.value * 100 );
               p = find_object_in_cell(
                        (( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 ),
                        (( U32 )( 'y' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 'l' )<< 0 ),
                        C_MAGNOTRON_OPERATION );
               v +=( p->current_state.value * 10 );
               p = find_object_in_cell(
                        (( U32 )( 's' )<< 24 )+(( U32 )( 'w' )<< 16 )+(( U32 )( 'i' )<< 8 )+(( U32 )( 't' )<< 0 ),
                        (( U32 )( 0 )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'd' )<< 0 ),
                        C_MAGNOTRON_OPERATION );
               if( 0 < p->current_state.value )
               {
                  v /=( p->current_state.value * 1 );
               }

               /* The optimum for any controller is its location * 25: 
                *    board 0 is 25, board 1 is 50, ..., board 6 is 150.
                * The actual setting for each controller is 
                *    ( blue + yellow )/ red
                *    e.g. 150 / 2 = 75.
                * The value is then the absolute difference between its optimum
                *    and actual setting, e.g. |75 - 25| = 50. 
                * So 0 is spot on, 100 is worst. */
               v = v -(( ring + 1 )* 25 );
               if( v < 0 )
               {
                  v = -v;
               }
               if( v > 100 )
               {
                  v = 100;
               }
            }
            break;
         }

         q->current_state.value = v;
      }
   }
   else
   {
      DEBUGF( "Illegal ring %d\n", ring );
   }
}

static void print_element_state( UCC element )
{
   object_t *p;
   int e;

   if( 7 > element )
   {
      p = find_object_in_cell(
                        (( U32 )( 'b' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'r' )<< 0 ),
                        0,( C_MAGNOTRON_ELEMENT_0 + element ));
      if( p )
      {
         e = ( 100 - p->current_state.value );
         PRINTS _RING, _CONTROLLER, _BOARD END_PRINT
         SPRINT_( outbuf, "%d", element );
         PRINTS _IS, _AT END_PRINT
         SPRINT_( outbuf, "%d%%", e );
         PRINTS _EFFICIENCY, _FULLSTOP, _NEWLINE END_PRINT

         /* above 95% efficiency will do */
         if( e > 95 )
         {
            magnotron_test( );
         }
      }
      else
      {
         PRINTS _THERE, _IS, _NO, _CONTROLLER, _BOARD, _INSTALLED, _IN, 
                  _MAGNOTRON, _ELEMENT END_PRINT
         SPRINT_( outbuf, "%d", element );
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }
}

static void energy_test( object_t *src )
{
   object_t *p;
   int m;


   /* Called when each of the energy sources has been achieved.
    * This is how we decide the game win;
    * if all 3 energy sources have been tried */
   p = find_object_with_avatar((( U32 )( 'p' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'p' )<< 8 )+(( U32 )( 'e' )<< 0 ),
                               (( U32 )( 's' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'e' )<< 0 ));
   if( 0 == p )
   {
      p = find_object_in_world((( U32 )( 'p' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'p' )<< 8 )+(( U32 )( 'e' )<< 0 ),
                               (( U32 )( 's' )<< 24 )+(( U32 )( 'h' )<< 16 )+(( U32 )( 'e' )<< 8 )+(( U32 )( 'e' )<< 0 ), 0 );
   }
   else
   {
      PRINTS _I, _CHECK, _THE, _SHEET, _OF, _PAPER END_PRINT
      SPRINT_( outbuf, "%s", adv_avatar[ A_LOFTY ].name );
      PRINTS _GAVE, _ME, _FULLSTOP, _NEWLINE END_PRINT
      actions[ ACTION_READ ]( 0, 0, p, 0 );
   }

   /* p=0 if sheet of paper is with Nelly already */
   if( p )
   {
      if( &adv_world.sources[ SOURCE_SUN ]== src )
      {
         adv_world.sources[ SOURCE_ENERGY ].current_state.value |= 
            SOURCE_SUN_TEST;
         p->current_state.value = 0;  /* allow selection of another task */
      }
      else
      if( &adv_world.sources[ SOURCE_MOON ]== src )
      {
         adv_world.sources[ SOURCE_ENERGY ].current_state.value |= 
            SOURCE_MOON_TEST;
         p->current_state.value = 0;  /* allow selection of another task */
      }
      else
      if( &adv_world.sources[ SOURCE_MAGNOTRON ]== src )
      {
         adv_world.sources[ SOURCE_ENERGY ].current_state.value |= 
            SOURCE_MAGNOTRON_TEST;
         p->current_state.value = 0;  /* allow selection of another task */
      }

      if(( C_CASTLE_SPIRE == cavatar->location )&&
         ( C_CASTLE_SPIRE == adv_avatar[ A_LOFTY ].location ))
      {
         SPRINT_( outbuf, "%s", adv_avatar[ A_LOFTY ].name );
         PRINTS _THE END_PRINT
         print_object_desc( &adv_avatar[ A_LOFTY ].output );
      }
      else
      {
         PRINTS _AN, _ETHEREAL, _VOICE END_PRINT
      }
      PRINTS _SAYS, _QUOTE_LEFT, _ESC_U, _WELL, _DONE, _COMMA, _THIS, _TASK, 
                    _IS, _COMPLETE, _FULLSTOP END_PRINT
      if( 0x7 > adv_world.sources[ SOURCE_ENERGY ].current_state.value )
      {
         PRINTS _FIND END_PRINT
         SPRINT_( outbuf, "%s", adv_avatar[ A_NELLY ].name );
         PRINTS _FOR, _YOUR, _NEXT, _TASK, _FULLSTOP, _QUOTE_RIGHT END_PRINT
      }
      PRINTS _NEWLINE END_PRINT

      /* give sheet of paper back to Nelly */
      test_inventory_free_space( &adv_avatar[ A_NELLY ], &m );
      if( 0 <= m )
      {
         adv_avatar[ A_NELLY ].inventory[ m ]= *p;
         *p = empty_object;

         /* wait for wilbury to come out of the spire */
         adv_avatar[ A_NELLY ].current_state.descriptor = 1;
         adv_avatar[ A_NELLY ].location = C_CASTLE_COURTYARD;
      }
   }

   if( 0x7 == adv_world.sources[ SOURCE_ENERGY ].current_state.value )
   {
      adv__game_over( GAME_OVER_AVATAR_WON );
      adv_world.errnum = ERR_game_won;
   }
}

static void mirror_test( void )
{
   int state;

   /* set the "energy" object current_state.value */
   state = get_lightbeam_state( );
   if((( L_TRIANGLE_DIAMOND == state )||( L_TRIANGLE_GOLD == state ))&&
      ( 0 ==( SOURCE_MOON_TEST & 
              adv_world.sources[ SOURCE_ENERGY ].current_state.value )))
   {
      /* lightbeam now forms a (multiple) triangle;
       * set the "energy" object current_state.value */
      energy_test( &adv_world.sources[ SOURCE_MOON ]);
   }
}

static void sun_test( void )
{
   object_t *filter;
   object_t *lens;
   object_t *dish;
   object_t *rack;


   rack = find_object_in_cell( 
                        ((( U32 )( 'r' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'c' )<< 8 )+(( U32 )( 'k' )<< 0 )), 
                        0, C_CASTLE_SPIRE_ROOF );
   dish = find_object_in_cell( 
                        ((( U32 )( 'd' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 's' )<< 8 )+(( U32 )( 'h' )<< 0 )), 
                        0, C_HILLTOP );
   if(( SOURCE_SUN == rack->current_state.value )&&
      ( DISH_SUN == dish->current_state.value )&&
      ( 0 ==( SOURCE_SUN_TEST & 
              adv_world.sources[ SOURCE_ENERGY ].current_state.value )))
   {
      filter = find_object_in_cell(
            (( U32 )( 'f' )<< 24 )+(( U32 )( 'i' )<< 16 )+(( U32 )( 'l' )<< 8 )+(( U32 )( 't' )<< 0 ), 0, 
            C_MOUNTAIN_OBSERVATORY_DOME );
      lens = find_object_in_cell(
            (( U32 )( 'l' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'n' )<< 8 )+(( U32 )( 's' )<< 0 ), 0, 
            C_MOUNTAIN_OBSERVATORY_DOME );
      if( _TRUE == test_fix( filter, lens ))
      {
         /* filter is fixed to lens;
          * set the "energy" object current_state.value */
         energy_test( &adv_world.sources[ SOURCE_SUN ]);
      }
      else
      {
         PRINTS _THERE, _IS, _A, _SUDDEN, _BLINDING, _EXPLOSION, _OF, _LIGHT,
                  _FULLSTOP, _NEWLINE END_PRINT
         adv__game_over( GAME_OVER_SUN_BLOWOUT );
         adv_world.errnum = ERR_game;
      }
   }
}

static void magnotron_test( void )
{
   int i;
   int e = 0;
   object_t *p;


   /* The optimum magnotron energy level is when all 7 ring elements
    * are at their optimum. Once the optimum level is reached, then call
    * energy_test indicating the magnotron is at peak. */
   for( i = C_MAGNOTRON_ELEMENT_0; i <= C_MAGNOTRON_ELEMENT_6; i++ )
   {
      p = find_object_in_cell((( U32 )( 'b' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'r' )<< 0 ), 0, i );
      if( p )
      {
         e +=( 100 - p->current_state.value );
      }
   }

   PRINTS _OVERALL, _MAGNOTRON, _EFFICIENCY, _IS, _AT END_PRINT
   SPRINT_( outbuf, "%d%%",( int )( e / 7 ));
   PRINTS _FULLSTOP, _NEWLINE END_PRINT

   /* allow 95% of optimum */
   if((( int )( 700 * 0.95 )<= e )&&
      ( 0 ==( SOURCE_MAGNOTRON_TEST & 
              adv_world.sources[ SOURCE_ENERGY ].current_state.value )))
   {
      energy_test( &adv_world.sources[ SOURCE_MAGNOTRON ]);
   }
}


/*
 ************************* Local Functions
 */

static void adv__print_status( TRISTATE const listobj )
{
   UCC c = cavatar->location;
   TRISTATE visible = _TRUE;
   object_t const *p = 0;


   /* if avatar is in the mines, then will need a lamp to see */
   if(( C_MINE_ENTRANCE <= c )&&( C_MINE_FACE >= c ))
   {
      p = find_noun_as_object_in_current(
                  ((( U32 )( 'l' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'm' )<< 8 )+(( U32 )( 'p' )<< 0 )), 0 );
      if( 0 == p )
      {
         visible = _FALSE;
      }
      else
      {
         if( 0 == p->current_state.value )
         {
            visible = _FALSE;
         }
      }

      if( _FALSE == visible )
      {
         /* still tell wilbury if the bar can be heard */
         ( void )print_cell_avatars( c, _FALSE );
      }
   }

   if( _TRUE == visible )
   {
      /* print name of current cell */
      PRINTS _I, _AM, _IN END_PRINT
      SPRINT_( outbuf, "%s", 
            IS_VOWEL( get_vowel_of_object( &( adv_world.cells[ c ].input ))));
      print_cell_desc( &( adv_world.cells[ c ].output ));
      PRINTS _FULLSTOP, _NEWLINE END_PRINT

      if( _TRUE == listobj )
      {
         /* list visible objects */
         print_cell_objects( c );
         ( void )print_cell_avatars( c, _FALSE );
      }
   }
   else
   {
      PRINTS _I, _CANNOT, _SEE, _IN, _THE, _DARK, _FULLSTOP, _NEWLINE END_PRINT
   }
}

static void adv__look_around( UC start, UC end, int j )
{
   UCC c = cavatar->location;
   TRISTATE visible = _TRUE;
   direction_t way;
   object_t const *p = 0;
   int i;


   /* if avatar is in the mines, then will need a lamp to see */
   if(( C_MINE_ENTRANCE <= c )&&( C_MINE_FACE >= c ))
   {
      p = find_noun_as_object_in_current(
                  ((( U32 )( 'l' )<< 24 )+(( U32 )( 'a' )<< 16 )+(( U32 )( 'm' )<< 8 )+(( U32 )( 'p' )<< 0 )), 0 );
      if( 0 == p )
      {
         visible = _FALSE;
      }
      else
      {
         if( 0 == p->current_state.value )
         {
            visible = _FALSE;
         }
      }
   }

   if( _TRUE == visible )
   {
      PRINTS _IN, _THE, _DISTANCE, _I, _SEE END_PRINT
      for( i = start; i <= end; i++ )
      {
         way = adv_world.cells[ cavatar->location ].ways[ i ];
         if( 0 < way )
         {
            /* print name of cell in that direction */
            print_cell_desc( &( adv_world.cells[ way ].output ));
            j--;
            if( j )
            {
               PRINTS _COMMA END_PRINT
            }
         }
      }
      PRINTS _FULLSTOP, _NEWLINE END_PRINT
   }
}

void increment_inputs( void )
{
   inputs++;
}

/* ERR adv__get_input( void ) woz here;
 *  but is moved to adv_main.c as part of the code that needs porting
 *  to new platforms.
 *
 * display_initial_help is called for every adv__get_input. */
void display_initial_help( void )
{
   /* test for both starting state and after a 'new' command */
   if( 0 == inputs )
   {
      cavatar->help = 0;
      actions[ ACTION_BUILTIN ]( 0x68656c70 /* help */, NAME_NULL, 0, 0 );
   }
}

void set_inbuf( UC const * const ins, size_t const len )
{
   int i;
   TRISTATE done = _FALSE;
   size_t const sz = sizeof( inbuf );


   ( void )memset( inbuf, 0, sz );

   for( i=0;( _FALSE == done )&&( i < len ); i++ )
   {
      if( isalnum( ins[ i ]) || ( ' ' == ins[ i ])
#       if defined( DEBUG )
           ||( '_' == ins[ i ])
#       endif
        )
      {
         inbuf[ i ]= ins[ i ];
      }
      else
      {
         done = _TRUE;
      }
   }

   /* also increment inputs here */
   inputs++;
}

static void get_verb_and_string( void )
{
   int i;
   int state = 0;


   /* undo get_words, by removing '\0's after the 2nd word */
   for( i=0;( i < sizeof( inbuf )); i++ )
   {
      if( 0 == inbuf[ i ])
      {
         if( 0 == inbuf[ i+1 ])
         {
            break;
         }
         else
         {
            if( 0 < state )
            {
               inbuf[ i ]= ' ';
            }
            state++;
            if( 1 == state )
            {
               noun = &inbuf[ i+1 ];
            }
         }
      }
   }

   /* bug fix if string happens to be sizeof(inbuf) */
   inbuf[ sizeof( inbuf )- 1 ]= 0;

   DEBUGF( "get_verb_and_string: %s %s\n", inbuf, noun );
}

/* parse input to find verb and noun elements */
static void get_verb_and_noun( UCC num, UC *w[ ])
{
   object_t const *p = 0;
   U32 ncode;
   index_t n;


   /* make sure all global pointers are reset */
   verb = w[0];
   noun = 0;
   nadj = 0;
   adverb = 0;

   switch( num )
   {
      case 0 :
      break;

      case 1 :
      {
         /* appears to be a shortcut <verb>, eg. n */
         DEBUGF( "case 1\n" );
      }
      break;

      case 2 :
      {
         /* appears to be simple <verb> <noun> e.g. climb tree */
         noun = w[1];
         DEBUGF( "case 2\n" );
      }
      break;

      case 3 :
      {
         /* either we have <verb> <nadj> <noun>, eg. open blue door
          * or we have <verb> <noun> <adverb>, eg. move lever left */

         /* test if w1 (2nd word) is a recognised object/avatar */
         ncode = adv__encode_name( w[1] );
         n = find_noun_as_avatar( ncode, 0, _TRUE );
         if( NAME_ENDS == n )
         {
            p = find_noun_as_object_in_world( ncode, 0 );
            if( p )
            {
               /* appears to be <verb> <noun> <adverb> e.g. move door left */
               noun = w[1];
               adverb = w[2];
               DEBUGF( "case 3a\n" );
            }
            else
            {
               /* appears to be <verb> <nadj> <noun> e.g. open blue door */
               nadj = w[1];
               noun = w[2];
               DEBUGF( "case 3b\n" );
            }
         }
         else
         {
            /* appears to be <verb> <avatar> <adverb> e.g. move rat left */
            noun = w[1];
            adverb = w[2];
            DEBUGF( "case 3c\n" );
         }
      }
      break;

      case 4 :
      {
         ncode = adv__encode_name( w[1] );
         n = find_noun_as_avatar( ncode, 0, _TRUE );
         if( NAME_ENDS == n )
         {
            /* appears to be <verb> <nadj> <noun> <adverb> eg. move blue door 
             * left */
            nadj = w[1];
            noun = w[2];
            adverb = w[3];
            DEBUGF( "case 4a\n" );
         }
         else
         {
            /* appears to be <verb> <avatar> <string> e.g. move rat xxx */
            noun = w[1];
            DEBUGF( "case 4b\n" );
         }
      }
      break;

      case 5 :
      default :
      {
         PRINTS _SORRY, _COMMA, _THATS, _TOO, _MANY, _WORDS, _FULLSTOP, 
                _YOULL, _HAVE, _TO, _BE, _MORE, _CONCISE, _FULLSTOP, 
                _NEWLINE END_PRINT
         DEBUGF( "case 5\n" );
      }
      break;
   }
}

static TRISTATE strip_word( UC * w[ 5 ], UCC idx )
{
   TRISTATE result = _FALSE;
   const size_t sz = sizeof( discards )/sizeof( discards[ 0 ]);
   int i;
   int j;

   for( i=0; i < sz; i++ )
   {
         /* discard only exact (whole word) matches */
      if( _TRUE == adv_strcmpi( w[ idx ],( UC* )( discards[ i ])))
      {
         for( j = idx; j < 4; j++ )
         {
            w[ j ]= w[ j+1 ];
         }
         w[ 4 ]= 0;

         result = _TRUE;

         break;
      }
   }

   return( result );
}

/* break up input buffer into separate words */
static void get_words( void )
{
   UC state = 0;
   int i;
   UC *w[5] ={ 0, 0, 0, 0, 0 };
   size_t const szw = sizeof( w )/ sizeof( w[ 0 ]);


   /* seperate input words */
   for( i=0;( i < sizeof( inbuf ))&&( 10 > state ); i++ )
   {
      if( isalnum( inbuf[ i ])
#       if defined( DEBUG )
          ||( '_' == inbuf[ i ])
#       endif
        )
      {
         if( 0 == state )
         {
            w[0]= &inbuf[ i ];
            state = 1;
         }
         else
         if( 2 == state )
         {
            w[1]= &inbuf[ i ];
            state = 3;
         }
         else
         if( 4 == state )
         {
            w[2]= &inbuf[ i ];
            state = 5;
         }
         else
         if( 6 == state )
         {
            w[3]= &inbuf[ i ];
            state = 7;
         }
         else
         if( 8 == state )
         {
            w[4]= &inbuf[ i ];
            state = 9;
         }
      }

      if( ' ' == inbuf[ i ])
      {
         if(( 0 < i )&&( 0 != inbuf[ i - 1 ]))
         {
            state++;
         }
         inbuf[ i ]= 0;
      }
   }

   for( i=0; i < szw; i++ )
   {
      if( w[ i ])
      {
         if( _TRUE == strip_word( w, i ))
         {
            state -= 2;

            /* don't increment i,
             * because the words have been shuffled down */
            --i;
         }
      }
   }

   /* decide verb, noun, adjective, adverb */
   get_verb_and_noun(( state+1 )/2, w );
}

/* encode both verbs and nouns */
static U32 adv__encode_name( UCC * const name )
{
   U32 code = 0;
   size_t const sz = sizeof( code );
   U32 i;


   if( name )
   {
      for( i=0; i < sz; i++ )
      {
         if( 0 == name[ i ])
         {
            break;
         }

         code <<= 8;
         code += tolower( name[ i ]);
      }
   }

   return( code );
}

static char get_vowel_of_object( description_t const *d )
{
   char c;


   if( d->adjective )
   {
      c =( d->adjective >> 24 );
   }
   else
   {
      c =( d->name >> 24 );
   }

   return( c );
}

static index_t find_code( 
      U32 const code,
      code_t const * words,
      size_t const sz,
      U32 idx,
      U32 sub,
      unsigned char const max_state
      )
{
   U32 fidx = 0;
   int state;


   for( state = max_state;( state )&&( idx >= 0 )&&( idx < sz ); )
   {
      DEBUGF( "trying index %ld\n", idx );
         /* hint: words may not be found if verbs is not properly sorted */
      if( code == words[ idx ].value )
      {
         fidx = idx;
         break;
      }
      else
      if( code < words[ idx ].value )
      {
         idx -= sub;
      }
      else
      if( code > words[ idx ].value )
      {
         idx += sub;
      }

      if( 1 < sub )
      {
         sub >>= 1;
      }
      else
      {
         /* try the next few in sequence, because sizeof( verbs ) is not a 
          * power of two and the binary chop falls incomplete.
         */
         state --;
      }
   }

   return( fidx );
}

static index_t find_action( U32 const vcode )
{
   index_t a;
   int const vsz = sizeof( verbs )/ sizeof( verbs[ 0 ]);
   unsigned int vidx = vsz >> 1;
   unsigned int vsub = vidx >> 1;
   unsigned int fidx;


   DEBUGF( "Looking for <verb> '%s' code = 0x%lx\n", verb, vcode );
#  if defined( DEBUG )
   if( __trace )
   {
   /* ( void )printf( "#verbs = %d\n", vsz ); */
   }
#  endif

   fidx = find_code( vcode, &verbs[0], vsz, vidx, vsub, 7 );

   if( fidx )
   {
      a = verbs[ fidx ].idx;
      DEBUGF( "Found <value> 0x%lx <action> %d\n", 
              verbs[ fidx ].value, verbs[ fidx ].idx );
   }
   else
   {
      a = ACTION_NULL;
      DEBUGF( "Not found\n" );
   }

   return( a );
}

static index_t const find_noun_as_avatar( U32C ncode, U32C adj, TRISTATE inCell )
{
   index_t a = NAME_ENDS;
   TRISTATE found = _FALSE;
   int i;


   DEBUGF( "Looking for <ncode> = 0x%lx as avatar\n", ncode );

   for( i = A_WILBURY;( i <= A_CHARLIE )&&( _FALSE == found ); i++ )
   {
      if( ncode == adv_avatar[ i ].input.name )
      {
         if((( _TRUE == inCell )&&
             ( cavatar->location == adv_avatar[ i ].location ))
            ||
             ( _FALSE == inCell ))
         {
            if(( 0 == adj )||( adj == adv_avatar[ i ].input.adjective ))
            {
               /* by type, in the same location */
               found = _TRUE;
            }
         }
      }

      if( ncode == adv__encode_name(( UCC* )&adv_avatar[ i ].name ))
      {
         /* by name, not necessarily in the same location */
         found = _TRUE;
      }

      if( _TRUE == found )
      {
         a = i + 128;   /* set top bit to distinguish avatar index from
                           names[] (as_builtin) index */
      }
   }

   return( a );
}

static index_t const find_noun_as_avatar_rtol( void )
{
   /* if get_verb_and_string( ) is called before this,
    * then noun will be a searchable string;
    * look right-to-left for the (last) avatar identify */

   index_t a = NAME_ENDS;
   U32 len = adv_strlen(( char* )noun );
   U32 ncode;
   int i;


   for( i = len - 1;( 0 <= i )&&( NAME_ENDS == a ); i-- )
   {
      if( ' ' == noun[ i ])
      {
         ncode = adv__encode_name( &( noun[ i+1 ]));
         a = find_noun_as_avatar( ncode, 0, _FALSE );
      }
   }

   return( a );
}

static index_t const find_noun_as_builtin( U32C ncode )
{
   index_t a;
   int const nsz = sizeof( names )/ sizeof( names[ 0 ]);
   unsigned int nidx = nsz >> 1;
   unsigned int nsub = nidx >> 1;
   unsigned int fidx;


   DEBUGF( "Looking for <noun> '%s' code = 0x%lx as builtin\n", noun, ncode );
#  if defined( DEBUG )
   if( __trace )
   {
      /* DEBUGF( "#names= %d\n", nsz ); */
   }
#  endif

   fidx = find_code( ncode, &names[0], nsz, nidx, nsub, 4 );
   if( fidx )
   {
      a = names[ fidx ].idx;
      DEBUGF( "Found <value> 0x%lx <name> %d\n", 
              names[ fidx ].value, names[ fidx ].idx );
   }
   else
   {
      a = NAME_ENDS;
      DEBUGF( "Not found\n" );
   }

   return( a );
}

/*
 * To find object, look in the cell.
 * This will return HIDDEN objects.
 * If you want to check the inventory, call find_object_with_avatar.
 */
static object_t *find_object_in_cell( U32C ncode, U32C acode, UCC cn )
{
   object_t *p = 0;
   object_t *temp;
   int i;
   cell_t /*const*/ * const c = &( adv_world.cells[ cn ]);


   DEBUGF( "Looking for object 0x%lx in ", ncode );
#  if defined( DEBUG )
      if( __trace )
      {
         print_cell_desc( &( c->output ));
         PRINTS _NEWLINE END_PRINT
      }
#  endif

   /* FIRST search the cell */
   DEBUGF( "Checking cell content\n" );
   for( i=0;( i < MAX_CELL_OBJECTS )&&( 0 == p ); i++ )
   {
      temp = &( c->objects[ i ]);
      if( temp->input.name != ncode )
      {
         if( PROPERTY_CONTAINER & temp->properties )
         {
            temp = &( containers[ temp->current_state.descriptor ]);
         }
      }

      if( temp->input.name == ncode )
      {
            /* There may be multiple <nouns> in any cell;
             * in which case they are distinguished by their adjectives. */
         if(( 0 == acode )||( acode == temp->input.adjective ))
         {
            p = temp;
            DEBUGF( "Found <noun> %s in cell\n", noun ? noun : "null" );
         }
      }
   }

   return( p );
}

/* This function searches the cells only;
 * it does not test the avatars. */
static object_t *find_object_in_world( U32C ncode, U32C adj, UC *cn )
{
   object_t *p = 0;
   cell_t const *c = 0;
   int w;


   DEBUGF( "Looking for <object> 0x%lx adjective 0x%lx, in world\n", ncode, adj );

   for( w=1;( w < C_NAMED_CELLS )&&( 0 == p ); w++ )
   {
      /* see if object lies in this world cell */
      c = &adv_world.cells[ w ];
      if( 0xce11 == c->c_magic )
      {
         p = find_object_in_cell( ncode, adj, w );
         if(( p )&&( cn ))
         {
            *cn = w;
         }
      }
   }

   if(( 0 == p )&&( cn ))
   {
      *cn = 0;
   }

   return( p );
}

static int find_space_in_cell( UC const cn )
{
   int n = -1;
   int i;
   cell_t *c;


   /* check the cell has space for an object */
   c = &( adv_world.cells[ cn ]);
   for( i=0;( i < MAX_CELL_OBJECTS )&&( -1 == n ); i++ )
   {
      if( 0 == c->objects[ i ].output.name )
      {
         n = i;
      }
   }

   return( n );
}

/* search current cell ways,
 * see if a cell name matches the noun, and return its direction;
 * if multiple cels match, display 'select n,s,e,w' */
static direction_t find_noun_as_cell_in_current( U32C ncode, U32C acode )
{
   direction_t w = 0;
   int i;
   direction_t j;


   if( ncode )
   {
      DEBUGF("Looking for code=0x%lx, 0x%lx as cell in current\n", ncode,
             acode);

      for( i = NAME_NORTH; i <= NAME_DOWN; i++ )
      {
         j = adv_world.cells[ cavatar->location ].ways[ i - 1 ];
         if(( 0 < j )&&( ncode == adv_world.cells[ j ].input.name ))
         {
            if( acode == adv_world.cells[ j ].input.adjective )
            {
               w = i;
               break;
            }
            else
            if( 0 == w )
            {
               w = i;
            }
            else
            {
               /* multiple possible solutions */
               w = -1;
               break;
            }
         }
      }

      /* see if it is the current cell */
      if( 0 == w )
      {
         j = cavatar->location;
         if( ncode == adv_world.cells[ j ].input.name )
         {
            if( acode == adv_world.cells[ j ].input.adjective )
            {
               /* is current cell; so indicate no move */
               w = -1;
            }
         }
      }
   }

   return( w );
}

/*
 * to find <noun> object, look in the current cell, and the avatar inventory.
 * will not retrieve hidden objects.
 */
static object_t const *find_noun_as_object_in_current( U32C ncode, U32C acode )
{
   object_t const *p = 0;


   DEBUGF( "Looking for <noun> '%s' code=0x%lx in current\n", noun, ncode );

   /* first see if object is held by avatar */
   p =( object_t const * )find_object_with_avatar( ncode, acode );

   if( 0 == p )
   {
      /* secondly, see if (visible) object lies in the current world cell */
      DEBUGF( "Checking current cell %d\n", cavatar->location );
      p = find_noun_in_cell( ncode, acode, cavatar->location );
   }

   return( p );
}

/* like test_inventory, but also checks containers;
 * only for cavatar. If you want to test all avatars, call
 * find_noun_as_object_with_avatars( ) */
static object_t *find_object_with_avatar( U32C ncode, U32C acode )
{
   object_t *p = 0;
   int i;
   object_t *temp;


   if( ncode )
   {
      DEBUGF("Looking for <noun> '%s' code=0x%lx in current\n", noun, ncode);

      /* first check the cavatar inventory */
      DEBUGF( "Checking cavatar inventory\n" );
      for( i = 0; i < MAX_AVATAR_OBJECTS; i++ )
      {
         temp = &( cavatar->inventory[ i ]);
         if( temp->input.name != ncode )
         {
            if( PROPERTY_CONTAINER & temp->properties )
            {
               temp = &( containers[ temp->current_state.descriptor ]);
            }
         }

         if( temp->input.name == ncode )
         {
            if(( 0 == acode )||( acode == temp->input.adjective ))
            {
               p = temp;
               DEBUGF( "Found <noun> %s in avatar inventory\n", noun );
               break;
            }
         }
      }
   }

   return( p );
}

/*
 * find <noun> object in any world cell
 * this is about object recognition and so it MUST retrieve hidden objects.
 */
static object_t const *find_noun_as_object_in_world( U32C ncode, U32C acode )
{
   object_t const *p = 0;
   cell_t const *c = 0;
   int w;


   if( ncode )
   {
      DEBUGF( "Looking for <noun> '%s' code=0x%lx in world\n", noun, ncode );

      for( w = 1;( w < C_NAMED_CELLS )&&( 0 == p ); w++ )
      {
         /* see if object lies in this world cell */
         c = &adv_world.cells[ w ];
         if( 0xce11 == c->c_magic )
         {
            p = find_object_in_cell( ncode, acode, w );
            if( 0 == p )
            {
               p = find_noun_as_object_with_avatars( ncode, acode, w );
            }
         }
      }
   }

   return( p );
}

static object_t const *find_noun_as_object_with_avatars( 
      U32C ncode, 
      U32C acode,
      UCC cn
      )
{
   object_t *p = 0;
   object_t *temp;
   int a;
   int i;


   if( ncode )
   {
      DEBUGF( "Checking avatar inventories\n" );
      for( a = A_WILBURY;( a <= A_CHARLIE )&&( 0 == p ); a++ )
      {
         if( adv_avatar[ a ].location == cn )
         {
            for( i = 0;( i < MAX_AVATAR_OBJECTS )&&( 0 == p ); i++ )
            {
               temp = &( adv_avatar[ a ].inventory[ i ]);
               if( temp->input.name != ncode )
               {
                  if( PROPERTY_CONTAINER & temp->properties )
                  {
                     temp = &( containers[ temp->current_state.descriptor ]);
                  }
               }

               if( temp->input.name == ncode )
               {
                  if(( 0 == acode )||( acode == temp->input.adjective ))
                  {
                     p = temp;
                     DEBUGF( "Found object 0x%lx with avatar %s\n",
                             ncode, adv_avatar[ a ].name );
                  }
               }
            }
         }
      }
   }

   return( p );
}

static UC find_noun_as_cell_in_world( U32C ncode )
{
   UC found = 0;
   cell_t *c = 0;
   int w;


   if( ncode )
   {
      DEBUGF( "Looking for <noun> '%s' code=0x%lx in world\n", noun, ncode );

      for( w = 1;( w < C_NAMED_CELLS )&&( _FALSE == found ); w++ )
      {
         /* see if object lies in this world cell */
         c = &adv_world.cells[ w ];
         if( 0xce11 == c->c_magic )
         {
            if( ncode == c->input.name )
            {
               found = w;
            }
         }
      }
   }

   return( found );
}

/* same as find_object_in_cell, but will not return HIDDEN objects:
 * to find object, look in the cell, 
 * and if not found then try the avatar inventory.
 * If you want to check the inventory first, call
 * find_noun_as_object_with_avatars()
 */
static object_t const *find_noun_in_cell( U32C ncode, U32C acode, UCC cn )
{
   object_t const *p = 0;
   object_t *temp;
   int i;
   cell_t /*const*/ * const c = &( adv_world.cells[ cn ]);


   if( ncode )
   {
      DEBUGF( "Looking for object 0x%lx in ", ncode );
#  if defined( DEBUG )
      if( __trace )
      {
         print_cell_desc( &( c->output ));
         PRINTS _NEWLINE END_PRINT
      }
#  endif

      /* first check the cell */
      for( i = 0;( i < MAX_CELL_OBJECTS )&&( 0 == p ); i++ )
      {
         temp = &( c->objects[ i ]);
         if( temp->input.name != ncode )
         {
            if( PROPERTY_CONTAINER & temp->properties )
            {
               temp = &( containers[ temp->current_state.descriptor ]);
            }
         }

         if( temp->input.name == ncode )
         {
            /* There may be multiple <nouns> in any cell;
             * in which case they are distinguished by their adjectives. */
            if(( 0 == acode )||( acode == temp->input.adjective ))
            {
               if( 0 ==( temp->properties & PROPERTY_HIDDEN ))
               {
                  p = temp;
                  DEBUGF( "Found <noun> %s in cell\n", noun );
               }
            }
         }
      }

      if( 0 == p )
      {
         /* second check the avatars inventory */
         p = find_noun_as_object_with_avatars( ncode, acode, cn );
      }
   }

   return( p );
}

/* Allowed action:
 *   _TRUE - yes
 *   _FALSE - no
 *   _UNDECIDED - not in current state
 *
 * This function tests the object ACTIONS, PRE-STATE descriptors and may
 * apply the object POST-STATE descriptor.
 *
 * Always test whether an action is allowed on an object.
 *
 * But conditionally check whether to apply the action. BEWARE - if the call
 * to test_and_set_verb is not guarded (for example do you have the correct keys
 * to unlock a door?) then you do not want to set the condition. Setting the
 * action has no apparent side-effect if the object descriptor post-state for 
 * this action is 0; but the game logic could still be wrong.
 */
static TRISTATE test_and_set_verb( 
      object_t * const obj, 
      enum _action_t const actn,
      TRISTATE allowed
      )
{
   int i;
   TRISTATE found;


   for( i=0, found = _FALSE;
        ( i < MAX_OBJECT_ACTIONS )&&( _FALSE == found ); 
        i++ )
   {
      if( obj->actions[ i ]== actn )
      {
         DEBUGF( "Action %d is allowed\n", actn );
         found = _TRUE;

         /* check condition on action */
         DEBUGF( "checking pre-condition\n" );
         if( obj->pre_cond[ i ].descriptor )
         {
            if(( obj->pre_cond[ i ].value == 
                 obj->current_state.value )&&
               ( obj->pre_cond[ i ].descriptor == 
                 obj->current_state.descriptor ))
            {
               /* action permitted in this state */
               allowed = _TRUE;
            }
            else
            {
               /* action not permitted in this state */
               allowed = _UNDECIDED;
            }
         }
         else
         {
            /* no pre-condition on action */
            allowed = _TRUE;
         }

         if( _TRUE == allowed )
         {
            /* apply action to this object */
            if( obj->post_state[ i ].descriptor )
            {
               obj->current_state = obj->post_state[ i ];
            }
         }
      }
   }

   return( allowed );
}

static TRISTATE container_test( object_t const * const test_val, int *n )
{
   TRISTATE result = _FALSE;
   int i;
   int can;


   *n = -1;

   for( i=0; ( i<MAX_AVATAR_OBJECTS )&&( _FALSE == result ); i++ )
   {
      if( PROPERTY_CONTAINER & cavatar->inventory[ i ].properties )
      {
         /* all other containers are either full or empty */
         can = cavatar->inventory[ i ].current_state.descriptor;
         if(( test_val->output.name == containers[ can ].output.name )&&
            ( test_val->output.adjective == 
              containers[ can ].output.adjective ))
         {
            result = _TRUE;
            *n = i;
         }
         else
         {
            /* it's not the specified contents ( 0 != test_val ) */
         }
      }
   }

   return( result );
}

   /* fix src to location */
static void make_fix( object_t *src, object_t *loc )
{
   if(( src )&&( loc ))
   {
      loc->current_state.value = src->output.name;
      src->current_state.descriptor |= STATE_FIXED;
   }
}

static void break_fix( object_t *obj )
{
   object_t *p;
   UC cn;
   int i;


   p = find_object_in_cell( obj->output.name, obj->output.adjective, 
                            cavatar->location );
   if( 0 == p )
   {
      p = find_object_in_world( obj->output.name, obj->output.adjective, &cn );
   }
   else
   {
      cn = cavatar->location;
   }

   if( 0 < cn )
   {
      for( i=0; i < MAX_CELL_OBJECTS ; i++ )
      {
         p = &adv_world.cells[ cn ].objects[ i ];
         if(( p )&&( PROPERTY_FIXPOINT & p->properties )&&
            ( p->current_state.value == obj->output.name ))
         {
            p->current_state.value = 0;
            obj->current_state.descriptor &=( ~STATE_FIXED | STATE_MASK__ );
            break;
         }
      }
   }
}

static TRISTATE test_fix( object_t const *src, object_t const *loc )
{
   TRISTATE res = _FALSE;

   if(( src )&&( loc ))
   {
      if(( loc->current_state.value == src->output.name )&&
         ( STATE_FIXED ==( STATE_FIXED & src->current_state.descriptor )))
      {
         res = _TRUE;
      }
   }

   return( res );
}

static void find_selected( object_t **obj )
{
   int i;


   /* see if any object is selected */
   test_inventory_selected( cavatar, &i );
   if( 0 <= i )
   {
      *obj = &( cavatar->inventory[ i ]);
   }
   else
   {
      test_cell_selected( &i );
      if( 0 <= i )
      {
         *obj = &( adv_world.cells[ cavatar->location ].objects[ i ]);
      }
   }
}

static int test_inventory( avatar_t *a, U32C name, U32C adj, int *invidx )
{
   int i;
   int cnt = 0;

      /* see also find_object_with_avatar( U32C ncode, U32C acode ),
       * like test_inventory, but searches cavatar for containers,
       * and returns a pointer to the found object. */

   /* indicate nothing found initially */
   *invidx = -1;

   /* check the avatar inventory contains this object */
   for( i=0;( i < MAX_AVATAR_OBJECTS ); i++ )
   {
      if(( a->inventory[ i ].input.name == name )&&
         (( a->inventory[ i ].input.adjective == adj )||( 0 == adj )))
      {
         if( 0 > *invidx )
         {
            /* return the first matching object */
            *invidx = i;
         }

         /* count the number of matching objects */
         cnt++;
      }
   }

   return( cnt );
}

static void test_inventory_free_space( 
      avatar_t *a, 
      int *invidx
      )
{
   int i;


   /* indicate no space found initially */
   *invidx = -1;

   /* check the avatar inventory has free space */
   for( i=0;( i < MAX_AVATAR_OBJECTS ); i++ )
   {
      if(( a->inventory[ i ].input.name == 0 )&&
         ( a->inventory[ i ].input.adjective == 0 ))
      {
         *invidx = i;

         break;
      }
   }
}

/* test_inventory_worn can be called iteratively, 
 * to get each item worn.
 * Initially *invidx should be set to -1 */
static void test_inventory_worn( avatar_t *a, int *invidx )
{
   int i;
   int start =( *invidx + 1 );


   if( 0 > start )
   {
      start = 0;
   }

   /* indicate nothing found initially */
   *invidx = -1;

   /* check the avatar inventory contains this object */
   for( i= start;( i < MAX_AVATAR_OBJECTS ); i++ )
   {
      /* find any currently selected object */
      if( PROPERTY_WORN & a->inventory[ i ].properties )
      {
         *invidx = i;
         break;
      }
   }
}

static void test_inventory_selected( avatar_t *a, int *invidx )
{
   int i;


   /* indicate nothing found initially */
   *invidx = -1;

   /* check the avatar inventory contains this object */
   for( i=0;( i < MAX_AVATAR_OBJECTS ); i++ )
   {
      /* find any currently selected object */
      if( PROPERTY_SELECTED & a->inventory[ i ].properties )
      {
         *invidx = i;  /* m == 0 if none is found, but no matter */
         break;
      }
   }
}

static void set_inventory_selected( avatar_t *a, object_t *obj )
{
   int i;
   int n = -1;


   /* check the avatar inventory contains a selected object */
   for( i=0;( i < MAX_AVATAR_OBJECTS ); i++ )
   {
      /* find any currently selected object */
      if( PROPERTY_SELECTED & a->inventory[ i ].properties )
      {
         /* un-select it */
         a->inventory[ i ].properties &= ~PROPERTY_SELECTED;
         break;
      }

      if(( obj )&&( &( a->inventory[ i ])== obj ))
      {
         n = i;
      }
   }

   if( 0 <= n )
   {
      /* now select the requested object */
      a->inventory[ n ].properties |= PROPERTY_SELECTED;
   }
}

static TRISTATE give_object( object_t *obj )
{
   TRISTATE allowed = _FALSE;
   cell_t *c;
   object_t *p;
   int n = -1;


   if( obj )
   {
      n = find_space_in_cell( cavatar->location );
      if( 0 <= n )
      {
         /* special cases */
         DEBUGF( "Test for wearing goggles\n" );
         /* test if wilbury is wearing the snow goggles */
         if( GOGGLES == obj->output.name )
         {
            /* wilbury is no longer wearing the goggles,
             * so hide mountain peak door */
            p = find_object_in_cell(
                    ((( U32 )( 'd' )<< 24 )+(( U32 )( 'o' )<< 16 )+(( U32 )( 'o' )<< 8 )+(( U32 )( 'r' )<< 0 )), 0, 
                    C_MOUNTAIN_PEAK );
            p->properties |= PROPERTY_HIDDEN;
         }

         /* make sure the object isn't selected */
         obj->properties &= ~PROPERTY_SELECTED;

         /* make sure the object isn't worn */
         obj->properties &= ~PROPERTY_WORN;

         c = &( adv_world.cells[ cavatar->location ]);

         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _IS, _LEFT, _IN, _THE END_PRINT
         print_cell_desc( &( c->output ));
         PRINTS _NEWLINE END_PRINT

         /* LASTLY
          * move this object into the world cell and outof the 
          * avatar inventory */
         CLEAR_AVATAR_INVENTORY( cavatar, c->objects[ n ], obj );

         /* obj = EMPTY */

         allowed = _TRUE;
      }
      else
      {
         PRINTS _I, _CANNOT END_PRINT
         SPRINT_( outbuf, "%s", verb );
         PRINTS _THE END_PRINT
         print_object_desc( &( obj->output ));
         PRINTS _FULLSTOP, _THERE, _IS, _NO, _SPACE, _FOR, _IT, _FULLSTOP, 
             _NEWLINE END_PRINT
      }
   }

   /* If the object is a container, the contents remain within */
   return( allowed );
}

static TRISTATE take_object( object_t *obj )
{
   TRISTATE allowed = _FALSE;
   int i;
   int n = -1;
   int m;
   int can = -1;
   TRISTATE found = _FALSE;


      /* check the avatar inventory has space for this object */
   test_inventory_free_space( cavatar, &n );
   if( 0 <= n )
   {
      /* the object could be a container, with an embedded object */
      allowed = _TRUE;
   }

   DEBUGF( "Testing precond spec\n" );
   if( _TRUE == allowed )
   {
      /* By default ACTION_TAKE can be applied to any object, so there
       * is no need to specify it in actions[].
       * However, some objects require a precondition (for example the
       * avatar inventory must include a container in order to store 
       * water) before the object can be taken. So check the 
       * actions[] and associated pre_cond[].
       */
      for( i=0;( i < MAX_OBJECT_ACTIONS )&&( _FALSE == found ); i++ )
      {
         if( obj->actions[ i ]== ACTION_TAKE )
         {
            DEBUGF( "ACTION_TAKE requires a pre-condition\n" );
            found = _TRUE;

            /* check condition on action */
            switch( obj->pre_cond[ i ].type )
            {
               case TEST_PRECOND :
               {
                  DEBUGF( "Testing Precond\n" );
                  if( obj->pre_cond[ i ].descriptor )
                  {
                     if(( obj->pre_cond[ i ].value == 
                          obj->current_state.value )&&
                        ( obj->pre_cond[ i ].descriptor == 
                          obj->current_state.descriptor ))
                     {
                        /* action permitted in this state */
                     }
                     else
                     {
                        allowed = _FALSE;

                        /* action not permitted in this state */
                        PRINTS _I, _CANNOT, _CURRENTLY, _CARRY, _THE
                               END_PRINT
                        print_object_desc( &( obj->output ));
                        PRINTS _NEWLINE END_PRINT
                     }
                  }
                  else
                  {
                     /* no pre-condition on action */
                  }
               }
               break;

               case TEST_CONTAINER :
               {
                  DEBUGF( "Testing container\n" );
                  allowed = container_test( &empty_object, &can );
                  if( _FALSE == allowed )
                  {
                     PRINTS _I, _CANNOT, _CARRY, _THE END_PRINT
                     print_object_desc( &( obj->output ));
                     PRINTS _FULLSTOP, _I, _DONT, _HAVE, _AN, _EMPTY,
                            _CONTAINER, _FULLSTOP, _NEWLINE END_PRINT
                  }
                  else
                  {
                     /* will populate can with obj below */
                  }
               }
               break;

               /* specific container & contents test */
               case TEST_CONTAINER_SPECIAL :
               {
                  DEBUGF( "Testing container spec\n" );
                  if( SOURCE == obj->output.name )
                  {
                     /* check the inventory for ceraminc crucible */
                     ( void )test_inventory( 
                                 cavatar,
                                 ((( U32 )( 'c' )<< 24 )+(( U32 )( 'r' )<< 16 )+(( U32 )( 'u' )<< 8 )+(( U32 )( 'c' )<< 0 )), 
                                 ((( U32 )( 'c' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'r' )<< 8 )+(( U32 )( 'a' )<< 0 )), 
                                 &m );
                  }
                  else
                  if( HEAVY == obj->output.adjective )
                  {
                     /* check the inventory for lead flask */
                     ( void )test_inventory( 
                                 cavatar,
                                 ((( U32 )( 'f' )<< 24 )+(( U32 )( 'l' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 's' )<< 0 )),
                                 ((( U32 )( 'l' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'a' )<< 8 )+(( U32 )( 'd' )<< 0 )),
                                 &m );
                  }

                  if( 0 > m )
                  {
                     allowed = _FALSE;
                     PRINTS _I, _AM, _NOT, _CARRYING, _A, _SUITABLE,
                              _CONTAINER, _FULLSTOP, _NEWLINE END_PRINT
                  }
                  else
                  {
                     /* check container at inventory 'm' is empty;
                      * container_test( 0, m ); */
                     can = cavatar->inventory[ m ].current_state.descriptor;
                     if( OBJECT_NULL != containers[ can ].output.name )
                     {
                        allowed = _FALSE;
                        PRINTS _THE, _REQUIRED, _CONTAINER, _COMMA 
                               END_PRINT
                        print_object_desc( 
                                    &( cavatar->inventory[ m ].output ));
                        PRINTS _COMMA, _IS, _NOT, _EMPTY, _FULLSTOP,
                                       _NEWLINE END_PRINT
                     }
                     else
                     {
                        /* will populate can with obj below;
                         * point at avatar inventory, not container list */
                        can = m;
                     }
                  }
               }
               break;

               default:
               {
                  DEBUGF( "UNKNOWN PRE-CONDITION" );
                  allowed = _FALSE;
               }
               break;
            }

            if( _TRUE == allowed )
            {
               /* apply action to this object */
               if( obj->post_state[ i ].descriptor )
               {
                  obj->current_state = obj->post_state[ i ];
               }
            }
         }
      }
   }

   if( _TRUE == allowed )
   {
      /* specific object tests */
      if( STATE_FIXED ==( STATE_FIXED & obj->current_state.descriptor ))
      {
         /* break fix relationship */
         break_fix( obj );
      }

      /* LASTLY
       * move this object outof the world cell and into the 
       * avatar inventory;
       * potentially the avatar is carrying multiple similar objects 
       * e.g. 2 glass test-tubes for energy */
      if( 0 <= can )
      {
         /* put the object into the required container */
         can = cavatar->inventory[ can ].current_state.descriptor;
         containers[ can ]= *obj;
         containers[ can ].properties &= ~PROPERTY_PERPETUAL;

         if( 0 ==( PROPERTY_PERPETUAL & obj->properties ))
         {
            *obj = empty_object;
         }
      }
      else
      {
         /* add the object to the avatar inventory */
         SET_AVATAR_INVENTORY( cavatar, &( cavatar->inventory[ n ]), *obj );
      }
   }         

   return( allowed );
}

static void test_cell_selected( int *idx )
{
   int i;
   int const cn = cavatar->location;


   *idx = -1;

   /* check if the cell contains a selected object */
   for( i=0;( i < MAX_CELL_OBJECTS ); i++ )
   {
      if( PROPERTY_SELECTED & adv_world.cells[ cn ].objects[ i ].properties )
      {
         *idx = i;
         break;
      }
   }
}

static void set_cell_selected( object_t * const obj )
{
   int i;
   int const cn = cavatar->location;


   /* check if the cell contains a selected object */
   for( i=0;( i < MAX_CELL_OBJECTS ); i++ )
   {
      if( PROPERTY_SELECTED & 
          adv_world.cells[ cn ].objects[ i ].properties )
      {
         /* un-select it */
         adv_world.cells[ cn ].objects[ i ].properties &= ~PROPERTY_SELECTED;
         break;
      }
   }

   /* make sure nothing in avatar inventory is selected */
   set_inventory_selected( cavatar, 0 );

   if( obj )
   {
      /* now select the requested object */
      obj->properties |= PROPERTY_SELECTED;
   }
}

/* There are some checks that need to be done periodically.
 * Since there is no clock, we just do them on certain events.
 */
static void do_periodic_checks( void )
{
   int i = -1;


   DEBUGF( "Perform periodic checks\n" );
   /* check if the crow needs to fly back to the treetops */
   if( C_NULL == adv_avatar[ A_LUCY ].location )
   {
      adv_avatar[ A_LUCY ].location = C_FOREST_CANOPY;
   }

   /* check if a rat needs to head on back to the dungeon */
   if( C_NULL == adv_avatar[ A_GARY ].location )
   {
      adv_avatar[ A_GARY ].location = C_CASTLE_DUNGEON;
   }

   if( C_NULL == adv_avatar[ A_OTTO ].location )
   {
      adv_avatar[ A_OTTO ].location = C_MOUNTAIN_LAB;
   }

   /* check if the bat needs to head on back to the dungeon */
   if( C_NULL == adv_avatar[ A_HARRY ].location )
   {
      adv_avatar[ A_HARRY ].location = C_MINE_TUNNEL;
   }

   /* check if the mole needs to head on back to the mine */
   if( C_NULL == adv_avatar[ A_BUSTER ].location )
   {
      adv_avatar[ A_BUSTER ].location = C_MAGNOTRON_COMPLEX;
   }
   else
   if(( 1 == adv_avatar[ A_BUSTER ].current_state.descriptor )&&
      ( test_inventory_worn( &adv_avatar[ A_BUSTER ], &i ),( 0 > i )))
   {
      /* mole stops roaming the forest and returns to magnotron complex */
      adv_avatar[ A_BUSTER ].current_state.descriptor = 0;
      adv_avatar[ A_BUSTER ].location = C_MAGNOTRON_COMPLEX;
   }
}

static void do_avatar_mobile_checks( TRISTATE const force_move )
{
   static int i = 0;
   direction_t way;
   cell_t *c;
   int n;
   int m;


   /* decide if nelson the gnome should move around */
   if(( 0 == adv_avatar[ A_NELLY ].current_state.descriptor )&&
      (( _TRUE == force_move )||( 0 ==( ++i % 3 )))&&
      ( cavatar->location != adv_avatar[ A_NELLY ].location ))
   {
      /* Nelly's current_state.value is the "random" selector in deciding
       * which way to go. */
      adv_avatar[ A_NELLY ].current_state.value++;
      adv_avatar[ A_NELLY ].current_state.value %= 6;
      
      if( C_CASTLE_THRONEROOM == cavatar->location )
      {
         way = C_CASTLE_THRONEROOM;
      }
      else
      if( C_CASTLE_DUNGEON == adv_avatar[ A_NELLY ].location )
      {
         way = C_CASTLE_CELLAR;
      }
      else
      if( C_CASTLE_CELLAR == adv_avatar[ A_NELLY ].location )
      {
         way = C_CASTLE_THRONEROOM;
      }
      else
      {
         way = adv_world.cells[ adv_avatar[ A_NELLY ].location ].
                 ways[ adv_avatar[ A_NELLY ].current_state.value ];
      }

      if( way )
      {
         if( 0 > way )
         {
            way = -way;
         }

         adv_avatar[ A_NELLY ].location = way;

         if(( way == cavatar->location )&&( &adv_avatar[ A_NELLY ]!= cavatar ))
         {
            if( 0 == adv_avatar[ A_NELLY ].current_state.value )
            {
               /* periodically let wilbury know nelson is in the room */
               PRINTS _I, _HEAR, _SOME, _MUTTERING, _ELIPSIS, _NEWLINE END_PRINT
            }
         }
      }
   }
   else
   if( 1 == adv_avatar[ A_NELLY ].current_state.descriptor )
   {
      /* Nelson needs to steer wilbury to the throne & control room;
       * so encourage wilbury to follow nelson to the throneroom. */
      if(( cavatar->location == adv_avatar[ A_NELLY ].location )&&
         ( &adv_avatar[ A_NELLY ]!= cavatar ))
      {
         SPRINT_( outbuf, "%s", adv_avatar[ A_NELLY ].name );
         PRINTS _SEES, _ME, _AND_, _SAYS, _QUOTE_LEFT, _FOLLOW, _ME, 
                _QUOTE_RIGHT, _COMMA END_PRINT

         if( C_CASTLE_COURTYARD == cavatar->location )
         {
            adv_avatar[ A_NELLY ].location = C_CASTLE_COMMONROOM;
         }
         else
         if( C_CASTLE_COMMONROOM == cavatar->location )
         {
            adv_avatar[ A_NELLY ].location = C_CASTLE_THRONEROOM;
            adv_avatar[ A_NELLY ].current_state.descriptor = 0;
         }

         c = &( adv_world.cells[ adv_avatar[ A_NELLY ].location ]);
         PRINTS _AND_, _GOES, _INTO, _THE END_PRINT
         print_cell_desc( &( c->output ));
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
   }

   /* check if nelson should give the remote to wilbury */
   if(( adv_avatar[ A_NELLY ].location == C_CASTLE_THRONEROOM )&&
      ( &adv_avatar[ A_NELLY ]!= cavatar )&&
      ( 1 >= distance( cavatar->location, adv_avatar[ A_NELLY ].location )))
   {
      ( void )test_inventory( 
                     &adv_avatar[ A_NELLY ], 
                     ((( U32 )( 'r' )<< 24 )+(( U32 )( 'e' )<< 16 )+(( U32 )( 'm' )<< 8 )+(( U32 )( 'o' )<< 0 )), 0, &n );
      m = find_space_in_cell( C_CASTLE_THRONEROOM );
      if(( 0 <= n )&&( 0 <= m )&&
         ( 1 == adv_avatar[ A_NELLY ].inventory[ n ].current_state.value ))
      {
         if(( C_CASTLE_THRONEROOM == cavatar->location )||
            ( C_CASTLE_COMMONROOM == cavatar->location ))
         {
            SPRINT_( outbuf, "%s", adv_avatar[ A_NELLY ].name );
            PRINTS _THE END_PRINT
            print_object_desc( &adv_avatar[ A_NELLY ].output );
            PRINTS _DROPS, _SOMETHING, _TO, _THE, _FLOOR, _FULLSTOP,
                     _NEWLINE END_PRINT
         }

         /* leave remote whether or not wilbury is here */
         adv_avatar[ A_NELLY ].inventory[ n ].current_state.value = 2;
         c = &( adv_world.cells[ C_CASTLE_THRONEROOM ]);
         CLEAR_AVATAR_INVENTORY( 
               &adv_avatar[ A_NELLY ], 
               c->objects[ m ], 
               &adv_avatar[ A_NELLY ].inventory[ n ]);
         /* obj = EMPTY */
      }
   }

   /* decide if gary should move around */
   if(( 0 == adv_avatar[ A_GARY ].current_state.descriptor )&&
      ( &adv_avatar[ A_GARY ]!= cavatar )&&
      ( cavatar->location != adv_avatar[ A_GARY ].location ))
   {
      adv_avatar[ A_GARY ].current_state.value++;
      adv_avatar[ A_GARY ].current_state.value %= 6;
      
      way = adv_world.cells[ adv_avatar[ A_GARY ].location ].
              ways[ adv_avatar[ A_GARY ].current_state.value ];
      if( way )
      {
         if( 0 > way )
         {
            way = -way;
         }

         adv_avatar[ A_GARY ].location = way;

         if(( way == cavatar->location )&&( &adv_avatar[ A_GARY ]!= cavatar ))
         {
            /* let wilbury know rat is in the room */
            PRINTS _I, _HEAR, _SOME, _SCRATCHING, _ELIPSIS, _NEWLINE END_PRINT
         }
      }
   }

   /* decide if harry should move around */
   if(( 0 == adv_avatar[ A_HARRY ].current_state.descriptor )&&
      ( &adv_avatar[ A_HARRY ]!= cavatar )&&
      ( cavatar->location != adv_avatar[ A_HARRY ].location ))
   {
      if(( C_MINE_TUNNEL == cavatar->location )&&
         ( C_MINE_TUNNEL != adv_avatar[ A_HARRY ].location ))
      {
         adv_avatar[ A_HARRY ].location = C_MINE_TUNNEL;
      }
   }

   /* decide if buster should move around */
   if(( 1 == adv_avatar[ A_BUSTER ].current_state.descriptor )&&
      ( &adv_avatar[ A_BUSTER ]!= cavatar )&&
      ( cavatar->location != adv_avatar[ A_BUSTER ].location ))
   {
      /* Buster's current_state.value is the "random" selector in deciding
       * which way to go. */
      adv_avatar[ A_BUSTER ].current_state.value++;
      adv_avatar[ A_BUSTER ].current_state.value %= 6;
      
      way = adv_world.cells[ adv_avatar[ A_BUSTER ].location ].
                 ways[ adv_avatar[ A_BUSTER ].current_state.value ];
      if( way )
      {
         if( 0 > way )
         {
            way = -way;
         }

         adv_avatar[ A_BUSTER ].location = way;

         if(( way == cavatar->location )&&( &adv_avatar[ A_BUSTER ]!= cavatar ))
         {
            if( 0 == adv_avatar[ A_BUSTER ].current_state.value )
            {
               /* periodically let wilbury know nelson is in the room */
               PRINTS _I, _HEAR, _SOME, _SNIFFING, _ELIPSIS, _NEWLINE END_PRINT
            }
         }
      }
   }
}

static ERR check_avatars( void )
{
   ERR e = ERR_none;
   TRISTATE force_move = _FALSE;
   int i;


   /* check wilbury alive */
   if( cavatar->hunger && cavatar->thirst )
   {
      /* check each avatar state */
      for( i = A_WILBURY; i <= A_CHARLIE; i++ )
      {
         if( &( adv_avatar[ i ])== cavatar )
         {
            /* If we decrement other avatars hunger, they end up getting
             * hungry, but what can wilbury do about it? */
            adv_avatar[ i ].hunger--;
         }

         /* check avatar hunger/thirst */
         if(( 16 >( adv_avatar[ i ].hunger ))&&
            ( 0 ==( adv_avatar[ i ].hunger % 5 )))
         {
            if( &( adv_avatar[ i ])== cavatar )
            {
               PRINTS _IM, _GETTING END_PRINT
            }
            else
            {
               SPRINT_( outbuf, "%s", adv_avatar[ i ].name );
               PRINTS _IS, _GETTING END_PRINT
            }

            if( 11 > cavatar->hunger )
            {
               PRINTS _VERY END_PRINT
            }
            PRINTS _HUNGRY, _FULLSTOP, _NEWLINE END_PRINT
         }

         if( &( adv_avatar[ i ])== cavatar )
         {
            /* If we decrement other avatars thirst, they end up getting
             * thirsty, but what can wilbury do about it? */
            adv_avatar[ i ].thirst--;
         }

         if(( 21 >( adv_avatar[ i ].thirst ))&&
            ( 0==( adv_avatar[ i ].thirst % 5 )))
         {
            if( &( adv_avatar[ i ])== cavatar )
            {
               PRINTS _IM, _GETTING END_PRINT
            }
            else
            {
               SPRINT_( outbuf, "%s", adv_avatar[ i ].name );
               PRINTS _IS, _GETTING END_PRINT
            }
            if( 11 > cavatar->thirst )
            {
               PRINTS _VERY END_PRINT
            }
            PRINTS _THIRSTY, _FULLSTOP, _NEWLINE END_PRINT
         }
      }

      if( 0 ==( inputs % 5 ))
      {
         /* perform avatar and worldy-state checks */
         do_periodic_checks( );
      }

      /* see if nelson should be forced to meet wilbury in the throneroom */
      if( C_CASTLE_THRONEROOM == cavatar->location )
      {
         force_move = _TRUE;
      }
      do_avatar_mobile_checks( force_move );
   }
   else
   {
      adv__game_over( GAME_OVER_AVATAR_RESOURCE );
      adv_world.errnum = e = ERR_game;
   }

   return( e );
}

static void swap_avatar( U32C mask )
{
   switch( mask )
   {
      case A_WILBURY :
      case A_LUCY :
      case A_GARY :
      case A_NELLY :
      case A_BUSTER :
      case A_LOFTY :
      case A_OTTO :
      case A_CHARLIE :
      {
         SPRINT_( outbuf, "%s", cavatar->name );
         PRINTS _SAYS, _BYE, _COMMA END_PRINT
         cavatar = &adv_avatar[ mask ];
         SPRINT_( outbuf, "%s", cavatar->name );
         PRINTS _SAYS, _HELLO, _FULLSTOP, _NEWLINE END_PRINT
      }
      break;

      default :
      break;
   }
}

/* we don't need a full graph distance summation;
 * but only a result: 0=same cell, 1=adjacent cell, 2=further afield */
static int distance( location_t const loc_a, location_t const loc_b )
{
   int d = 0;   /* assume same cell */
   cell_t *c;
   int i;


   if( loc_a != loc_b )
   {
      c = &( adv_world.cells[ loc_a ]);
      for( i=0;( i < MAX_WAYS )&&( 0 == d ); i++ )
      {
         if( c->ways[ i ]== loc_b )
         {
            d = 1;
         }
      }

      if( 0 == d )
      {
         d = 2;  /* means not adjacent cell, further away */
      }
   }

   return( d );
}


/*
 * Input is normally of the form <verb> <noun>;
 *  it can however be <verb> <adverb> e.g. go north.
 *
 * Sometimes a command only has meaning in certain states, like the sequence
 *    "select ax", "chop tree" i.e. if you do "chop tree" without "select ax"
 *    the correct response would be "with what?"
 *
 * If it is of the form <verb>, then apply to the avatar.
 *
 * If the <verb> isn't recognised, the response should be 
 *    "I don't know how to do that"
 *
 * If the <noun> isn't recognised, the response should be
 *    "I don't know what <noun> is"
 *
 * If the <noun> is recognised, but not in the current cell, the response is
 *    "I don't see <noun> here"
 */
ERR adv__perform_input( void )
{
   ERR e = ERR_none;
   U32 vcode;
   U32 ncode;
   U32 acode;
   object_t const *p = 0;
   direction_t w = 0;
   UC loc;
   index_t a;
   index_t n;
   TRISTATE acted = _FALSE;


   get_words( );
   if( verb )
   {
      vcode = adv__encode_name( verb );
      a = find_action( vcode );
      if( ACTION_NULL != a )
      {
         /* special cases where <verb> <noun> doesn't apply */
         if( ACTION_SAY == a )
         {
            /* apply <verb,string> to avatar */
            actions[ a ]( vcode, 0, 0, 0 );
            acted = _TRUE;
         }
         else
         if( ACTION_BUILTIN == a )
         {
            /* apply <verb,string> to avatar */
            actions[ a ]( vcode, 0, 0, 0 );
         }
         else
         if( noun )
         {
            ncode = adv__encode_name( noun );
            acode = adv__encode_name( nadj );

            n = find_noun_as_avatar( ncode, acode, _FALSE );
            if( NAME_ENDS == n )
            {
               p = find_noun_as_object_with_avatars( 
                              ncode, acode, cavatar->location );
               if( 0 == p )
               {
                     /* test for noun as object ahead of noun as cell,
                      * because sometimes conditions block entry to a cell,
                      * such as a closed door */
                  p = find_noun_as_object_in_current( ncode, acode );
                  if( 0 == p )
                  {
                     if(( ACTION_GO == a )||( ACTION_LOOK == a ))
                     {
                           /* see if the named cell is adjacent */
                        w = find_noun_as_cell_in_current( ncode, acode );
                     }
                     if( 0 == w )
                     {
                        loc = find_noun_as_cell_in_world( ncode );
                        if( 0 == loc )
                        {
                           p = find_noun_as_object_in_world( ncode, acode );
                           if( 0 == p )
                           {
                              n = find_noun_as_builtin( ncode );
                              if( NAME_ENDS == n )
                              {
                                 PRINTS _I, _DONT, _KNOW, _WHAT END_PRINT
                                 SPRINT_( outbuf, "%s", IS_VOWEL( noun[ 0 ]));
                                 PRINTS _LESS_ END_PRINT
                                 SPRINT_( outbuf, "%s", noun );
                                 PRINTS _MORE_, _IS, _FULLSTOP, _NEWLINE
                                        END_PRINT
                              }
                              else
                              {
                                 /* apply <verb,built-in name> to avatar */
                                 actions[ a ]( vcode, ncode, 0, n );
                                 acted = _TRUE;
                              }
                           }
                           else
                           {
                              if( ACTION_FIND == a )
                              {
                                 actions[ a ]( 
                                       vcode, ncode,( object_t * const )p, 0 );
                              }
                              else
                              {
                                 /* object lies in a different world cell */
                                 PRINTS _I, _DONT, _SEE, _THE END_PRINT
                                 if( nadj )
                                 {
                                    SPRINT_( outbuf, "%s", nadj );
                                 }
                                 SPRINT_( outbuf, "%s", noun );
                                 PRINTS _HERE, _FULLSTOP, _NEWLINE END_PRINT
                              }
                           }
                        }
                        else
                        if( loc == cavatar->location )
                        {
                           PRINTS _I, _LOOK, _AT, _MYSELF, _SUSPICIOUSLY,
                               _ELIPSIS, _ESC_L, _IM, _ALREADY, _THERE, 
                               _FULLSTOP, _NEWLINE END_PRINT
                        }
                        else
                        {
                           /* cell lies elsewhere in the world */
                           PRINTS _I, _DONT, _SEE END_PRINT
                           if( nadj )
                           {
                              SPRINT_( outbuf, "%s", nadj );
                           }
                           SPRINT_( outbuf, "%s", noun );
                           PRINTS _HERE, _FULLSTOP, _NEWLINE END_PRINT
                        }
                     }
                     else
                     if( 0 < w )
                     {
                        /* move/look to cell */
                        actions[ a ]( vcode, 0, 0, w );
                        acted = _TRUE;
                     }
                     else
                     {
                        PRINTS _I, _CANNOT, _DISTINGUISH, _WHERE, _TO, _GO, 
                            _FULLSTOP, _PLEASE, _STATE, _A, _DIRECTION, 
                            _BRACKET_LEFT, _N_, _E_, _S_, _W_, _U_, _D_, 
                            _BRACKET_RIGHT, _FULLSTOP, _NEWLINE END_PRINT
                     }
                  }
                  else
                  {
                     /* apply verb to object (e.g. climb tree) */
                     actions[ a ]( vcode, ncode,( object_t * const )p, 0 );
                     acted = _TRUE;
                  }
               }
               else
               {
                  if( ACTION_TAKE == a )
                  {
                     if( 0 != find_object_with_avatar(
                           p->input.name, p->input.adjective ))
                     {
                        PRINTS _I, _LOOK, _AT, _MYSELF, _SUSPICIOUSLY, 
                               _ELIPSIS, _I, _ALREADY, _HAVE, _IT, _FULLSTOP, 
                               _NEWLINE END_PRINT
                     }
                     else
                     {
                        /* object lies with an avatar */
                        PRINTS _ANOTHER, _AVATAR, _HAS, _THE END_PRINT
                        print_object_desc( &( p->output ));
                        PRINTS _FULLSTOP, _BUT, _YOU, _CAN, _TRY, _ASKING, _FOR,
                               _IT, _FULLSTOP, _NEWLINE END_PRINT
                     }
                  }
                  else
                  {
                     /* apply <verb,built-in name> to avatar-object */
                     actions[ a ]( vcode, ncode,( object_t * const )p, 0 );
                     acted = _TRUE;
                  }
               }
            }
            else
            {
               /* apply <verb> to avatar<name> */
               actions[ a ]( vcode, ncode, 0, n );
               acted = _TRUE;
            }
         }
         else
         {
            /* apply verb to avatar (or BUILTIN) */
            actions[ a ]( vcode, NAME_NULL, 0, 0 );
            acted = _TRUE;
         }
      }
      else
      {
         PRINTS _I, _DONT, _KNOW, _HOW, _TO, _DO, _THAT, _FULLSTOP, _NEWLINE
                END_PRINT
         actions[ ACTION_BUILTIN ]( 0x68656c70 /* help */, NAME_NULL, 0, 0 );
      }
   }

   /* check resources */
   if( _TRUE == acted )
   {
      e = check_avatars( );
      lightbeam_check( );
      lightbeam_test( );
   }

   return( e );
}

UC adv__get_location( void )
{
   return( cavatar->location );
}



/*************************************************************
 * The following test functions are to support the (java) GUI
 *************************************************************/

/*
 * Input is normally of the form <verb> <noun>;
 *  it can however be <verb> <adverb> e.g. go north.
 */
ERR adv__test_input( TRISTATE *res )
{
   ERR e = ERR_none;
   U32 vcode;
   U32 ncode;
   U32 acode;
   object_t const *p = 0;
   UC loc;
   direction_t w = 0;
   index_t a;
   index_t n;


   *res = _TRUE;

   get_words( );
   if( verb )
   {
      vcode = adv__encode_name( verb );
      a = find_action( vcode );
      if( ACTION_NULL == a )
      {
         /* don't know verb */
         *res = _FALSE;
      }
      else
      if(( ACTION_SAY == a )||( ACTION_BUILTIN == a ))
      {
         /* don't process any more detail, just accept it */
         *res = _TRUE;
      }
      else
      {
         if( noun )
         {
            ncode = adv__encode_name( noun );
            acode = adv__encode_name( nadj );

            n = find_noun_as_avatar( ncode, acode, _FALSE );
            if( NAME_ENDS == n )
            {
               p = find_noun_as_object_with_avatars( 
                              ncode, acode, cavatar->location );
               if( 0 == p )
               {
                     /* test for noun as object ahead of noun as cell,
                      * because sometimes conditions block entry to a cell,
                      * such as a closed door */
                  p = find_noun_as_object_in_current( ncode, acode );
                  if( 0 == p )
                  {
                        /* see if the named cell is adjacent */
                     w = find_noun_as_cell_in_current( ncode, acode );
                     if( 0 == w )
                     {
                        loc = find_noun_as_cell_in_world( ncode );
                        if( 0 == loc )
                        {
                           p = find_noun_as_object_in_world( ncode, acode );
                           if( 0 == p )
                           {
                              n = find_noun_as_builtin( ncode );
                              if( NAME_ENDS == n )
                              {
                                 /* don't know noun */
                                 *res = _FALSE;
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }

   return( e );
}

/*
 * Input is normally of the form <verb> <noun>;
 *  it can however be <verb> <adverb> e.g. go north.
 */
ERR adv__test_input_verb( UC **res )
{
   ERR e = ERR_none;
   U32 vcode;
   index_t a;


   get_words( );
   if( verb )
   {
      vcode = adv__encode_name( verb );
      a = find_action( vcode );
      if( ACTION_NULL == a )
      {
         /* don't know verb */
         *res = NULL;
      }
      else
      {
         *res = verb;
      }
   }

   return( e );
}

/*
 * Input is normally of the form <verb> <noun>;
 *  it can however be <verb> <adverb> e.g. go north.
 */
ERR adv__test_input_adverb( UC **res )
{
   ERR e = ERR_none;

   get_words( );
   *res = adverb;

   return( e );
}

/*
 * Input is normally of the form <verb> <noun>;
 *  it can however be <verb> <adverb> e.g. go north.
 */
ERR adv__test_input_noun( UC **nounres, UC** adjres )
{
   ERR e = ERR_none;
   U32 ncode;
   U32 acode;
   object_t const *p = 0;
   UC loc;
   direction_t w = 0;
   index_t n;


   get_words( );
   if( noun )
   {
      *nounres = noun;
      *adjres = nadj;

      ncode = adv__encode_name(noun);
      acode = adv__encode_name(nadj);

      n = find_noun_as_avatar(ncode, acode, _FALSE);
      if (NAME_ENDS == n)
      {
         p = find_noun_as_object_with_avatars(
                 ncode, acode, cavatar->location);
         if (0 == p)
         {
            /* test for noun as object ahead of noun as cell,
             * because sometimes conditions block entry to a cell,
             * such as a closed door */
            p = find_noun_as_object_in_current(ncode, acode);
            if (0 == p)
            {
               /* see if the named cell is adjacent */
               w = find_noun_as_cell_in_current(ncode, acode);
               if (0 == w)
               {
                  loc = find_noun_as_cell_in_world(ncode);
                  if (0 == loc)
                  {
                     p = find_noun_as_object_in_world( ncode, acode );
                     if (0 == p)
                     {
                        n = find_noun_as_builtin(ncode);
                        if (NAME_ENDS == n)
                        {
                           /* don't know noun */
                           *nounres = 0;
                        }
                     }
                  }
               }
            }
         }
      }
   }

   return( e );
}
