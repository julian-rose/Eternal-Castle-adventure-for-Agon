/*
 * Module: adv_main.c
 *
 * Purpose: Entry point for adventure game
 *
 *          This source also collects all the functions that will need porting
 *          to another platform (e.g. Android, iOS).
 *          The other sources shouldn't need to be modified.
 *
 * Author: Copyright (c) julian rose, jhr
 *
 * Compile: gcc -Wall -std=c90 -pedantic -DDEBUG -DVARARGS -o adventure \
 *              adv_main.c adv_file.c adv_run.c adv_dict.c adv_util.c
 *
 *          compile for Agon using AgDev make
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
#include <stdio.h>
#include <stdlib.h>
#if !defined( __clang__ )
#  include <unistd.h>
#endif
#include <ctype.h>
#include <string.h>

#include "adv_types.h"
#include "adv_dict.h"


/*
 *************************** Constants
 */
#define WORLD_FILE "ETERNAL.DAT"


/*
 ************************* Private Data
 */
static UC ins[ INBUF_LEN ];     /* input buffer */


/*
 ************************* Global Data
 */
extern world_t  adv_world;
char world_file[ 128 ];              /* load/save WORLD_FILE name */
U32C screen_width = 80;
TRISTATE fullGameEnabled = _TRUE;  /* false limits to forest zone */
TRISTATE fullGameLimitHit = _FALSE;/* true when avatar tries limited location */



/*
 ************************* Local Functions
 */
int main( int const argc, char const **argv )
{
   ERR e = ERR_none;
   char const *c = ".";


   ( void )strncpy( world_file, c, 108 );  /* allow space for "/<file.name> */
   ( void )strcat( world_file, "/" );
   ( void )strcat( world_file, WORLD_FILE );


   adv__load( );

   if( ERR_none !=( e = adv__welcome( )))
   {
      throw( __LINE__, e );
   }

   if( ERR_game == adv_world.errnum )
   {
      /* game was previously ended, so start anew */
      PRINTS _THE, _GAME, _WAS, _PREVIOUSLY, _ENDED, _COMMA,
            _SO, _STARTING, _ANEW, _FULLSTOP, _NEWLINE END_PRINT
      adv__new( );
   }

   while(( ERR_none == e )&&( ERR_game != adv_world.errnum ))
   {
      if( ERR_none !=( e = adv__get_input( )))
      {
         throw( __LINE__, e );
      }
      if( ERR_none !=( e = adv__perform_input( )))
      {
         throw( __LINE__, e );
      }
   }

   /* atomic */
   {
      adv__on_exit( _FALSE );
   }
   return( 0 );
}

/* on_exit is "connected" to the ipad END button */
void adv__on_exit( TRISTATE const bye )
{
   adv__goodbye( bye );
#  if !defined( __clang__)
      _exit( 0 );
#  endif
}

void throw( int const l, ERR const e )
{
   adv__print_error( e );
   adv__goodbye( _FALSE );
#  if !defined( __clang__ )
      _exit( 1 );
#  endif
}

ERR adv__get_input( void )
{
   ERR e = ERR_none;
   int i;
   TRISTATE done = _FALSE;
   size_t const sz = sizeof( ins );
   int c;


   display_initial_help( );

   ( void )fwrite( "\n> ", sizeof( char ), 3, stdout );  /* prompt */
   ( void )fflush( stdout );
   ( void )memset( ins, 0, sz );

   for( i=0;( _FALSE == done )&&( i < sz ); i++ )
   {
      c = fgetc( stdin );
      if( isalnum( c ) || ( ' ' == c )
#       if defined( DEBUG )
           ||( '_' == c )
#       endif
        )
      {
         ins[ i ]= tolower( c );
      }
      else
      {
         done = _TRUE;
      }
   }

   /* throw away rest of any input */
   while(( c != '\n' )&&( c != EOF ))
   {
      c = fgetc( stdin );
   }

   set_inbuf( ins, strlen(( char* )ins ));

   PRINTS _NEWLINE END_PRINT

   return( e );
}

void adv__print( char const * const p )
{
   ( void )printf( "%s\n", p );
}

