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
#include "net/nbr-table.h"
#include "lib/random.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

#define CONGESTION_INTERVAL_S ((1UL << RPL_MULTIPATH_CONGESTION_INTERVAL) * CLOCK_SECOND)

typedef struct {
  uint16_t rx;
  uint16_t rx_exp;
  uint16_t tx_dao;
} packet_counter_t;
static packet_counter_t packet_counter;

/*---------------------------------------------------------------------------*/
static struct ctimer congestion_timer;

static void handle_congestion_timer(void *ptr);
static void rpl_multipath_reset(void);

void detect_congestion(void);
/* A net-layer sniffer for packets sent and received */
static void rpl_multipath_input_callback(void);
static void rpl_multipath_output_callback(int mac_status);
NETSTACK_SNIFFER(rpl_multipath_sniffer, rpl_multipath_input_callback, rpl_multipath_output_callback);

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
  rpl_multipath_reset();
  netstack_sniffer_add(&rpl_multipath_sniffer);
  ctimer_set(&congestion_timer, CONGESTION_INTERVAL_S, handle_congestion_timer, NULL);
}
/*---------------------------------------------------------------------------*/
static void
handle_congestion_timer(void *ptr)
{
  detect_congestion();
  if (curr_instance.cong_stat.self_congested) {
    send_congestion_notification();
  }
  if (!curr_instance.cong_stat.parent_congested) {
    rpl_multipath_stop();
  }
  rpl_multipath_reset();
  ctimer_reset(&congestion_timer);
}
/*---------------------------------------------------------------------------*/
static void
rpl_multipath_reset(void)
{
  rpl_nbr_t *nbr;

  packet_counter.rx = 0;
  packet_counter.rx_exp = 0;
  packet_counter.tx_dao = 0;

  nbr = nbr_table_head(rpl_neighbors);
  while(nbr != NULL) {
    nbr->cn = false;
  }
}
/*---------------------------------------------------------------------------*/
/*------------------------- Congestion detection --------------------------- */
/*---------------------------------------------------------------------------*/
void
detect_congestion(void)
{
  curr_instance.cong_stat.self_congested = false;

  uint16_t threshold = packet_counter.rx_exp * RPL_MULTIPATH_CONGESTION_THRESHOLD / RPL_MULTIPATH_CONGESTION_RANGE;
  if(packet_counter.rx < threshold) {
    curr_instance.cong_stat.self_congested = true;
    return;
  }

  uint8_t total, congested;
  rpl_nbr_t *nbr;
  total = 0;
  congested = 0;
  nbr = nbr_table_head(rpl_neighbors);
  for(nbr = nbr_table_head(rpl_neighbors); nbr != NULL; nbr = nbr_table_next(rpl_neighbors, nbr)) {
    ++total;
    if (nbr->cn) {
      ++congested;
    }
  }
  if(total < (congested << 1)) {
    curr_instance.cong_stat.self_congested = true;
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_multipath_tx_dao_update(uint16_t tx_dao)
{
  packet_counter.rx_exp += tx_dao;
}
/*---------------------------------------------------------------------------*/
uint16_t
rpl_multipath_tx_dao_reset(void)
{
  uint16_t tx_dao = packet_counter.tx_dao;
  packet_counter.tx_dao = 0;
  return tx_dao;
}
/*---------------------------------------------------------------------------*/
static void
rpl_multipath_input_callback(void) {
  packet_counter.rx++;
}
/*---------------------------------------------------------------------------*/
static void
rpl_multipath_output_callback(int mac_status) {
  packet_counter.tx_dao++;
}
/*---------------------------------------------------------------------------*/
/*------------------------- Congestion notification ------------------------ */
/*---------------------------------------------------------------------------*/
// TODO: probably need to modify rpl_dio_t
void
rpl_multipath_handle_congestion_notification(uip_ipaddr_t *from)
{
  rpl_nbr_t *nbr;

  nbr = rpl_neighbor_get_from_ipaddr(from);
  if (nbr == curr_instance.dag.preferred_parent) {
    rpl_multipath_start();
  }
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
  rpl_icmp6_dio_output(NULL);
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
