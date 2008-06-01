#ifdef COMPRESSION_BZIP2

#include <pkg.h>

#include <bzlib.h>

#define CHUNK_SIZE 16384

static void close_bzip2_read( void * );
static void close_bzip2_write( void * );
static long read_bzip2( void *, void *, long );
static long write_bzip2( void *, void *, long );

typedef struct {
  union {
    FILE *fp;
    union {
      read_stream *rs;
      write_stream *ws;
    } streams;
  } u;
  bz_stream strm;
  void *buf;
  int error;
  int use_stream;
} bzip2_private;

static void close_bzip2_read( void *vp ) {
  bzip2_private *p;

  p = (bzip2_private *)vp;
  if ( p ) {
    BZ2_bzDecompressEnd( &(p->strm) );
    if ( p->buf ) free( p->buf );
    if ( !(p->use_stream) ) {
      if ( p->u.fp ) fclose( p->u.fp );
    }
    free( p );
  }
}

static void close_bzip2_write( void *vp ) {
  bzip2_private *p;
  size_t result;
  int def_status;

  p = (bzip2_private *)vp;
  if ( p ) {
    if ( p->error == 0 && p->buf &&
	 ( ( p->use_stream && p->u.streams.ws ) ||
	   ( !(p->use_stream) && p->u.fp ) ) ) {
      p->strm.next_in = NULL;
      p->strm.avail_in = 0;
      do {
	if ( p->strm.avail_out == 0 ) {
	  if ( p->use_stream )
	    result = write_to_stream( p->u.streams.ws, p->buf, CHUNK_SIZE );
	  else
	    result = fwrite( p->buf, 1, CHUNK_SIZE, p->u.fp );
	  if ( result == CHUNK_SIZE ) {
	    p->strm.next_out = p->buf;
	    p->strm.avail_out = CHUNK_SIZE;
	  }
	  else {
	    /* We couldn't write all the output we had */
	    p->error = 1;
	    break;
	  }
	}
	def_status = BZ2_bzCompress( &(p->strm), BZ_FINISH );
	if ( !( def_status == BZ_FINISH_OK ||
		def_status == BZ_STREAM_END ) ) {
	  p->error = 1;
	  break;
	}
      } while ( def_status != BZ_STREAM_END );
      /* Write out the last partial block */
      if ( def_status == BZ_STREAM_END ) {
	if ( p->use_stream )
	  result = write_to_stream( p->u.streams.ws, p->buf,
				    CHUNK_SIZE - p->strm.avail_out );
	else
	  result = fwrite( p->buf, 1,
			   CHUNK_SIZE - p->strm.avail_out, p->u.fp );
	if ( result != CHUNK_SIZE - p->strm.avail_out ) p->error = 1;
      }
    }
    BZ2_bzCompressEnd( &(p->strm) );
    if ( p->buf ) free( p->buf );
    if ( !(p->use_stream) ) {
      if ( p->u.fp ) fclose( p->u.fp );
    }
    free( p );
  }
}

read_stream * open_read_stream_from_stream_bzip2( read_stream *rs ) {
  read_stream *r;
  bzip2_private *p;
  int status;

  if ( rs ) {
    r = malloc( sizeof( *r ) );
    if ( r ) {
      p = malloc( sizeof( *p ) );
      if ( p ) {
	r->private = (void *)p;
	p->error = 0;
	p->buf = malloc( CHUNK_SIZE );
	if ( p->buf ) {
	  p->strm.bzalloc = NULL;
	  p->strm.bzfree = NULL;
	  p->strm.opaque = NULL;
	  p->strm.avail_in = 0;
	  p->strm.next_in = NULL;
	  /* 15 bit window with gzip format */
	  status = BZ2_bzDecompressInit( &(p->strm), 0, 0 );
	  if ( status == BZ_OK ) {
	    p->use_stream = 1;
	    p->u.streams.rs = rs;
	    r->close = close_bzip2_read;
	    r->read = read_bzip2;
	  }
	  else {
	    free( p->buf );
	    free( p );
	    free( r );
	    r = NULL;
	  }
	}
	else {
	  free( p );
	  free( r );
	  r = NULL;
	}
      }
      else {
	free( r );
	r = NULL;
      }
    }
  }
  else r = NULL;
  return r;
}

read_stream * open_read_stream_bzip2( char *filename ) {
  read_stream *r;
  bzip2_private *p;
  int status;

  if ( filename ) {
    r = malloc( sizeof( *r ) );
    if ( r ) {
      p = malloc( sizeof( *p ) );
      if ( p ) {
	r->private = (void *)p;
	p->error = 0;
	p->buf = malloc( CHUNK_SIZE );
	if ( p->buf ) {
	  p->strm.bzalloc = NULL;
	  p->strm.bzfree = NULL;
	  p->strm.opaque = NULL;
	  p->strm.avail_in = 0;
	  p->strm.next_in = NULL;
	  /* 15 bit window with gzip format */
	  status = BZ2_bzDecompressInit( &(p->strm), 0, 0 );
	  if ( status == BZ_OK ) {
	    p->use_stream = 0;
	    p->u.fp = fopen( filename, "r" );
	    if ( p->u.fp ) {
	      r->close = close_bzip2_read;
	      r->read = read_bzip2;
	    }
	    else {
	      BZ2_bzDecompressEnd( &(p->strm) );
	      free( p->buf );
	      free( p );
	      free( r );
	      r = NULL;
	    }
	  }
	  else {
	    free( p->buf );
	    free( p );
	    free( r );
	    r = NULL;
	  }
	}
	else {
	  free( p );
	  free( r );
	  r = NULL;
	}
      }
      else {
	free( r );
	r = NULL;
      }
    }
  }
  else r = NULL;
  return r;
}

write_stream * open_write_stream_from_stream_bzip2( write_stream *ws ) {
  write_stream *w;
  bzip2_private *p;
  int status;

  if ( ws ) {
    w = malloc( sizeof( *w ) );
    if ( w ) {
      p = malloc( sizeof( *p ) );
      if ( p ) {
	w->private = (void *)p;
	p->error = 0;
	p->buf = malloc( CHUNK_SIZE );
	if ( p->buf ) {
	  p->strm.bzalloc = NULL;
	  p->strm.bzfree = NULL;
	  p->strm.opaque = NULL;
	  status = BZ2_bzCompressInit( &(p->strm),
				       9,
				       0,
				       30 );
	  if ( status == BZ_OK ) {
	    p->strm.next_out = p->buf;
	    p->strm.avail_out = CHUNK_SIZE;
	    p->use_stream = 1;
	    p->u.streams.ws = ws;
	    w->close = close_bzip2_write;
	    w->write = write_bzip2;
	  }
	  else {
	    free( p->buf );
	    free( p );
	    free( w );
	    w = NULL;
	  }
	}
	else {
	  free( p );
	  free( w );
	  w = NULL;
	}
      }
      else {
	free( w );
	w = NULL;
      }
    }
  }
  else w = NULL;
  return w;
}
write_stream * open_write_stream_bzip2( char *filename ) {
  write_stream *w;
  bzip2_private *p;
  int status;

  if ( filename ) {
    w = malloc( sizeof( *w ) );
    if ( w ) {
      p = malloc( sizeof( *p ) );
      if ( p ) {
	w->private = (void *)p;
	p->error = 0;
	p->buf = malloc( CHUNK_SIZE );
	if ( p->buf ) {
	  p->strm.bzalloc = NULL;
	  p->strm.bzfree = NULL;
	  p->strm.opaque = NULL;
	  status = BZ2_bzCompressInit( &(p->strm),
				       9,
				       0,
				       30 );
	  if ( status == BZ_OK ) {
	    p->strm.next_out = p->buf;
	    p->strm.avail_out = CHUNK_SIZE;
	    p->use_stream = 0;
	    p->u.fp = fopen( filename, "w" );
	    if ( p->u.fp ) {
	      w->close = close_bzip2_write;
	      w->write = write_bzip2;
	    }
	    else {
	      BZ2_bzCompressEnd( &(p->strm) );
	      free( p->buf );
	      free( p );
	      free( w );
	      w = NULL;
	    }
	  }
	  else {
	    free( p->buf );
	    free( p );
	    free( w );
	    w = NULL;
	  }
	}
	else {
	  free( p );
	  free( w );
	  w = NULL;
	}
      }
      else {
	free( w );
	w = NULL;
      }
    }
  }
  else w = NULL;
  return w;
}

static long read_bzip2( void *vp, void *buf, long len ) {
  bzip2_private *p;
  size_t result;
  long status;
  int inf_status;

  p = (bzip2_private *)vp;
  if ( p && buf && len > 0 ) {
    if ( p->error == 0 ) {
      status = 0;
      p->strm.next_out = buf;
      p->strm.avail_out = len;
      while ( p->strm.avail_out > 0 ) {
	if ( p->strm.avail_in == 0 ) {
	  p->strm.next_in = p->buf;
	  if ( p->use_stream ) {
	    result = read_from_stream( p->u.streams.rs, p->buf, CHUNK_SIZE );
	    if ( result > 0 ) p->strm.avail_in = result;
	    else if ( result == 0 ) break; /* EOF */
	    else {
	      status = STREAMS_INTERNAL_ERROR;
	    }
	  }
	  else {
	    result = fread( p->buf, 1, CHUNK_SIZE, p->u.fp );
	    if ( result > 0 ) p->strm.avail_in = result;
	    else {
	      if ( ferror( p->u.fp ) ) status = STREAMS_INTERNAL_ERROR;
	      break;
	    }
	  }
	}
	inf_status = BZ2_bzDecompress( &(p->strm) );
	if ( inf_status == BZ_STREAM_END ) break;
	else if ( inf_status != BZ_OK ) {
	  p->error = 1;
	  status = STREAMS_INTERNAL_ERROR;
	  break;
	}
      }
      if ( status >= 0 ) status = len - p->strm.avail_out;
      return status;
    }
    else return STREAMS_INTERNAL_ERROR;
  }
  else return STREAMS_BAD_ARGS;
}

static long write_bzip2( void *vp, void *buf, long len ) {
  bzip2_private *p;
  size_t result;
  long status;
  int def_status;

  p = (bzip2_private *)vp;
  if ( p && buf && len > 0 ) {
    if ( p->error == 0 ) {
      status = 0;
      p->strm.next_in = buf;
      p->strm.avail_in = len;
      while ( p->strm.avail_in > 0 ) {
	if ( p->strm.avail_out == 0 ) {
	  if ( p->use_stream )
	    result = write_to_stream( p->u.streams.ws, p->buf, CHUNK_SIZE );
	  else
	    result = fwrite( p->buf, 1, CHUNK_SIZE, p->u.fp );
	  if ( result == CHUNK_SIZE ) {
	    p->strm.next_out = p->buf;
	    p->strm.avail_out = CHUNK_SIZE;
	  }
	  else {
	    /* We couldn't write all the output we had */
	    status = STREAMS_INTERNAL_ERROR;
	    p->error = 1;
	    break;
	  }
	}
	/* Always Z_NO_FLUSH here, we do Z_FINISH in close */
	def_status = BZ2_bzCompress( &(p->strm), BZ_RUN );
	if ( def_status != BZ_RUN_OK ) {
	  p->error = 1;
	  status = STREAMS_INTERNAL_ERROR;
	  break;
	}
      }
      if ( status >= 0 ) status = len - p->strm.avail_in;
      return status;
    }
    else return STREAMS_INTERNAL_ERROR;
  }
  else return STREAMS_BAD_ARGS;
}

#endif /* COMPRESSION_BZIP2 */
