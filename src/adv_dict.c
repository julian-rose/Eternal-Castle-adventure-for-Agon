/*
 * Module: adv_dict.c
 *
 * Purpose: Runtime for adventure game
 *
 * Author: Copyright (c) julian rose, jhr
 *
 * Version:	who	when	what
 *		---	------	----------------------------------------
 *		jhr	150705	wrote initial version
 */

#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#if defined( VARARGS )
# include <stdarg.h>
#endif
#include <assert.h>

#include "adv_types.h"
#include "adv_dict.h"


/*
 ************************* Global Data
 */
static UC pbuf[ OUTBUF_LEN ]={ 0 };
static int pbuf_idx = 0;
extern U32C screen_width;
extern int __trace;

static char const * const adv_words[ __END ]=  /* SAME ORDER AS enum */
{
   "0",
   "1",
   "2",
   "3",
   "4",
   "5",
   "6",
   "7",
   "8",
   "9",
   "a",
   "about",
   "above",
   "acts",
   "actually",
   "addition",
   "adjust",
   "admittance",
   "adventure",
   "affect",
   "after",
   "again",
   "aged",
   "ah",
   "al",
   "all",
   "aloft",
   "already",
   "also",
   "am",
   "an",
   "ancestral",
   "and",
   "anew",
   "another",
   "anti",
   "any",
   "anything",
   "anyway",
   "apparatus",
   "appear",
   "appears",
   "appreciative",
   "arbaro",
   "are",
   "around",
   "arrives",
   "as",
   "ascend",
   "ascending",
   "ascent",
   "ash",
   "ask",
   "asking",
   "asks",
   "assembling",
   "assembly",
   "at",
   "attach",
   "attached",
   "atop",
   "audible",
   "avatar",
   "avatars",
   "away",
   "back",
   "band",
   "\b",   /* BACKSPACE */
   "baking",
   "balcony",
   "barred",
   "bars",
   "based",
   "bat",
   "be",
   "beam",
   "bearings",
   "beech",
   "before",
   "begin",
   "begins",
   "below",
   "beware",
   "beyond",
   "bit",
   "birch",
   "black",
   "blinding",
   "blonde",
   "blue",
   "board",
   "boards",
   "book",
   "boots",
   "bottom",
   "bounces",
   "bouncing",
   "bracket",
   "(",   /* BRACKET_LEFT */
   ")",   /* BRACKET_RIGHT */
   "bread",
   "break",
   "briefly",
   "brighter",
   "bring",
   "brings",
   "broken",
   "bronze",
   "broth",
   "brown",
   "brushes",
   "bubbling",
   "build",
   "burden",
   "burette",
   "burner",
   "but",
   "button",
   "buttons",
   "buzzer",
   "by",
   "bye",
   "cable",
   "cage",
   "campfire",
   "can",
   "cannot",
   "canopy",
   "cap",
   "carbon",
   "card",
   "care",
   "carriage",
   "carries",
   "carry",
   "carrying",
   "cart",
   "castle",
   "cavern",
   "cell",
   "cellar",
   "ceramic",
   "certain",
   "chain",
   "chair",
   "chairs",
   "change",
   "changes",
   "charge",
   "check",
   "chime",
   "chimes",
   "chronometric",
   "circumnavigation",
   "clank",
   "click",
   "clicks",
   "climb",
   "cloak",
   "clockwise",
   "close",
   "cogs",
   "cold",
   "collecting",
   "collector",
   ":",   /* COLON - add a backspace for punctuation */
   "colour",
   "combinations",
   "coming",
   ",",   /* COMMA - add a backspace for punctuation */
   "command",
   "commands",
   "common",
   "complete",
   "complex",
   "concise",
   "concur",
   "connected",
   "constructed",
   "container",
   "containing",
   "contains",
   "context",
   "continue",
   "continues",
   "control",
   "controller",
   "controls",
   "cool",
   "could",
   "course",
   "courtyard",
   "crack",
   "crib",
   "crossed",
   "crow",
   "crucible",
   "current",
   "currently",
   "cutting",
   "D",
   "dark",
   "darkness",
   "deadly",
   "deceased",
   "deck",
   "decrease",
   "deep",
   "degrees",
   "deposits",
   "descend",
   "descending",
   "descent",
   "desert",
   "destination",
   "destroy",
   "destroyed",
   "diagrams",
   "diamond",
   "diamonds",
   "did",
   "didn't",
   "difference",
   "different",
   "difficult",
   "digging",
   "digsmanship",
   "dining",
   "direct",
   "direction",
   "directions",
   "directly",
   "disappears",
   "discover",
   "disembark",
   "disengage",
   "dish",
   "disperses",
   "distance",
   "distant",
   "distinguish",
   "divide",  /* _DIVIDE */
   "divingsuit",
   "do",
   "doesn't",
   "doff",
   "dome",
   "done",
   "don't",
   "door",
   "doubt",
   "down",
   "downwards",
   "draw",
   "drawbridge",
   "drink",
   "drips",
   "drives",
   "dropping",
   "drops",
   "dungeon",
   "dust",
   "E",
   "each",
   "east",
   "eat",
   "edge",
   "effect",
   "effervescing",
   "efficiency",
   "effortlessly",
   "eggs",
   "eh",
   "electric",
   "element",
   "elements",
   "...",   /* ELIPSIS */
   "else",
   "elusive",
   "empty",
   "ended",
   "energio",
   "energy",
   "engage",
   "engaging",
   "enter",
   "entering",
   "entrance",
   "entries",
   "entry",
   "=",     /* _EQUALS */
   "equations",
   "equipment",
   "error",
   "\\h",   /* ESCAPE-h, erase current line */
   "\\l",   /* ESCAPE-l, force lower case */
   "\\u",   /* ESCAPE-u, force upper case */
   "estraro",
   "etching",
   "ethereal",
   "example",
   "exchange",
   "!",     /* EXCLAMATION - add a backspace for punctuation */
   "expectantly",
   "experiment",
   "experiments",
   "explodes",
   "explosion",
   "eye",
   "eyes",
   "face",
   "faces",
   "facility",
   "factory",
   "faint",
   "faintly",
   "falls",
   "far",
   "fari",
   "fast",
   "feel",
   "fellow",
   "fetch",
   "few",
   "fill",
   "filled",
   "filler",
   "fills",
   "filter",
   "filters",
   "filtrilon",
   "finally",
   "find",
   "finding",
   "finish",
   "finished",
   "first",
   "fish",
   "fit",
   "fix",
   "fixed",
   "flask",
   "flies",
   "floods",
   "floor",
   "flourish",
   "fluttering",
   "focus",
   "focused",
   "focusing",
   "follow",
   "follows",
   "followed",
   "food",
   "footnote",
   "for",
   "forest",
   "form",
   "M=E/C-squared",  /* _FORMULA_ */
   "forwards",
   "fresh",
   "from",
   "fuel",
   "full",
   ".",   /* FULLSTOP - add a backspace for punctuation */
   "function",
   "funicular",
   "future",
   "gain",
   "game",
   "gave",
   "gear",
   "gears",
   "general",
   "get",
   "getting",
   "gets",
   "give",
   "gives",
   "glade",
   "glances",
   "glass",
   "glide",
   "goggles",
   "gnome",
   "go",
   "goes",
   "going",
   "gold",
   "golden",
   "gone",
   "good",
   "goodbye",
   "got",
   "graph",
   "gravitational",
   "great",
   "green",
   "grey",
   "ground",
   "guard",
   "guides",
   "had",
   "halo",
   "handle",
   "hands",
   "happen",
   "happens",
   "hard",
   "has",
   "hat",
   "have",
   "he",
   "head",
   "hear",
   "heart",
   "heat",
   "heavy",
   "heck",
   "hello",
   "help",
   "here",
   "he's",
   "hewn",
   "hidden",
   "hide",
   "hilltop",
   "him",
   "hmm",
   "hoist",
   "hold",
   "holds",
   "hope",
   "horizon",
   "how",
   "huff",
   "hundreds",
   "hungry",
   "i",
   "I", 
   "idea",
   "ideas",
   "if",
   "ignores",
   "illegible",
   "i'm",
   "image",
   "impressed",
   "impurities",
   "in",
   "including",
   "increase",
   "indicates",
   "inked",
   "innovate",
   "insert",
   "inside",
   "install",
   "installed",
   "instead",
   "instructions",
   "interact",
   "interest",
   "into",
   "invented",
   "inventory",
   "is",
   "it",
   "items",
   "it's",
   "itself",
   "i've",
   "jacket",
   "jar",
   "journey",
   "joystick",
   "jumps",
   "just",
   "kastelo",
   "keep",
   "kettle",
   "key",
   "keys",
   "kitchen",
   "KM",
   "knobs",
   "know",
   "la",
   "lab",
   "laboratory",
   "lack",
   "ladder",
   "lake",
   "lamp",
   "lapping",
   "large",
   "laser",
   "last",
   "latest",
   "lead",
   "leads",
   "left",
   "lens",
   "lenso",
   "<",     /* LESS */
   "let's",
   "level",
   "lever",
   "light",
   "lightbeam",
   "like",
   "line",
   "lining",
   "linking",
   "liquid",
   "listen",
   "little",
   "load",
   "loading",
   "locate",
   "locations",
   "lock",
   "locked",
   "long",
   "long-haired",
   "longer",
   "look",
   "looking",
   "looks",
   "lose",
   "loud",
   "low",
   "lucy",
   "luckily",
   "luminous",
   "made",
   "magic",
   "magnetic",
   "magnet",
   "magnets",
   "magnotron",
   "make",
   "making",
   "man",
   "manage",
   "manual",
   "manufacture",
   "many",
   "map",
#  if defined( __CYGWIN__ )
   "  Reflections off the dark side            ()",
   "  -----------------------------           /  \\",
   "                                         /    \\",
   "                                        /      \\",
   "               /\\                      /        \\",
   "              /  \\                    /          \\",
   "       /\\    / [] \\                  /            \\",
   "      /  \\  /      \\/\\              /        /|\\   \\",
   "     /    \\        /  \\    _   _   _ -----/|\\/|\\--- -)",
   "    /      \\      /    \\  | |_| |_| |    /|\\/|\\/|\\   |\\",
   "   /        \\    /        |   ___   |    /|\\ | /|\\",
   "  /          \\  /         |  |   |  |     |     |",
   " /            \\/          ===========",
#  else
   "MAP0..12",
   "",
   "",
   "",
   "",
   "",
   "",
   "",
   "",
   "",
   "",
   "",
   "",
#  endif
   "mapo",
   "margin",
   "marks",
   "marvellous",
   "mass",
   "mast",
   "master",
   "math",
   "may",
   "me",
   "measure",
   "mechanism",
   "melting",
   "metal",
   "meter",
   "method",
   "might",
   "milky",
   "mind",
   "mine",
   "miners",
   "mines",
   "-",    /* MINUS */
   "minute",
   "mirror",
   "mirrors",
   "missing",
   "mission",
   "mister",
   "moat",
   "mole",
   "monkey",
   "months",
   "montoj",
   "monument",
   "moon", 
   "moonlit",
   "more",
   ">",    /* MORE */
   "most",
   "mostly",
   "motions",
   "mountain",
   "mountains",
   "move",
   "moves",
   "moving",
   "much",
   "multiple",
   "mushrooms",
   "must",
   "muted",
   "muttering",
   "my",
   "myself",
   "N",
   "name",
   "navigate",
   "need",
   "needs",
   "nest",
   "network",
   "new",
   "\n",   /* NEWLINE */
   "newly",
   "next",
   "no",
   "nods",
   "noise",
   "north",
   "noun",
   "now",
   "not",
   "note",
   "nothing",
   "oak",
   "object",
   "observation",
   "observatory",
   "of",
   "off",
   "oiling",
   "oh",
   "old",
   "on",
   "once",
   "one",
   "onto",
   "oops",
   "open",
   "operating",
   "operation",
   "optimum",
   "or",
   "orange",
   "order",
   "other",
   "otherwise",
   "our",
   "out",
   "output",
   "outside",
   "over",
   "overall",
   "pace",
   "page",
   "pages",
   "paintings",
   "pair",
   "panel",
   "paper",
   "part",
   "particularly",
   "pass",
   "passage",
   "path",
   "peak",
   "photon",
   "phrases",
   "pick",
   "pinion",
   "pink",
   "places",
   "play",
   "please",
   "plough",
   "plus", /* _PLUS */
   "point",
   "pointing",
   "points",
   "polished",
   "polishing",
   "pool",
   "pop",
   "portcullis",
   "portrait",
   "portraits",
   "position",
   "possible",
   "pot",
   "pour",
   "pouring",
   "power",
   "press",
   "pressed",
   "previously",
   "problem",
   "projection",
   "protection",
   "protective",
   "pull",
   "push",
   "?",   /* QUESTION */
   "quickly",
   "quit",
   "quite",
   "`",   /* QUOTE_LEFT */
   "'",   /* QUOTE_RIGHT */
   "rack",
   "range",
   "rat",
   "ray",
   "reaching",
   "read",
   "reading",
   "ready",
   "really",
   "reappears",
   "reasons",
   "receive",
   "receiver",
   "red",
   "reduce",
   "reflected",
   "reflections",
   "reflecting",
   "reflective",
   "reflector",
   "reflects",
   "refreshing",
   "remaining",
   "remember",
   "remote",
   "remove",
   "replies",
   "required",
   "requiring",
   "resembles",
   "respond",
   "rest",
   "resulting",
   "resume",
   "returning",
   "reverse",
   "rigged",
   "right",
   "ring",
   "risky",
   "rock",
   "roof",
   "room",
   "roster",
   "rotary",
   "rotates",
   "rotting",
   "rough",
   "round",
   "rumbling",
   "running",
   "runs",
   "S",
   "safety",
   "saltilo",
   "sand",
   "sat",
   "save",
   "saving",
   "say",
   "says",
   "scampers",
   "scorch",
   "scratching",
   "screen",
   "scurries",
   "seat",
   "seaweed",
   "second",
   "see", 
   "seeks",
   "seems",
   "sees",
   "select",
   "selected",
   ";",  /* SEMICOLON */
   "sense",
   "sensitive",
   "set",
   "setting",
   "settings",
   "several",
   "shades",
   "shaft",
   "shallow",
   "she",
   "sheet",
   "short",
   "shortcuts",
   "shorts",
   "should",
   "shovel",
   "show",
   "shows",
   "side",
   "sight",
   "sign",
   "silver",
   "silvered",
   "similarly",
   "sink",
   "sins",
   "size",
   "sketch",
   "sky",
   "slider",
   "slowly",
   "slosilo",
   "small",
   "smell",
   "smells",
   "smoothly",
   "sniffing",
   "snow",
   "so",
   "soldering",
   "some",
   "someplace",
   "something",
   "sorry",
   "sorts",
   "sound",
   "sounds",
   "source",
   "south",
   " ",  /* _SP */
   "space",
   "spare",
   "speaks",
   "special",
   "specific",
   "spectacles",
   "speculatively",
   "spegulon",
   "spills",
   "spire",
   "stairway",
   "stand",
   "standing",
   "stands",
   "stars",
   "start",
   "starting",
   "starts",
   "state",
   "states",
   "step",
   "still",
   "straight",
   "strange",
   "strangely",
   "straw",
   "stretch",
   "store",
   "stores",
   "subterranean",
   "success",
   "such",
   "sudden",
   "suddenly",
   "suggest",
   "suitable",
   "sum",
   "sun",
   "sunlight",
   "sunlit",
   "supply",
   "sure",
   "survive",
   "suspiciously",
   "sustaining",
   "swap",
   "swapped",
   "switch",
   "switches",
   "system",
   "\t",     /* _TAB */
   "table",
   "take",
   "takes",
   "taking",
   "tall",
   "tap",
   "task",
   "telescope", 
   "tells",
   "tens",
   "teo",
   "test",
   "text",
   "than",
   "that",
   "that's",
   "the", 
   "them",
   "theme",
   "then",
   "there",
   "there's",
   "they",
   "third",
   "thirsty",
   "this",
   "three",
   "throne",
   "through",
   "ticket",
   "time",
   "times",
   "titled",
   "to",
   "too",
   "tools",
   "top",
   "torn",
   "torus",
   "tosses",
   "towards",
   "tower",
   "track",
   "trail",
   "trapped",
   "travelator",
   "travels",
   "tray",
   "tree",
   "triangle",
   "try",
   "tube",
   "tunnel",
   "turn",
   "turned",
   "turning",
   "turns",
   "twists",
   "two",
   "U",
   "under",
   "underground",
   "underneath",
   "understand",
   "unfiltered",
   "units",
   "unknown",
   "unlock",
   "unlocked",
   "unusual",
   "up",
   "upon",
   "upwards",
   "use",
   "user",
   "using",
   "utilising",
   "vegetation",
   "verb",
   "version",
   "very",
   "violet",
   "voice",
   "W",
   "wall",
   "walls",
   "want",
   "ward",
   "warm",
   "warning",
   "was",
   "water",
   "wave",
   "way",
   "we",
   "wear",
   "wearing",
   "welcome",
   "well",
   "wellington",
   "we're",
   "west",
   "what",
   "wheel",
   "when",
   "where",
   "which",
   "while",
   "whine",
   "white",
   "who",
   "wilbury",
   "window",
   "with",
   "wizard",
   "word",
   "words",
   "work",
   "world",
   "worn",
   "write",
   "writing",
   "written",
   "years",
   "yellow",
   "yellowing",
   "yet",
   "you",
   "you'll",
   "young",
   "your"
};


/*
 ***************** Private function declarations
 */
static void flush_line( TRISTATE );


/*
 ************************* Function definitions
 */

static void flush_line( TRISTATE v )
{
   if( _TRUE == v )
   {
      adv__print(( char const * const )pbuf );
   }
   ( void )memset( pbuf, 0, OUTBUF_LEN );
   pbuf_idx = 0;
}

TRISTATE adv_strstr( UCC *a, UCC *b, U32C len )
{
   /* a string should be more than a character long;
    * you can do it with one character, but then matches
    * could be found which may not be intended. */

   TRISTATE result = _FALSE;
   U32C blen = strlen(( char * )b );
   int i;


   for( i=0;( i <( len - blen ))&&( _FALSE == result ); i++ )
   {
      if( a[ i ]== *b ) 
      {
         result = adv_strcmpi( a + i, b );
      }
   }

   return( result );
}

TRISTATE adv_strcmpi( UC const *a, UC const *b )
{
   TRISTATE result = _FALSE;

   while(( *a )&&( *b )&&( tolower( *a )== tolower( *b )))
      a++, b++
      ;

   if(( ! *a )&&( ! *b ))
   {
      /* if both strings reach null terminator, then an exact match */
      result = _TRUE;
   }
   else
   if(( ! *a )||( ! *b ))
   {
      /* if either string reaches null terminator, then part match */
      result = _UNDECIDED;
   }

   return( result );
}

UC *adv_strcpy( UC *a, char const *b )
{
   while( 0 !=( *a = *b ))
      a++, b++
      ;

   return( a );
}

U32 adv_strlen( char *a )
{
   U32 len = 0;

   while( a[ len ])
      len++
      ;

   return( len );
}

#if !defined( VARARGS )

void adv_print( U32C n, U32C parr[ ])
{
   int i;

   for( i=0; i < n; i++ )
   {
      adv_print_buf(( char* )adv_words[ parr[ i ]]);
   }
}

#elif defined( VARARGS )

void adv_print( U32C n, ... )
{
   va_list al;
   int v;

   va_start( al, n );   /* initialise the argument list */
   {
      for( v = n;( 0 <= v ); )
      {
         if( __END > v )
         {
            adv_print_buf(( char* )adv_words[ v ]);
         }
         else
         {
            adv_print_buf( "\n" );
            DEBUGF( "bad word in sentence\n" );
         }
         v = va_arg( al, int );   /* get the next argument */
      }
   }
   va_end( al );        /* clean up argument list */
}

#endif

void adv_print_buf( char *word )
{
   static int upper = 1;
   int len;
   TRISTATE copy = _TRUE;
   U32 escape = 0;

   assert( screen_width <= OUTBUF_LEN );

   len = adv_strlen( word );

   /* if line full, print it and start next line */
   if(( '\n' != word[ 0 ])&&(( screen_width - 1 )<=( pbuf_idx + len + 1 )))
   {
         /* flush_line has to insert a newline character */
      flush_line( _TRUE );
   }

   switch( word[ 0 ])
   {
      case '.' :
      case ',' :
      case ':' :
      case ';' :
      case '!' :
      case '?' :
      case '-' :
      case '+' :
      case '*' :
      case '/' :
      case '=' :
      case '\'' :  /* _QUOTE_RIGHT */
      case ')' :   /* _BRACKET_RIGHT */
      case '>' :   /* _MORE_ */
      {
         /* remove space preceding this character */
         pbuf_idx--;
      }
      break;

      case '\b' :
      {
         len--;
         copy = _FALSE;
         pbuf_idx--;
      }
      break;

      case '\\' :  /* ESCAPE sequence */
      {
         escape = 1;
         copy = _FALSE;
         len = 0;

         switch( word[ 1 ])
         {
            case 'h' :
            {
               flush_line( _FALSE );   /* erase current line */
            }
            break;

            case 'l' :
            {
               upper = 0;   /* force next word with lower case */
            }
            break;

            case 'u' :
            {
               upper = 1;   /* force next word with upper case */
            }
            break;

            default:
            break;
         }
      }
      break;

      default :
      break;
   }

   if( _TRUE == copy )
   {
      /* copy this word to the end of the print buffer */
      ( void )adv_strcpy( &( pbuf[ pbuf_idx ]), word );
   }

   /* force first letter of sentence to upper case */
   if(( 1 == upper )&&( 0 == escape ))
   {
      if( islower(( int )pbuf[ pbuf_idx ]))
      {
         pbuf[ pbuf_idx ]=(( word[ 0 ]- 'a' )+ 'A' );
      }
      upper = 0;
   }

   pbuf_idx += len;

   if( '\n' == pbuf[ pbuf_idx - 1 ])
   {
         /* strip out last \n as a newline is printed in flush_line */
      pbuf[ pbuf_idx - 1 ]=' ';
      flush_line( _TRUE );   /* display current line */
      upper = 1;
   }

   switch( word[ 0 ])
   {
      case '.' :
      case '?' :
      case '!' :
      {
         upper = 1;
      }
      break;

      default :
      break;
   }

   if( 0 < pbuf_idx )
   {
      switch( word[ 0 ])
      {
         case '\b' :  /* _BACKSPACE */
         case '<' :   /* _LESS_ */
         case '`' :   /* _QUOTE_LEFT */
         case '(' :   /* _BRACKET_LEFT */
         case '-' :   /* _MINUS */
         case '\\' :  /* Escape sequence */
         case ' ' :   /* _SP */
         {
            /* don't add a space after this character */
         }
         break;

         default :
         {
            /* add a space after word */
            pbuf[ pbuf_idx++ ]= ' ';
         }
         break;
      }
   }
}

