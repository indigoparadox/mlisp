
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

#define MLISP_CB_SHAPE_FLAG_RECT 0x10

#define MLISP_CB_SHAPE_FLAG_ELLIPSE 0x20

#define MLISP_CB_SHAPE_FLAG_FILL 0x80

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

   if( !retroflat_is_waiting_for_frame() ) {
      /* This call draws, so only call it on a drawing frame! */
      debug_printf( MLISP_EXEC_TRACE_LVL, "waiting for frame..." );
      retroflat_wait_for_frame();
      retval = MERROR_PREEMPT;
      goto cleanup;
   }

   /* We were already waiting for frame when this was called, so this must
    * be a frame! */

   debug_printf( MLISP_EXEC_TRACE_LVL, "entered frame!" );

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

MERROR_RETVAL mlisp_cb_shape(
   struct MLISP_PARSER* parser, struct MLISP_EXEC_STATE* exec, size_t n_idx,
   size_t args_c, void* cb_data, uint8_t flags
) {
   MERROR_RETVAL retval = MERROR_OK;
   struct MLISP_STACK_NODE val;
   int16_t coords[4]; /* x, y, w, h */
   int16_t color = 0;
   ssize_t i = 0;

   if( !retroflat_is_waiting_for_frame() ) {
      /* This call draws, so only call it on a drawing frame! */
      debug_printf( MLISP_EXEC_TRACE_LVL, "waiting for frame..." );
      retroflat_wait_for_frame();
      retval = MERROR_PREEMPT;
      goto cleanup;
   }

   /* We were already waiting for frame when this was called, so this must
    * be a frame! */

   debug_printf( MLISP_EXEC_TRACE_LVL, "entered frame!" );

   /* Grab coordinates. */
   for( i = 3 ; 0 <= i ; i-- ) {
      retval = mlisp_stack_pop( exec, &(val) );
      maug_cleanup_if_not_ok();
      debug_printf( 1, "popped: %d", val.value.integer );

      if( MLISP_TYPE_INT != val.type ) {
         error_printf( "invalid coordinate type: %d", val.type );
         retval = MERROR_EXEC;
         goto cleanup;
      }

      coords[i] = val.value.integer;
   }

   /* Grab color. */
   retval = mlisp_stack_pop( exec, &(val) );
   maug_cleanup_if_not_ok();
   if( MLISP_TYPE_INT != val.type ) {
      error_printf( "invalid color type: %d", val.type );
      retval = MERROR_EXEC;
      goto cleanup;
   }
   color = val.value.integer;

   debug_printf( 1, "draw: %d, %d %dx%d",
      coords[0], coords[1], coords[2], coords[3] );

   if( MLISP_CB_SHAPE_FLAG_RECT == (MLISP_CB_SHAPE_FLAG_RECT & flags) ) {
      retroflat_rect(
         NULL, color,
         coords[0], coords[1], coords[2], coords[3],
         MLISP_CB_SHAPE_FLAG_FILL == (MLISP_CB_SHAPE_FLAG_FILL & flags) ?
            RETROFLAT_FLAGS_FILL : 0 );
   } else if(
      MLISP_CB_SHAPE_FLAG_ELLIPSE == (MLISP_CB_SHAPE_FLAG_ELLIPSE & flags)
   ) {
      retroflat_ellipse(
         NULL, color,
         coords[0], coords[1], coords[2], coords[3],
         MLISP_CB_SHAPE_FLAG_FILL == (MLISP_CB_SHAPE_FLAG_FILL & flags) ?
            RETROFLAT_FLAGS_FILL : 0 );
   }

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
   int16_t color_i = 0;

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
      &(data->parser), &(data->exec), "write", 5,
      MLISP_TYPE_CB, mlisp_cb_write,
      data, MLISP_ENV_FLAG_BUILTIN );
   maug_cleanup_if_not_ok();

   color_i = RETROFLAT_COLOR_BLACK;
   retval = mlisp_env_set(
      &(data->parser), &(data->exec), "black", 5,
      MLISP_TYPE_INT, &color_i, data, MLISP_ENV_FLAG_BUILTIN );
   color_i = RETROFLAT_COLOR_BLUE;
   retval = mlisp_env_set(
      &(data->parser), &(data->exec), "blue", 4,
      MLISP_TYPE_INT, &color_i, data, MLISP_ENV_FLAG_BUILTIN );

   retval = mlisp_env_set(
      &(data->parser), &(data->exec), "rect", 4,
      MLISP_TYPE_CB, mlisp_cb_shape,
      data, MLISP_ENV_FLAG_BUILTIN | MLISP_CB_SHAPE_FLAG_RECT );
   retval = mlisp_env_set(
      &(data->parser), &(data->exec), "rectf", 5,
      MLISP_TYPE_CB, mlisp_cb_shape,
      data, MLISP_ENV_FLAG_BUILTIN | MLISP_CB_SHAPE_FLAG_RECT | 
         MLISP_CB_SHAPE_FLAG_FILL );
   retval = mlisp_env_set(
      &(data->parser), &(data->exec), "ellipse", 4,
      MLISP_TYPE_CB, mlisp_cb_shape,
      data, MLISP_ENV_FLAG_BUILTIN | MLISP_CB_SHAPE_FLAG_ELLIPSE );
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

   retroflat_loop(
      (retroflat_loop_iter)mlisp_loop, (retroflat_loop_iter)mlisp_loop, data );

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

