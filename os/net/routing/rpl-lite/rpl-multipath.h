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

extern int self_is_congested = 0;
extern uip_ipaddr_t *parent_addr = NULL;
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
