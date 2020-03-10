/**
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *  Multipath routing functionalities for RPL.
 */

/**
 * Initialize rpl-multipath module
*/
void rpl_multipath_init(void);

void rpl_multipath_packet_callback(void);

void rpl_multipath_dio_input_callback(rpl_dio_t *);

void rpl_multipath_start();

void rpl_multipath_stop();