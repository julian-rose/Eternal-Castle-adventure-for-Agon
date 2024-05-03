/*
 * Module: adv_file.c
 *
 * Purpose: File ops for adventure game
 *
 * Author: Copyright (c) julian rose, jhr
 *
 * Version:	who	when	what
 *		      ---	------	----------------------------------------
 *		      jhr	150417	wrote initial version on cygwin
 *				               test data structure ideas
 *	               			test basic command line
 *                16xxxx   ported to Android (native backend to a Java frontend)
 *                240429   port to Agon light (eZ80) with 24-bit ints and 32-bit longs
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#if defined( __clang__ )
# include "mos_api.h"
#else
# include <fcntl.h>
# include <unistd.h>
#endif
#include <ctype.h>

#include "adv_types.h"



/*
 *************************** Global data
 */
extern int __trace;
extern char world_file[];

world_t  adv_world ={ 0 };
avatar_t adv_avatar[ A_NUM_AVATARS ]=
            {{ 0 },{ 0 },{ 0 },{ 0 },{ 0 },{ 0 },{ 0 },{ 0 },{ 0 }};

static data_t world_and_avatar[ ]=
{
   { &adv_world, sizeof( adv_world ), _TRUE },
   { &adv_avatar[ 0 ], sizeof( adv_avatar ), _TRUE }
};

const world_t adv_default_world =
{
   0x01d,             /* W_MAGIC */
   "Eternal Castle",  /* world name */
   {  /* cells */
      {  /* cell[C_NULL] */
         0xce11   /* The NULL CELL
                   * Not to be populated (specially it prevents '0' as a way).
                   */
      },

      {  /* cell[C_TRASH] */
         0xce11   /* The Trash can is where "dead" objects go to rest;
                   * we need to keep them for noun matches. */
      },

      {  /* cell[C_FOREST_GLADE] */
         0xce11,
         { 0, FOREST | GLADE },  /* output */
         {
            ((( U32 )( 'f' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'g' )<<24 )+(( U32 )( 'l' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */
               0x0bec,              /* magic */
               { OAK, TREE },  /* description */
               {
                  ((( U32 )0<<24 )+(( U32 )( 'o' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'k' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */
               0x0bec,              /* magic */
               { BEECH, TREE },     /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'c' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */
               0x0bec,              /* magic */
               { 0, MUSHROOM },      /* description */
               {
                  0,  /* adj */
                  ((( U32 )( 'm' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 'h' )<<0 ))  /* noun */
               },
               PROPERTY_MOBILE,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { C_FOREST, -C_FOREST, -C_FOREST, C_FOREST_GLADE, C_FOREST_CANOPY, 
           C_FOREST_POOL }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_FOREST] */
         0xce11,
         { 0, FOREST },  /* output */
         {
            0,
            (( ( U32 )( 'f' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 )),  /* noun */
         },
         {                /* objects */
            {  /* objects[0] */
               0x0bec,              /* magic */
               { OAK, TREE },     /* output description */
               {
                  ((( U32 )0<<24 )+(( U32 )( 'o' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'k' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */
               0x0bec,              /* magic */
               { ASH, TREE },     /* output description */
               {
                  ((( U32 )0<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 'h' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */
               0x0bec,              /* magic */
               { BEECH, TREE },     /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'c' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { C_FOREST, -C_FOREST, C_DEEP_FOREST, C_FOREST_GLADE, C_FOREST_CANOPY, 
           0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_FOREST_CANOPY] */
         0xce11,
         { 0, FOREST | CANOPY },  /* output */
         {
            (( ( U32 )( 'f' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'o' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, NEST },         /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 'n' )<<24)+(( U32 )( 'e' )<<16)+(( U32 )( 's' )<<8)+(( U32 )( 't' )<<0))  /* noun */
               },
               0,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },   /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },
                { 0, 0, 0 },
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},/* avatar post-states */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { 0, EGGS },         /* output description */
               {                    /* input description */
                  0,   /* adj */
                  ((( U32 )( 'e' )<<24)+(( U32 )( 'g' )<<16)+(( U32 )( 'g' )<<8)+(( U32 )( 's' )<<0))  /* noun */
               },
               PROPERTY_MOBILE,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_TAKE, ACTION_EAT, 0, 0 },   /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },     /* ACTION_EAT */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},/* avatar post-states */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, C_FOREST }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_FOREST_POOL] */
         0xce11,
         { 0, FOREST | POOL },  /* output */
         {
            (( ( U32 )( 'f' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'p' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'l' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, FISH },         /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 'f' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 's' )<<8)+(( U32 )( 'h' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL | PROPERTY_NOFREE,
               { 0, 0 },            /* current state */
               { ACTION_EAT, ACTION_TAKE, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_EAT */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},/* avatar post-states */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { COLD, WATER },        /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 'w' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 't' )<<8)+(( U32 )( 'e' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL | PROPERTY_NOFREE,
               { 0, 0 },            /* current state */
               { ACTION_DRINK, ACTION_TAKE, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_DRINK */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },

         { /* ways (N, E, S, W, U, D) */ 
            C_FOREST_POOL, -C_FOREST_POOL, -C_FOREST_POOL, -C_FOREST_POOL, 
            C_FOREST_GLADE, C_FOREST_POOL_FLOOR
         }
      },

      {  /* cell[C_FOREST_POOL_FLOOR] */
         0xce11,
         { DEEP, POOL | FLOOR },       /* output */
         {
            ((( U32 )( 'p' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'l' )<<0 )),  /* adj */
            (( ( U32 )( 'f' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'o' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, FISH },         /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 'f' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 's' )<<8)+(( U32 )( 'h' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL | PROPERTY_NOFREE,
               { 0, 0 },            /* current state */
               { ACTION_EAT, ACTION_TAKE, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_EAT */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { COLD, WATER },        /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 'w' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 't' )<<8)+(( U32 )( 'e' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL | PROPERTY_NOFREE,
               { 0, 0 },            /* current state */
               { ACTION_DRINK, ACTION_TAKE, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_DRINK */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { 0, FLASK },        /* description */
               {
                  0, /* adj */
                  ((( U32 )( 'f' )<<24)+(( U32 )( 'l' )<<16)+(( U32 )( 'a' )<<8)+(( U32 )( 's' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_CONTAINER,     /* properties */
               { CONTAINER_0, 0 },   /* current state */
               { ACTION_FILL, 0, 0, 0 },      /* avatar actions */
               {                    /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {                    /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
            
            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { 0, JAR },          /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 0 )<<24)+(( U32 )( 'j' )<<16)+(( U32 )( 'a' )<<8)+(( U32 )( 'r' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_CONTAINER,     /* properties */
               { CONTAINER_1, 0 },   /* current state */
               { ACTION_FILL, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {                    /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[4] */ 
               0x0bec,              /* magic */
               { YELLOW, SAND },        /* description */
               {
                  ((( U32 )( 'y' )<<24)+(( U32 )( 'e' )<<16)+(( U32 )( 'l' )<<8)+(( U32 )( 'l' )<<0)), /* adj */
                  ((( U32 )( 's' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 'n' )<<8)+(( U32 )( 'd' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_TAKE, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[5] */ 
               0x0bec,              /* magic */
               { GOLD, DUST },        /* description */
               {
                  ((( U32 )( 'g' )<<24)+(( U32 )( 'o' )<<16)+(( U32 )( 'l' )<<8)+(( U32 )( 'd' )<<0)), /* adj */
                  ((( U32 )( 'd' )<<24)+(( U32 )( 'u' )<<16)+(( U32 )( 's' )<<8)+(( U32 )( 't' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_TAKE, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },

         { /* ways (N, E, S, W, U, D) */ 
            C_FOREST_POOL_FLOOR, -C_FOREST_POOL_FLOOR, -C_FOREST_POOL_FLOOR, 
            -C_FOREST_POOL_FLOOR, C_FOREST_POOL, 0
         }
      },

      {  /* cell[C_DEEP_FOREST] */
         0xce11,
         { DEEP, FOREST },  /* output */
         {
            ((( U32 )( 'd' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'p' )<<0 )),
            (( ( U32 )( 'f' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 ))
         },
         {                /* objects */
            {  /* objects[0] */
               0x0bec,              /* magic */
               { OAK, TREE },     /* output description */
               {
                  ((( U32 )0<<24 )+(( U32 )( 'o' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'k' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            },
            {  /* objects[1] */
               0x0bec,              /* magic */
               { BEECH, TREE },     /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'c' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            },
            {  /* objects[1] */
               0x0bec,              /* magic */
               { BIRCH, TREE },     /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'c' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            },
            {  /* objects[2] */
               0x0bec,              /* magic */
               { ASH, TREE },     /* output description */
               {
                  ((( U32 )0<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 'h' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { WORN, TRAIL },    /* output description */
               {
                  (( ( U32 )( 'f' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'i' )<<0 ))   /* noun */
               },
               0,  /* properties */
               { 0, 0 },             /* current state */
               { ACTION_GO, ACTION_FOLLOW, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { C_FOREST_EDGE, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[4] */
               0x0bec,              /* magic */
               { COLD, CAMP_FIRE },     /* output description */
               {
                  ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'm' )<<8 )+(( U32 )( 'p' )<<0 )),  /* adj */
                  (( ( U32 )( 'f' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { C_FOREST, -C_FOREST, C_DEEP_FOREST, -C_DEEP_FOREST, C_FOREST_CANOPY, 
           0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_FOREST_EDGE] */
         0xce11,
         { 0, FOREST | EDGE },  /* output */
         {
            ((( U32 )( 'f' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'e' )<<24 )+(( U32 )( 'd' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */
               0x0bec,              /* magic */
               { BIRCH, TREE },     /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'c' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */
               0x0bec,              /* magic */
               { ASH, TREE },     /* output description */
               {
                  ((( U32 )0<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 'h' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_FOREST_CANOPY, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { WORN, TRAIL },    /* output description */
               {
                  ((( U32 )( 'f' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'i' )<<0 ))   /* noun */
               },
               0,  /* properties */
               { 0, 0 },             /* current state */
               { ACTION_GO, ACTION_FOLLOW, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { C_DEEP_FOREST, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */
               0x0bec,              /* magic */
               { 0, SPECTACLES },     /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'p' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'c' )<<0 ))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_WEARABLE,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[4] */
               0x0bec,              /* magic */
               { OLD, PLOUGH },     /* output description */
               {
                  ((( U32 )0<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'd' )<<0 )),  /* adj */
                  ((( U32 )( 'p' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'u' )<<0 ))  /* noun */
               },
               0,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { C_DEEP_FOREST, 0, C_HILLTOP, -C_DEEP_FOREST, C_FOREST_CANOPY, 
           0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_HILLTOP] */
         0xce11,
         { 0, HILLTOP },  /* output */
         {
            0,
            ((( U32 )( 'h' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'l' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */
               0x0bec,              /* magic */
               { 0, MONUMENT },     /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'm' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'u' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */
               0x0bec,              /* magic */
               { COLLECTOR, DISH },     /* output description */
               {
                  ((( U32 )( 'c' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'l' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 'h' )<<0 ))  /* noun */
               },
               0,               /* properties */
               { STATE_NUMBER, DISH_MOON },  /* state: *10deg rotatation per */
               { 0, 0, 0, 0 },  /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { TALL, LADDER },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'd' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, C_HILLTOP_DISH, 0 }  /* ways (N, E, S, W, U, D) */
            }
         },
         { C_FOREST_EDGE, -C_FOREST_EDGE, -C_FOREST_EDGE, -C_FOREST_EDGE, 
           0, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_HILLTOP_DISH] */
         0xce11,
         { LARGE, HILLTOP_DISH },  /* output */
         {
            ((( U32 )( 'h' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'l' )<<0 )),  /* adj */
            ((( U32 )( 'd' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 'h' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { TALL, LADDER },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'd' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, C_HILLTOP, 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { METAL, BRACKET },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
               },
               PROPERTY_FIXPOINT,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_DRAWBRIDGE] */
         0xce11,
         { 0, CASTLE | DRAWBRIDGE },  /* output */
         {
            0,
            ((( U32 )( 'd' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'w' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, O_CASTLE },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { 0, PORTCULLIS },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'p' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, C_CASTLE_GUARDSROOM, 
                  0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, 0, 0, 0, 0, C_CASTLE_MOAT }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_MOAT] */
         0xce11,
         { 0, CASTLE | MOAT },  /* output */
         {
            0,
            ((( U32 )( 'm' )<<24 )+(( U32 )( 'o' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { COLD, WATER },        /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 'w' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 't' )<<8)+(( U32 )( 'e' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL | PROPERTY_NOFREE,
               { 0, 0 },            /* current state */
               { ACTION_DRINK, ACTION_TAKE, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_DRINK */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { GREEN, VEGETATION },    /* output description */
               {
                  ((( U32 )( 'g' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 'v' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               PROPERTY_PERPETUAL,  /* properties */
               { 0, 0 },            /* current state */
               { ACTION_EAT, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */  /* unlocks the castle tower door */
               0x0bec,              /* magic */
               { BRONZE, KEY },    /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'n' )<<0 )),   /* adj */
                  ((( U32 )( 0 )<<24)+(( U32 )( 'k' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'y' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, 0, 0, 0, C_CASTLE_DRAWBRIDGE, 0 }   /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_GUARDSROOM] */
         0xce11,
         { 0, CASTLE | GUARD | ROOM },  /* output */
         {
            0,
            ((( U32 )( 'g' )<<24 )+(( U32 )( 'u' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { BROKEN, CHAIN },    /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'k' )<<0 )),   /* adj */
                  ((( U32 )( 'c' )<<24)+(( U32 )( 'h' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'i' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_PUSH, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },    /* ACTION_PUSH (Pull) */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 
                 C_CASTLE_DRAWBRIDGE } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { 0, CLOAK },    /* output description */
               {
                  0,   /* adj */
                  ((( U32 )( 'c' )<<24)+(( U32 )( 'l' )<<16 )+(( U32 )( 'o' )<<8 )+( ( U32 )( 'a' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_WEARABLE,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, 0, C_CASTLE_COMMONROOM, 0, 0, 0 }  /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_COMMONROOM] */
         0xce11,
         { 0, CASTLE | COMMON | ROOM },  /* output */
         {
            0,
            ((( U32 )( 'c' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'm' )<<8 )+(( U32 )( 'm' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { DINING, TABLE },    /* output description */
               {
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'i' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'b' )<<8 )+(( U32 )( 'l' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { DINING, CHAIRS },    /* output description */
               {
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'i' )<<0 )),  /* adj */
                  ((( U32 )( 'c' )<<24 )+(( U32 )( 'h' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'i' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { ACTION_SIT, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { C_CASTLE_KITCHEN, C_CASTLE_GUARDSROOM, 
           C_CASTLE_THRONEROOM, C_CASTLE_TOWER, 
           0, C_CASTLE_COURTYARD } /* ways (N, E, S, W, U, D) */
      },
      
      {  /* cell[C_CASTLE_TOWER] */
         0xce11,
         { 0, CASTLE | TOWER },  /* output */
         {
            0,
            ((( U32 )( 't' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'w' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, SIGN },    /* output description */
               {
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'g' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { ACTION_READ, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] 
                * can only be unlocked from within the maproom;
                * this object state is "copied" by the runtime software
                * from the state of the bronze door in the maproom. */ 
               0x0bec,              /* magic */
               { BRONZE, DOOR },    /* output description */
               {
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'w' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_LOCKED, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 },    /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },    /* ACTION_CLOSE */
                  { 0, 0 },
                  { 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 },      /* ACTION_OPEN */
                  { STATE_OPEN, 0 },      /* ACTION_CLOSE */
                  { 0, 0 },
                  { 0, 0 }
               },
               { C_CASTLE_MAPROOM, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { C_CASTLE_COMMONROOM, 0, 0, 0, 0, 
           C_CASTLE_DUNGEON }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_MAPROOM] */
         0xce11,
         { 0, CASTLE | C_MAP | ROOM },  /* output */
         {
            0,
            ((( U32 )0<<24 )+(( U32 )( 'm' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'p' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { BRONZE, DOOR },    /* output description */
               {
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'w' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_LOCKED, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, ACTION_LOCK, ACTION_UNLOCK },
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 }, 
                  { TEST_PRECOND, STATE_OPEN, 1 }, 
                  { TEST_PRECOND, STATE_OPEN, 0 }, 
                  { TEST_PRECOND, STATE_LOCKED, 1 }, 
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 },    /* ACTION_OPEN */
                  { STATE_OPEN, 0 },    /* ACTION_CLOSE */
                  { STATE_LOCKED, 1 },  /* ACTION_LOCK */
                  { STATE_OPEN, 0 },  /* ACTION_UNLOCK */
               },
               { 0, 0, 0, 0, C_CASTLE_TOWER /* if door is unlocked */,
                 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { 0, O_MAP },    /* output description */
               {
                  ((( U32 )( 'w' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'l' )<<0 )),  /* adj */
                  ((( U32 )0<<24 )+(( U32 )( 'm' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'p' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { ACTION_READ, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { 0, WINDOW },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'w' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_DUNGEON] */
         0xce11,
         { 0, CASTLE | DUNGEON },  /* output */
         {
            0,
            ((( U32 )( 'd' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'g' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { BARRED, WINDOW },    /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'r' )<<0 )),  /* adj */
                  ((( U32 )( 'w' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            },
            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { 0, O_STRAW },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 't' )<<16 )+(( U32 )( 'r' )<<8 )+( ( U32 )( 'a' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
              /* the only way out of the dungeon is to say the magic phrase
               * taking you up to the maproom. */
      },

      {  /* cell[C_CASTLE_THRONEROOM] */
         0xce11,
         { 0, CASTLE | C_THRONE | ROOM },  /* output */
         {
            0,
            ((( U32 )( 't' )<<24 )+(( U32 )( 'h' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'o' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, O_THRONE },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'h' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'o' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_SPECIAL, 0 },            /* current state */
               { ACTION_SIT, ACTION_STAND, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_SPECIAL, 0 },   /* ACTION_SIT */
                  { TEST_PRECOND, STATE_SPECIAL, 1 },   /* ACTION_STAND */
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_SPECIAL, 1 },   /* ACTION_SIT */
                  { STATE_SPECIAL, 0 },   /* ACTION_STAND */
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               { 0, 0, 0, 0, C_CASTLE_THRONEROOM, 
                 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { YELLOW, HEADBAND },    /* output description */
               {
                  ((( U32 )( 'y' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'l' )<<0 )),  /* adj */
                  ((( U32 )( 'h' )<<24 )+(( U32 )( 'e' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
               },
                  /* visible once seated on the throne */
               PROPERTY_HIDDEN | PROPERTY_WEARABLE | PROPERTY_MOBILE,
               { STATE_SPECIAL, 0 },            /* current state */
               { ACTION_WEAR, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { 0, PAINTINGS },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'p' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { C_CASTLE_COMMONROOM, 0, 0, 0, 0, 0 }  /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_CONTROLROOM] */
         0xce11,
         { 0, CASTLE | C_CONTROL | ROOM },  /* output */
         {
            0,
            ((( U32 )( 'c' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, O_THRONE },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'h' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'o' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_SPECIAL, 1 },            /* current state */
               { ACTION_SIT, ACTION_STAND, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_SPECIAL, 0 },   /* ACTION_SIT */
                  { TEST_PRECOND, STATE_SPECIAL, 1 },   /* ACTION_STAND */
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_SPECIAL, 1 },   /* ACTION_SIT */
                  { STATE_SPECIAL, 0 },   /* ACTION_STAND */
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               { 0, 0, 0, 0, C_CASTLE_CONTROLROOM, 
                 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { WHITE, SCREEN },    /* output description */
               {
                  ((( U32 )( 'w' )<<24 )+(( U32 )( 'h' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'c' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
                  /* visible once seated on the throne */
               0,
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { CONTROL, PANEL },
               {
                  ((( U32 )( 'c' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
                  ((( U32 )( 'p' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { WHITE, BUTTON /* projector */ },
               {
                  ((( U32 )( 'w' )<<24 )+(( U32 )( 'h' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 't' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { ACTION_PRESS, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 }  /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_COURTYARD] */
         0xce11,
         { 0, CASTLE | COURTYARD },  /* output */
         {
            0,
            ((( U32 )( 'c' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'u' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { SILVER, DOOR },    /* output description */
               {
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'v' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_LOCKED, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, ACTION_LOCK, ACTION_UNLOCK },
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 }, 
                  { TEST_PRECOND, STATE_OPEN, 1 }, 
                  { TEST_PRECOND, STATE_OPEN, 0 }, 
                  { TEST_PRECOND, STATE_LOCKED, 1 }, 
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 },    /* ACTION_OPEN */
                  { STATE_OPEN, 0 },    /* ACTION_CLOSE */
                  { STATE_LOCKED, 1 },  /* ACTION_LOCK */
                  { STATE_OPEN, 0 },  /* ACTION_UNLOCK */
               },
               { 0, 0, 0, 0, C_CASTLE_KEEP /* if door is unlocked */,
                 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { TALL, LADDER },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'd' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, C_CASTLE_SPIRE_ROOF, 
                 0 }      /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, 0, 0, 0, C_CASTLE_COMMONROOM, 0 }   /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_KEEP] */
         0xce11,
         { 0, CASTLE | KEEP },  /* output */
         {
            0,
            ((( U32 )( 'k' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'p' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { LARGE, BOOK },    /* output description */
               {
                  ((( U32 )( 'k' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'p' )<<0 )),  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'k' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_READ, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1];
                * this object is updated by the runtime software from the
                * state of the keep door. It can only be locked/unlocked
                * from outside the keep. */ 
               0x0bec,              /* magic */
               { SILVER, DOOR },    /* output description */
               {
                  ((( U32 )( 'k' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'p' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 }, 
                  { TEST_PRECOND, STATE_OPEN, 1 }, 
                  { 0, 0 },
                  { 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 },    /* ACTION_OPEN */
                  { STATE_OPEN, 0 },    /* ACTION_CLOSE */
                  { 0, 0 },
                  { 0, 0 }
               },
               { 0, 0, 0, 0, C_CASTLE_COURTYARD/* if door is unlocked */,
                 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] 
                * The spire door has no lock; it lies immediately behind the
                * locked keep door. */ 
               0x0bec,              /* magic */
               { GOLD, DOOR },    /* output description */
               {
                  ((( U32 )( 'g' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'd' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 0 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 },    /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },    /* ACTION_CLOSE */
                  { 0, 0 },
                  { 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 },      /* ACTION_OPEN */
                  { STATE_OPEN, 0 },      /* ACTION_CLOSE */
                  { 0, 0 },
                  { 0, 0 }
               },
               { 0, 0, 0, 0, C_CASTLE_SPIRE /* if door is unlocked */,
                 0 }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_SPIRE]
            above the keep, the laserbeam control room */
         0xce11,
         { 0, KEEP | SPIRE },  /* output */
         {
            0,
            ((( U32 )( 's' )<<24 )+(( U32 )( 'p' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { GOLD, DOOR },    /* output description */
               {
                  ((( U32 )( 'g' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'd' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 },    /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },    /* ACTION_CLOSE */
                  { 0, 0 },
                  { 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 },      /* ACTION_OPEN */
                  { STATE_OPEN, 0 },      /* ACTION_CLOSE */
                  { 0, 0 },
                  { 0, 0 }
               },
               { C_CASTLE_KEEP, 0, 0, 0, 0, 0 }   /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { BLUE, SWITCH },    /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'u' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'w' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, ACTION_PRESS, 
                 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 }, 
                  { TEST_PRECOND, STATE_OPEN, 1 }, 
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 },   /* ACTION_OPEN */
                  { STATE_OPEN, 0 },   /* ACTION_CLOSE */
                  { 0, 0 },            /* ACTION_PRESS */
                  { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { YELLOW, SWITCH },    /* output description */
               {
                  ((( U32 )( 'y' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'l' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'w' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, ACTION_PRESS, 
                 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 }, 
                  { TEST_PRECOND, STATE_OPEN, 1 }, 
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 },    /* ACTION_OPEN */
                  { STATE_OPEN, 0 },    /* ACTION_CLOSE */
                  { 0, 0 },             /* ACTION_PRESS */
                  { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { ENERGY, CAP },    /* output description */
               {
                  ((( U32 )( 'e' )<<24 )+(( U32 )( 'n' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'r' )<<0 )),  /* adj */
                  ((( U32 )0<<24 )+(( U32 )( 'c' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'p' )<<0 ))   /* noun */
               },
               PROPERTY_CONTAINER,     /* properties */
               { CONTAINER_9, 0 },   /* current state */
               { ACTION_FILL, ACTION_OPEN, ACTION_CLOSE, 
                  0 },  /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, 
                  { TEST_PRECOND, CONTAINER_9, 0 }, 
                  { TEST_PRECOND, CONTAINER_9, 1 }, 
                  { 0, 0, 0 }, 
               },
               {  /* avatar post-state */
                  { 0, 0 }, 
                  { CONTAINER_9, 1 },    /* ACTION_OPEN */
                  { CONTAINER_9, 0 },    /* ACTION_CLOSE */
                  { 0, 0 }
               },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[4] */  /* get state of lightbeam */
               0x0bec,              /* magic */
               { 0, METER },    /* output description */
               {
                  0,   /* adj */
                  ((( U32 )( 'm' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 't' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_READ, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_SPIRE_ROOF]
            atop the spire, the laserbeam itself */
         0xce11,
         { 0, ROOF | SPIRE },  /* output */
         {
            ((( U32 )( 's' )<<24 )+(( U32 )( 'p' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 'r' )<<0 )),  /* adj */
            ((( U32 )( 'r' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+( ( U32 )( 'f' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, LIGHTBEAM },    /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'e' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'm' )<<0 )),  /* adj */
                  ((( U32 )( 'l' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'h' )<<0 ))   /* noun */
               },
               0,            /* properties */
               { STATE_SPECIAL, L_ENGAGE_DIAMOND }, /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { 0, RACK },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'r' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'c' )<<8 )+(( U32 )( 'k' )<<0 ))   /* noun */
               },
               0,            /* properties */
               { STATE_SPECIAL, SOURCE_MOON }, /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { 0, PINION },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'p' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'i' )<<0 ))   /* noun */
               },
               0,            /* properties */
               { 0, 0 }, /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { 0, WHEEL },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'w' )<<24 )+(( U32 )( 'h' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               0,            /* properties */
               { 0, 0 }, /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[4] */ 
               0x0bec,              /* magic */
               { METAL, BRACKET },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
               },
               PROPERTY_FIXPOINT,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[5] */ 
               0x0bec,              /* magic */
               { TALL, LADDER },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'd' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, 0, C_CASTLE_COURTYARD 
               }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, C_CASTLE_COURTYARD }    /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_KITCHEN] */
         0xce11,
         { 0, CASTLE | KITCHEN },  /* output */
         {
            0,
            ((( U32 )( 'k' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 't' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { LARGE, POT },          /* description */
               {
                  ((( U32 )( 'l' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 'r' )<<8)+(( U32 )( 'g' )<<0)), /* adj */
                  ((( U32 )( 0 )<<24)+(( U32 )( 'p' )<<16)+(( U32 )( 'o' )<<8)+(( U32 )( 't' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_CONTAINER,     /* properties */
               { CONTAINER_2, 0 },   /* current state */
               { ACTION_FILL, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },          
               {                    /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { BAKING, TRAY },          /* description */
               {
                  ((( U32 )( 'b' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 'k' )<<8)+(( U32 )( 'i' )<<0)), /* adj */
                  ((( U32 )( 't' )<<24)+(( U32 )( 'r' )<<16)+(( U32 )( 'a' )<<8)+(( U32 )( 'y' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_CONTAINER,     /* properties */
               { CONTAINER_3, 0 },   /* current state */
               { ACTION_FILL, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },          
               {                    /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { BRONZE, JAR },          /* description */
               {
                  ((( U32 )( 'b' )<<24)+(( U32 )( 'r' )<<16)+(( U32 )( 'o' )<<8)+(( U32 )( 'n' )<<0)),   /* adj */
                  ((( U32 )( 0 )<<24)+(( U32 )( 'j' )<<16)+(( U32 )( 'a' )<<8)+(( U32 )( 'r' )<<0))      /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_CONTAINER,     /* properties */
               { CONTAINER_4, 0 },   /* current state */
               { ACTION_FILL, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },          
               {                    /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { 0, TAP },          /* description */
               {
                  0, /* adj */
                  ((( U32 )( 0 )<<24)+(( U32 )( 't' )<<16)+(( U32 )( 'a' )<<8)+(( U32 )( 'p' )<<0))  /* noun */
               },
               0,     /* properties */
               { STATE_OPEN, 0 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 },   /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_CLOSE */
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 }, 
                  { STATE_OPEN, 0 }, 
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[4] */ 
               0x0bec,              /* magic */
               { 0, SINK },          /* description */
               {
                  0, /* adj */
                  ((( U32 )( 's' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 'n' )<<8)+(( U32 )( 'k' )<<0))  /* noun */
               },
               PROPERTY_CONTAINER,     /* properties */
               { CONTAINER_8, 0 },   /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, C_CASTLE_COMMONROOM, 0, 0, 0 }   /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_CASTLE_CELLAR]
            a secret room somewhere under the castle, perhaps adjacent to
            the dungeon. */
         0xce11,
         { 0, CASTLE | CELLAR },  /* output */
         {
            0,
            ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'v' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
            }
         },
         { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_PASS] */
         0xce11,
         { COLD, MOUNTAIN | PASS },  /* output */
         {
            0,
            ((( U32 )( 'p' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 's' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, O_MOUNTAIN },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'm' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'u' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { C_MOUNTAIN_PEAK, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { WHITE, SNOW },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'n' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'w' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_EAT, ACTION_DRINK, ACTION_TAKE, 
                  0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_EAT */
                { 0, 0, 0 },                  /* ACTION_DRINK */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { WHITE, DOOR },    /* output description */
               {
                  ((( U32 )( 'w' )<<24 )+(( U32 )( 'h' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 0 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 },   /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_CLOSE */
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 }, 
                  { STATE_OPEN, 0 }, 
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               { C_MOUNTAIN_TUNNEL, 0, 0, 0, 0, 
                  0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, 0, 0, C_MOUNTAIN_PEAK, 0, 0 }   /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_PEAK] */
         0xce11,
         { COLD, MOUNTAIN | PEAK },  /* output */
         {
            0,
            ((( U32 )( 'p' )<<24 )+(( U32 )( 'e' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'k' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { WHITE, SNOW },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'n' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'w' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_EAT, ACTION_DRINK, ACTION_TAKE, 
                  0 }, /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_EAT */
                { 0, 0, 0 },                  /* ACTION_DRINK */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { BLUE, DOOR },    /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'u' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               PROPERTY_HIDDEN,                   /* properties */
               { STATE_OPEN, 0 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 },   /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_CLOSE */
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 }, 
                  { STATE_OPEN, 0 }, 
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               { C_MOUNTAIN_OBSERVATORY, 0, 0, 0, 0, 
                  0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { METAL, LADDER },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'd' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 
                 C_MOUNTAIN_OBSERVATORY_DOME, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, C_MOUNTAIN_PASS, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_OBSERVATORY] */
         0xce11,
         { 0, MOUNTAIN | OBSERVATORY },  /* output */
         {
            0,
            ((( U32 )( 'o' )<<24 )+(( U32 )( 'b' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { BLUE, DOOR },    /* output description */
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'u' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 },   /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_CLOSE */
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 }, 
                  { STATE_OPEN, 0 }, 
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               { C_MOUNTAIN_PEAK, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { 0, TELESCOPE },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               0,                /* properties */
               { 0, NAME_LEFT }, /* current state (actually of the lightbeam) */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { 0, SEAT },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'e' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { ACTION_SIT, ACTION_STAND, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, C_MOUNTAIN_OBSERVATORY, 
                 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { CONTROL, PANEL },
               {
                  ((( U32 )( 'c' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
                  ((( U32 )( 'p' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[4] */ 
               0x0bec,              /* magic */
               { 0, JOYSTICK },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'j' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'y' )<<8 )+(( U32 )( 's' )<<0 ))   /* noun */
               },
               PROPERTY_HIDDEN,   /* properties */
               { 0, 0 },            /* current state */
               { ACTION_MOVE, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[5] */ 
               0x0bec,              /* magic */
               { ORANGE, BUTTON /* movement granularity */ },
               {
                  ((( U32 )( 'o' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 't' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               PROPERTY_HIDDEN,   /* properties */
               { STATE_ON, 0 },            /* current state */
               { ACTION_PRESS, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[6] */ 
               0x0bec,              /* magic */
               { ROTARY, SWITCH  /* stepper motor selector */ },
               {
                  ((( U32 )( 'r' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 't' )<<8 )+( ( U32 )( 'a' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'w' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               PROPERTY_HIDDEN,   /* properties */
               { STATE_NUMBER, ROTARY_OFF },            /* current state */
               { ACTION_SET, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_OBSERVATORY_DOME] */
         0xce11,
         { 0, MOUNTAIN | OBSERVATORY | DOME },  /* output */
         {
            ((( U32 )( 'o' )<<24 )+(( U32 )( 'b' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'r' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+( ( U32 )( 'f' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { METAL, LADDER },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'd' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, C_MOUNTAIN_PEAK } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { TELESCOPE, LENS },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'l' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 's' )<<0 ))   /* noun */
               },
               PROPERTY_FIXPOINT,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 
                 C_MOUNTAIN_OBSERVATORY }  /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, C_MOUNTAIN_PEAK }   /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_TUNNEL] */
         0xce11,
         { 0, MOUNTAIN | TUNNEL },  /* output */
         {
            0,
            ((( U32 )( 't' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { WHITE, DOOR },    /* output description */
               {
                  ((( U32 )( 'w' )<<24 )+(( U32 )( 'h' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 },   /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_CLOSE */
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 }, 
                  { STATE_OPEN, 0 }, 
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               { C_MOUNTAIN_PASS, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { 0, SIGN },    /* output description */
               {
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_READ, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { 0, O_FUNICULAR },    /* output description */
               {
                  0,  /* adj */
                  (( ( U32 )( 'f' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'i' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 0 },             /* current state */
               { ACTION_GO, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_GO */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { C_MOUNTAIN_FUNICULAR, 
                 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { RED, BUTTON /* funicular power */ },
               {
                  ((( U32 )0<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'd' )<<0 )),  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 't' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { STATE_ON, 0 },            /* current state */
               { ACTION_PRESS, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, C_MOUNTAIN_STORES, 0, 0, 0, 0 }  /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_STORES ] */
         0xce11,
         { 0, MOUNTAIN | STOREROOM },  /* output */
         {
            0,
            ((( U32 )( 's' )<<24 )+(( U32 )( 't' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, GOGGLES },    /* output description */
               {
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'n' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'w' )<<0 )),  /* adj */
                  ((( U32 )( 'g' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'g' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_WEARABLE,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { HARD, HAT },    /* output description */
               {
                  ((( U32 )( 'h' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'd' )<<0 )),  /* adj */
                  ((( U32 )0<<24 )+(( U32 )( 'h' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { WELLINGTON, BOOTS },    /* output description */
               {
                  ((( U32 )( 'w' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'l' )<<0 )),  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { REFLECTIVE, JACKET },    /* output description */
               {
                  ((( U32 )( 'r' )<<24 )+(( U32 )( 'e' )<<16 )+( ( U32 )( 'f' )<<8 )+(( U32 )( 'l' )<<0 )),  /* adj */
                  ((( U32 )( 'j' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'c' )<<8 )+(( U32 )( 'k' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, 0, 0, C_MOUNTAIN_TUNNEL, 0, 0 }    /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_FUNICULAR ] */
         0xce11,
         { 0, MOUNTAIN | FUNICULAR },  /* output */
         {
            0,
            (( ( U32 )( 'f' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'i' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, DOOR },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { STATE_OPEN, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 },   /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_CLOSE */
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 }, 
                  { STATE_OPEN, 0 }, 
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1]:
                * the lever is used to raise and lower the funicular;
                * there are 6 levels 5..0:
                * level 5 is the top, level 0 is the bottom;
               */ 
               0x0bec,              /* magic */
               { 0, LEVER },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'l' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'v' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { STATE_NUMBER, 5 },            /* current state */
               { ACTION_PUSH, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_CAVERN] */
         0xce11,
         { 0, MOUNTAIN | CAVERN },  /* output */
         {
            0,
            ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'v' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, O_FUNICULAR },    /* output description */
               {
                  0,  /* adj */
                  (( ( U32 )( 'f' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'i' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 1 },             /* current state */
               { ACTION_GO, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_GO */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { C_MOUNTAIN_FUNICULAR, 
                 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, C_MOUNTAIN_LAB, 0, C_MOUNTAIN_LAKE, 0, 
           C_MINE_HEAD } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_LAB] */
         0xce11,
         { 0, MOUNTAIN | LABORATORY },  /* output */
         {
            0,
            ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'b' )<<8 )+(( U32 )( 'o' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { LAB, BOOK },    /* output description */
               {
                  ((( U32 )0<<24 )+(( U32 )( 'l' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'b' )<<0 )),  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'k' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,                   /* properties */
               { STATE_NUMBER, 0 },             /* current state */
               { ACTION_READ, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */  /* unlocks the castle keep door */
               0x0bec,              /* magic */
               { SILVER, KEY },    /* output description */
               {
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'v' )<<0 )),   /* adj */
                  ((( U32 )( 0 )<<24)+(( U32 )( 'k' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'y' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,     /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, C_MOUNTAIN_LAB_STORES, 0, C_MOUNTAIN_CAVERN, 0, 
           0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_LAB_STORES ] */
         0xce11,
         { 0, MOUNTAIN | LAB_STORES },  /* output */
         {
            ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'b' )<<8 )+(( U32 )( 'o' )<<0 )),  /* adj */
            ((( U32 )( 's' )<<24 )+(( U32 )( 't' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ /* used to carry heavy water */
               0x0bec,              /* magic */
               { LEAD, FLASK },    /* output description */
               {
                  ((( U32 )( 'l' )<<24 )+(( U32 )( 'e' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'd' )<<0 )),  /* adj */
                  (( ( U32 )( 'f' )<<24 )+(( U32 )( 'l' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 's' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_CONTAINER,    /* properties */
               { CONTAINER_5, 0 },             /* current state */
               { ACTION_FILL, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },          
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ /* used in making energy */
               0x0bec,              /* magic */
               { GLASS, BURET },    /* output description */
               {
                  ((( U32 )( 'g' )<<24 )+(( U32 )( 'l' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 's' )<<0 )),  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,                   /* properties */
               { 0, 0 },             /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */  /* used in making energy */
               0x0bec,              /* magic */
               { 0, BURNER },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,                   /* properties */
               { 0, 0 },             /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ /* used to store liquid energy */
               0x0bec,              /* magic */
               { GLASS, TESTTUBE },    /* output description */
               {
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'b' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,   /* properties */
               { STATE_PERCENTAGE, 0 },             /* current state */
               { ACTION_FILL, ACTION_EXCHANGE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[4] */ /* used to carry source from mines */
               0x0bec,              /* magic */
               { CERAMIC, CRUCIBLE },    /* output description */
               {
                  ((( U32 )( 'c' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'r' )<<8 )+( ( U32 )( 'a' )<<0 )),  /* adj */
                  ((( U32 )( 'c' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'u' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_CONTAINER,  /* properties */
               { CONTAINER_6, 0 },   /* current state */
               { ACTION_FILL, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[5] */  /* used in making energy */
               0x0bec,              /* magic */
               { METAL, STAND },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 't' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,                   /* properties */
               { 0, 0 },             /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[6] */ /* needed to collect heavy water */
               0x0bec,              /* magic */
               { 0, DIVINGSUIT },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'v' )<<8 )+(( U32 )( 'i' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_WEARABLE,   /* properties */
               { 0, 0 },             /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, C_MOUNTAIN_LAB, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_MANUFACTORY]
            a secret room somewhere under the mountain, perhaps adjacent to
            the laboratory. */
         0xce11,
         { 0, MOUNTAIN | MANUFACTORY },  /* output */
         {
           (( U32 )( 'a' )<<0 ),
            ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'u' )<<0 ))   /* noun */
         },
         {                /* objects */

              /* Objects parked here for convenience, used as the game
               * progresses.
               * The mirrors are used in make_stuff(). */

            {  /* objects[0] */
               0x0bec,              /* magic */
               { 0, MISSION },  /* description */
               {
                  0,  /* adj */
                  ((( U32 )( 'm' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 's' )<<0 ))  /* noun */
               },
               0,                   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1]
                  A Mirror is made in make_stuff(), which copies this
                  manufactory object in the laborartory.
               */
               0x0bec,              /* magic */
               { SILVER, MIRROR },          /* description */
               {
                  ((( U32 )( 's' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 'l' )<<8)+(( U32 )( 'v' )<<0)),   /* adj */
                  ((( U32 )( 'm' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 'r' )<<8)+(( U32 )( 'r' )<<0))      /* noun */
               },
               PROPERTY_MOBILE,     /* properties */
               { 0, NAME_RIGHT },   /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {                    /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2]
                  A Mirror is made in make_stuff(), which copies this
                  manufactory object in the laborartory.
               */
               0x0bec,              /* magic */
               { GOLD, MIRROR },          /* description */
               {
                  ((( U32 )( 'g' )<<24)+(( U32 )( 'o' )<<16)+(( U32 )( 'l' )<<8)+(( U32 )( 'd' )<<0)),   /* adj */
                  ((( U32 )( 'm' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 'r' )<<8)+(( U32 )( 'r' )<<0))      /* noun */
               },
               PROPERTY_MOBILE,     /* properties */
               { 0, NAME_UP },   /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },          
               {                    /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3]
                  It should be fixed to the telescope lens
                  atop the observatory roof. */
               0x0bec,              /* magic */
               { DARK, FILTER },          /* description */
               {
                  ((( U32 )( 'd' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 'r' )<<8)+(( U32 )( 'k' )<<0)),   /* adj */
                  ((( U32 )( 'f' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 'l' )<<8)+(( U32 )( 't' )<<0))      /* noun */
               },
               PROPERTY_MOBILE,     /* properties */
               { 0, 0 },   /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },          
               {                    /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[4] 
                  One controller board is placed at each magnotron facility
                  around the ring. Each controls the magnets at its facility,
                  used to direct the lightbeam. Each is calibrated using three
                  slider switches in the magnotron operations room: an <X,Y,Z>
                  function. */
               0x0bec,              /* magic */
               { CONTROL, BOARD },  /* description */
               {
                  ((( U32 )( 'c' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'o' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,     /* properties */
               { STATE_NUMBER, 0 }, /* current state (value is optimum %age) */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MOUNTAIN_LAKE] */
         0xce11,
         { 0, MOUNTAIN | LAKE },  /* output */
         {
            0,
            ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'k' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, FISH },         /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 'f' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 's' )<<8)+(( U32 )( 'h' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL | PROPERTY_NOFREE,
               { 0, 0 },            /* current state */
               { ACTION_EAT, ACTION_TAKE, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_EAT */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},/* avatar post-states */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { COLD, WATER },        /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 'w' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 't' )<<8)+(( U32 )( 'e' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL | PROPERTY_NOFREE,
               { 0, 0 },            /* current state */
               { ACTION_DRINK, ACTION_TAKE, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_DRINK */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { /* ways (N, E, S, W, U, D) */ 
            C_MOUNTAIN_LAKE, -C_MOUNTAIN_LAKE, -C_MOUNTAIN_LAKE, 
            -C_MOUNTAIN_LAKE, C_MOUNTAIN_CAVERN, C_MOUNTAIN_LAKE_MID 
         }
      },

      {  /* cell[C_MOUNTAIN_LAKE_MID] */
         0xce11,
         { DEEP, MOUNTAIN | LAKE },  /* output */
         {
            ((( U32 )( 'd' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'p' )<<0 )),  /* adj */
            ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'k' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, FISH },         /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 'f' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 's' )<<8)+(( U32 )( 'h' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL | PROPERTY_NOFREE,
               { 0, 0 },            /* current state */
               { ACTION_EAT, ACTION_TAKE, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_EAT */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},/* avatar post-states */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { COLD, WATER },        /* description */
               {
                  0,   /* adj */
                  ((( U32 )( 'w' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 't' )<<8)+(( U32 )( 'e' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL | PROPERTY_NOFREE,
               { 0, 0 },            /* current state */
               { ACTION_DRINK, ACTION_TAKE, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_DRINK */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { /* ways (N, E, S, W, U, D) */ 
            C_MOUNTAIN_LAKE_MID, -C_MOUNTAIN_LAKE_MID, -C_MOUNTAIN_LAKE_MID, 
            -C_MOUNTAIN_LAKE_MID, C_MOUNTAIN_LAKE, C_MOUNTAIN_LAKE_BOTTOM
         }
      },

      {  /* cell[C_MOUNTAIN_LAKE_BOTTOM] */
         0xce11,
         { 0, MOUNTAIN | LAKE | BOTTOM },   /* output */
         {
            ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'k' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'b' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 't' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { HEAVY, WATER },        /* description */
               {
                  ((( U32 )( 'h' )<<24)+(( U32 )( 'e' )<<16)+(( U32 )( 'a' )<<8)+(( U32 )( 'v' )<<0)), /* adj */
                  ((( U32 )( 'w' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 't' )<<8)+(( U32 )( 'e' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL | PROPERTY_NOFREE,
               { 0, 0 },            /* current state */
               { ACTION_DRINK, ACTION_TAKE, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { 0, 0, 0 },                  /* ACTION_DRINK */
                { TEST_CONTAINER_SPECIAL, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { SILVER, SAND },        /* description */
               {
                  ((( U32 )( 's' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 'l' )<<8)+(( U32 )( 'v' )<<0)), /* adj */
                  ((( U32 )( 's' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 'n' )<<8)+(( U32 )( 'd' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_TAKE, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
                { 0, 0, 0 },
                { 0, 0, 0 },
                { 0, 0, 0 }
               },          
               {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },

         { /* ways (N, E, S, W, U, D) */ 
            C_MOUNTAIN_LAKE_BOTTOM, -C_MOUNTAIN_LAKE_BOTTOM, 
            -C_MOUNTAIN_LAKE_BOTTOM, -C_MOUNTAIN_LAKE_BOTTOM, 
            C_MOUNTAIN_LAKE_MID, 0
         }
      },

      {  /* cell[C_MINE_HEAD] */
         0xce11,
         { 0, MINE | HEAD },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'h' )<<24 )+(( U32 )( 'e' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, HOIST },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'h' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 's' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_ON, 0 },             /* current state */
               { ACTION_SET, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { 0, SHAFT_HEAD },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'h' )<<16 )+( ( U32 )( 'a' )<<8 )+( ( U32 )( 'f' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { 0, 0 },             /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, C_MINE_OBSERVATION_DECK, 
                 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { MINERS, O_CAGE },    /* output description */
               {
                  ((( U32 )( 'm' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 0 },             /* current state */
               { ACTION_GO, ACTION_OPEN, ACTION_CLOSE, 0 }, /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_GO */
                  { TEST_PRECOND, STATE_OPEN, 0 },   /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_CLOSE */
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 },             /* ACTION_GO */
                  { STATE_OPEN, 1 },    /* ACTION_OPEN */
                  { STATE_OPEN, 0 },    /* ACTION_CLOSE */
                  { 0, 0 } 
               },
               { C_MINE_CAGE, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { SAFETY, HAT },          /* description */
               {
                  ((( U32 )( 's' )<<24)+(( U32 )( 'a' )<<16)+(( U32 )( 'f' )<<8)+(( U32 )( 'e' )<<0)), /* adj */
                  ((( U32 )( 0 )<<24)+(( U32 )( 'h' )<<16)+(( U32 )( 'a' )<<8)+(( U32 )( 't' )<<0))  /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_CONTAINER,     /* properties */
               { CONTAINER_7, 0 },   /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {                           /* avatar pre-conds */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },          
               {                    /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, C_MOUNTAIN_CAVERN, 0 }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MINE_OBSERVATION_DECK] */
         0xce11,
         { 0, OBSERVATION | DECK },  /* output */
         {
            ((( U32 )( 'o' )<<24 )+(( U32 )( 'b' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'd' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'c' )<<8 )+(( U32 )( 'k' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, O_CAGE },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 0 },             /* current state */
               { ACTION_GO, ACTION_OPEN, ACTION_CLOSE, 0 }, /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_GO */
                  { TEST_PRECOND, STATE_OPEN, 0 },   /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_CLOSE */
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 },             /* ACTION_GO */
                  { STATE_OPEN, 1 },    /* ACTION_OPEN */
                  { STATE_OPEN, 0 },    /* ACTION_CLOSE */
                  { 0, 0 } 
               },
               { C_MINE_CAGE, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },
         },
         { 0, 0, 0, 0, 0, C_MINE_HEAD }      /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MINE_CAGE] */
         0xce11,
         { 0, MINE | CAGE },  /* output */
         {
            0,  /* adj */
            ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, DOOR },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { STATE_OPEN, 1 },            /* current state */
               { ACTION_OPEN, ACTION_CLOSE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 0 },   /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_CLOSE */
                  { 0, 0, 0 },
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { STATE_OPEN, 1 }, 
                  { STATE_OPEN, 0 }, 
                  { 0, 0 }, 
                  { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1]:
                * the lever is used to raise and lower the cage;
                * there are 6 levels 5..0:
                * level 5 is the minehead, level 0 is the mine;
                * level 1 is the phlux magnatron complex, only accessible
                * when wearing the gold ring of admittance.
               */ 
               0x0bec,              /* magic */
               { 0, LEVER },    /* output description */
               {
                  ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 'l' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'v' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { STATE_NUMBER, 5 },            /* current state */
               { ACTION_PUSH, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { ELECTRIC, LAMP },    /* output description */
               {
                  ((( U32 )( 'e' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'c' )<<0 )),  /* adj */
                  ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'm' )<<8 )+(( U32 )( 'p' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { STATE_LIGHT, 1 },            /* current state */
               { ACTION_SET, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MINE_SHAFT] */
         0xce11,
         { 0, MINE | SHAFT },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 's' )<<24 )+(( U32 )( 'h' )<<16 )+( ( U32 )( 'a' )<<8 )+( ( U32 )( 'f' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { MINERS, O_CAGE },    /* output description */
               {
                  ((( U32 )( 'm' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               0,                   /* properties */
               { STATE_OPEN, 1 },             /* current state */
               { ACTION_GO, ACTION_OPEN, ACTION_CLOSE, 0 }, /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_GO */
                  { TEST_PRECOND, STATE_OPEN, 0 },   /* ACTION_OPEN */
                  { TEST_PRECOND, STATE_OPEN, 1 },   /* ACTION_CLOSE */
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, 
                  { STATE_OPEN, 1 },    /* ACTION_OPEN */
                  { STATE_OPEN, 0 },    /* ACTION_CLOSE */
                  { 0, 0 } 
               },
               { C_MINE_CAGE, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { C_MINE_ENTRANCE, -C_MINE_ENTRANCE, -C_MINE_ENTRANCE, 
           -C_MINE_ENTRANCE, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MINE_ENTRANCE] */
         0xce11,
         { 0, MINE | ENTRANCE },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'e' )<<24 )+(( U32 )( 'n' )<<16 )+(( U32 )( 't' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, PICK },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'p' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'c' )<<8 )+(( U32 )( 'k' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,  /* properties */
               { 0, 0 },             /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, 
                  { 0, 0, 0 }, 
                  { 0, 0, 0 }, 
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { 0, SHOVEL },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'h' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'v' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,  /* properties */
               { 0, 0 },             /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, 
                  { 0, 0, 0 }, 
                  { 0, 0, 0 }, 
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { 0, SIGN },    /* output description */
               {
                  ((( U32 )( 'm' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,     /* properties */
               { 0, 0 },            /* current state */
               { ACTION_READ, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { C_MINE_TUNNEL, -C_MINE_TUNNEL, C_MINE_SHAFT, 
           -C_MINE_TUNNEL, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MINE_TUNNEL] */
         0xce11,
         { 0, MINE | TUNNEL },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 't' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
            }
         },
         { C_MINE_TUNNEL, -C_MINE_TUNNEL, -C_MINE_TUNNEL, 
           -C_MINE_TUNNEL, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MINE_FACE] */
         0xce11,
         { 0, MINE | FACE },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            (( ( U32 )( 'f' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'c' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { DIAMOND, SOURCE },    /* output description */
               {
                  ((( U32 )( 'd' )<<24 )+(( U32 )( 'i' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'm' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'u' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE | PROPERTY_PERPETUAL,  /* properties */
               { 0, 0 },             /* current state */
               { ACTION_TAKE, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { TEST_CONTAINER_SPECIAL, 0, 0 }, 
                  { 0, 0, 0 }, 
                  { 0, 0, 0 }, 
                  { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { C_MINE_TUNNEL, -C_MINE_TUNNEL, -C_MINE_TUNNEL, 
           -C_MINE_TUNNEL, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MAGNOTRON_COMPLEX] */
         0xce11,
         { 0, MAGNOTRON | COMPLEX },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
            ((( U32 )( 'c' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'm' )<<8 )+(( U32 )( 'p' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { 0, TRAVELATOR },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'v' )<<0 ))   /* noun */
               },
               0,  /* properties */
               { 0, 0 },             /* current state */
               { ACTION_GO, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { C_MAGNOTRON_OPERATION, 0, 0, 0, 0, 
                 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, C_MINE_CAGE, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MAGNOTRON_OPERATION] */
         0xce11,
         { 0, MAGNOTRON | OPERATION },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
            ((( U32 )( 'o' )<<24 )+(( U32 )( 'p' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'r' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { CONTROL, PANEL },
               {
                  ((( U32 )( 'c' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
                  ((( U32 )( 'p' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { ROTARY, SWITCH  /* magnotron control selector */ },
               {
                  ((( U32 )( 'r' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 't' )<<8 )+( ( U32 )( 'a' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'w' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { STATE_NUMBER, ROTARY_0 },            /* current state */
               { ACTION_SET, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[2] */ 
               0x0bec,              /* magic */
               { BLUE, SLIDER_SWITCH  /* magnotron adjust */ },
               {
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'u' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'w' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { STATE_NUMBER, 0 },            /* current state */
               { ACTION_SET, ACTION_MOVE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[3] */ 
               0x0bec,              /* magic */
               { YELLOW, SLIDER_SWITCH  /* magnotron adjust */ },
               {
                  ((( U32 )( 'y' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'l' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'w' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { STATE_NUMBER, 0 },            /* current state */
               { ACTION_SET, ACTION_MOVE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[4] */ 
               0x0bec,              /* magic */
               { RED, SLIDER_SWITCH  /* magnotron adjust */ },
               {
                  ((( U32 )0<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'd' )<<0 )),  /* adj */
                  ((( U32 )( 's' )<<24 )+(( U32 )( 'w' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { STATE_NUMBER, 0 },            /* current state */
               { ACTION_SET, ACTION_MOVE, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[5] */ 
               0x0bec,              /* magic */
               { CONTROL, BOOK },
               {
                  ((( U32 )( 'c' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 'k' )<<0 ))   /* noun */
               },
               PROPERTY_MOBILE,   /* properties */
               { 0, 0 },            /* current state */
               { ACTION_READ, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[5] */ 
               0x0bec,              /* magic */
               { ELECTRIC, CART },
               {
                  ((( U32 )( 'e' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'c' )<<0 )),  /* adj */
                  ((( U32 )( 'c' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { STATE_SPECIAL, 0 },            /* current state */
               { ACTION_GO, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, C_MAGNOTRON_ELEMENT_0, 0, 
                 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[6] */ 
               0x0bec,              /* magic */
               { 0, TRAVELATOR },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 't' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'v' )<<0 ))   /* noun */
               },
               0,  /* properties */
               { 0, 0 },             /* current state */
               { ACTION_GO, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { C_MAGNOTRON_COMPLEX, 0, 0, 0, 0, 
                 0 } /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MAGNOTRON_ELEMENT_0] */
         0xce11,
         { 0, MAGNOTRON | ELEMENT_0 },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
            ((( U32 )( 'e' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'm' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { RING, MAGNET },
               {
                  ((( U32 )( 'r' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'g' )<<0 )),  /* adj */
                  ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { METAL, BRACKET },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
               },
               PROPERTY_FIXPOINT,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MAGNOTRON_ELEMENT_1] */
         0xce11,
         { 0, MAGNOTRON | ELEMENT_1 },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
            ((( U32 )( 'e' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'm' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { RING, MAGNET },
               {
                  ((( U32 )( 'r' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'g' )<<0 )),  /* adj */
                  ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { METAL, BRACKET },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
               },
               PROPERTY_FIXPOINT,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MAGNOTRON_ELEMENT_2] */
         0xce11,
         { 0, MAGNOTRON | ELEMENT_2 },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
            ((( U32 )( 'e' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'm' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { RING, MAGNET },
               {
                  ((( U32 )( 'r' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'g' )<<0 )),  /* adj */
                  ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { METAL, BRACKET },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
               },
               PROPERTY_FIXPOINT,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MAGNOTRON_ELEMENT_3] */
         0xce11,
         { 0, MAGNOTRON | ELEMENT_3 },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
            ((( U32 )( 'e' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'm' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { RING, MAGNET },
               {
                  ((( U32 )( 'r' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'g' )<<0 )),  /* adj */
                  ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { METAL, BRACKET },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
               },
               PROPERTY_FIXPOINT,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MAGNOTRON_ELEMENT_4] */
         0xce11,
         { 0, MAGNOTRON | ELEMENT_4 },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
            ((( U32 )( 'e' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'm' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { RING, MAGNET },
               {
                  ((( U32 )( 'r' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'g' )<<0 )),  /* adj */
                  ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { METAL, BRACKET },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
               },
               PROPERTY_FIXPOINT,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MAGNOTRON_ELEMENT_5] */
         0xce11,
         { 0, MAGNOTRON | ELEMENT_5 },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
            ((( U32 )( 'e' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'm' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { RING, MAGNET },
               {
                  ((( U32 )( 'r' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'g' )<<0 )),  /* adj */
                  ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { METAL, BRACKET },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
               },
               PROPERTY_FIXPOINT,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* cell[C_MAGNOTRON_ELEMENT_6] */
         0xce11,
         { 0, MAGNOTRON | ELEMENT_6 },  /* output */
         {
            ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 )),  /* adj */
            ((( U32 )( 'e' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'm' )<<0 ))   /* noun */
         },
         {                /* objects */
            {  /* objects[0] */ 
               0x0bec,              /* magic */
               { RING, MAGNET },
               {
                  ((( U32 )( 'r' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'g' )<<0 )),  /* adj */
                  ((( U32 )( 'm' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'g' )<<8 )+(( U32 )( 'n' )<<0 ))   /* noun */
               },
               0,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
               },
               { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
            },

            {  /* objects[1] */ 
               0x0bec,              /* magic */
               { METAL, BRACKET },    /* output description */
               {
                  0,  /* adj */
                  ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'c' )<<0 ))   /* noun */
               },
               PROPERTY_FIXPOINT,   /* properties */
               { 0, 0 },            /* current state */
               { 0, 0, 0, 0 },      /* avatar actions */
               {  /* avatar pre-cond */
                  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
               },
               {  /* avatar post-state */
                  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
               { 0, 0, 0, 0, 0, 0 }      /* ways (N, E, S, W, U, D) */
            }
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      }
   },

   {   /* containers */
      {  /* 0 = FLASK */
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },
      {  /* 1 = JAR */
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },

      {  /* 2 = LARGE POT */
         0x0bec,              /* magic */
         { WARM, BROTH },    /* output description */
         {
            0,  /* adj */
            ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'o' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
         },
         0,                   /* properties */
         { 0, 0 },            /* current state */
         { ACTION_EAT, ACTION_TAKE, 0, 0 },      /* avatar actions */
         {                           /* avatar pre-conds */
            { 0, 0, 0 },                  /* ACTION_EAT */
            { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
            { 0, 0, 0 },
            { 0, 0, 0 }
         },          
         {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* 3 = BAKING TRAY */
         0x0bec,              /* magic */
         { WARM, BREAD },    /* output description */
         {
            0,  /* adj */
            ((( U32 )( 'b' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'e' )<<8 )+( ( U32 )( 'a' )<<0 ))   /* noun */
         },
         0,                   /* properties */
         { 0, 0 },            /* current state */
         { ACTION_EAT, ACTION_TAKE, 0, 0 },      /* avatar actions */
         {                           /* avatar pre-conds */
            { 0, 0, 0 },                  /* ACTION_EAT */
            { TEST_CONTAINER, 0, 0 },     /* ACTION_TAKE */
            { 0, 0, 0 },
            { 0, 0, 0 }
         },          
         {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* 4 = BRONZE JAR */
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },

      {  /* 5 = LEAD FLASK */
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },

      {  /* 6 = CERAMIC CRUCIBLE */
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },

      {  /* 7 = SAFETY HAT */
         0x0bec,              /* magic */
         { SAFETY, LAMP },    /* output description */
         {
            ((( U32 )( 's' )<<24 )+( ( U32 )( 'a' )<<16 )+( ( U32 )( 'f' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'l' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'm' )<<8 )+(( U32 )( 'p' )<<0 ))   /* noun */
         },
         0,   /* properties */
         { STATE_LIGHT, 0 },           /* current state */
         { ACTION_SET, 0, 0, 0 },      /* avatar actions */
         {  /* avatar pre-cond */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },
         {  /* avatar post-state */
            { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* 8 = SINK */
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },

      {  /* 9 = ENERGY CAP */
         0x0bec,              /* magic */
         { GLASS, TESTTUBE },    /* output description */
         {
            ((( U32 )( 't' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 's' )<<8 )+(( U32 )( 't' )<<0 )),  /* adj */
            ((( U32 )( 't' )<<24 )+(( U32 )( 'u' )<<16 )+(( U32 )( 'b' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         PROPERTY_MOBILE | PROPERTY_HIDDEN,
         { STATE_PERCENTAGE, 99 },   /* current state */
         { ACTION_EXCHANGE, 0, 0, 0 },      /* avatar actions */
         {  /* avatar pre-cond */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },
         {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },

      {
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },

      {
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },

      {
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },

      {
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      },

      {
         0xce11   /* The NULL OBJECT
                   * Not to be populated 
                   */
      }
   },

   { /* sources */
      {
         0x0bec,              /* magic */
         { 0, SUN },    /* output description */
         {
            0,  /* adj */
            0   /* noun */
         },
         0,                   /* properties */
         { STATE_NUMBER, 0 },            /* current state */
         { 0, 0, 0, 0 },      /* avatar actions */
         {                           /* avatar pre-conds */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },          
         {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {
         0x0bec,              /* magic */
         { 0, MOON },    /* output description */
         {
            0,  /* adj */
            0   /* noun */
         },
         0,                   /* properties */
         { STATE_NUMBER, 0 },            /* current state */
         { 0, 0, 0, 0 },      /* avatar actions */
         {                           /* avatar pre-conds */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },          
         {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {
         0x0bec,              /* magic */
         { 0, MAGNOTRON },    /* output description */
         {
            0,  /* adj */
            0   /* noun */
         },
         0,                   /* properties */
         { STATE_NUMBER, 0 },            /* current state */
         { 0, 0, 0, 0 },      /* avatar actions */
         {                           /* avatar pre-conds */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },          
         {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {
         0x0bec,              /* magic */
         { 0, ENERGY },    /* output description */
         {
            0,  /* adj */
            0   /* noun */
         },
         0,                   /* properties */
         { STATE_NUMBER, 0 },            /* current state */
         { 0, 0, 0, 0 },      /* avatar actions */
         {                           /* avatar pre-conds */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },          
         {{ 0, 0 },{ 0, 0 },{ 0, 0 },{ 0, 0 }},  /* avatar post-state */
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      }
   },

   ERR_none   /* errnum */
};



const avatar_t adv_default_wilbury =
{
   0xAAA,        /* A_MAGIC */
   "Wilbury",   /* avatar name */
   { 0, WILBURY },  /* output description */
   {                         /* input description */
      0,   /* adj */
      ((( U32 )( 'w' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 'l' )<<8)+(( U32 )( 'b' )<<0))  /* noun */
   },
   C_FOREST_GLADE,  /* current cell location */
   { 0, 0 },             /* current_state */
   {             /* objects */
      {  /* inventory[0] */
         0x0bec,
      }
   },
   0,             /* help */
   30,           /* hunger */
   22            /* thirst */
};

const avatar_t adv_default_nelly =
{
   0xAAA,        /* A_MAGIC */
   "Nelly",   /* avatar name */
   { RED, GNOME },  /* output description */
   {
      ((( U32 )( 0 )<<24)+(( U32 )( 'r' )<<16)+(( U32 )( 'e' )<<8)+(( U32 )( 'd' )<<0)), /* adj */
      ((( U32 )( 'g' )<<24)+(( U32 )( 'n' )<<16)+(( U32 )( 'o' )<<8)+(( U32 )( 'm' )<<0))  /* noun */
   },
   C_CASTLE_THRONEROOM,  /* current cell location */
   { 0, 0 },             /* current_state */
   {             /* objects */
      {  /* objects[0] */ 
         0x0bec,              /* magic */
         { 0, REMOTE },    /* output description */
         {
            0,  /* adj */
            ((( U32 )( 'r' )<<24 )+(( U32 )( 'e' )<<16 )+(( U32 )( 'm' )<<8 )+(( U32 )( 'o' )<<0 ))   /* noun */
         },
         PROPERTY_MOBILE,                   /* properties */
         { STATE_SPECIAL, 0 },             /* current state */
         { ACTION_PRESS, 0, 0, 0 },      /* avatar actions */
         {  /* avatar pre-cond */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },
         {  /* avatar post-state */
            { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* objects[1] */ 
         0x0bec,              /* magic */
         { SHEET, PAPER },    /* output description */
         {
            ((( U32 )( 's' )<<24 )+(( U32 )( 'h' )<<16 )+(( U32 )( 'e' )<<8 )+(( U32 )( 'e' )<<0 )),  /* adj */
            ((( U32 )( 'p' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'p' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         PROPERTY_MOBILE,                   /* properties */
         { STATE_SPECIAL, 0 },             /* current state */
         { ACTION_READ, 0, 0, 0 },      /* avatar actions */
         {  /* avatar pre-cond */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },
         {  /* avatar post-state */
            { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      }
   },
   0,             /* help */
   255,           /* hunger */
   255            /* thirst */
};

const avatar_t adv_default_lucy =
{
   0xAAA,        /* A_MAGIC */
   "Lucy",   /* avatar name */
   { BLONDE, CROW },  /* output description */
   {
      ((( U32 )( 'b' )<<24)+(( U32 )( 'l' )<<16)+(( U32 )( 'o' )<<8)+(( U32 )( 'n' )<<0)), /* adj */
      ((( U32 )( 'c' )<<24)+(( U32 )( 'r' )<<16)+(( U32 )( 'o' )<<8)+(( U32 )( 'w' )<<0))  /* noun */
   },
   C_FOREST_CANOPY,  /* current cell location */
   { 0, 0 },             /* current_state */
   {             /* objects */
      {  /* objects[0] */ 
         0x0bec,              /* magic */
         { STRAW, HAT },    /* output description */
         {
            ((( U32 )( 's' )<<24 )+(( U32 )( 't' )<<16 )+(( U32 )( 'r' )<<8 )+( ( U32 )( 'a' )<<0 )),  /* adj */
            ((( U32 )0<<24 )+(( U32 )( 'h' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
         },
         PROPERTY_WORN,                   /* properties */
         { 0, 0 },             /* current state */
         { 0, 0, 0, 0 },      /* avatar actions */
         {  /* avatar pre-cond */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },
         {  /* avatar post-state */
            { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      }
   },
   0,             /* help */
   255,           /* hunger */
   255            /* thirst */
};

const avatar_t adv_default_buster =
{
   0xAAA,        /* A_MAGIC */
   "Buster",   /* avatar name */
   { PINK, MOLE },  /* output description */
   {
      ((( U32 )( 'p' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 'n' )<<8)+(( U32 )( 'k' )<<0)), /* adj */
      ((( U32 )( 'm' )<<24)+(( U32 )( 'o' )<<16)+(( U32 )( 'l' )<<8)+(( U32 )( 'e' )<<0))  /* noun */
   },
   C_MAGNOTRON_COMPLEX,  /* current cell location */
   { 0, 0 },             /* current_state */
   {             /* objects */
      {  /* objects[0] */ 
         0x0bec,              /* magic */
         { GOLD, O_RING },    /* output description */
         {
            ((( U32 )( 'g' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 'l' )<<8 )+(( U32 )( 'd' )<<0 )),  /* adj */
            ((( U32 )( 'r' )<<24 )+(( U32 )( 'i' )<<16 )+(( U32 )( 'n' )<<8 )+(( U32 )( 'g' )<<0 ))   /* noun */
         },
         PROPERTY_WORN | PROPERTY_WEARABLE | PROPERTY_MOBILE, /* properties */
         { STATE_SPECIAL, 0 },             /* current state */
         { 0, 0, 0, 0 },      /* avatar actions */
         {  /* avatar pre-cond */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },
         {  /* avatar post-state */
            { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* objects[1] */ 
         0x0bec,              /* magic */
         { HARD, HAT },    /* output description */
         {
            ((( U32 )( 'h' )<<24 )+( ( U32 )( 'a' )<<16 )+(( U32 )( 'r' )<<8 )+(( U32 )( 'd' )<<0 )),  /* adj */
            ((( U32 )0<<24 )+(( U32 )( 'h' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 't' )<<0 ))   /* noun */
         },
         PROPERTY_WORN,                   /* properties */
         { 0, 0 },             /* current state */
         { 0, 0, 0, 0 },      /* avatar actions */
         {  /* avatar pre-cond */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },
         {  /* avatar post-state */
            { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* objects[2] */ 
         0x0bec,              /* magic */
         { CRIB, NOTE },    /* output description */
         {
            ((( U32 )( 'c' )<<24 )+(( U32 )( 'r' )<<16 )+(( U32 )( 'i' )<<8 )+(( U32 )( 'b' )<<0 )),  /* adj */
            ((( U32 )( 'n' )<<24 )+(( U32 )( 'o' )<<16 )+(( U32 )( 't' )<<8 )+(( U32 )( 'e' )<<0 ))   /* noun */
         },
         PROPERTY_HIDDEN,     /* properties */
         { 0, 0 },            /* current state */
         { ACTION_READ, 0, 0, 0 },      /* avatar actions */
         {  /* avatar pre-cond */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },
         {  /* avatar post-state */
            { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      }
   },
   0,             /* help */
   255,           /* hunger */
   255            /* thirst */
};

const avatar_t adv_default_charlie =
{
   0xAAA,        /* A_MAGIC */
   "Charlie",   /* avatar name */
   { LONG_HAIRED, WIZARD },  /* output description */
   {
      ((( U32 )( 'l' )<<24)+(( U32 )( 'o' )<<16)+(( U32 )( 'n' )<<8)+(( U32 )( 'g' )<<0)), /* adj */
      ((( U32 )( 'w' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 'z' )<<8)+(( U32 )( 'a' )<<0))  /* noun */
   },
   C_MOUNTAIN_OBSERVATORY, /* current cell location */
   { 0, 0 },             /* current_state */
   {             /* objects */
      {  /* inventory[0] */
         0x0bec
      }
   },
   0,             /* help */
   255,           /* hunger */
   255            /* thirst */
};

const avatar_t adv_default_gary =
{
   0xAAA,        /* A_MAGIC */
   "Gary",   /* avatar name */
   { GREEN, RAT },  /* output description */
   {
      ((( U32 )( 'g' )<<24)+(( U32 )( 'r' )<<16)+(( U32 )( 'e' )<<8)+(( U32 )( 'e' )<<0)), /* adj */
      ((( U32 )( 0 )<<24)+(( U32 )( 'r' )<<16)+(( U32 )( 'a' )<<8)+(( U32 )( 't' )<<0))  /* noun */
   },
   C_CASTLE_DUNGEON,  /* current cell location */
   { 0, 0 },             /* current_state */
   {             /* objects */
      {  /* objects[0] */ 
         0x0bec              /* magic */
      }
   },
   0,             /* help */
   255,           /* hunger */
   255            /* thirst */
};

const avatar_t adv_default_lofty =
{
   0xAAA,        /* A_MAGIC */
   "Lofty",   /* avatar name */
   { BLACK, WIZARD },  /* output description */
   {
      ((( U32 )( 'b' )<<24)+(( U32 )( 'l' )<<16)+(( U32 )( 'a' )<<8)+(( U32 )( 'c' )<<0)), /* adj */
      ((( U32 )( 'w' )<<24)+(( U32 )( 'i' )<<16)+(( U32 )( 'z' )<<8)+(( U32 )( 'a' )<<0))  /* noun */
   },
   C_CASTLE_SPIRE,  /* current cell location */
   { 0, 0 },             /* current_state */
   {             /* objects */
      {  /* objects[0] */ 
         0x0bec              /* magic */
      }
   },
   0,             /* help */
   255,           /* hunger */
   255            /* thirst */
};

const avatar_t adv_default_otto =
{
   0xAAA,        /* A_MAGIC */
   "Otto",   /* avatar name */
   { BLUE, MONKEY },  /* output description */
   {
      ((( U32 )( 'b' )<<24)+(( U32 )( 'l' )<<16)+(( U32 )( 'u' )<<8)+(( U32 )( 'e' )<<0)), /* adj */
      ((( U32 )( 'm' )<<24)+(( U32 )( 'o' )<<16)+(( U32 )( 'n' )<<8)+(( U32 )( 'k' )<<0))  /* noun */
   },
   C_MOUNTAIN_LAB,  /* current cell location */
   { 0, 0 },             /* current_state */
   {             /* objects */
      {  /* objects[0] */ 
         0x0bec,              /* magic */
         { DARK, SHADES },    /* output description */
         {
            0,  /* adj */
            ((( U32 )( 's' )<<24 )+(( U32 )( 'h' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'd' )<<0 ))   /* noun */
         },
         PROPERTY_WORN,                   /* properties */
         { 0, 0 },             /* current state */
         { 0, 0, 0, 0 },      /* avatar actions */
         {  /* avatar pre-cond */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },
         {  /* avatar post-state */
            { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      },

      {  /* objects[0] */ 
         0x0bec,              /* magic */
         { LAB, CLOAK },    /* output description */
         {
            ((( U32 )0<<24 )+(( U32 )( 'l' )<<16 )+( ( U32 )( 'a' )<<8 )+(( U32 )( 'b' )<<0 )),  /* adj */
            ((( U32 )( 'c' )<<24 )+(( U32 )( 'l' )<<16 )+(( U32 )( 'o' )<<8 )+( ( U32 )( 'a' )<<0 ))   /* noun */
         },
         PROPERTY_WORN,                   /* properties */
         { 0, 0 },             /* current state */
         { 0, 0, 0, 0 },      /* avatar actions */
         {  /* avatar pre-cond */
            { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
         },
         {  /* avatar post-state */
            { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } 
         },
         { 0, 0, 0, 0, 0, 0 } /* ways (N, E, S, W, U, D) */
      }
   },
   0,             /* help */
   255,           /* hunger */
   255            /* thirst */
};

const avatar_t adv_default_harry =
{
   0xAAA,        /* A_MAGIC */
   "arry",   /* avatar name */
   { FAST, BAT },  /* output description */
   {
      0, /* adj */
      ((( U32 )( 0 )<<24)+(( U32 )( 'b' )<<16)+(( U32 )( 'a' )<<8)+(( U32 )( 't' )<<0))  /* noun */
   },
   C_MINE_TUNNEL,  /* current cell location */
   { 0, 0 },             /* current_state */
   {             /* objects */
      {  /* inventory[0] */
         0x0bec
      }
   },
   0,             /* help */
   255,           /* hunger */
   255            /* thirst */
};

/*
 ************************* Local Functions
 */

static ERR adv__read_file( 
      char const * const fname,
      data_t d[],
      size_t const cnt
    )
{
   ERR e = ERR_none;
   int f;
   int i;
   size_t sz;
   size_t n;
   char *p;


   DEBUGF( "adv__read_file '%s'\n", fname );
   #if defined( __clang__ )
      f = mos_fopen( fname, FA_READ );
   #else
      f = open( fname, O_RDONLY );
   #endif
   if( 0 < f )
   {
      for( i=0;( i < cnt )&&( ERR_none == e ); i++ )
      {
         DEBUGF( "reading module\n" );

         sz = d[ i ].data_sz;
         n = 0;
         p = d[ i ].data;

         while( sz > 0 )
         {
#           if defined( __clang__ )
               n = mos_fread( f, p, sz );
#           else
               n = read( f, p, sz );
#           endif
            if( 0 >= n )
            {
               /* failed to read from file */
               DEBUGF( "error reading file\n" );
               e = ERR_file;
               sz = 0;
            }
            else
            {
               sz -= n;
               p += n;
            }
         }
      }

#     if defined( __clang__ )
         mos_fclose( f );
#     else
         close( f );
#     endif
      DEBUGF( "finished reading file\n" );
   }
   else
   {
      DEBUGF( "failed to open file\n" );
      e = ERR_file;
   }

   return( e );
}

/* adv__write_file will overwrite the data file;
 * only call if anything has changed, to reduce wear & tear on SRAMs
 */
static ERR adv__write_file( 
      char const * const fname,
      data_t d[],
      size_t const cnt
    )
{
   ERR e = ERR_none;
   int f;
   int i;
   size_t sz;
   size_t n;
   char *p;


   DEBUGF( "adv__write_file '%s'\n", fname );

   /* if the file exists, delete it */
#  if defined( __clang__ )
      f = mos_fopen( fname, FA_OPEN_EXISTING );
#  else
      f = open( fname, O_RDONLY );
#  endif
   if( 0 < f )
   {
#  if defined( __clang__ )
      mos_fclose( f );
#  else
         close( f );
#  endif
      DEBUGF( "deleting old file\n" );
#     if defined( __clang__ )
         mos_del( fname );
#     else
         ( void )unlink( fname );
#     endif
   }

   /* create a new file */
#  if defined( __clang__ )
      f = mos_fopen( fname, FA_WRITE | FA_CREATE_NEW );
#  else
      f = open( fname, O_WRONLY | O_CREAT | O_TRUNC, 0440 /* S_IRUSR | S_IRGRP */ );
#  endif
   if( 0 < f )
   {
      DEBUGF( "writing new file\n" );
      for( i=0;( i < cnt )&&( ERR_none == e ); i++ )
      {
         DEBUGF( "writing file module\n" );

         sz = d[ i ].data_sz;
         p = d[ i ].data;
         n = 0;

         while( sz > 0 )
         {
#           if defined( __clang__ )
               n = mos_fwrite( f, p, sz );
#           else
               n = write( f, p, sz );
#           endif
            if( 0 >= n )
            {
               /* failed to write to file */
               DEBUGF( "failed to write file\n" );
               e = ERR_file;
               sz = 0;
            }
            else
            {
               sz -= n;
               p += n;
            }
         }
      }
#     if defined( __clang__ )
         mos_fclose( f );
#     else
         close( f );
#     endif
      DEBUGF( "close new file\n" );
   }
   else
   {
      DEBUGF( "failed to create new file\n" );
      e = ERR_file;
   }

   return( e );
}

ERR adv__read( void )
{
   ERR e = ERR_none;

   DEBUGF( "adv__read\n" );
   e = adv__read_file( world_file,
                       world_and_avatar, 
                       sizeof( world_and_avatar )/ sizeof( data_t ));

   return( e );
}

ERR adv__write( void )
{
   ERR e = ERR_none;

   DEBUGF( "adv__write\n" );
   e = adv__write_file( world_file,
                        world_and_avatar, 
                        sizeof( world_and_avatar )/ sizeof( data_t ));

   return( e );
}
