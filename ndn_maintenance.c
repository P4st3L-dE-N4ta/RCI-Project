/**********************************************************************************************
 * Filename:    ndn_maintenance.c
 * Description: Separete .c file with several functions crucial for the main program.
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

#include "ndn.h"



/************************************************************************************
 * CreateListenSocket()
 * 
 * Cria o primeiro socket do no que vai estar sempre a escuta para novas conexoes
 * e nunca saira desse estado ate ser fechado. 
 * 
 * Step by Step:
 * -Cria socket
 * -Inicializa hints
 * -getaddrinfo
 * -bind
 * -listen
 * 
 * Inputs:
 * -> hints - structure that contains the characteristics of the socket to be created
 * -> res - structure that saves the information of the getaddrinfo searches
 * -> sourcePort - Port necessario para tirarmos as informacoes de nos proprios com
 *                 o getaddrinfo com IP = NULL
 *************************************************************************************/
int CreateListenSocket(char *sourcePort){
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP Socket 
    if(fd == -1){
        perror("Erro na criação do socket\n");
        exit(EXIT_FAILURE);
    }

    // Set SO_REUSEADDR to allow immediate reuse of the port
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP Socket
    hints.ai_flags = AI_PASSIVE; // Socket apenas para bind e accept

    /* With NULL Node it will be refered to our own machine */
    errcode = getaddrinfo(NULL, sourcePort, &hints, &res);
    if(errcode != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        perror("Error in binding\n");
        exit(EXIT_FAILURE);
    }
    
    /* 5 Connections will be queued before further requests are refused */
    if(listen(fd, 5) == -1){
        perror("Erro durante a escuta\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(res);
    my_node.myself.fd = fd;
    return fd;
}



/************************************************************************************
 * updateNodeStruct()
 * 
 * updates automatically the node struct depending on the message
 * ONLY ON THE RECEIVING END
 * 
 * Input: state(if ALONE safe=myself)
 *        buffer with the message
 *        
 * 
 ***********************************************************************************/
void updateNodeStruct (State state, char *buffer){
    char message_type[20];
    char arg1[20]; /* IP */
    char arg2[20]; /* TCP */

    sscanf(buffer, "%s %s %s", message_type, arg1, arg2);
    if(strcmp(message_type, "ENTRY") == 0){
        if(my_node.n_intr <= MAX_INTR_SIZE){
            /* ENTRY -- vzintr = conID */
            strcpy(my_node.vz_intr[my_node.n_intr].ip, arg1);
            strcpy(my_node.vz_intr[my_node.n_intr].tcp_port, arg2);
            my_node.n_intr++;
        }

    } else if(strcmp(message_type, "SAFE") == 0) {
        strcpy(my_node.vz_safe.ip, arg1);
        strcpy(my_node.vz_safe.tcp_port, arg2);
    }

}



/************************************************************************************
 * ENTRY_MESSAGE_SEND()
 * 
 * Writes in buffer the Entry Message, writes in stdin echo and
 * updates vz_extr to target node
 * 
 * Input: 
 *  -> fd - file descriptor of target node
 *  -> targetIP - IP of target node
 *  -> targetCP - TCP of target node
 **********************************************************************************/
void ENTRY_MESSAGE_SEND(int fd, char *targetIP, char *targetTCP){
    int n = 0;
    char buffer[128];

    snprintf(buffer, sizeof(buffer), "ENTRY %s %s\n", my_node.myself.ip, my_node.myself.tcp_port);
    n = write(fd, buffer, strlen(buffer));
    if(n == -1){
        perror("Error in write - ENTRY");
        exit(EXIT_FAILURE);
    }
    write(1, "echo: ", 6);
    write(1, buffer, n);

    strcpy(my_node.vz_extr.ip, targetIP);
    strcpy(my_node.vz_extr.tcp_port, targetTCP);
}



/************************************************************************************
 * SAFE_MESSAGE_SEND()
 * 
 * Writes in buffer the Safe Message, writes in stdin echo 
 * 
 * Input: 
 *  -> fd - file descriptor of target node
 * 
 **********************************************************************************/
void SAFE_MESSAGE_SEND(int fd) {
    int n = 0;
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "SAFE %s %s\n", my_node.vz_extr.ip, my_node.vz_extr.tcp_port);
    n = write(fd, buffer, strlen(buffer));
    if(n == -1){
        perror("Error in write - SAFE");
        exit(EXIT_FAILURE);
    }
    write(1, "echo: ", 6);
    write(1, buffer, n);
}



/***************************************************************************************************************
 * DirectJoin()
 * 
 * Creates a new socket to write in. Gets the target node address info 
 * and finally it connects to it. After connecting, it also sends an ENTRY Message
 * and updates the node structure and saves the file descriptor of the 
 * target node in its vizinho externo
 * 
 * Inputs:
 * -> readfds - file descriptors cluster to be monitored by select
 * -> connectIP - IP of the target Node
 * -> connectPort - Port of the target Node
 * -> hints - structure that contains the characteristics of the socket to be created
 * -> res - structure that saves the information of the getaddrinfo searches
 * 
 **************************************************************************************************************/
int DirectJoin(char *connectIP, char *connectPort){
    int fd, errcode, n;
    int new_fd;
    //char buffer[128];
    //State state = NORMAL;
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    /* Criacao do socket de escuta do no*/
    if(my_node.myself.fd == -1){
        new_fd = CreateListenSocket(my_node.myself.tcp_port);
        if(new_fd == -1){
            fprintf(stderr, "Something went wrong when creating Listen Socket");
            exit(EXIT_FAILURE);
        }
    }
    

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        perror("Error creating socket\n");
        exit(EXIT_FAILURE);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP Socket

    errcode = getaddrinfo(connectIP, connectPort, &hints, &res);
    if(errcode != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }
    if(strcmp(connectIP, "0.0.0.0") == 0){
        fd = -1;
        fprintf(stderr, "Cannot connect to 0.0.0.0. Will start its own network.\n");
        return fd;
    } 

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        perror("Error in connect\n");
        exit(EXIT_FAILURE);
    }
    /* Quando envio entry o socket criado para o connect fica como meu
     vizinho externo, ou seja, para quem eu estou a enviar, esse sera o meu 
     vizinho externo */
    my_node.vz_extr.fd = fd;
    
    /* remetente - ENTRY MESSAGE */
    ENTRY_MESSAGE_SEND(fd, connectIP, connectPort);
    


    freeaddrinfo(res);
    return fd;
        
}



/**********************************************************************************
 * Join()
 * 
 * Esta funcao faz apenas a criacao do socket para comunicacoes UDP e envia logo
 * para o servidor uma mensagem a perguntar a lista de nos
 * 
 * Inputs:
 * -> regIP - IP do servidor
 * -> regUDP - UDP servidor
 * -> readfds - file descriptors cluster to be monitored by select
 *******************************************************************************/
int Join(char *regIP, char *regUDP, char*netID){
    int fd_UDP, errcode;
    ssize_t n;
    //socklen_t addrlen;
    struct addrinfo hints, *res;
    //struct sockaddr_in addr;

    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, MAX_BUFFER_SIZE);
    char message[64];
    memset(message, 0,sizeof(message));

    fd_UDP = socket(AF_INET, SOCK_DGRAM, 0); // UDP Socket 
    if(fd_UDP == -1){
        perror("Erro na criação do socket\n");
        exit(EXIT_FAILURE);
    }
    /*FD_SET(fd_UDP, &readfds);
    nfds = MAX(nfds, fd_UDP);*/


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP Socket

    errcode = getaddrinfo(regIP, regUDP, &hints, &res);
    if(errcode != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        exit(EXIT_FAILURE);
    }

    /* Enviamos o pedido da lista ao servidor */
    sprintf(message, "NODES %s\n", netID);
    n = sendto(fd_UDP, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        perror("Error Sending UDP Message\n");
        exit(EXIT_FAILURE);
    }
    write(1, "echo: ", 6);
    write(1, message, strlen(message));

    freeaddrinfo(res);
    return fd_UDP;

}


/*********************************************************************************
 * REG_MESSAGE_SEND()
 * 
 * Uma funcao simples que escreve no socket udp a mensagem a pedir registo no
 * servidor
 * 
 * Inputs:
 * -> fd - file descriptor do socket udp conecatado ao servidor
 * -> netID - ID do servidor, numero constituido por 3 digitos
 * -> addr - variavel que descreve um endereco, normalmente utilizado 
 *           em sendto and recvfrom
 * -> addrlen - tamanho de addr
 *********************************************************************************/
void REG_MESSAGE_SEND(int fd, char *netID, struct sockaddr_in addr, socklen_t addrlen) {
    char buffer[64];
    int n;
    memset(buffer, 0, sizeof(buffer));

    snprintf(buffer, sizeof(buffer), "REG %s %s %s\n", netID, my_node.myself.ip, my_node.myself.tcp_port);
    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
    if (n == -1) {
        perror("Error: sendto (REG)");
        exit(EXIT_FAILURE);
    }
    write(1, "echo: ", 6);
    write(1, buffer, strlen(buffer));

    return;
}



/*********************************************************************************
 * UNREG_MESSAGE_SEND()
 * 
 * Uma funcao simples que escreve no socket udp a mensagem a pedir saida do
 * servidor e apago do registo
 * 
 * Inputs:
 * -> fd - file descriptor do socket udp conecatado ao servidor
 * -> netID - ID do servidor, numero constituido por 3 digitos
 * -> addr - variavel que descreve um endereco, normalmente utilizado 
 *           em sendto and recvfrom
 * -> addrlen - tamanho de addr
 *********************************************************************************/
void UNREG_MESSAGE_SEND(int fd, char *netID, struct sockaddr_in addr, socklen_t addrlen) {
    char buffer[64];
    int n;
    memset(buffer, 0, sizeof(buffer));

    if (fcntl(fd, F_GETFD) == -1) {
        perror("Socket is invalid");
    }

    snprintf(buffer, sizeof(buffer), "UNREG %s %s %s\n", netID, my_node.myself.ip, my_node.myself.tcp_port);
    n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *) &addr, addrlen);
    if (n == -1) {
        perror("Error: sendto (UNREG)");
        exit(EXIT_FAILURE);
    }
    write(1, "echo: ", 6);
    write(1, buffer, strlen(buffer));

    return;
}



/*************************************************************************************
 * testUDP()
 * 
 * Function used to test when UDP might stop working and doesnt return any errors.
 * 
 * Inputs:
 * -> addr - variavel que descreve um endereco, normalmente utilizado 
 *           em sendto and recvfrom
 * -> addrlen - address length 
 *************************************************************************************/
void testUDP(struct sockaddr_in addr, socklen_t addrlen){
    ssize_t n;
    char *test = "BANANA\n";
    if(my_node.fd_UDP != -1){
        n = sendto(my_node.fd_UDP, test, strlen(test), 0, (struct sockaddr *) &addr, addrlen);
        if (n == -1) {
            perror("Error: sendto (UNREG)");
            exit(EXIT_FAILURE);
        }
    }
    write(1, "echo: ", 6);
    write(1, test, strlen(test));

}


/***************************************************************************
 * processNodeList()
 * 
 * Quando recebemos a NODESLIST do servidor esta funcao processa-a.
 * Temos um pointeiro que vai guardando linha a linha e a partir desse 
 * ponteiro retiramos as informacoes que nos interesssam.
 * 
 * Inputs: 
 * -> NodeList - ponteiro para a lista de nos que recebemos
 * -> n_nodes - ponteiro para o numero de nos que queremos devolver
 * -> nodes - estrutura do tipo node_identifier que contem e ira guardar
 *            o ip e port de cada no na lista
 ****************************************************************************/
void processNodeList(char *NodeList, int *n_nodes, node_identifier *nodes) {
    const char *ptr = NodeList;

    // Queremos devolver este numero logo passamos por referencia
    *n_nodes = 0;

    if(NodeList == NULL){
        perror("Error in NodeList aquiring\n");
        exit(EXIT_FAILURE);
    }

    // Saltamos a primeira linha que diz "NODESLIST <net>"
    ptr = strchr(ptr, '\n'); // Returns pointer to the first \n found
    if(!ptr) return; // if there's no newline, return
    ptr++; // Moves the pointer to the next line


    while(sscanf(ptr, "%s %s", nodes[*n_nodes].ip, nodes[*n_nodes].tcp_port) == 2){
        (*n_nodes)++;
        ptr = strchr(ptr, '\n');
        if(!ptr) break;
        ptr++;
    }

}



/**********************************************************************************
 * missingINTR()
 * 
 * The function updates the number of sons a node has, 
 * nullifies the target node in the position and reorders the list of 
 * sons of the node.
 * 
 * Inputs:
 * -> position - Position in the list of sons where the node is to be 
 *               removed. 
 **********************************************************************************/
void missingINTR(int position){
    my_node.vz_intr[position].fd = -1;
    strcpy(my_node.vz_intr[position].ip, "\0");
    strcpy(my_node.vz_intr[position].tcp_port, "\0");

    
    // Se for o ultimo no da lista
    if(position == my_node.n_intr - 1){
        my_node.n_intr--;
    } else { 
        int i = position;
        char temp1[50], temp2[50];
        memset(temp1, 0, sizeof(temp1));
        memset(temp2, 0, sizeof(temp2));
        // Reordena a lista de nos 
        while(i+1 < my_node.n_intr){
            //Guarda o reset do no 
            strcpy(temp1, my_node.vz_intr[i].ip);
            strcpy(temp2, my_node.vz_intr[i].tcp_port);

            //Troca ate o no para largar estiver na ultima posicao
            strcpy(my_node.vz_intr[i].ip, my_node.vz_intr[i+1].ip);
            strcpy(my_node.vz_intr[i].tcp_port, my_node.vz_intr[i+1].tcp_port);
            my_node.vz_intr[i].fd = my_node.vz_intr[i+1].fd;
            i++;
        }
        // Atualiza o ultimo no a vazio
        strcpy(my_node.vz_intr[i].ip, temp1);
        strcpy(my_node.vz_intr[i].tcp_port, temp2);
        my_node.vz_intr[i].fd = -1;

        // Atualiza nr de vizinhos internos ativos
        my_node.n_intr--;
    }

    return;
}


/***************************************************************************************************************
 * missingEXTR()
 * 
 * Quando um no tem o seu vizinho externo em falta esta funcao processa os passos a 
 * seguir para coser o buraco feito. 
 * O programa divide-se em duas situacoes, se o no de salvaguarda for ele proprio e 
 * se o no de salvaguarda nao for ele proprio. 
 * 
 * 
 * Se o no de salvaguarda NAO for ele proprio, a funcao confirma se o vizinho externo
 * se encontra dentro dos seus vizinhos internos (por mais que neste caso isso nao deva
 * acontecer) e se encontrar elimina o no da lista de vizinhos internos. Assim liga-se ao vizinho 
 * de salvaguarda e envia uma mensagem de SAFE aos seus vizinhos internos.
 * 
 * Return value: int fd - file descriptor da conexao feita por dj
 * 
 * Inputs: None
 * 
 **********************************************************************************************************/
int missingEXTR(){

    int new_fd;
    int rand = -1;

    // Se encontrar o target na lista de vz_intr, elimina-o e reordena a lista
    for(int i = 0; i < my_node.n_intr; i++){ 
        if(strcmp(my_node.vz_extr.ip, my_node.vz_intr[i].ip) == 0 && strcmp(my_node.vz_extr.tcp_port, my_node.vz_intr[i].tcp_port) == 0){
            missingINTR(i);
            break;
        }
    }
    printf("I still have %d sons\n", my_node.n_intr);
    if(my_node.n_intr != 0)
        rand =  random() % my_node.n_intr;
    // Se o no de salvaguarda for ele proprio
    if(strcmp(my_node.vz_safe.ip, my_node.myself.ip) == 0 && strcmp(my_node.vz_safe.tcp_port, my_node.myself.tcp_port) == 0){
        
        // Caso de ter vizinhos internos
        if(my_node.n_intr != 0){
            printf("I chose %s %s as my external node\n", my_node.vz_intr[rand].ip, my_node.vz_intr[rand].tcp_port);
            // Will choose it's external from internal
            new_fd = DirectJoin(my_node.vz_intr[rand].ip, my_node.vz_intr[rand].tcp_port);

            // Send Message of SAFE 
            for(int i = 0; i < my_node.n_intr; i++){
                SAFE_MESSAGE_SEND(my_node.vz_intr[i].fd);
            }


        } else{ // Caso de n ter vizinhos internos
            printf("I dont have childs, so I'll stay alone for a bit\n");
            strcpy(my_node.vz_extr.ip, my_node.myself.ip);
            strcpy(my_node.vz_extr.tcp_port, my_node.myself.tcp_port);
            my_node.vz_extr.fd = -1;

            strcpy(my_node.vz_safe.ip, "/0");
            strcpy(my_node.vz_safe.tcp_port, "/0");
        }

    } else { // Se o no de salvaguarda nao for ele proprio

        // Se tiver vizinhos internos
        if(my_node.n_intr != 0){
            // Se encontrar o target na lista de vz_intr, elimina-o e reordena a lista
            for(int i = 0; i < my_node.n_intr; i++){ 
                if(strcmp(my_node.vz_extr.ip, my_node.vz_intr[i].ip) == 0 && strcmp(my_node.vz_extr.tcp_port, my_node.vz_intr[i].tcp_port) == 0){
                    missingINTR(i);
                    break;
                }
            }
        }
        printf("I chose %s %s as my external node (my safe node)\n", my_node.vz_safe.ip, my_node.vz_safe.tcp_port);
        new_fd = DirectJoin(my_node.vz_safe.ip, my_node.vz_safe.tcp_port);


        if(my_node.n_intr != 0){
            for(int i = 0; i < my_node.n_intr; i++){
                SAFE_MESSAGE_SEND(my_node.vz_intr[i].fd);
            }
        }

    }

    return new_fd;
}



/*********************************************************************************
 * LeaveDetected()
 * 
 * Basically it will detect what left from the network and sew together
 * what's left of the network.
 * 
 * Inputs: 
 * -> reads 
 *********************************************************************************/
 int LeaveDetected(fd_set readfds, char *choice, int position){
    int new_fd = -1;

    // Vizinho externo saiu da rede 
    if(strcmp(choice, "EXTR") == 0){
        FD_CLR(my_node.vz_extr.fd, &readfds);
        new_fd = missingEXTR();
    } else if(strcmp(choice, "INTR") == 0){ // Vizinho interno saiu da rede 
        // Pode ocorrer de ele detetar primeiro o interno do q o externo
        if(strcmp(my_node.vz_extr.ip, my_node.vz_intr[position].ip) == 0 && strcmp(my_node.vz_extr.tcp_port, my_node.vz_intr[position].tcp_port) == 0){
            new_fd = LeaveDetected(readfds, "EXTR", position);
        } else {
            FD_CLR(my_node.vz_intr[position].fd, &readfds);
            printf("INTR Node has left\n");
            missingINTR(position);
        }
    }
    

    return new_fd;
}



/********************************************************************
 * ShowTopology()
 * 
 * Escreve em stdin todas as informacoes relevantes de um dado no, 
 * nomeadamente:
 * - IP e TCP do proprio no
 * - IP e TCP do vizinho externo
 * - IP e TCP do vizinho de salvaguarda
 * - IP e TCP de todos os vizinhos internos que tiver
 ********************************************************************/
void ShowTopology() {

    printf("My Node:\n");
    printf("  IP: %s\n", my_node.myself.ip);
    printf("  TCP Port: %s\n", my_node.myself.tcp_port);
    
    printf("External Neighbor:\n");
    printf("  IP: %s\n", my_node.vz_extr.ip);
    printf("  TCP Port: %s\n", my_node.vz_extr.tcp_port);
    
    printf("Safe Neighbor:\n");
    printf("  IP: %s\n", my_node.vz_safe.ip);
    printf("  TCP Port: %s\n", my_node.vz_safe.tcp_port);
    
    printf("Internal Neighbors (%d):\n", my_node.n_intr);
    for (int i = 0; i < my_node.n_intr; i++) {
        printf("  [%d] IP: %s, TCP Port: %s\n", i + 1, my_node.vz_intr[i].ip, my_node.vz_intr[i].tcp_port);
    }

    return;
}


