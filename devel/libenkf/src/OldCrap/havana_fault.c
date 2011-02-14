/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'havana_fault.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <enkf_types.h>
#include <fortio.h>
#include <util.h>
#include <havana_fault_config.h>
#include <havana_fault.h>
#include <enkf_util.h>
#include <math.h>
#include <scalar.h>
#include <assert.h>
#include <subst.h>



GET_DATA_SIZE_HEADER(havana_fault);


struct havana_fault_struct 
{
  int                             __type_id; 
  const havana_fault_config_type *config;
  scalar_type                    *scalar;
};

/*****************************************************************/

void havana_fault_free_data(havana_fault_type *havana_fault) {
  assert(havana_fault->scalar);
  scalar_free_data(havana_fault->scalar);
}



void havana_fault_free(havana_fault_type *havana_fault) {
  scalar_free(havana_fault->scalar);
  free(havana_fault);
}



void havana_fault_realloc_data(havana_fault_type *havana_fault) {
  scalar_realloc_data(havana_fault->scalar);
}


void havana_fault_output_transform(const havana_fault_type * havana_fault) {
  scalar_transform(havana_fault->scalar);
}

void havana_fault_set_data(havana_fault_type * havana_fault , const double * data) {
  scalar_set_data(havana_fault->scalar , data);
}


void havana_fault_get_data(const havana_fault_type * havana_fault , double * data) {
  scalar_get_data(havana_fault->scalar , data);
}

void havana_fault_get_output_data(const havana_fault_type * havana_fault , double * output_data) {
  scalar_get_output_data(havana_fault->scalar , output_data);
}


const double * havana_fault_get_data_ref(const havana_fault_type * havana_fault) {
  return scalar_get_data_ref(havana_fault->scalar);
}


const double * havana_fault_get_output_ref(const havana_fault_type * havana_fault) {
  return scalar_get_output_ref(havana_fault->scalar);
}



havana_fault_type * havana_fault_alloc(const havana_fault_config_type * config) {
  havana_fault_type * havana_fault  = util_malloc(sizeof * havana_fault , __func__);
  havana_fault->config = config;
  gen_kw_config_type * gen_kw_config = havana_fault_config_get_gen_kw_config( config );
  havana_fault->scalar               = scalar_alloc(gen_kw_config_get_scalar_config( gen_kw_config )); 
  havana_fault->__type_id = HAVANA_FAULT;
  return havana_fault;
}



void havana_fault_clear(havana_fault_type * havana_fault) {
  scalar_clear(havana_fault->scalar);
}



havana_fault_type * havana_fault_copyc(const havana_fault_type *havana_fault) {
  havana_fault_type * new = havana_fault_alloc(havana_fault->config); 
  scalar_memcpy(new->scalar , havana_fault->scalar);
  return new; 
}



void havana_fault_load(havana_fault_type * havana_fault , buffer_type * buffer) {
  enkf_util_assert_buffer_type( buffer , HAVANA_FAULT );
  scalar_buffer_fload( havana_fault->scalar , buffer);
}

void havana_fault_upgrade_103(const char * filename) {
  FILE * stream            = util_fopen( filename , "r");
  enkf_impl_type impl_type = util_fread_int( stream );
  int size                 = util_fread_int( stream );
  double * data            = util_malloc( size * sizeof * data , __func__ ); 
  util_fread( data , sizeof * data , size , stream , __func__);
  fclose( stream );
  {
    buffer_type * buffer = buffer_alloc( 100 );
    buffer_fwrite_time_t( buffer , time(NULL));
    buffer_fwrite_int( buffer , impl_type );
    buffer_fwrite(buffer , data , sizeof * data    ,size);
    buffer_store( buffer , filename);
    buffer_free( buffer );
  }
  free( data );
}




bool havana_fault_store(const havana_fault_type *havana_fault , buffer_type * buffer,  bool internal_state) {
  buffer_fwrite_int( buffer , HAVANA_FAULT );
  scalar_buffer_fsave(havana_fault->scalar , buffer , internal_state);
  return true;
}


void havana_fault_truncate(havana_fault_type * havana_fault) {
  scalar_truncate( havana_fault->scalar );  
}


bool  havana_fault_initialize(havana_fault_type *havana_fault, int iens) { 
  scalar_sample(havana_fault->scalar);
  return true;
} 


int havana_fault_serialize(const havana_fault_type *havana_fault , serial_state_type * serial_state , size_t serial_offset , serial_vector_type * serial_vector) {
  return scalar_serialize(havana_fault->scalar , serial_state , serial_offset , serial_vector);
}

void havana_fault_deserialize(havana_fault_type *havana_fault , serial_state_type * serial_state, const serial_vector_type * serial_vector) {
  scalar_deserialize(havana_fault->scalar , serial_state , serial_vector);
}


void havana_fault_matrix_serialize(const havana_fault_type *havana_fault , const active_list_type * active_list , matrix_type * A , int row_offset , int column) {
  scalar_matrix_serialize(havana_fault->scalar , active_list , A , row_offset , column);
}


void havana_fault_matrix_deserialize(havana_fault_type *havana_fault , const active_list_type * active_list , const matrix_type * A , int row_offset , int column) {
  scalar_matrix_deserialize(havana_fault->scalar , active_list , A , row_offset , column);
}






//havana_fault_type * havana_fault_alloc_mean(int ens_size , const havana_fault_type **havana_fault_ens) {
//  int iens;
//  havana_fault_type * avg_havana_fault = havana_fault_copyc(havana_fault_ens[0]);
//  for (iens = 1; iens < ens_size; iens++) 
//    havana_fault_iadd(avg_havana_fault , havana_fault_ens[iens]);
//  havana_fault_iscale(avg_havana_fault , 1.0 / ens_size);
//  return avg_havana_fault;
//}


void havana_fault_filter_file(const havana_fault_type * havana_fault , const char * run_path, int *ntarget_ref, char ***target_ref) 
{
  const int size             = havana_fault_config_get_data_size(havana_fault->config);
  const double * output_data = scalar_get_output_ref(havana_fault->scalar);
  subst_list_type * subst_list = subst_list_alloc();
  int ikw;
  int ntemplates = 0;
  char ** target;
  
  havana_fault_output_transform(havana_fault);
  for (ikw = 0; ikw < size; ikw++) {
    char * tagged_fault = enkf_util_alloc_tagged_string( havana_fault_config_get_name(havana_fault->config , ikw) );
    subst_list_insert_owned_ref( subst_list , tagged_fault , util_alloc_sprintf( "%g" , output_data[ikw] ));
    free( tagged_fault );
  }
  

  /* 
     Scan through the list of template files and create target files. 
  */
 {
   const char *template_file_list = havana_fault_config_get_template_ref(havana_fault->config);
   FILE * stream = util_fopen(template_file_list,"r");
   bool end_of_file;
   fscanf(stream,"%d",&ntemplates);
   printf("%s %d\n","Number of template files: ",ntemplates);
   
   target             = util_malloc(ntemplates * sizeof(char *) , __func__);
   
   for( int i=0; i < ntemplates; i++) {
       char * target_file_root;
       char * template_file;
       
       util_forward_line(stream,&end_of_file);
       if(end_of_file) 
	util_abort("%s: Premature end of file when reading list of template files for Havana from:%s \n",__func__ , template_file_list);
      
      /* Read template file */
      template_file =  util_fscanf_alloc_token(stream);
      /* printf("%s\n",template_file); */

      /* Read target file root */
      target_file_root = util_fscanf_alloc_token(stream);
      /* printf("%s\n",target_file_root); */

      target[i] = util_alloc_filename(run_path , target_file_root , NULL);


      printf("%s   %s  \n",template_file,target[i]); 
      
      subst_list_filter_file(subst_list , template_file , target[i]);
      free(target_file_root);
      free(template_file);
   }
   fclose(stream);
 }
  
  
  /* Return values */
  *ntarget_ref     = ntemplates;
  *target_ref      = target;
  subst_list_free(subst_list);
}








/**
  This function writes the results for eclipse to use. Observe that
  for this function the second argument is a target_path (the
  config_object has been allocated with target_file == NULL).

*/


void havana_fault_ecl_write(const havana_fault_type * havana_fault , const char * run_path , const char * file /* This is NOT used. */ , fortio_type * fortio) {
  havana_fault_config_run_havana(havana_fault->config , havana_fault->scalar ,  run_path);
}


void havana_fault_export(const havana_fault_type * havana_fault , int * _size , char ***_kw_list , double **_output_values) {
  havana_fault_output_transform(havana_fault);

  *_kw_list       = havana_fault_config_get_name_list(havana_fault->config);
  *_size          = havana_fault_config_get_data_size(havana_fault->config);
  *_output_values = (double *) scalar_get_output_ref(havana_fault->scalar);

}

const char * havana_fault_get_name(const havana_fault_type * havana_fault, int kw_nr) {
  return  havana_fault_config_get_name(havana_fault->config , kw_nr);
}



double havana_fault_user_get(const havana_fault_type * havana_fault , const char * index_string , bool * valid) {
  const bool internal_value = false;
  gen_kw_config_type * gen_kw_config = havana_fault_config_get_gen_kw_config( havana_fault->config );
  int index                          = gen_kw_config_get_index( gen_kw_config , index_string );
  if (index < 0) {
    *valid = false;
    return 0;
  } else {
    *valid = true;
    return scalar_iget_double(havana_fault->scalar , internal_value , index);
  }
}




/******************************************************************/
/* Anonumously generated functions used by the enkf_node object   */
/******************************************************************/

SAFE_CAST(havana_fault , HAVANA_FAULT);
//MATH_OPS_SCALAR(havana_fault);
//ALLOC_STATS(havana_fault);
VOID_USER_GET(havana_fault);
VOID_ALLOC(havana_fault);
VOID_REALLOC_DATA(havana_fault);
VOID_SERIALIZE (havana_fault);
VOID_DESERIALIZE (havana_fault);
VOID_INITIALIZE(havana_fault);
VOID_FREE_DATA(havana_fault)
VOID_COPYC  (havana_fault)
VOID_FREE   (havana_fault)
VOID_ECL_WRITE(havana_fault)
VOID_LOAD(havana_fault)
VOID_STORE(havana_fault)
VOID_MATRIX_SERIALIZE(havana_fault)
VOID_MATRIX_DESERIALIZE(havana_fault)

