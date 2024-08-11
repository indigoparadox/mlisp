
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

#define MLISP_FLAG_DO_STEP 0x02

struct MLISP_DATA {
   int init;
   uint8_t flags;
   struct MLISP_PARSER parser;
   struct MLISP_EXEC_STATE exec;
   MAUG_MHANDLE font_h;
   uint8_t do_exec;
   size_t last_y;
   char open_filename[RETROFLAT_PATH_MAX + 1];
};

MERROR_RETVAL mlisp_cb_write(
   struct MLISP_PARSER* parser, struct MLISP_EXEC_STATE* exec, size_t n_idx,
   size_t args_c, void* cb_data, uint8_t flags
) {
   MERROR_RETVAL retval = MERROR_OK;
   struct MLISP_STACK_NODE val;
   char write_buf[256];
   struct MLISP_DATA* mlisp_data = (struct MLISP_DATA*)cb_data;

   maug_mzero( write_buf, 256 );

   #  define _MLISP_TYPE_TABLE_WRITE( idx, ctype, name, const_name, fmt ) \
      } else if( MLISP_TYPE_ ## const_name == val.type ) { \
         maug_snprintf( write_buf, 255, fmt, val.value.name );

   retval = mlisp_stack_pop( exec, &(val) );
   maug_cleanup_if_not_ok();

   if( 0 ) {
   MLISP_TYPE_TABLE( _MLISP_TYPE_TABLE_WRITE )
   }

   retrofont_string(
      NULL, RETROFLAT_COLOR_WHITE, write_buf, 0, mlisp_data->font_h,
      10, mlisp_data->last_y,
      0, 0, 0 );

   mlisp_data->last_y += 10;

cleanup:

   return retval;
}

void mlisp_loop( struct MLISP_DATA* data ) {
   RETROFLAT_IN_KEY input = 0;
   struct RETROFLAT_INPUT input_evt;
   MERROR_RETVAL retval = MERROR_EXEC;
   uint8_t do_next =
      (MLISP_FLAG_DO_STEP == (MLISP_FLAG_DO_STEP & data->flags) ? 0 : 1 );

   /* Input */

   input = retroflat_poll_input( &input_evt );

   switch( input ) {
   case RETROFLAT_KEY_SPACE:
      do_next = 1;
      break;

   case RETROFLAT_KEY_ESC:
      retroflat_quit( 0 );
      break;
   }

   /* Drawing */

   retroflat_draw_lock( NULL );

   if( do_next ) {
      if(
         data->do_exec &&
         (retval = mlisp_step( &(data->parser), &(data->exec) ))
      ) {
         mlisp_stack_dump( &(data->parser), &(data->exec) );
         mlisp_env_dump( &(data->parser), &(data->exec) );
         data->do_exec = 0;

      } else if( MERROR_OK == retval ) {
         mlisp_ast_dump( &(data->parser), 0, 0, 0 );
         mlisp_stack_dump( &(data->parser), &(data->exec) );
         mlisp_env_dump( &(data->parser), &(data->exec) );
      }
   }

   /*
   retroflat_rect(
      NULL, RETROFLAT_COLOR_BLACK, 0, 0,
      retroflat_screen_w(), retroflat_screen_h(),
      RETROFLAT_FLAGS_FILL );
   */

   retroflat_draw_release( NULL );
}

static int mlisp_cli_step(
   const char* arg, ssize_t arg_c, struct MLISP_DATA* data
) {
   if( 1 == arg_c ) {
      data->flags |= MLISP_FLAG_DO_STEP;
   }
   return RETROFLAT_OK;
}

static int mlisp_cli_file(
   const char* arg, ssize_t arg_c, struct MLISP_DATA* data
) {
   debug_printf( 1, "POS ARG: " SSIZE_T_FMT, arg_c );
   if( 2 == arg_c ) {
      strncpy( data->open_filename, arg, RETROFLAT_PATH_MAX );
   }
   return RETROFLAT_OK;
}

int main( int argc, char** argv ) {
   int retval = 0;
   struct RETROFLAT_ARGS args;
   MAUG_MHANDLE data_h = (MAUG_MHANDLE)NULL;
   struct MLISP_DATA* data = NULL;
   mfile_t lisp_file;
   char c;

   /* === Setup === */

   logging_init();

   data_h = maug_malloc( 1, sizeof( struct MLISP_DATA ) );
   maug_cleanup_if_null_alloc( MAUG_MHANDLE, data_h );
   maug_mlock( data_h, data );
   maug_cleanup_if_null_alloc( struct MLISP_DATA*, data );
   maug_mzero( data, sizeof( struct MLISP_DATA ) );
   
   data->last_y = 10;

   memset( &args, '\0', sizeof( struct RETROFLAT_ARGS ) );

   args.title = "mlisp";
   args.assets_path = "assets";

	retval = maug_add_arg( MAUG_CLI_SIGIL "s", MAUG_CLI_SIGIL_SZ + 1,
      "Step through script with SPACE.", 0,
      (maug_cli_cb)mlisp_cli_step, data );
   maug_cleanup_if_not_ok();

	retval = maug_add_arg( MAUG_CLI_SIGIL "f", MAUG_CLI_SIGIL_SZ + 1,
      "Run the given file.", 0,
      (maug_cli_cb)mlisp_cli_file, data );
   maug_cleanup_if_not_ok();
   
   retval = retroflat_init( argc, argv, &args );
   if( RETROFLAT_OK != retval ) {
      goto cleanup;
   }

   retval = retrofont_load(
      "unscii-8.hex", &(data->font_h), 0, 33, 93 );
   maug_cleanup_if_not_ok();

   retval = mlisp_parser_init( &(data->parser) );
   maug_cleanup_if_not_ok();

   retval = mfile_open_read( data->open_filename, &lisp_file );
   maug_cleanup_if_not_ok();

   while( mfile_has_bytes( &lisp_file ) ) {
      retval = mfile_file_read_int( &lisp_file, (uint8_t*)&c, 1, 0 );
      maug_cleanup_if_not_ok();
      retval = mlisp_parse_c( &(data->parser), c );
      maug_cleanup_if_not_ok();
   }
   mlisp_ast_dump( &(data->parser), 0, 0, 0 );
   assert( 0 == data->parser.base.pstate_sz );

   retval = mlisp_exec_init( &(data->parser), &(data->exec) );

   retval = mlisp_env_set(
      &(data->parser), &(data->exec), "write", 5, MLISP_TYPE_CB, mlisp_cb_write,
      data, MLISP_ENV_FLAG_BUILTIN );
   maug_cleanup_if_not_ok();

   debug_printf( 1,
      "mlisp parser uses: " SIZE_T_FMT ", exec uses: " SIZE_T_FMT " bytes RAM",
      sizeof( struct MLISP_PARSER ) +
         mdata_vector_sz( &(data->parser.ast) ) +
         mdata_strpool_sz( &(data->parser.strpool) ),
      sizeof( struct MLISP_EXEC_STATE ) +
         mdata_vector_sz( &(data->exec.per_node_child_idx) ) +
         mdata_vector_sz( &(data->exec.stack) ) +
         mdata_vector_sz( &(data->exec.lambda_trace) ) +
         mdata_vector_sz( &(data->exec.env) ) );

   data->do_exec = 1;

   /* === Main Loop === */

   retroflat_loop( (retroflat_loop_iter)mlisp_loop, NULL, data );

   debug_printf( 1, "mlisp uses: " SIZE_T_FMT " bytes RAM",
      sizeof( struct MLISP_PARSER ) +
      mdata_vector_sz( &(data->parser.ast) ) +
      mdata_strpool_sz( &(data->parser.strpool) ) +
      sizeof( struct MLISP_EXEC_STATE ) +
      mdata_vector_sz( &(data->exec.per_node_child_idx) ) +
      mdata_vector_sz( &(data->exec.stack) ) +
      mdata_vector_sz( &(data->exec.env) ) );

cleanup:

#ifndef RETROFLAT_OS_WASM

   mfile_close( &lisp_file );

   maug_mfree( data->font_h );

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

