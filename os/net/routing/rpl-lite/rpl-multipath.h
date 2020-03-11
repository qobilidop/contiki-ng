/**
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *         Header file for rpl-multipath.
 *
 */

#ifndef RPL_MULTIPATH_H
#define RPL_MULTIPATH_H

/********** Public functions **********/

/**
 * Initialize rpl-multipath module
*/

#include <stdlib.h>

extern int self_is_congested = 0;
//extern uip_ipaddr_t *parent_addr = NULL;
extern int parent_is_congested = 0;

struct congestion_info {
    uip_ipaddr_t *node_addr;
    int is_valid;
    int is_congested;
    struct congestion_info *next_node;
    struct congestion_info *last_node;
};
typedef struct congestion_info congestion_info_t;

void handle_congestion_info(uip_ipaddr_t *node_addr, int is_congested);

void rpl_multipath_init(void);

void rpl_multipath_dao_input_callback(unsigned char *buffer, int pos);

int rpl_multipath_dao_output_callback(unsigned char *buffer, int pos, uint8_t subopt_type);

void rpl_multipath_dio_input_callback(rpl_dio_t *dio);

void rpl_multipath_dio_output_callback(rpl_dio_t *dio);

/** @} */

#endif /* RPL_MULTIPATH_H */
