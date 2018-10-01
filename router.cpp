/*
 * router.cpp - CS 456 A3
 * 20610147 | Yufei Yang
 * Acknowledgement:
 *   - I am the sole contributor to this assignment
 *   - help from A1
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <limits.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "router.h"
using namespace std;

/*
 * Global variables
 */
ofstream log_file;

int     router_id;      // Unique id for each router
char *  nse_host;       // host for the Network State Emulator
int     nse_port;       // port number for the Network State Emulator
int     router_port;    // router port

int     udp_sock_fd;                // socket descriptor
struct sockaddr_in udp_serv_addr;   // socket address

/*
 * Function to put out a message on error exits (condition true).
 */
void checkError(bool condition, string str) {
    if (condition) {
        cerr << str << endl;
        exit(0);
    }
}

/*
 * Function to check if an argument is integer
 */
bool isInteger(char * arg) {
    char *end;
    strtol(arg, &end, 10);
    return (*end == '\0');
}

/*
 * Initialize arguments and log file
 */
void initialize(int argc, char *argv[]) {
    // Check number and formats of arguments
    checkError(argc != 5, "ERROR wrong number of arguments (expected 4).");
    checkError(!isInteger(argv[3]), "ERROR argument nse_port should be integer");
    checkError(!isInteger(argv[4]), "ERROR argument router_port should be integer");
    
    // Assign input values
    router_id = atoi(argv[1]);
    nse_host = argv[2];
    nse_port = atoi(argv[3]);
    router_port = atoi(argv[4]);
    
    // log file
    string file_name = "router" + to_string(router_id) + ".log";
    log_file.open(file_name.c_str(), ofstream::out);
}

/*
 * Set UDP connection to the Network State Emulator, reference to A1
 */
void setupConnection() {
    struct hostent *udp_server;

    udp_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    checkError(udp_sock_fd == -1, "ERROR opening UDP socket");
    
    udp_server = gethostbyname(nse_host);
    checkError(udp_server == NULL, "ERROR unkown host");
    
    memset((char *) &udp_serv_addr, 0, sizeof(udp_serv_addr));
    udp_serv_addr.sin_family = AF_INET;
    memcpy((char *) udp_server->h_addr, (char *) &udp_serv_addr.sin_addr, udp_server->h_length);
    udp_serv_addr.sin_port = htons(nse_port);
}

/*
 * "constructor" for struct pkt_LSPDU
 */
struct pkt_LSPDU constructLSPDU(int sender, int router_id, int link_id, int cost, int via) {
    struct pkt_LSPDU lspdu_pkt;
    
    lspdu_pkt.sender    = sender;
    lspdu_pkt.router_id = router_id;
    lspdu_pkt.link_id   = link_id;
    lspdu_pkt.cost      = cost;
    lspdu_pkt.via       = via;
    
    return lspdu_pkt;
}

/*
 * "constructor" for struct RIB
 */
struct RIB constructRIB(int path, int min_cost, bool is_subset) {
    struct RIB rib;
    
    rib.path      = path;
    rib.min_cost  = min_cost;
    rib.is_subset = is_subset;
    
    return rib;
}

/*
 * Write link state databse to log file
 */
void logLSDB(lsdb_map &ls_db) {
    int id, link, cost;
    unsigned long nbr_link;
    ls_entry link_costs;
    
    log_file << "# Topology databse" << endl;
    for (lsdb_map_it db_it = ls_db.begin(); db_it != ls_db.end(); ++db_it) {
        id = db_it->first;
        link_costs = db_it->second;
        nbr_link = link_costs.size();
        
        log_file << "R" << router_id << " -> R" << id << " nbr link " << nbr_link << endl;
        
        if (nbr_link > 0) {
            for (ls_entry_it en_it = link_costs.begin(); en_it != link_costs.end(); ++en_it) {
                link = en_it->first;
                cost = en_it->second;
                
                log_file << "R" << router_id << " -> R" << id << " link " << link << " cost " << cost << endl;
            }
        }
    }
}

/*
 * Write RIB to log file
 */
void logRIB(rib_map &rib) {
    int id;
    struct RIB rib_entry;
    
    log_file << "# RIB" << endl;
    for (rib_map_it rm_it = rib.begin(); rm_it != rib.end(); ++rm_it) {
        id = rm_it->first;
        rib_entry = rm_it->second;
        
        if (id == router_id && rib_entry.is_subset) {
            log_file << "R" << router_id << " -> R" << id << " -> Local, " << rib_entry.min_cost << endl;
        } else if (rib_entry.path > 0 && rib_entry.is_subset) {
            log_file << "R" << router_id << " -> R" << id << " -> R" << rib_entry.path << ", " << rib_entry.min_cost << endl;
        }
    }
}

/*
 * Write circuit_DB to log file
 */
void logCircuitDB(struct circuit_DB &circuit_db) {
    int link, cost;
    unsigned int nbr_link = circuit_db.nbr_link;
    struct link_cost *linkcost = circuit_db.linkcost;

    log_file << endl;
    log_file << "# circuit DB" << endl;
    log_file << "nbr_link: " << nbr_link << endl;
    
    for (int i = 0; i < nbr_link; i++) {
        link = linkcost[i].link;
        cost = linkcost[i].cost;
        log_file << "L " << link << ", " << cost << endl;
    }
    
    log_file << endl;
}

/*
 * Send an INIT packet to the Network State Emulator
 */
void sendINIT() {
    ssize_t num_bytes;
    unsigned int length = sizeof(struct sockaddr_in);
    
    struct pkt_INIT init_pkt;
    init_pkt.router_id = router_id;
    
    num_bytes = sendto(udp_sock_fd, &init_pkt, sizeof(init_pkt), 0, (struct sockaddr *) &udp_serv_addr, length);
    checkError(num_bytes == -1, "ERROR sending INIT packet");
    
    // log INIT send
    log_file << "R" << router_id <<" sends INIT -> router_id: " << router_id << endl;
}

/*
 * Receive circuir database from the Network State Emulator
 */
void receiveCircuitDB(struct circuit_DB &circuit_db) {
    ssize_t num_bytes;
    struct sockaddr_in from;
    unsigned int length = sizeof(struct sockaddr_in);
    
    num_bytes = recvfrom(udp_sock_fd, &circuit_db, sizeof(struct circuit_DB), 0, (struct sockaddr *) &from, &length);
    checkError(num_bytes == -1, "ERROR receiving circuit_DB packet");
    
    // log circuit_DB receive
    logCircuitDB(circuit_db);
}

/*
 * Initialize link state database with the circuit_DB received
 */
void initLSDB(lsdb_map &ls_db, struct circuit_DB &circuit_db) {
    int nbr_link, link, cost;
    ls_entry link_costs;
    
    nbr_link = circuit_db.nbr_link;
    
    for (int i = 0; i < nbr_link; i++) {
        link = circuit_db.linkcost[i].link;
        cost = circuit_db.linkcost[i].cost;
        
        link_costs[link] = cost;
    }
    
    ls_db[router_id] = link_costs;
}

/*
 * Send HELLO packets to neighbours
 */
void sendHELLOs(struct circuit_DB &circuit_db) {
    ssize_t num_bytes;
    unsigned int length = sizeof(struct sockaddr_in);
    
    for (unsigned int i = 0; i < circuit_db.nbr_link; i++) {
        struct pkt_HELLO hello_pkt;
        hello_pkt.router_id = router_id;
        hello_pkt.link_id = circuit_db.linkcost[i].link;
        
        num_bytes = sendto(udp_sock_fd, &hello_pkt, sizeof(hello_pkt), 0, (struct sockaddr *) &udp_serv_addr, length);
        checkError(num_bytes == -1, "ERROR sending HELLO packet");
        
        // log HELLO send
        log_file << "R" << router_id <<" sends HELLO -> router_id: " << router_id << ", link_id: " << hello_pkt.link_id << endl;
    }
}

/*
 * When receive HEELO, send LS_PDU packets to the neighbour
 */
void processHELLO(lsdb_map &ls_db, valid_map &validation, char *buffer, ssize_t &num_bytes) {
    struct pkt_HELLO hello_pkt;
    struct pkt_LSPDU lspdu_pkt;
    int hello_router_id, hello_link_id, ls_router_id, en_link, en_cost;
    ls_entry link_costs;
    unsigned int length = sizeof(struct sockaddr_in);
    
    memcpy((void *) &hello_pkt, (const void *) buffer, num_bytes);
    
    hello_router_id = hello_pkt.router_id;
    hello_link_id = hello_pkt.link_id;
    
    validation[hello_router_id] = hello_link_id;
    // log HELLO receive
    log_file << "R" << router_id <<" receives HELLO -> router_id: " << hello_router_id << ", link_id: " << hello_link_id << endl;
    
    // Send LS_PDU packets to neighbours of the router
    for (lsdb_map_it db_it = ls_db.begin(); db_it != ls_db.end(); ++db_it) {
        ls_router_id    = db_it->first;
        link_costs      = db_it->second;
        
        if (link_costs.size() > 0) {
            for (ls_entry_it en_it = link_costs.begin(); en_it != link_costs.end(); ++en_it) {
                en_link     = en_it->first;
                en_cost     = en_it->second;
                
                // Sned packet
                lspdu_pkt = constructLSPDU(router_id, ls_router_id, en_link, en_cost, hello_link_id);
                num_bytes = sendto(udp_sock_fd, &lspdu_pkt, sizeof(lspdu_pkt), 0, (struct sockaddr *) &udp_serv_addr, length);
                checkError(num_bytes == -1, "ERROR sending LS_PDU packet (HELLO)");
                
                // log LS_PDU send
                log_file << "R" << router_id <<" sends LS_PDU -> ";
                log_file << "sender: " << router_id;
                log_file << ", router_id: " << ls_router_id;
                log_file << ", link_id: " << en_link;
                log_file << ", cost: " << en_cost;
                log_file << ", via: " << hello_link_id;
                log_file << endl;
            }
        }
    }
}

/*
 * Inform the rest of neighbours by forwarding the LS_PDU
 */
void forwardLSPDUs(lsdb_map &ls_db, valid_map &validation, struct pkt_LSPDU &lspdu_pkt) {
    int en_link, lspdu_router_id, lspdu_link_id, lspdu_cost, lspdu_via, link_id;
    ssize_t num_bytes;
    ls_entry router_entry;
    struct pkt_LSPDU pkt_data;
    unsigned int length = sizeof(struct sockaddr_in);
    
    lspdu_router_id = lspdu_pkt.router_id;
    lspdu_link_id   = lspdu_pkt.link_id;
    lspdu_cost      = lspdu_pkt.cost;
    lspdu_via       = lspdu_pkt.via;
    
    // go thourgh the neighbours
    router_entry = ls_db[router_id];
    for (ls_entry_it en_it = router_entry.begin(); en_it != router_entry.end(); ++en_it) {
        en_link = en_it->first;
        
        // don't send packet to the sender
        if (en_link != lspdu_via) {
            for (valid_map_it va_it = validation.begin(); va_it != validation.end(); ++va_it) {
                link_id = va_it->second;
                // check received HELLO routers
                if (link_id == en_link) {
                    pkt_data = constructLSPDU(router_id, lspdu_router_id, lspdu_link_id, lspdu_cost, en_link);
                    num_bytes = sendto(udp_sock_fd, &pkt_data, sizeof(pkt_data), 0, (struct sockaddr *) &udp_serv_addr, length);
                    checkError(num_bytes == -1, "ERROR sending LS_PDU packet (Forward)");
                    
                    // log LS_PDU send
                    log_file << "R" << router_id <<" sends LS_PDU -> ";
                    log_file << "sender: " << router_id;
                    log_file << ", router_id: " << lspdu_router_id;
                    log_file << ", link_id: " << lspdu_link_id;
                    log_file << ", cost: " << lspdu_cost;
                    log_file << ", via: " << en_link;
                    log_file << endl;
                }
            }
        }
    }
}

/*
 * Find the router id of minimum cost
 */
int getMinCostId(rib_map &rib, lsdb_map &ls_db){
    unsigned int min_cost = INT_MAX;
    int min_id = -1;
    int id;
    ls_entry link_costs;
    
    for (lsdb_map_it db_it = ls_db.begin(); db_it != ls_db.end(); ++db_it) {
        id = db_it->first;
        link_costs = db_it->second;
        
        if (link_costs.size() > 0 && !rib[id].is_subset && rib[id].min_cost < min_cost) {
            min_cost = rib[id].min_cost;
            min_id = id;
        }
    }
    
    return min_id;
}

/*
 * Loop for the SPF (Dijkstra’s) algorithm
 */
void runSPFLoop(rib_map &rib, lsdb_map &ls_db) {
    int minId, destId, link, cost;
    ls_entry link_costs;
    
    for(int i = 0; i < NBR_ROUTER; i++) {
        minId = getMinCostId(rib, ls_db);
        // If we can't find min, exit
        if (minId == -1) {
            break;
        } else {
            rib[minId].is_subset = true;
        }
        
        link_costs = ls_db[minId];
        for (ls_entry_it en_it = link_costs.begin(); en_it != link_costs.end(); en_it++) {
            link = en_it->first;
            cost = en_it->second;
            
            destId = -1;
            for (lsdb_map_it db_it = ls_db.begin(); db_it != ls_db.end(); ++db_it) {
                if (db_it->first != minId && db_it->second.find(link) != db_it->second.end()) {
                    destId = db_it->first;
                    break;
                }
            }
            
            // update
            if (destId != -1 && cost + rib[minId].min_cost < rib[destId].min_cost && !rib[destId].is_subset) {
                rib[destId].path = rib[minId].path;
                rib[destId].min_cost = cost + rib[minId].min_cost;
            }
        }
    }
}

/*
 * Run SPF (Dijkstra’s) algorithm to build RIB
 */
void buildRIB(rib_map &rib, valid_map &validation, lsdb_map &ls_db) {
    valid_map_it va_it;
    ls_entry_it en_it;
    ls_entry link_costs;
    int id, link, cost;
    
    link_costs = ls_db[router_id];
    
    // Initialization
    for (int i = 1; i <= NBR_ROUTER; i++) {
        if (i == router_id) {
            rib[i] = constructRIB(0, 0, true);
        } else {
            rib[i] = constructRIB(0, INT_MAX, false);
        }
    }
    
    for (lsdb_map_it db_it = ls_db.begin(); db_it != ls_db.end(); ++db_it) {
        id = db_it-> first;
        link_costs = db_it->second;
        
        va_it = validation.find(id);
        if (va_it != validation.end() && validation[id] > 0) {
            for (ls_entry_it en_it = link_costs.begin(); en_it != link_costs.end(); ++en_it) {
                link = en_it->first;
                cost = en_it->second;
                
                if (link == validation[id]) {
                    rib[id].path       = id;
                    rib[id].min_cost   = cost;
                }
            }
        }
    }
    
    // Loop
    runSPFLoop(rib, ls_db);
}

/*
 * When receive LS_PDU, the function does:
 *   - add (unique) LS_PDU information to ls_db
 *   - inform the rest of neighbours by forwarding the LS_PDU
 *   - run SPF algorithm on ls_db
 */
void processLSPDU(lsdb_map &ls_db, valid_map &validation, char *buffer, ssize_t &num_bytes, rib_map &rib) {
    struct pkt_LSPDU lspdu_pkt;
    lsdb_map_it it;
    int lspdu_router_id, lspdu_link_id, lspdu_cost, en_link, en_cost;
    ls_entry link_costs;
    bool duplicated = false;
    
    memcpy((void *) &lspdu_pkt, (const void *) buffer, num_bytes);
    
    lspdu_router_id = lspdu_pkt.router_id;
    lspdu_link_id   = lspdu_pkt.link_id;
    lspdu_cost      = lspdu_pkt.cost;
    
    // log LS_PDU receive
    log_file << "R" << router_id <<" receives LS_PDU -> ";
    log_file << "sender: " << lspdu_pkt.sender;
    log_file << ", router_id: " << lspdu_pkt.router_id;
    log_file << ", link_id: " << lspdu_pkt.link_id;
    log_file << ", cost: " << lspdu_pkt.cost;
    log_file << ", via: " << lspdu_pkt.via;
    log_file << endl;
    
    // If lspdu_pkt router exist in the link state databse, check duplicate
    it = ls_db.find(lspdu_router_id);
    if (it != ls_db.end()) {
        for (ls_entry_it en_it = it->second.begin(); en_it != it->second.end(); ++en_it) {
            en_link = en_it->first;
            en_cost = en_it->second;
            
            if (en_link == lspdu_link_id && en_cost == lspdu_cost) {
                duplicated = true;
                break;
            }
        }
        
        if (!duplicated) {
            // assign exsisting link_costs
            ls_db[lspdu_router_id][lspdu_link_id] = lspdu_cost;
        }
    } else {
        // new link_costs
        link_costs[lspdu_link_id] = lspdu_cost;
        ls_db[lspdu_router_id] = link_costs;
    }
    
    if (!duplicated) {
        buildRIB(rib, validation, ls_db);
        log_file << endl;
        logLSDB(ls_db);
        log_file << endl;
        logRIB(rib);
        log_file << endl;
        forwardLSPDUs(ls_db, validation, lspdu_pkt);
    }
}

/*
 * Process received packet (either HELLO or LS_PDU)
 */
void receivePacket(lsdb_map &ls_db, valid_map &validation, rib_map &rib) {
    char buffer[MAX_BUFFER];
    ssize_t num_bytes;
    struct sockaddr_in from;
    unsigned int length = sizeof(struct sockaddr_in);
    
    // Clean up buffer
    memset(&buffer, 0, MAX_BUFFER);
    
    num_bytes = recvfrom(udp_sock_fd, buffer, MAX_BUFFER, 0, (struct sockaddr *) &from, &length);
    checkError(num_bytes == -1, "ERROR receiving HELLO or LS_PDU packet");
    
    if (num_bytes == sizeof(struct pkt_HELLO)) {
        processHELLO(ls_db, validation, buffer, num_bytes);
    } else if (num_bytes == sizeof(struct pkt_LSPDU)) {
        processLSPDU(ls_db, validation, buffer, num_bytes, rib);
    } else {
        checkError(true, "ERROR receiving mistrious packet");
    }
}

/*
 * router <router_id> <nse_host> <nse_port> <router_port>
 */
int main(int argc, char *argv[]) {
    struct circuit_DB circuit_db;
    lsdb_map ls_db;         // link state databse
    valid_map validation;   // store received HELLO router status
    rib_map rib;
    
    initialize(argc, argv);                     // 1. Initialize
    setupConnection();                          // 2. Prepare socket
    sendINIT();                                 // 3. Send INIT packet
    receiveCircuitDB(circuit_db);               // 4. Receive circuit_DB
    initLSDB(ls_db, circuit_db);                // 5. Initialize ls_db
    sendHELLOs(circuit_db);                     // 6. Send HELLOs
    while (true) {
        receivePacket(ls_db, validation, rib);  // 7. Process received packets (repeat)
    }

    return 0;
}
