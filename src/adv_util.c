/*
 * Module: adv_util.c
 *
 * Purpose: Utility functions for adventure game
 *
 * Author: Copyright (c) julian rose, jhr
 *
 * Version:	who	when	what
 *		---	------	----------------------------------------
 *		jhr	150417	wrote initial version
 *				test data structure ideas
 *				test basic command line
 */

#include <stdio.h>
#include <stdlib.h>
#if !defined( __clang__ )
#  include <unistd.h>
#endif

#include "adv_types.h"
#include "adv_dict.h"



/*
 ************************* Global Data
 */
extern world_t  adv_world;
extern avatar_t adv_avatar[ A_NUM_AVATARS ];
extern world_t adv_default_world;
extern avatar_t adv_default_wilbury;
extern avatar_t adv_default_lucy;
extern avatar_t adv_default_gary;
extern avatar_t adv_default_nelly;
extern avatar_t adv_default_harry;
extern avatar_t adv_default_buster;
extern avatar_t adv_default_lofty;
extern avatar_t adv_default_otto;
extern avatar_t adv_default_charlie;

extern int __trace;
extern char outbuf[ ];



/*
 ************************* Local Functions
 */

void adv__print_welcome( void )
{
   /* display a welcome screen shot based on world */
   PRINTS _WELCOME, _TO END_PRINT
   SPRINT_( outbuf, "%s.\n", adv_world.name );
#  if defined( __CYGWIN__ )
   SPRINT_( outbuf, "%s", "Copyright (C) Julian Rose," );
   SPRINT_( outbuf, "version %s.", __DATE__ );
   SPRINT_( outbuf, "%s\n\n", "All rights reserved." );
#  endif

   PRINTS _I, _AM END_PRINT
   SPRINT_( outbuf, "%s", adv_avatar[ A_WILBURY ].name );
   PRINTS _COMMA, _YOUR, _AVATAR, _IN, _THIS, _TEXT, _BASED, _ADVENTURE, 
            _GAME, _FULLSTOP, _NEWLINE END_PRINT
}

ERR adv__welcome( void )
{
   adv__print_welcome( );
   return( ERR_none );
}

void adv__game_over( enum _GAME_OVER const reason )
{
   PRINTS _GAME, _OVER, _MINUS END_PRINT

   switch( reason )
   {
      case GAME_OVER_AVATAR_WON :
      {
         SPRINT_( outbuf, "%s WON!!\n", adv_avatar[ A_WILBURY ].name );
      }
      break;

      case GAME_OVER_AVATAR_RESOURCE :
      {
         SPRINT_( outbuf, "%s", adv_avatar[ A_WILBURY ].name );
         PRINTS _DECEASED, _FROM, _LACK, _OF END_PRINT
         if( 0 == adv_avatar[ A_WILBURY ].thirst )
         {
            PRINTS _WATER END_PRINT
         }
         else
         {
            PRINTS _FOOD END_PRINT
         }
         PRINTS _FULLSTOP, _NEWLINE END_PRINT
      }
      break;

      case GAME_OVER_LIGHTBEAM_BLOWOUT :
      {
         PRINTS _THE, _LIGHTBEAM, _EXPLODES, _EXCLAMATION, _NEWLINE END_PRINT
      }
      break;

      case GAME_OVER_SUN_BLOWOUT :
      {
         PRINTS _THE, _UNFILTERED, _SUNLIGHT, _IS, _TOO, _MUCH, _FOR, _THE,
                _SYSTEM, _EXCLAMATION, _NEWLINE END_PRINT
      }
      break;

      case GAME_OVER_TRAPPED :
      {
         PRINTS _AVATAR, _IS, _TRAPPED, _EXCLAMATION, _NEWLINE END_PRINT
      }
      break;

      case GAME_OVER_BOARD_BLOWOUT :
      {
         PRINTS _IT, _IS, _NOT, _POSSIBLE, _TO, _MAKE, _ANY, _MORE, _BOARDS,
                _EXCLAMATION, _NEWLINE END_PRINT
      }
      break;

      default:
      {
         PRINTS _UNKNOWN, _REASONS, _NEWLINE END_PRINT
      }
      break;
   }
}

void adv__print_goodbye( void )
{
   /* display a goodbye screen shot based on world */
   PRINTS _GOODBYE, _FROM END_PRINT
   SPRINT_( outbuf, "%s\n", adv_world.name );
}

void adv__save( void )
{
   ERR e = ERR_none;


#  if defined( __CYGWIN__ )
   PRINTS _WRITING, _SAVE, _POINT, _ELIPSIS, _NEWLINE END_PRINT
#  endif
   e = adv__write( );
   if( ERR_none != e )
   {
      throw( __LINE__, e );
   }
}

ERR adv__goodbye( TRISTATE const silent )
{
   if( _FALSE == silent )
   {
      adv__print_goodbye( );
   }

   if( ERR_game != adv_world.errnum )
   {
      adv__save( );
   }

   return( ERR_none );
}

void adv__new( void )
{
   DEBUGF( "adv__new\n" );

   /* use defaults */
   adv_world = adv_default_world;
   adv_avatar[0]= adv_default_wilbury;
   adv_avatar[1]= adv_default_lucy;
   adv_avatar[2]= adv_default_gary;
   adv_avatar[3]= adv_default_nelly;
   adv_avatar[4]= adv_default_buster;
   adv_avatar[5]= adv_default_harry;
   adv_avatar[6]= adv_default_lofty;
   adv_avatar[7]= adv_default_otto;
   adv_avatar[8]= adv_default_charlie;
}

void adv__load( void )
{
   ERR e = ERR_none;


#  if defined( __CYGWIN__ )
   PRINTS _READING, _SAVE, _POINT, _ELIPSIS, _NEWLINE END_PRINT
#  endif

   /* reads world and avatar data;
    * if absent or an error, use defaults silently. */
   e = adv__read( );
   if( ERR_none != e )
   {
      adv__new( );
   }
}

void adv__print_error( ERR const e )
{
#  if defined( DEBUG )
   PRINTS _ERROR, _COLON END_PRINT
   SPRINT_( outbuf, "%d\n", e );
#  endif
}
