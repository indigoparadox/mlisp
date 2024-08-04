
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
#include <mlispp.h>
#include <mlispe.h>

struct MLISP_DATA {
   int init;
   struct MLISP_PARSER parser;
   struct MLISP_EXEC_STATE exec;
   uint8_t do_exec;
};

void mlisp_loop( struct MLISP_DATA* data ) {
   RETROFLAT_IN_KEY input = 0;
   struct RETROFLAT_INPUT input_evt;
   MERROR_RETVAL retval = MERROR_EXEC;

   /* Input */

   input = retroflat_poll_input( &input_evt );

   switch( input ) {
   case RETROFLAT_KEY_ESC:
      retroflat_quit( 0 );
      break;
   }

   /* Drawing */

   retroflat_draw_lock( NULL );

   if(
      data->do_exec &&
      (retval = mlisp_step( &(data->parser), &(data->exec) ))
   ) {
      debug_printf( 1, "execution terminated with retval: %d", retval );
      mlisp_stack_dump( &(data->parser), &(data->exec) );
      mlisp_env_dump( &(data->parser), &(data->exec) );
      data->do_exec = 0;
   } else if( MERROR_OK == retval ) {
      mlisp_ast_dump( &(data->parser), 0, 0, 0 );
      mlisp_stack_dump( &(data->parser), &(data->exec) );
      mlisp_env_dump( &(data->parser), &(data->exec) );
   }

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
   size_t i = 0;
   FILE* lisp_f = NULL;
   size_t lisp_sz = 0;
   char* lisp_buf = NULL;
   size_t lisp_read = 0;

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

   retval = mlisp_parser_init( &(data->parser) );
   maug_cleanup_if_not_ok();

   lisp_f = fopen( "test.lisp", "r" );
   fseek( lisp_f, 0, SEEK_END );
   lisp_sz = ftell( lisp_f );
   fseek( lisp_f, 0, SEEK_SET );
   lisp_buf = calloc( lisp_sz + 1, 1 );
   maug_cleanup_if_null_alloc( char*, lisp_buf );
   lisp_read = fread( lisp_buf, 1, lisp_sz, lisp_f );
   assert( lisp_read == lisp_sz );

   debug_printf( 1, "%s", lisp_buf );
   for( i = 0 ; lisp_sz > i ; i++ ) {
      retval = mlisp_parse_c( &(data->parser), lisp_buf[i] );
      maug_cleanup_if_not_ok();
   }
   mlisp_ast_dump( &(data->parser), 0, 0, 0 );
   assert( 0 == data->parser.base.pstate_sz );

   retval = mlisp_exec_init( &(data->parser), &(data->exec) );

   data->do_exec = 1;

   /* === Main Loop === */

   retroflat_loop( (retroflat_loop_iter)mlisp_loop, NULL, data );

cleanup:

#ifndef RETROFLAT_OS_WASM

   if( NULL != lisp_f ) {
      fclose( lisp_f );
   }

   if( NULL != lisp_buf ) {
      free( lisp_buf );
   }

   mlisp_exec_free( &(data->exec) );

   mlisp_parser_free( &(data->parser) );

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

