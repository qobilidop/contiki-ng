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

/********** Includes **********/

#include "net/routing/rpl-lite/rpl.h"

/********** Data structures **********/

struct rpl_multipath {
  bool on;
  bool skip;
  rpl_nbr_t *alt_route;
};
typedef struct rpl_multipath rpl_multipath_t;

/********** Public symbols **********/

extern rpl_multipath_t rpl_multipath;

/********** Public functions **********/

/**
 * Initialize rpl-multipath module
*/
void rpl_multipath_init(void);

/**
 * Update packet counter using the tx_dao value passed along DAO
*/
void rpl_multipath_tx_dao_update(uint16_t tx_dao);

/**
 * Return the current tx_dao value and reset
*/
uint16_t rpl_multipath_tx_dao_reset(void);

/**
 * Update internal states when a congestion notification is received
*/
void rpl_multipath_handle_congestion_notification(uip_ipaddr_t *from);

/** @} */

#endif /* RPL_MULTIPATH_H */
