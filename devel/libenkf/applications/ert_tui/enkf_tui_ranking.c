/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'enkf_tui_ranking.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <util.h>
#include <stdio.h>
#include <enkf_main.h>
#include <menu.h>
#include <enkf_obs.h>
#include <enkf_tui_util.h>



static void enkf_tui_ranking_make_misfit_table( void * arg) {
  enkf_main_type  * enkf_main  		       = enkf_main_safe_cast( arg );  
  enkf_fs_type               * fs              = enkf_main_get_fs(enkf_main);
  enkf_obs_type              * enkf_obs        = enkf_main_get_obs( enkf_main );
  const ensemble_config_type * ensemble_config = enkf_main_get_ensemble_config(enkf_main);
  const int history_length                     = enkf_main_get_history_length( enkf_main );
  const int ens_size                           = enkf_main_get_ensemble_size( enkf_main );
  misfit_table_type * misfit_table             = misfit_table_alloc( ensemble_config , fs , history_length , ens_size , enkf_obs );
  enkf_main_set_misfit_table( enkf_main , misfit_table );
}


static void enkf_tui_ranking_create( void * arg ) {
  enkf_main_type    * enkf_main       = enkf_main_safe_cast( arg );  
  enkf_obs_type     * enkf_obs        = enkf_main_get_obs( enkf_main );
  misfit_table_type * misfit_table    = enkf_main_get_misfit( enkf_main );
  
  if (misfit_table == NULL) {
    fprintf(stderr,"Sorry: must create a misfit table first \n");
    return;
  } else {
    const int history_length    = enkf_main_get_history_length( enkf_main );
    const int    prompt_len = 50;
    const char * prompt1      = "Observations to use for ranking";
    const char * prompt2      = "Name to store ranking under";
    const char * store_prompt = "Name of file to store ranking";
    int step1,step2;
    stringlist_type * ranking_keys;
    char * obs_keys_input;
    char * ranking_key;
    char * ranking_file;
    util_printf_prompt(prompt1 , prompt_len , '=' , "=> ");
    obs_keys_input = util_alloc_stdin_line();
    ranking_keys   = enkf_obs_alloc_matching_keylist( enkf_obs , obs_keys_input );
    enkf_tui_util_scanf_report_steps(history_length , prompt_len , &step1 , &step2);

    util_printf_prompt(prompt2 , prompt_len , '=' , "=> ");
    ranking_key = util_alloc_stdin_line();
    if (ranking_key == NULL) 
      ranking_key = util_alloc_string_copy( MISFIT_DEFAULT_RANKING_KEY );

    util_printf_prompt(store_prompt , prompt_len , '=' , "=> ");
    ranking_file = util_alloc_stdin_line();
        
    if (stringlist_get_size( ranking_keys ) > 0) {
      misfit_table_create_ranking( misfit_table  , ranking_keys , step1 , step2 , ranking_key , ranking_file);
      misfit_table_display_ranking( misfit_table , ranking_key);
    } else
      fprintf(stderr,"The input string : \"%s\" did not resolve to any valid observation keys \n", obs_keys_input);
    
    free( obs_keys_input );
    stringlist_free( ranking_keys );
    free( ranking_key );
    util_safe_free( ranking_file );
  }
}


static void enkf_tui_ranking_display( void * arg ) {
  enkf_main_type    * enkf_main       = enkf_main_safe_cast( arg );  
  misfit_table_type * misfit_table    = enkf_main_get_misfit( enkf_main );

  if (misfit_table == NULL) {
    fprintf(stderr,"Sorry: must create a misfit table first \n");
    return;
  } else {
    const int prompt_len  = 50;
    const char * prompt1  = "Ranking to display";
    char * ranking_key;
    
    util_printf_prompt(prompt1 , prompt_len , '=' , "=> ");
    ranking_key    = util_alloc_stdin_line();
    if (ranking_key == NULL) 
      ranking_key = util_alloc_string_copy( MISFIT_DEFAULT_RANKING_KEY);
    if (misfit_table_has_ranking( misfit_table , ranking_key))
      misfit_table_display_ranking( misfit_table , ranking_key);
    else
      fprintf(stderr,"Sorry: could not find ranking key: %s \n", ranking_key );
    
    free( ranking_key );
  }
}


void enkf_tui_ranking_menu(void * arg) {
    
  enkf_main_type  * enkf_main  = enkf_main_safe_cast( arg );  
  {
    menu_type * menu = menu_alloc("Ranking of results" , "Back" , "bB");
    menu_add_item(menu , "Create/update misfit table" , "cC"  , enkf_tui_ranking_make_misfit_table   , enkf_main , NULL);
    menu_add_separator( menu );
    menu_add_item(menu , "New ranking" , "nN" , enkf_tui_ranking_create  , enkf_main , NULL);
    menu_add_item(menu , "Display ranking"    , "dD" , enkf_tui_ranking_display , enkf_main , NULL);
    
    menu_run(menu);
    menu_free(menu);
  }
  
}
