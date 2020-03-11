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
void rpl_multipath_init(void);

void rpl_multipath_dao_input_callback(unsigned char *buffer, int pos);

int rpl_multipath_dao_output_callback(unsigned char *buffer, int pos, uint8_t subopt_type);

void rpl_multipath_dio_input_callback(rpl_dio_t *dio);

void rpl_multipath_dio_output_callback(rpl_dio_t *dio);

/** @} */

#endif /* RPL_MULTIPATH_H */
