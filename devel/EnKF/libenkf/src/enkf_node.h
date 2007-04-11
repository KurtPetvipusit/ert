#ifndef __ENKF_NODE_H__
#define __ENKF_NODE_H__
#include <stdlib.h>



/*****************************************************************/

typedef void (ecl_write_ftype)  (const void *);
typedef void (ecl_read_ftype)   (      void *);
typedef void (ens_read_ftype)   (      void *);
typedef void (ens_write_ftype)  (const void *);
typedef void (sample_ftype)     (      void *);
typedef void (free_ftype)       (      void *);

typedef struct enkf_node_struct enkf_node_type;


enkf_node_type * enkf_node_alloc(void * , ecl_read_ftype , ecl_write_ftype * , ens_read_ftype , ens_write_ftype , sample_ftype *, free_ftype);
void             enkf_sample    (enkf_node_type *);

void             enkf_node_ecl_write (const enkf_node_type *);
void             enkf_node_ecl_read  (enkf_node_type *);

void             enkf_node_ens_write (const enkf_node_type *);
void             enkf_node_ens_read  (enkf_node_type *);

#endif
