/**********************************************************************************************
 * Filename:    main.c
 * Description: main .c file with sequential code for a normal run.
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
#define DEFAULT_regIP "193.136.138.142"
#define DEFAULT_regUDP "59000"

myNode my_node;

/*  argc = Argument Count
    argv = Argument Vector */
int main (int argc, char *argv[]) {

    int cache_size = 0;
    char source_ip[16], source_port[6]; 
    memset(source_ip, 0, sizeof(source_ip));
    memset(source_port, 0, sizeof(source_port));
    char reg_ip[16] = DEFAULT_regIP;
    char reg_port[6] = DEFAULT_regUDP;

    /* Confirmation of correct usage of app invocation*/
    if(argc > 6){
        fprintf(stdout, "Too many arguments!\n");
        fprintf(stdout, "Aplication Invocation is made with the following structure:\n ./ndn cache IP TCP regIp regUDP\n");
        return 1;
    }
    if(argc < 4){
        fprintf(stdout, "At least the first 4 arguments are required to function.\n");
        fprintf(stdout, "Aplication Invocation is made with the following structure:\n ./ndn cache IP TCP regIp regUDP\n");
        return 1;
    }

    cache_size = atoi(argv[1]);
    if(strlen(argv[2]) > 15 || strlen(argv[3]) > 15) {
        fprintf(stdout, "IP com tamanho superior a tamanho máximo.\n");
        fprintf(stdout, "IP max: 255.255.255.255\n");
        return 1;
    }
    strcpy(source_ip, argv[2]);
    strcpy(source_port, argv[3]);


    if(argc > 4){ 
        if(argc < 6) /* 4 < argc < 6 => 5*/ { 
            strcpy(reg_ip, argv[4]);
        } else /* argc == 6 */ {
            strcpy(reg_ip, argv[4]);
            strcpy(reg_port, argv[5]);
        } 
    }

    fprintf(stdout, "You have chose as:\n");
    fprintf(stdout, "Cache size: %d\n", cache_size);
    fprintf(stdout, "Source App IP: %s\n", source_ip);
    fprintf(stdout, "Source TCP Port: %s\n", source_port);
    if(strcmp(reg_ip, DEFAULT_regIP) == 0)
        fprintf(stdout, "Server IP (default): %s\n", reg_ip);
    else
        fprintf(stdout, "Server IP: %s\n", reg_ip);
        
    if(strcmp(reg_port, DEFAULT_regUDP) == 0)
        fprintf(stdout, "Server UDP Port (default): %s\n", reg_port);
    else
        fprintf(stdout, "Server UDP Port: %s\n", reg_port);


    /* socket para escuta */
    //int my_fd = -1;
    /* socket geral de uso */
    int new_fd = -1;

    ssize_t n;
    //struct addrinfo hints, *res = NULL;
    struct sockaddr_in addrTCP, addrUDP;
    socklen_t addrlenTCP = sizeof(addrTCP);
    socklen_t addrlenUDP = sizeof(addrUDP);
    char buffer[128];

    /* Iniciar as informações dentro do no */
    my_node.CacheSize = cache_size;
    my_node.receivedObjs = malloc (my_node.CacheSize * sizeof(Obj));
    initNode(source_ip, source_port, 0);
    




    /* Variaveis para o selec */
    int nfds = 0, choice_maker = -1;
    fd_set readfds;
    fd_set mask;
    //fd_set writefds;
    //fd_set exceptfds;
    struct timeval timeout;

    /* Variaveis para comandos */
    char command[3];
    char arg1[16];
    char arg2[16];
    char messageType[10];
    char messageIP[16];
    char messagePort[6];
    char netID[4];
    int n_command_args = 0;
    State state = NORMAL;
    
    nfds = 0;
    FD_ZERO(&readfds); // Remove todos os fds de 'to read'
    FD_SET(0, &readfds); // Adiciona stdin a 'to read'
    /* Adiciona o fd do nosso socket a 'to read' que neste
    caso significara que vai monotorizar se algm quer se 
    connectar */

    /* Comeco do ciclo infinito */
    while(1) {
        FD_ZERO(&mask);
        FD_SET(0, &readfds); 
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        nfds = 0;

        if(state == UNREGISTERED){
            CloseAll(readfds);
            state = WAS_CLOSED;
            printf("Now you can exit (x) if you wish\n");
            continue;
        }

        if(my_node.myself.fd != -1){
            FD_SET(my_node.myself.fd, &readfds); 
            nfds = MAX(nfds, my_node.myself.fd);
        }

        if(my_node.vz_extr.fd != -1){
            FD_SET(my_node.vz_extr.fd, &readfds);
            nfds = MAX(nfds, my_node.vz_extr.fd);
        }


        for(int i  = 0; i < my_node.n_intr; i++){
            if(my_node.vz_intr[i].fd != -1){
                FD_SET(my_node.vz_intr[i].fd, &readfds);
                nfds = MAX(nfds, my_node.vz_intr[i].fd);
            }  
        }

        if(my_node.fd_UDP != -1){
            FD_SET(my_node.fd_UDP, &readfds);
            nfds = MAX(nfds, my_node.fd_UDP);
        }
        mask = readfds;

        // Verificacao de erros 
        for (int i = 0; i < nfds; i++) {
            if (FD_ISSET(i, &mask)) {
                //printf("Monitoring fd: %d\n", i);
                if (i < 0) {
                    fprintf(stderr, "Invalid file descriptor: %d\n", i);
                } else if (fcntl(i, F_GETFD) == -1) {
                    fprintf(stderr, "File descriptor %d is closed\n", i);
                }
            }
        }
        /*printf("\n");
        notePrompt();*/
        choice_maker = select(nfds+1, &mask, (fd_set *) NULL, (fd_set *) NULL, &timeout);
        if(choice_maker == -1){
            perror("Erro no retorno de select\n");
            fprintf(stderr, "errno = %d\n", errno);  // Check the error code
            exit(EXIT_FAILURE);
        } /*else if(choice_maker == 0){
            printf("\n-------- TIMEOUT --------\n");
        }*/

        // Se tiver algo em stdin para ler
        if(FD_ISSET(0, &mask) != 0){
            memset(buffer, 0, sizeof(buffer));
            memset(command, 0, sizeof(command));
            memset(arg1, 0, sizeof(arg1));
            memset(arg2, 0, sizeof(arg2));

            if((fgets(buffer, sizeof(buffer), stdin)) != NULL){
                n_command_args = sscanf(buffer, "%s %s %s", command, arg1, arg2);
                // conjunto de condicoes para cada commando
                if(n_command_args == 3){
                    // Chamada a direct join (dj) 
                    if(strcmp(command, "dj") == 0){
                        // Direct Join 
                        new_fd = DirectJoin(arg1, arg2);
                        state = NORMAL;
                    }else{
                        fprintf(stderr, "There is no such command\n");
                    }

                } else if(n_command_args == 2){ 
                    // Chamada a join (j)
                    if(strcmp(command, "j") == 0){
                        my_node.fd_UDP = Join(reg_ip, reg_port, arg1);
                        state = NORMAL;
                    }
                
                    /* Chamada a create (c) name */
                    if(strcmp(command, "c") == 0){
                        createObject(arg1);
                    }
                    /* Chamada a delete (dl) name */
                    if(strcmp(command, "d") == 0){
                        deleteObject(arg1);
                    }
                    /* Chamada a retrieve (r) name */
                    if(strcmp(command, "r") == 0){
                        retrieveObject(arg1);
                    }

                } else if(n_command_args == 1) {
                    // Chamada  a  show topology (st)
                    if(strcmp(command, "st") == 0){
                        ShowTopology(my_node);
                    }
                    /* Chamada a  show names (sn) */
                    if(strcmp(command, "sn") == 0)
                        showNames();
                    /* Chamada a show cache (sc) */
                    if(strcmp(command, "sc") == 0)
                        showCache();
                    /* Chamada a show interesttable (si) */
                    if(strcmp(command, "si") == 0)
                        showInterestTable();
                    // Chamada a leave (l)
                    if(strcmp(command, "l") == 0){
                        //TODO: Deixar c que eu n consiga dar dj ao no que foi fechado
                        if(my_node.fd_UDP != -1){
                            UNREG_MESSAGE_SEND(my_node.fd_UDP, netID, addrUDP, addrlenUDP);
                        } else {
                            state = UNREGISTERED;
                        }
                        
                    }

                    // Chamada a exit (x) 
                    if(strcmp(command, "x") == 0){
                        // Se n tiver ainda feito leave e tiver sozinho
                        if(my_node.myself.fd != -1){
                            
                            printf("You have not executed leave command, so it will be now executed before exiting\n");
                            // Leave command 
                            if(my_node.fd_UDP != -1)
                                UNREG_MESSAGE_SEND(my_node.fd_UDP, netID, addrUDP, addrlenUDP);
                            else 
                                state = UNREGISTERED;

                        } else {
                            initNode(my_node.myself.ip, my_node.myself.tcp_port, 0);              
                            exit(EXIT_SUCCESS);
                        }
                        
                    }
                } else {
                    perror("Zero arguments");
                    exit(EXIT_FAILURE);
                }
            }

        }


        // Se tiver alguem a tentar conectar-se 
        if(FD_ISSET(my_node.myself.fd, &mask)!=0){
            if(state == WAS_CLOSED){
                fprintf(stderr, "-------- ERROR -------\n");
                fprintf(stderr, "This node isnt registered in a network so you can't connect to it\n");
            } else {
                //testUDP(addr, addrlen);

                // Accept any node 
                if((new_fd = accept(my_node.myself.fd, (struct sockaddr*) &addrTCP, &addrlenTCP)) == -1){
                    perror("Error in accept\n");
                    exit(EXIT_FAILURE);
                }
                
                // O n_intr e aumentado no update da struct
                my_node.vz_intr[my_node.n_intr].fd = new_fd;
                
                
                memset(buffer, 0, sizeof(buffer));
                memset(messageType, 0, sizeof(messageType));
                memset(messageIP, 0, sizeof(messageIP));
                memset(messagePort, 0, sizeof(messagePort));

                ReadFunction(my_node.vz_intr[my_node.n_intr].fd, buffer);

                sscanf(buffer, "%s %s %s", messageType, messageIP, messagePort); // Needed to update it after the vz_extr
                if(strcmp(messageType, "ENTRY") != 0)
                    fprintf(stderr, "This should not happen\n");

                // Da update dos vizinhos internos 
                updateNodeStruct(state, buffer);
                write(1, "received: ", 10);
                write(1, buffer, strlen(buffer));

                /* Se não tivermos vzextr, ou seja, tem vzextr = a si proprio ent enviamos entry ao vzintr */
                if(strcmp(my_node.vz_extr.ip, my_node.myself.ip) == 0 && strcmp(my_node.vz_extr.tcp_port, my_node.myself.tcp_port) == 0){
                    // Send ENTRY Message 
                    /* Atualizar vz_extr para o vz_intr q acabou de fazer */
                    ENTRY_MESSAGE_SEND(new_fd, messageIP, messagePort);

                    /* Envia ainda a mensagem de SAFE */
                    SAFE_MESSAGE_SEND(new_fd);

                } else {
                    // Procedimento normal, escreve safe e ta feito
                    SAFE_MESSAGE_SEND(new_fd);
                }
            }

        }

        /* Se eu tiver feito dj e enviado uma entry este sera
        o socket que recebera a msg SAFE */
        if(my_node.vz_extr.fd != -1){
            if(FD_ISSET(my_node.vz_extr.fd, &mask)!= 0){

                // Verifica se o socket do vz extr foi fechado
                if(recv(my_node.vz_extr.fd, buffer, sizeof(buffer), MSG_PEEK) == 0){
                    printf("EXTR Node has left\n");
                    new_fd = LeaveDetected(readfds, "EXTR", -1);
                    continue;
                } else if(recv(my_node.vz_extr.fd, buffer, sizeof(buffer), MSG_PEEK) == -1 && errno == ECONNRESET){
                    perror("Neighbour socket was closed abruptly\n");
                    new_fd = LeaveDetected(readfds, "EXTR", -1);
                    continue;
                }

                memset(buffer, 0, sizeof(buffer));
                memset(messageType, 0, sizeof(messageType));
                memset(messageIP, 0, sizeof(messageIP));
                memset(messagePort, 0, sizeof(messagePort));

                ReadFunction(my_node.vz_extr.fd, buffer);
                //fprintf(stdout, "delulu %s\n", buffer);
                sscanf(buffer, "%s %s %s", messageType, messageIP, messagePort);
                
                if(strcmp(messageType, "SAFE") == 0){
                    updateNodeStruct(state, buffer);
                    write(1, "received: ", 10);
                    write(1, buffer, strlen(buffer));
                } else if(strcmp(messageType, "ENTRY") == 0) {
                    updateNodeStruct(state, buffer);
                    write(1, "received: ", 10);
                    write(1, buffer, strlen(buffer));

                    SAFE_MESSAGE_SEND(my_node.vz_extr.fd);
                } else if(strcmp(messageType, "INTEREST") == 0) {
                    write(1, "received: ", 10);
                    write(1, buffer, strlen(buffer));
                    processINTEREST_received(my_node.vz_extr.fd, messageIP);
                } else if(strcmp(messageType, "OBJECT") == 0) {
                    write(1, "received: ", 10);
                    write(1, buffer, strlen(buffer));
                    processOBJECT_received(messageIP);
                } else if(strcmp(messageType, "NOOBJECT") == 0) {
                    write(1, "received: ", 10);
                    write(1, buffer, strlen(buffer));
                    processNOOBJECT_received(my_node.vz_extr.fd, messageIP);
                } else {
                    perror("Error, neither safe or entry message was received\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        


        // Se receber alguma mensagem dos meus filhos
        int flag = 0;
        for(int i = 0; i < my_node.n_intr; i++){
            if(my_node.vz_intr[i].fd != -1){
                if(FD_ISSET(my_node.vz_intr[i].fd, &mask)!=0){

                    // Verifica se algum socket dos filhos foi fechado
                    if(recv(my_node.vz_intr[i].fd, buffer, sizeof(buffer), MSG_PEEK) == 0){
                        new_fd = LeaveDetected(readfds, "INTR", i);
                        flag = 1;
                        break;
                        //continue;
                    } else if(recv(my_node.vz_intr[i].fd, buffer, sizeof(buffer), MSG_PEEK) == -1 && errno == ECONNRESET){
                        perror("Neighbour INTR socket was closed abruptly\n");
                        new_fd = LeaveDetected(readfds, "INTR", i);
                        flag = 1;
                        break;
                        //continue;
                    }

                    /* Aguarda mensagem */
                    memset(buffer, 0, sizeof(buffer));
                    memset(messageType, 0, sizeof(messageType));
                    memset(messageIP, 0, sizeof(messageIP));
                    memset(messagePort, 0, sizeof(messagePort));
                    
                    ReadFunction(my_node.vz_intr[i].fd, buffer);
                    sscanf(buffer, "%s %s %s", messageType, messageIP, messagePort);

                    if(strcmp(messageType, "SAFE") == 0){
                        // Da update dos ao no de salvaguarda 
                        updateNodeStruct(state, buffer);
                        write(1, "received: ", 10);
                        write(1, buffer, strlen(buffer));
                    }
                    if(strcmp(messageType, "INTEREST") == 0){
                        write(1, "received: ", 10);
                        write(1, buffer, strlen(buffer));
                        processINTEREST_received(my_node.vz_intr[i].fd, messageIP);
                    } else if(strcmp(messageType, "OBJECT") == 0){
                        write(1, "received: ", 10);
                        write(1, buffer, strlen(buffer));
                        processOBJECT_received(messageIP);
                    } else if(strcmp(messageType, "NOOBJECT") == 0){
                        write(1, "received: ", 10);
                        write(1, buffer, strlen(buffer));
                        processNOOBJECT_received(my_node.vz_intr[i].fd, messageIP);
                    }
                    
                }
            }
        }
        if(flag == 1){
            continue;
        }

        // se receber algo do servidor
        if(my_node.fd_UDP != -1){
            if(FD_ISSET(my_node.fd_UDP, &mask)!=0){
                char buffer_list[MAX_BUFFER_SIZE];
                /* Esperamos agora receber a lista de nós registados no servidor */
                addrlenTCP = sizeof(addrTCP);
                memset(buffer_list, 0, sizeof(buffer_list));
                memset(messageType, 0, sizeof(messageType));
                memset(messagePort, 0, sizeof(messagePort));

                n = recvfrom(my_node.fd_UDP, buffer_list, sizeof(buffer_list), 0, (struct sockaddr*)&addrUDP, &addrlenUDP);
                if(n == -1){
                    perror("Error Sending UDP Message\n");
                    exit(EXIT_FAILURE);
                }
                sscanf(buffer_list, "%s %s", messageType, messagePort);

                /*printf("  ---Mensagem UDP recebida--- \n");
                write(1, buffer_list, strlen(buffer_list));
                printf("------------------------- \n");*/



                if(strcmp(messageType, "NODESLIST") == 0){

                    strcpy(netID, messagePort);
                    node_identifier nodes[MAX_N_NODES_IN_LIST];
                    for(int i = 0; i < MAX_N_NODES_IN_LIST; i++){
                        strcpy(nodes[i].ip, "0");
                        strcpy(nodes[i].tcp_port, "0");
                    }
                    int n_nodes = 0;

                    processNodeList(buffer_list, &n_nodes, nodes);
                    if(n_nodes == 0){
                        fprintf(stdout, "No nodes present on the list\n");
                        state = ALONE;
                    }
                    // Print 
                    for (int i = 0; i < n_nodes; i++) {
                        printf("Node %d: IP=%s, TCP=%s\n", i + 1, nodes[i].ip, nodes[i].tcp_port);
                    }

                    if(state == ALONE){
                        DirectJoin("0.0.0.0", "0");
                        REG_MESSAGE_SEND(my_node.fd_UDP, netID, addrUDP, addrlenUDP);
                        state = NORMAL;
                    
                    } else { // Junta se a um no aleatorio 
                        int random = rand() % n_nodes;
                        int temp = random;
                        printf("Will dj to node %s %s\n", nodes[temp].ip, nodes[temp].tcp_port);
                        new_fd = DirectJoin(nodes[random].ip, nodes[random].tcp_port);

                        // Regist
                        REG_MESSAGE_SEND(my_node.fd_UDP, netID, addrUDP, addrlenUDP);
                
                    }

                } else if(strcmp(messageType, "OKREG")==0){

                    /* Receives OKREG */
                    write(1, "received: ", 10);
                    write(1, messageType, strlen(messageType));
                    write(1, "\n", 1);
            
            
                    if(strcmp(messageType, "OKREG") == 0){
                        printf("Node successfully registered\n");
                    } else {
                        perror("Node not registered sucessfully\n");
                        exit(EXIT_FAILURE);
                    }

                } else if(strcmp(messageType, "OKUNREG") == 0) {
                    /* Receives OKUNREG */
                    write(1, "received: ", 10);
                    write(1, messageType, strlen(messageType));
                    write(1, "\n", 1);
            
            
                    if(strcmp(messageType, "OKUNREG") == 0){
                        printf("Node successfully unregistered\n");
                        state = UNREGISTERED;
                    } else { 
                        perror("Node not unregistered sucessfully\n");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    fprintf(stderr, "----------------- ERROR ---------------\n");
                    fprintf(stderr, "Message unknown\n");
                    exit(EXIT_FAILURE);
                }

                
            }
        }
    }

    fprintf(stderr, "------------ ERROR -----------\n");
    fprintf(stderr, "Not suppose to exit from here\n");
    exit(EXIT_FAILURE);
}