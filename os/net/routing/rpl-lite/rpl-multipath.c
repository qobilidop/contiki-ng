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
    if (packet_count  < packet_count_threshold) {
        send_congestion_notification();
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
  //
}
/*---------------------------------------------------------------------------*/
bool is_parent_congested()
{
  return false;
}
/*---------------------------------------------------------------------------*/
// TODO: figure out where to insert the code
void
rpl_multipath_packet_callback(void)
{
  ++packet_count;
}
/*---------------------------------------------------------------------------*/
// TODO: probably need to modify rpl_dio_t
void
rpl_multipath_dio_input_callback(rpl_dio_t *dio)
{
  congestion_notification_record(dio);
  if (is_parent_congested()) {
    rpl_multipath_start();
  }
  // n_congested_neighbors / n_neighbors
  if (get_parents_congestion_ratio() > 0.5) {
    // Pass on congestion notifications to children
    send_congestion_notification();
  }
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
