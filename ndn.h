/**********************************************************************************************
 * Filename:    ndn.h
 * Description: Header responsible for all the shared functions, strucutures
 *              and even variables.
 * Author1:     Ricardo Rodrigues
 * Author2:     Rodrigo Quina
 * Date:        YYYY-MM-DD
 * 
 *
 * Usage:
 * Compile with: make
 * Run with: ./ndn <cache size> <ip> <tcp port>
 * 
 *********************************************************************************************/


#ifndef NDN_H
#define NDN_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/param.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_CACHE 100  // Definir tamanho da cache
#define MAX_INTR_SIZE 32
#define MAX_BUFFER_SIZE 1024 //Especifcamente para a lista de nós
#define MAX_N_NODES_IN_LIST 100 
#define MAX_N_OBJECTS 20

/********************************************************
 * State 
 * 
 * Estrutura que ira servir como "flags" para executar
 * apenas certas partes do codigo
 ********************************************************/
typedef enum state {
    NORMAL,
    ALONE, 
    UNREGISTERED, 
    WAS_CLOSED
} State;


/**********************************************
 * node_identifier
 * 
 * Estrutura que guarda:
 * -> o ip do no
 * -> o port, normalmente tcp
 * -> e o file descriptor caso necessário
 ********************************************/
typedef struct node_identifier {
    char ip[16];
    char tcp_port[6];
    int fd;
} node_identifier;


typedef struct OBJNAME {
    char objName[101];
}Obj;

typedef enum SOCKETSTATE {
    WAITING,
    CLOSED, 
    ANSWER, 
    NUN 
} IDstate;

typedef struct OBJSOCKET {
    int socket_id;
    IDstate socketState;
} ObjLine;

typedef struct VECTTAB {
    char objectName[101];
    ObjLine socketInfo[MAX_INTR_SIZE];
} requestedObject;

typedef struct INTTAB {
    int n_objects_iwant;
    requestedObject objPending[MAX_N_OBJECTS];
} interestTable;

/***************************************************************
 * myNode
 * 
 * Estrutura que guarda as informacoes essencias
 * associadas a um no:
 * -> informacoes sobre mim proprio
 * -> informacoes do meu vizinho externo
 * -> informacoes do meu vizinho de salvaguarda
 * -> informacoes sobre os meus vizinhos 
 *    internos, com um máximo de 32 vizinhos
 * -> numero de vizinhos internos que temos atualmente
 ************************************************************/
typedef struct node {
    node_identifier myself;
    node_identifier vz_extr;
    node_identifier vz_safe;
    /* Arrary de vizinhos internos */
    node_identifier vz_intr[MAX_INTR_SIZE];
    int n_intr;
    int fd_UDP;
    interestTable intTab;
    int CacheSize;
    int n_objs_inCache;
    Obj *receivedObjs;
    int n_inherentObjs;
    Obj inherentObjs[MAX_N_OBJECTS];
} myNode;


/*****************************************************
 * my_node
 * 
 * Variavel global do nosso no. Uma vez que
 * cada ocorrencia deste programa representara um no
 * podemos assumir que cada programa tera de ter 
 * uma estrutura geral a todo o seu codigo
 ****************************************************/
extern myNode my_node;



/*************************************************************************************
 * ---------------------------  Funcoes de inicializacao -----------------------------
 * 
 * - Funcoes estas que sao usadas normalmente uma vez no inicio do codigo
 *************************************************************************************/

void initNode(char *ip, char *port, int flag);
int CreateListenSocket(char *sourcePort);


/***************************************************************************************************************
 * -------------------------------------- Funcoes de comandos -------------------------------------------------
 * 
 * - Funcoes que sao acionadas imediamente se executado o seu respetivo comando
 **************************************************************************************************************/

int DirectJoin(char *connectIP, char *connectPort);
int Join(char *regIP, char *regUDP, char*netID);

void createObject(char *name);
void deleteObject(char *name);
void retrieveObject(char *targetName);

void ShowTopology();
void showInterestTable();
void showNames();
void showCache();


/*************************************************************************************
 * -------------------------  Funcoes read and write  --------------------------------
 * 
 * - Funcoes que estao associadas a escrever e ler do socket, tanto TCP como UDP
 *************************************************************************************/

void ENTRY_MESSAGE_SEND(int fd, char *targetIP, char *targetTCP);
void SAFE_MESSAGE_SEND(int fd);
void REG_MESSAGE_SEND(int fd, char *netID, struct sockaddr_in addr, socklen_t addrlen);
void UNREG_MESSAGE_SEND(int fd, char *netID, struct sockaddr_in addr, socklen_t addrlen);

void ReadFunction(int fd, char *buffer, ssize_t buffer_size);
int LeaveDetected(fd_set readfds, char *choice, int position);

int missingEXTR();
void missingINTR(int position);


/*************************************************************************************
 * ------------------------------  Funcoes para objetos  -----------------------------
 * 
 * - Funcoes relacionadas a procura de objetos
 *************************************************************************************/

 void processINTEREST_received(int fd, char *targetName);
 void processOBJECT_received(char *name);
 void processNOOBJECT_received(int fd, char *name);
 
 void INTEREST_SEND_MESSAGE(int fd, char *name);
 void OBJECT_SEND_MESSAGE(int fd, char *name);
 void NOOBJECT_SEND_MESSAGE(int fd, char *name);

 void deleteEntry(int obj_position);
 void accessCache(const char *object);


/*************************************************************************************
 * --------------------------------  Outras Funcoes  ---------------------------------
 * 
 * - Funcoes para tratamento da informacao dos nos
 *************************************************************************************/

void processNodeList(char *NodeList, int *n_nodes, node_identifier *nodes);
void updateNodeStruct (State state, char *buffer);
void CloseAll(fd_set readfds);

void testUDP(struct sockaddr_in addr, socklen_t addrlen);
void notePrompt();

#endif