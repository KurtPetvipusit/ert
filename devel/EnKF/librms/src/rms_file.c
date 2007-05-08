#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <hash.h>
#include <list.h>
#include <list_node.h>
#include <util.h>

#include <rms_type.h>
#include <rms_util.h>
#include <rms_tag.h>
#include <rms_file.h>
#include <rms_tagkey.h>

/*****************************************************************/
static const char * rms_ascii_header      = "roff-asc";
static const char * rms_binary_header     = "roff-bin";

static const char * rms_comment1          = "ROFF file";
static const char * rms_comment2          = "Creator: RMS - Reservoir Modelling System, version 7.5.2";
/*
  static const char * rms_parameter_tagname = "parameter";
*/





struct rms_file_struct {
  const char * filename;
  FILE       * stream;
  bool         endian_convert;
  bool         binary;
  hash_type  * type_map;
  list_type  * tag_list;
};



/*****************************************************************/
/* Pure roff routines */




static bool rms_binary(const rms_file_type *rms_file) {
  bool binary;
  char filetype[9];
  rms_util_fread_string( filetype , 9 , rms_file->stream);

  if (strncmp(filetype , rms_binary_header , 8) == 0)
    binary = true;
  else if (strncmp(filetype , rms_ascii_header , 8) == 0)
    binary = false;
  else {
    fprintf(stderr,"%s: header : %8s not recognized in file: %s - aborting \n",__func__ , filetype , rms_file->filename);
    abort();
  }
  return binary;
}


    
static void rms_file_add_tag(rms_file_type *rms_file , const rms_tag_type *tag) {
  list_append_ref(rms_file->tag_list , tag);
}

rms_tag_type * rms_file_get_tag(const rms_file_type *rms_file , const char *tagname, const char *keyname, const char *keyvalue) {
  bool cont = true;
  rms_tag_type *return_tag = NULL;
  list_node_type *tag_node = list_get_head(rms_file->tag_list);
  while (cont) {
    rms_tag_type *tag = list_node_value_ptr(tag_node);
    if (rms_tag_name_eq(tag , tagname , keyname , keyvalue)) {
      return_tag = tag;
      cont = false;
    } else 
      tag_node = list_node_get_next(tag_node);
    if (tag_node == NULL)
      cont = false;
  }
  if (return_tag == NULL) {
    if (keyname != NULL && keyvalue != NULL) 
      fprintf(stderr,"%s: failed to find tag:%s with key:%s=%s in file:%s - aborting \n",__func__ , tagname , keyname , keyvalue , rms_file->filename);
    else
      fprintf(stderr,"%s: failed to find tag:%s in file:%s - aborting \n",__func__ , tagname , rms_file->filename);
  }
  return return_tag;
}



static rms_file_type * rms_alloc_file(const char *filename) {
  rms_file_type *rms_file = malloc(sizeof *rms_file);
  rms_file->filename     = util_alloc_string_copy(filename);
  rms_file->endian_convert  = false;
  rms_file->type_map  	 = hash_alloc(10);
  rms_file->tag_list  	 = list_alloc();

  hash_insert_hash_owned_ref(rms_file->type_map , "byte"   , rms_type_alloc(rms_byte_type ,    1) ,  rms_type_free);
  hash_insert_hash_owned_ref(rms_file->type_map , "bool"   , rms_type_alloc(rms_bool_type,     1) ,  rms_type_free);
  hash_insert_hash_owned_ref(rms_file->type_map , "int"    , rms_type_alloc(rms_int_type ,     4) ,  rms_type_free);
  hash_insert_hash_owned_ref(rms_file->type_map , "float"  , rms_type_alloc(rms_float_type  ,  4) ,  rms_type_free);
  hash_insert_hash_owned_ref(rms_file->type_map , "double" , rms_type_alloc(rms_double_type ,  8) ,  rms_type_free);
  hash_insert_hash_owned_ref(rms_file->type_map , "char"   , rms_type_alloc(rms_char_type   , -1) ,  rms_type_free);
  
  return rms_file;
}



static void rms_init_existing_file(rms_file_type * rms_file) {
  if (util_file_exists(rms_file->filename)) {
    if ( (rms_file->stream = fopen(rms_file->filename , "r")) == NULL) {
      fprintf(stderr,"%s: failed to open: %s - aborting \n",__func__ , rms_file->filename);
      abort();
    }
  } else {
    fprintf(stderr,"%s file:%s does not exist - aborting \n",__func__ , rms_file->filename);
    abort();
  }
    
  
  rms_file->binary = rms_binary(rms_file);
  if (!rms_file->binary) {
    fprintf(stderr,"%s only binary files implemented - aborting \n",__func__);
    abort();
  }
  /* Skipping two comment lines ... */
  rms_util_fskip_string(rms_file->stream);
  rms_util_fskip_string(rms_file->stream);
  {
    bool eof_tag;
    rms_tag_type    * filedata_tag = rms_tag_fread_alloc(rms_file->stream , rms_file->type_map , rms_file->endian_convert , &eof_tag);
    rms_tagkey_type * byteswap_key = rms_tag_get_key(filedata_tag , "byteswaptest");
    int byteswap_value             = *( int *) rms_tagkey_get_data_ref(byteswap_key);
    if (byteswap_value == 1)
      rms_file->endian_convert = false;
    else
      rms_file->endian_convert = true;
    rms_tag_free(filedata_tag);
  }
}


static void rms_init_new_file(rms_file_type * rms_file , bool binary) {
  rms_file->binary = binary;
  if (!rms_file->binary) {
    fprintf(stderr,"%s only binary files implemented - aborting \n",__func__);
    abort();
  }
}



rms_file_type * rms_open(const char *filename , bool new_file , bool create_binary) {
  rms_file_type * rms_file = rms_alloc_file(filename);
  if (new_file)
    rms_init_new_file(rms_file , create_binary);
  else 
    rms_init_existing_file(rms_file);
  return rms_file;
}


void rms_close(rms_file_type * rms_file) {
  hash_free(rms_file->type_map);
  {
    list_node_type    *tag_node , *next_tag_node;
    list_type         *tag_list = rms_file->tag_list;
    
    tag_node = list_get_head(tag_list);
    while (tag_node) {
      next_tag_node = list_node_get_next(tag_node);
      rms_tag_free( list_node_value_ptr(tag_node) );
      tag_node = next_tag_node;
    }
  }

  list_free(rms_file->tag_list);
  free( (char *) rms_file->filename);
  fclose(rms_file->stream); 
  free(rms_file);
}


static int rms_file_get_dim(const rms_tag_type *tag , const char *dim_name) {
  rms_tagkey_type *key = rms_tag_get_key(tag , dim_name);
  return * (int *) rms_tagkey_get_data_ref(key);
}



void rms_file_assert_dimensions(const rms_file_type *rms_file , int nx , int ny , int nz) {
  bool OK = true;  
  rms_tag_type    *tag    = rms_file_get_tag(rms_file , "dimensions" , NULL , NULL);
  OK =       (nx == rms_file_get_dim(tag , "nX"));
  OK = OK && (ny == rms_file_get_dim(tag , "nY"));
  OK = OK && (nz == rms_file_get_dim(tag , "nZ"));

  if (!OK) {
    fprintf(stderr,"%s: dimensions on file: %s (%d, %d, %d) did not match with input dimensions (%d,%d,%d) - aborting \n",__func__ , rms_file->filename,
	    rms_file_get_dim(tag , "nX"), rms_file_get_dim(tag , "nY"), rms_file_get_dim(tag , "nZ"),
	    nx , ny , nz);
    abort();
  }
}


void rms_file_load(rms_file_type *rms_file) {
  bool eof_tag = false;
  
  while (!eof_tag) {
    rms_tag_type * tag = rms_tag_fread_alloc(rms_file->stream ,  rms_file->type_map , rms_file->endian_convert , &eof_tag );
    if (!eof_tag)
      rms_file_add_tag(rms_file , tag);
    else
      rms_tag_free(tag);
  }
}


void rms_file_fwrite(const rms_file_type * rms_file, const char * filetype) {
  FILE * stream = fopen(rms_file->filename , "w");
  if (stream == NULL) {
    fprintf(stderr,"%s: failed to open:%s for writing - aborting \n",__func__ , rms_file->filename);
    abort();
  }
  if (rms_file->binary)
    rms_util_fwrite_string(rms_binary_header , stream);
  else
    rms_util_fwrite_string(rms_ascii_header , stream);
  rms_util_fwrite_string(rms_comment1 , stream);
  rms_util_fwrite_string(rms_comment2 , stream);
  {
    rms_tag_type * filedata = rms_tag_alloc_filedata(filetype);
    rms_tag_fwrite(filedata , stream);
    rms_tag_free(filedata);
  }
  
  {
    list_node_type * tag_node = list_get_head(rms_file->tag_list);
    while (tag_node != NULL) {
      const rms_tag_type *tag = list_node_value_ptr(tag_node);
      rms_tag_fwrite(tag , stream);
      tag_node = list_node_get_next(tag_node);
    }
  }

  fclose(stream);
}


/*****************************************************************/
/* Old hack version: */
  

void old_rms_roff_load(const char *filename , const char *param_name , float *param) {
  const int offset = 327 + strlen(param_name);
  int n_read;
  int size;
  FILE *stream     = fopen(filename , "r");
  
  fseek(stream , offset , SEEK_SET);
  fread(&size  , 1 , sizeof size , stream);
  n_read = fread(param , sizeof *param , size , stream);
  
  fclose(stream);
  if (n_read != size) {
    fprintf(stderr,"%s: wanted:%d elements - only read:%d - aborting \n",__func__, size , n_read);
    abort();
  }
}
