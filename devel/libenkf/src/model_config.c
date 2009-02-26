#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <util.h>
#include <stdlib.h>
#include <string.h>
#include <enkf_fs.h>
#include <path_fmt.h>
#include <enkf_sched.h>
#include <model_config.h>
#include <hash.h>
#include <history.h>
#include <config.h>
#include <sched_file.h>
#include <ecl_sum.h>
#include <ecl_util.h>
#include <ecl_grid.h>
#include <menu.h>
#include <enkf_types.h>
#include <plain_driver.h>
#include <forward_model.h>
#include <bool_vector.h>

/**
   This struct contains configuration which is specific to this
   particular model/run. Such of the information is actually accessed
   directly through the enkf_state object; but this struct is the
   owner of the information, and responsible for allocating/freeing
   it.

   Observe that the distinction of what goes in model_config, and what
   goes in ecl_config is not entirely clear; ECLIPSE is unfortunately
   not (yet ??) exactly 'any' reservoir simulator in this context.
*/


struct model_config_struct {
  forward_model_type  * std_forward_model;   	    /* The forward_model - as loaded from the config file. Each enkf_state object internalizes its private copy of the forward_model. */  
  bool                  use_lsf;             	    /* The forward models need to know whether we are using lsf. */  
  enkf_fs_type        * ensemble_dbase;      	    /* Where the ensemble files are stored */
  history_type        * history;             	    /* The history object. */
  path_fmt_type       * result_path;         	    /* path_fmt instance for results - should contain one %d which will be replaced report_step */
  path_fmt_type       * runpath;             	    /* path_fmt instance for runpath - runtime the call gets arguments: (iens, report_step1 , report_step2) - i.e. at least one %d must be present.*/  
  char                * plot_path;           	    /* A dumping ground for PLOT files. */
  enkf_sched_type     * enkf_sched;          	    /* The enkf_sched object controlling when the enkf is ON|OFF, strides in report steps and special forward model - allocated on demand - right before use. */ 
  char                * enkf_sched_file;     	    /* THe name of file containg enkf schedule information - can be NULL to get default behaviour. */
  int                   last_history_restart;    /* The end of the history. */
  int                   abs_last_restart;           /* The total end of schedule file - will be updated with enkf_sched_file. */  
  bool                  has_prediction;      	    /* Is the SCHEDULE_PREDICTION_FILE option set ?? */
  char                * lock_path;           	    /* Path containing lock files */
  lock_mode_type        runlock_mode;        	    /* Mode for locking run directories - currently not working.*/ 
  bool_vector_type    * internalize_state;   	    /* Should the (full) state be internalized (at this report_step). */
  bool_vector_type    * internalize_results; 	    /* Should the results (i.e. summary in ECLIPSE speak) be intrenalized at this report_step? */
  bool_vector_type    * __load_state;        	    /* Internal variable: is it necessary to load the state? */
  bool_vector_type    * __load_results;      	    /* Internal variable: is it necessary to load the results? */
};



static enkf_fs_type * fs_mount(const char * root_path , const char * lock_path) {
  const char * mount_map = "enkf_mount_info";
  return enkf_fs_mount(root_path , mount_map , lock_path);
}


void model_config_set_runpath_fmt(model_config_type * model_config, const char * fmt){
  if (model_config->runpath != NULL)
    path_fmt_free( model_config->runpath );
  
  model_config->runpath  = path_fmt_alloc_directory_fmt( fmt );
}

/**
   This function is not called at bootstrap time, but rather as part of an initialization
   just before the run. Can be called maaaanye times for one application invokation.

   Observe that the 'total' length is set as as the return value from this function.
*/


void model_config_set_enkf_sched(model_config_type * model_config , const ext_joblist_type * joblist , run_mode_type run_mode) {
  if (model_config->enkf_sched != NULL)
    enkf_sched_free( model_config->enkf_sched );
  
  model_config->enkf_sched  = enkf_sched_fscanf_alloc(model_config->enkf_sched_file       , 
						      model_config->last_history_restart  , 
						      model_config->abs_last_restart     , 
						      run_mode                            , 
						      joblist                             , 
						      model_config->use_lsf );
}


void model_config_set_enkf_sched_file(model_config_type * model_config , const char * enkf_sched_file) {
  model_config->enkf_sched_file = util_realloc_string_copy( model_config->enkf_sched_file , enkf_sched_file);
}


model_config_type * model_config_alloc(const config_type * config , const ext_joblist_type * joblist , int last_history_restart , const sched_file_type * sched_file , bool use_lsf) {
  model_config_type * model_config = util_malloc(sizeof * model_config , __func__);

  model_config->use_lsf            = use_lsf;
  model_config->plot_path          = NULL;
  model_config->result_path        = path_fmt_alloc_directory_fmt( config_get(config , "RESULT_PATH") );
  model_config->std_forward_model  = forward_model_alloc( config_alloc_joined_string( config , "FORWARD_MODEL" , " ") , joblist , model_config->use_lsf);
  model_config->runlock_mode       = lock_none;
  {
    char * cwd = util_alloc_cwd();
    model_config->lock_path      = util_alloc_filename(cwd , "locks" , NULL);
    free(cwd);
  }
  util_make_path( model_config->lock_path );
  model_config->ensemble_dbase = fs_mount( config_get(config , "ENSPATH") , model_config->lock_path);
  model_config->runpath         = NULL;
  model_config->enkf_sched      = NULL;
  model_config->enkf_sched_file = NULL;
  model_config_set_enkf_sched_file(model_config , config_safe_get(config , "ENKF_SCHED_FILE"));
  model_config_set_runpath_fmt( model_config , config_get(config , "RUNPATH") );
  model_config->history                 = history_alloc_from_sched_file(sched_file);  

  /**
     last_history_restart and abs_last_restart are inclusive upper limits.
  */
  model_config->last_history_restart = last_history_restart;
  
  /* 
     Currently only the historical part - if a prediction file is in use, 
     the extended length is pushed from enkf_state.
  */
  model_config->abs_last_restart     = last_history_restart;

  
  if (config_item_set(config ,  "SCHEDULE_PREDICTION_FILE"))
    model_config->has_prediction = true;
  else
    model_config->has_prediction = false;
  
  {
    const char * history_source = config_get(config , "HISTORY_SOURCE");
    const char * refcase        = NULL;
    bool  use_history;

    if (strcmp(history_source , "REFCASE_SIMULATED") == 0) {
      refcase = config_get(config , "REFCASE");
      use_history = false;
    } else if (strcmp(history_source , "REFCASE_HISTORY") == 0) {
      refcase = config_get(config , "REFCASE");
      use_history = true;
    }

    if ((refcase != NULL) && (strcmp(history_source , "SCHEDULE") != 0)) {
      char  * refcase_path;
      char  * refcase_base;
      char  * header_file;
      char ** summary_file_list;
      int     files;
      bool    fmt_file ,unified;
      ecl_sum_type * ecl_sum;
      
      util_alloc_file_components( refcase , &refcase_path , &refcase_base , NULL);
      printf("Loading summary from: %s \n",refcase_path);
      ecl_util_alloc_summary_files( refcase_path , refcase_base , &header_file , &summary_file_list , &files , &fmt_file , &unified);

      ecl_sum = ecl_sum_fread_alloc( header_file , files , (const char **) summary_file_list , true , true /* Endian convert */);
      history_realloc_from_summary( model_config->history , ecl_sum , use_history);        
      util_safe_free(header_file);
      util_safe_free(refcase_base);
      util_safe_free(refcase_path);
      util_free_stringlist(summary_file_list, files);
      ecl_sum_free(ecl_sum);
    }
  }
  {
    int num_restart = history_get_num_restarts(model_config->history);
    model_config->internalize_state   = bool_vector_alloc( num_restart , false);
    model_config->internalize_results = bool_vector_alloc( num_restart , false);
    model_config->__load_state        = bool_vector_alloc( num_restart , false); 
    model_config->__load_results      = bool_vector_alloc( num_restart , false);
  }
  model_config_set_plot_path( model_config , config_get(config , "PLOT_PATH"));

  return model_config;
}




void model_config_free(model_config_type * model_config) {
  free(model_config->plot_path);
  path_fmt_free(  model_config->result_path );
  path_fmt_free(  model_config->runpath );
  if (model_config->enkf_sched != NULL)
    enkf_sched_free( model_config->enkf_sched );
  util_safe_free( model_config->enkf_sched_file );
  free(model_config->lock_path);
  enkf_fs_free(model_config->ensemble_dbase);
  history_free(model_config->history);
  forward_model_free(model_config->std_forward_model);
  bool_vector_free(model_config->internalize_results);
  bool_vector_free(model_config->internalize_state);
  bool_vector_free(model_config->__load_state);
  bool_vector_free(model_config->__load_results);
  free(model_config);
}


enkf_fs_type * model_config_get_fs(const model_config_type * model_config) {
  return model_config->ensemble_dbase;
}


path_fmt_type * model_config_get_runpath_fmt(const model_config_type * model_config) {
  return model_config->runpath;
}

char * model_config_alloc_result_path(const model_config_type * config , int report_step) {
  return path_fmt_alloc_path(config->result_path , true , report_step);
}

enkf_sched_type * model_config_get_enkf_sched(const model_config_type * config) {
  return config->enkf_sched;
}

history_type * model_config_get_history(const model_config_type * config) {
  return config->history;
}


/**
   Because the different enkf_state instances can have different
   schedule prediction files they can in principle have different
   number of dates. This variable only records the longest.
*/
void model_config_update_last_restart(model_config_type * config, int last_restart) {
  if (config->abs_last_restart < last_restart)
    config->abs_last_restart = last_restart;
}



int model_config_get_last_history_restart(const model_config_type * config) {
  return config->last_history_restart;
}

int model_config_get_abs_last_restart(const model_config_type * config) {
  return config->abs_last_restart;
}

bool model_config_has_prediction(const model_config_type * config) {
  return config->has_prediction;
}


void model_config_set_plot_path(model_config_type * config , const char * path) {
  config->plot_path = util_realloc_string_copy( config->plot_path , path );
}

const char * model_config_get_plot_path(const model_config_type * config) {
  return config->plot_path;
}


void model_config_interactive_set_runpath__(void * arg) {
  arg_pack_type * arg_pack = arg_pack_safe_cast( arg );
  model_config_type * model_config = arg_pack_iget_ptr(arg_pack , 0);
  menu_item_type    * item         = arg_pack_iget_ptr(arg_pack , 1);
  char runpath_fmt[256];
  printf("Give runpath format ==> ");
  scanf("%s" , runpath_fmt);
  model_config_set_runpath_fmt(model_config , runpath_fmt);
  {
    char * menu_label = util_alloc_sprintf("Set new value for RUNPATH:%s" , runpath_fmt);
    menu_item_set_label( item , menu_label );
    free(menu_label);
  }
}


forward_model_type * model_config_get_std_forward_model( const model_config_type * config) {
  return config->std_forward_model;
}


/*****************************************************************/

/* Setting everything back to the default value: false. */
void model_config_init_internalization( model_config_type * config ) {
  bool_vector_reset(config->internalize_state);
  bool_vector_reset(config->__load_state);
  bool_vector_reset(config->internalize_results);
  bool_vector_reset(config->__load_results);
}


/**
   This function sets the internalize_state flag to true for
   report_step. Because of the coupling to the __load_state variable
   this function can __ONLY__ be used to set internalize to true. 
*/

void model_config_set_internalize_state( model_config_type * config , int report_step) {
  bool_vector_iset(config->internalize_state , report_step , true);
  bool_vector_iset(config->__load_state      , report_step , true);
}


void model_config_set_internalize_results( model_config_type * config , int report_step) {
  bool_vector_iset(config->internalize_results , report_step , true);
  bool_vector_iset(config->__load_results      , report_step , true);
}

void model_config_set_load_results( model_config_type * config , int report_step) {
  bool_vector_iset(config->__load_results , report_step , true);
}

void model_config_set_load_state( model_config_type * config , int report_step) {
  bool_vector_iset(config->__load_state , report_step , true);
}



/* Query functions. */

bool model_config_internalize_state( const model_config_type * config , int report_step) {
  return bool_vector_iget(config->internalize_state , report_step);
}

bool model_config_internalize_results( const model_config_type * config , int report_step) {
  return bool_vector_iget(config->internalize_results , report_step);
}


bool model_config_load_state( const model_config_type * config , int report_step) {
  return bool_vector_iget(config->__load_state , report_step);
}

bool model_config_load_results( const model_config_type * config , int report_step) {
  return bool_vector_iget(config->__load_results , report_step);
}


