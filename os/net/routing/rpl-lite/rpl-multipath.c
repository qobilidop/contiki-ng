#include "net/routing/rpl-lite/rpl.h"
#include "lib/random.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

#define CONGESTION_INTERVAL_S ((1UL << RPL_MULTIPATH_CONGESTION_INTERVAL) * CLOCK_SECOND)
//#define SW_INTERVAL_S (CLOCK_SECOND)
/*---------------------------------------------------------------------------*/
static struct ctimer congestion_timer;
//static struct ctimer sw_parent_timer;
//rpl_nbr_t* default_parent;
uint16_t packet_count;
static void handle_congestion_timer(void *ptr);
void congestion_notification_clear();
static congestion_info_t *first_congestion_info = malloc(sizeof(*first_congestion_info));
first_congestion_info->node_addr = NULL;
first_congestion_info->is_valid = 0;
first_congestion_info->next_node = NULL;
first_congestion_info->last_node = NULL;
//free();

/*---------------------------------------------------------------------------*/
void
rpl_multipath_init(void)
{
    ctimer_set(&congestion_timer, CONGESTION_INTERVAL_S, handle_congestion_timer, NULL);
}
/*---------------------------------------------------------------------------*/
static void
handle_congestion_timer(void *ptr)
{
  uint16_t packet_count_threshold = packet_count_estimate() * RPL_MULTIPATH_CONGESTION_THRESHOLD / RPL_MULTIPATH_CONGESTION_RANGE;
  if (packet_count < packet_count_threshold) {
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
  packet_count = 0;
  congestion_notification_clear();
  ctimer_reset(&congestion_timer);
}
/*---------------------------------------------------------------------------*/
uint16_t
packet_count_estimate()
{
  return 0;
}
/*---------------------------------------------------------------------------*/
void
congestion_notification_clear()
{
  parent_is_congested = 0;
  congestion_info_t *t;
  for (t = first_congestion_info; t->next_node != NULL; t = t->next_node) {
    if (t->next_node->node_addr == node_addr) {
      t->next_node->is_valid = 0;
    }
  }
}
/*---------------------------------------------------------------------------*/
bool is_parent_congested()
{
  return parent_is_congested != 0;
}
/*---------------------------------------------------------------------------*/
// TODO: figure out where to insert the code
void
rpl_multipath_packet_callback(void)
{
  ++packet_count;
}
/*---------------------------------------------------------------------------*/
void
send_congestion_notification()
{
  // TODO: implement tricle timer based sending policy
  send_congestion_notification_immediately();
}
/*---------------------------------------------------------------------------*/
void
send_congestion_notification_immediately()
{
  rpl_icmp6_dio_output(NULL);
}
/*---------------------------------------------------------------------------*/
void rpl_multipath_start() {

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
void rpl_multipath_stop() {
  //rpl_neighbor_set_preferred_parent(default_parent);
  uip_ds6_defrt_multi(0,NULL);
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
  congestion_info_t *new_congestion_info = malloc(sizeof(*congestion_info_t));
  new_congestion_info->node_addr = node_addr;
  new_congestion_info->is_valid = 1;
  new_congestion_info->is_congested = is_congested;
  new_congestion_info->next_node = NULL;
  new_congestion_info->last_node = NULL;
  t->next_node = new_congestion_info;
  return;
}

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
void rpl_multipath_stop() {
    //rpl_neighbor_set_preferred_parent(default_parent);
    uip_ds6_defrt_multi(0,NULL);
}
