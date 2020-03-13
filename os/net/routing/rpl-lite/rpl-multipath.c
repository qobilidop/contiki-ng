/**
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *         RPL multipath extension.
 *
 */

#include "contiki.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/nbr-table.h"
#include "lib/random.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "M-RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

#define MRPL_RANDOM MRPL_CONF_RANDOM
#define MRPL_RANDOM_TIMES 3

#define CONGESTION_INTERVAL_S ((1UL << RPL_MULTIPATH_CONGESTION_INTERVAL) * CLOCK_SECOND / 1000)

#if RPL_WITH_MULTIPATH

typedef struct {
  uint16_t rx;
  uint16_t rx_exp;
  uint16_t tx_dao;
} packet_counter_t;
static packet_counter_t packet_counter;
rpl_multipath_t rpl_multipath;

/*---------------------------------------------------------------------------*/
static struct ctimer congestion_timer;

static void handle_congestion_timer(void *ptr);

void detect_congestion(void);
/* A net-layer sniffer for packets sent and received */
static void rpl_multipath_input_callback(void);
static void rpl_multipath_output_callback(int mac_status);
NETSTACK_SNIFFER(rpl_multipath_sniffer, rpl_multipath_input_callback, rpl_multipath_output_callback);

void send_congestion_notification(void);
void send_congestion_notification_immediately(void);

void rpl_multipath_start(void);
void rpl_multipath_stop(void);
rpl_nbr_t *find_alt_route(void);
rpl_nbr_t *find_alt_route_random(int times);

/*---------------------------------------------------------------------------*/
/*------------------------------- Main ------------------------------------- */
/*---------------------------------------------------------------------------*/
void
rpl_multipath_init(void)
{
  LOG_INFO("use multipath rpl.\n");

  packet_counter.rx = 0;
  packet_counter.rx_exp = 0;
  packet_counter.tx_dao = 0;
  netstack_sniffer_add(&rpl_multipath_sniffer);

  rpl_multipath.on = false;
  rpl_multipath.skip = false;
  rpl_multipath.alt_route = NULL;

  ctimer_set(&congestion_timer, CONGESTION_INTERVAL_S, handle_congestion_timer, NULL);

  LOG_INFO("after use multipath rpl.\n");
}
/*---------------------------------------------------------------------------*/
static void
handle_congestion_timer(void *ptr)
{
  LOG_INFO("handle congestion timer\n");
  detect_congestion();
  if (curr_instance.congested) {
    send_congestion_notification();
  }
  if (curr_instance.dag.preferred_parent != NULL && !curr_instance.dag.preferred_parent->cn) {
    rpl_multipath_stop();
  }
  packet_counter.rx = 0;
  packet_counter.rx_exp = 0;
  ctimer_reset(&congestion_timer);
}
/*---------------------------------------------------------------------------*/
/*------------------------- Congestion detection --------------------------- */
/*---------------------------------------------------------------------------*/
void
detect_congestion(void)
{
  LOG_INFO("detect_congestion\n");
  uint16_t threshold = packet_counter.rx_exp * RPL_MULTIPATH_CONGESTION_THRESHOLD / RPL_MULTIPATH_CONGESTION_RANGE;
  LOG_INFO("rx: %u, threshold: %u\n", packet_counter.rx, packet_counter.rx_exp);
  if(packet_counter.rx < threshold) {
    curr_instance.congested = true;
    return;
  }
  else{
    curr_instance.congested = false;
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
    curr_instance.congested = true;
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
void
rpl_multipath_handle_congestion_notification(uip_ipaddr_t *from)
{
  if (!rpl_multipath.on &&
      rpl_neighbor_get_from_ipaddr(from) == curr_instance.dag.preferred_parent) {
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
  rpl_nbr_t *alt_route;
  #if !MRPL_RANDOM
    alt_route= find_alt_route();
  #else
    alt_route= find_alt_route_random(MRPL_RANDOM_TIMES);
  #endif
  LOG_INFO_6ADDR(rpl_neighbor_get_ipaddr(alt_route));
  if (alt_route != NULL) {
    uip_ds6_defrt_add(rpl_neighbor_get_ipaddr(alt_route), 0);
    rpl_multipath.alt_route = alt_route;
    rpl_multipath.on = true;
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_multipath_stop(void)
{
  rpl_multipath.on = false;
  rpl_multipath.skip = false;
  if(rpl_multipath.alt_route != NULL) {
    uip_ds6_defrt_rm(uip_ds6_defrt_lookup(
      rpl_neighbor_get_ipaddr(rpl_multipath.alt_route)));
    rpl_multipath.alt_route = NULL;
  }
}
/*---------------------------------------------------------------------------*/
rpl_nbr_t *
find_alt_route(void)
{
  rpl_nbr_t *nbr = nbr_table_head(rpl_neighbors);
  while(nbr != NULL) {
    if (rpl_neighbor_is_parent(nbr) && !nbr->cn &&
        nbr != curr_instance.dag.preferred_parent) {
      return nbr;
    }
    nbr = nbr_table_next(rpl_neighbors, nbr);
  }
  return NULL;
}

rpl_nbr_t *
find_alt_route_random(int times)
{
  int i;
  rpl_nbr_t *nbr;
  for(i=0;i<times;i++){
    int rand = random_rand() % rpl_neighbor_count();
    nbr = nbr_table_head(rpl_neighbors);
    while(nbr != NULL) {
      if(rand == 0){
        if (rpl_neighbor_is_parent(nbr) && !nbr->cn &&
            nbr != curr_instance.dag.preferred_parent) {
          return nbr;
        }
        break;
      }
      nbr = nbr_table_next(rpl_neighbors, nbr);
      --rand;
    }
  }
  return NULL;
}
#endif /* RPL_WITH_MULTIPATH */
/*---------------------------------------------------------------------------*/

/** @} */
