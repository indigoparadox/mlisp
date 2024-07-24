
#define MAUG_C
#include <maug.h>

#define RETROFLT_C
#include <retroflt.h>

#include <mhtml.h>
#include <mcss.h>
#include <mcssmerg.h>
#include <retrofnt.h>
#include <retrogui.h>
#include <retrohtr.h>
#include <mlisp.h>

struct MLISP_DATA {
   int init;
};

void mlisp_loop( struct MLISP_DATA* data ) {
   RETROFLAT_IN_KEY input = 0;
   struct RETROFLAT_INPUT input_evt;

   /* Input */

   input = retroflat_poll_input( &input_evt );

   switch( input ) {
   case RETROFLAT_KEY_ESC:
      retroflat_quit( 0 );
      break;
   }

   /* Drawing */

   retroflat_draw_lock( NULL );

   retroflat_rect(
      NULL, RETROFLAT_COLOR_BLACK, 0, 0,
      retroflat_screen_w(), retroflat_screen_h(),
      RETROFLAT_FLAGS_FILL );

   retroflat_draw_release( NULL );
}

int main( int argc, char** argv ) {
   int retval = 0;
   struct RETROFLAT_ARGS args;
   MAUG_MHANDLE data_h = (MAUG_MHANDLE)NULL;
   struct MLISP_DATA* data = NULL;
   struct MLISP_PARSER parser;
   size_t i = 0;
   char test_lisp[] = "(begin  (define pi 3.14)  (define r 10)\n (* pi (* r r)))";
   struct MLISP_EXEC_STATE exec;

   /* === Setup === */

   logging_init();

   memset( &args, '\0', sizeof( struct RETROFLAT_ARGS ) );

   args.title = "mlisp";
   args.assets_path = "assets";
   
   retval = retroflat_init( argc, argv, &args );
   if( RETROFLAT_OK != retval ) {
      goto cleanup;
   }

   data_h = maug_malloc( 1, sizeof( struct MLISP_DATA ) );
   maug_cleanup_if_null_alloc( MAUG_MHANDLE, data_h );
   maug_mlock( data_h, data );
   maug_cleanup_if_null_alloc( struct MLISP_DATA*, data );
   maug_mzero( data, sizeof( struct MLISP_DATA ) );

   retval = mlisp_parser_init( &parser );
   maug_cleanup_if_not_ok();

   debug_printf( 1, "%s", test_lisp );
   for( i = 0 ; sizeof( test_lisp ) > i ; i++ ) {
      retval = mlisp_parse_c( &parser, test_lisp[i] );
      maug_cleanup_if_not_ok();
   }
   assert( 0 == parser.base.pstate_sz );
   mlisp_ast_dump( &parser, 0, 0, 0 );

   parser.ast_node_iter = 0;
   maug_mzero( &exec, sizeof( struct MLISP_EXEC_STATE ) );
   while( !mlisp_step( &parser, &exec ) ) {
      
   }
   mlisp_ast_dump( &parser, 0, 0, 0 );
   mlisp_stack_dump( &parser, &exec );

   /* === Main Loop === */

   retroflat_loop( (retroflat_loop_iter)mlisp_loop, NULL, data );

cleanup:

#ifndef RETROFLAT_OS_WASM

   mlisp_exec_free( &exec );

   mlisp_parser_free( &parser );

   if( NULL != data ) {
      maug_munlock( data_h, data );
   }

   if( NULL != data_h ) {
      maug_mfree( data_h );
   }

   retroflat_shutdown( retval );

   logging_shutdown();

#endif /* !RETROFLAT_OS_WASM */

   return retval;
}
END_OF_MAIN()

