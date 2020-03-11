/**
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *         RPL multipath extension.
 *
 */

#include "net/routing/rpl-lite/rpl.h"
#include "net/ipv6/uip-ds6-route.h"
#include "lib/random.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

#define CONGESTION_INTERVAL_S ((1UL << RPL_MULTIPATH_CONGESTION_INTERVAL) * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
static struct ctimer congestion_timer;

static void handle_congestion_timer(void *ptr);
static void rpl_multicast_reset_state(void);

bool self_is_congested(void);
/* A net-layer sniffer for packets sent and received */
static void rpl_multipath_input_callback(void);
static void rpl_multipath_output_callback(int mac_status);
NETSTACK_SNIFFER(rpl_multipath_sniffer, rpl_multipath_input_callback, rpl_multipath_output_callback);

bool parent_is_congested(void);
void send_congestion_notification(void);
void send_congestion_notification_immediately(void);

void rpl_multipath_start(void);
void rpl_multipath_stop(void);

/*---------------------------------------------------------------------------*/
/*------------------------------- Main ------------------------------------- */
/*---------------------------------------------------------------------------*/
void
rpl_multipath_init(void)
{
  rpl_multicast_reset_state();
  netstack_sniffer_add(&rpl_multipath_sniffer);
  ctimer_set(&congestion_timer, CONGESTION_INTERVAL_S, handle_congestion_timer, NULL);
}
/*---------------------------------------------------------------------------*/
static void
handle_congestion_timer(void *ptr)
{
  if (self_is_congested()) {
    send_congestion_notification();
  }
  if (!parent_is_congested()) {
    rpl_multipath_stop();
  }
  rpl_multicast_reset_state();
  ctimer_reset(&congestion_timer);
}
static void
rpl_multicast_reset_state(void)
{
  curr_instance.mp.pkt_cnt_rx = 0;
  curr_instance.mp.pkt_cnt_rx_exp = 0;
  curr_instance.mp.pkt_cnt_tx_dao = 0;
}
/*---------------------------------------------------------------------------*/
/*------------------------- Congestion detection --------------------------- */
/*---------------------------------------------------------------------------*/
bool
self_is_congested(void)
{
  uint16_t pkt_cnt_rx_threshold = curr_instance.mp.pkt_cnt_rx_exp * RPL_MULTIPATH_CONGESTION_THRESHOLD / RPL_MULTIPATH_CONGESTION_RANGE;
  return curr_instance.mp.pkt_cnt_rx < pkt_cnt_rx_threshold;
}
/*---------------------------------------------------------------------------*/
void rpl_multipath_dao_input_callback(unsigned char *buffer, int pos)
{
  uint16_t pkt_cnt_tx_dao;
  memcpy(&pkt_cnt_tx_dao, buffer + pos + 2, 2);
  curr_instance.mp.pkt_cnt_rx_exp += pkt_cnt_tx_dao;
}
/*---------------------------------------------------------------------------*/
int rpl_multipath_dao_output_callback(unsigned char *buffer, int pos, uint8_t subopt_type)
{
  buffer[pos++] = subopt_type;
  buffer[pos++] = 2;
  memcpy(buffer + pos, &curr_instance.mp.pkt_cnt_tx_dao, 2);
  curr_instance.mp.pkt_cnt_tx_dao = 0;
  return pos + 2;
}
/*---------------------------------------------------------------------------*/
static void
rpl_multipath_input_callback(void) {
  curr_instance.mp.pkt_cnt_rx++;
}
/*---------------------------------------------------------------------------*/
static void
rpl_multipath_output_callback(int mac_status) {
  curr_instance.mp.pkt_cnt_tx_dao++;
}
/*---------------------------------------------------------------------------*/
/*------------------------- Congestion notification ------------------------ */
/*---------------------------------------------------------------------------*/
// TODO: probably need to modify rpl_dio_t
void
rpl_multipath_dio_input_callback(rpl_dio_t *dio)
{
  // congestion_notification_record(dio);
  // if (is_parent_congested()) {
  //   rpl_multipath_start();
  // }
  // // n_congested_neighbors / n_neighbors
  // if (get_parents_congestion_ratio() > 0.5) {
  //   // Pass on congestion notifications to children
  //   send_congestion_notification();
  // }
}
/*---------------------------------------------------------------------------*/
// void
// rpl_multipath_dio_output_callback(rpl_dio_t *dio)
// {
// }
/*---------------------------------------------------------------------------*/
// void
// congestion_notification_clear()
// {
// }
/*---------------------------------------------------------------------------*/
bool
parent_is_congested(void)
{
  return false;
}
/*---------------------------------------------------------------------------*/
void
send_congestion_notification(void)
{
  // TODO: implement tricle timer based sending policy
  send_congestion_notification_immediately();
}
/*---------------------------------------------------------------------------*/
void
send_congestion_notification_immediately(void)
{
}
/*---------------------------------------------------------------------------*/
/*---------------------------- Multipath routing --------------------------- */
/*---------------------------------------------------------------------------*/
void
rpl_multipath_start(void)
{
 // default_parent = curr_instance.dag.preferred_parent;

     /*select path use probability 
      int nbr_count = rpl_neighbor_count();
      total_weight;
      int rand = random_rand() % total_weight;
    */

    int nbr_count = rpl_neighbor_count();
    int rand = random_rand() % 2;
    if(rand == 0){
      uip_ds6_defrt_multi(1, NULL);
    }
    else{
      rand = random_rand() % nbr_count;
      rpl_nbr_t *nbr = rpl_neighbor_get_from_index(rand);
      uip_ds6_defrt_multi(1,uip_ds6_defrt_lookup(rpl_neighbor_get_ipaddr(nbr)));
    }

}
/*---------------------------------------------------------------------------*/
void
rpl_multipath_stop(void)
{
    //rpl_neighbor_set_preferred_parent(default_parent);
    uip_ds6_defrt_multi(0,NULL);
}
/*---------------------------------------------------------------------------*/

/** @} */
