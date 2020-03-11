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
//static struct ctimer sw_parent_timer;
//rpl_nbr_t* default_parent;
uint16_t packet_count;
static void handle_congestion_timer(void *ptr);
void congestion_notification_clear();
void send_congestion_notification();
void send_congestion_notification_immediately()
static congestion_info_t *first_congestion_info = malloc(sizeof(*first_congestion_info));
first_congestion_info->node_addr = NULL;
first_congestion_info->is_valid = 0;
first_congestion_info->next_node = NULL;
first_congestion_info->last_node = NULL;
//free();

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
  // if (self_is_congested()) {
  //   send_congestion_notification();
  // }
  // if (!parent_is_congested()) {
  //   rpl_multipath_stop();
  // }
  // rpl_multicast_reset_state();
  if (self_is_congested_func()) {
    self_is_congested = 1;
    send_congestion_notification();
  } else if (get_neighbors_congestion_status()) {
    self_is_congested = 1;
    send_congestion_notification();
  } else {
    self_is_congested = 0;
  }
  if (!is_parent_congested()) {
    rpl_multipath_stop();
  }
  rpl_multipath_reset_state();
  congestion_notification_clear();
  ctimer_reset(&congestion_timer);
}
static void
rpl_multipath_reset_state(void)
{
  curr_instance.mp.pkt_cnt_rx = 0;
  curr_instance.mp.pkt_cnt_rx_exp = 0;
  curr_instance.mp.pkt_cnt_tx_dao = 0;
}
/*---------------------------------------------------------------------------*/
/*------------------------- Congestion detection --------------------------- */
/*---------------------------------------------------------------------------*/
bool
self_is_congested_func(void)
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
int is_parent_congested()
{
  return parent_is_congested != 0;
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
void
congestion_notification_clear()
{
  parent_is_congested = 0;
  congestion_info_t *t;
  for (t = first_congestion_info; t->next_node != NULL; t = t->next_node) {
    t->next_node->is_valid = 0;
  }
}
/*---------------------------------------------------------------------------*/
bool
is_parent_congested(void)
{
  return parent_is_congested != 0;
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
void
handle_congestion_info(uip_ipaddr_t *node_addr, int is_congested)
{
  congestion_info_t *t;
  for (t = first_congestion_info; t->next_node != NULL; t = t->next_node) {
    if (t->next_node->node_addr == node_addr) {
      t->next_node->is_valid = 1;
      t->next_node->is_congested = is_congested;
      return;
    }
  }
  congestion_info_t *new_congestion_info = malloc(sizeof(*new_congestion_info));
  new_congestion_info->node_addr = node_addr;
  new_congestion_info->is_valid = 1;
  new_congestion_info->is_congested = is_congested;
  new_congestion_info->next_node = NULL;
  new_congestion_info->last_node = NULL;
  t->next_node = new_congestion_info;
  return;
}
/*---------------------------------------------------------------------------*/
int
get_neighbors_congestion_status()
{
  // if more than half of the neighbors are congested, send congestion info to child
  int congested_count = 0;
  int total_count = 0;
  congestion_info_t *t;
  for (t = first_congestion_info; t->next_node != NULL; t = t->next_node) {
    if (t->next_node->is_valid) {
      ++total_count;
      if (t->next_node->is_congested) {
        ++congested_count;
      }
    }
  }
  return congested_count > total_count / 2;
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
