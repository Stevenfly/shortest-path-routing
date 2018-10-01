/*
 * router.h - CS 456 A3
 * 20610147 | Yufei Yang
 */

#include <map>

#define NBR_ROUTER 5            /* for simplicity we consider only 5 routers */
#define MAX_BUFFER 1024         /* max size for buffer */

struct pkt_HELLO
{
    unsigned int router_id;     /* id for the router who sends the HELLO PDU */
    unsigned int link_id;       /* id of the link through which it is sent */
};

struct pkt_LSPDU
{
    unsigned int sender;        /* sender of the LS PDU */
    unsigned int router_id;     /* router id */
    unsigned int link_id;       /* link id */
    unsigned int cost;          /* cost of the link */
    unsigned int via;           /* id of the link through which the LS PDU is sent */
};

struct pkt_INIT
{
    unsigned int router_id;     /* id of the router that send the INIT PDU */
};

struct link_cost
{
    unsigned int link;          /* link id */
    unsigned int cost;          /* associated cost */
};

struct circuit_DB
{
    unsigned int nbr_link;                  /* number of links attached to a router */
    struct link_cost linkcost[NBR_ROUTER];  /* we assume that at most NBR_ROUTER links are attached to each router */
};

/*
 * structure for Routing Information Base
 */
struct RIB
{
    unsigned int path;          /* the path to the dest */
    unsigned int min_cost;      /* minimum cost */
    bool is_subset;             /* in N' such that D(w) is a minimum */
};

/*
 * Type definitions
 */
typedef std::map<int, int> valid_map;                       /* valid_map    ->  map(router_id, link_id)     */
typedef std::map<int, int>::iterator valid_map_it;
typedef std::map<int, int> ls_entry;                        /* ls_entry     ->  map(link, cost)             */
typedef std::map<int, int>::iterator ls_entry_it;
typedef std::map<int, ls_entry> lsdb_map;                 /* lsdb_map     ->  map(router_id, ls_entry)    */
typedef std::map<int, ls_entry>::iterator lsdb_map_it;
typedef std::map<int, struct RIB> rib_map;                       /* rib_map      ->  map(router_id, rib)         */
typedef std::map<int, struct RIB>::iterator rib_map_it;
