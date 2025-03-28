/**********************************************************************************************
 * Filename:    CommonFunc.c
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


 
 /**
  * 
  */
 void notePrompt(){
    printf("\t-->");
    fflush(stdout);
}



/*****************************************************************************
 * initNode()
 * 
 * Iniciates the Node with default values:
 *  -> myself - ip tcp
 *  -> extr - myself
 *  -> safe - /0
 *  -> intr - /0
 *  -> n_intr = 0
 *  -> fd's = -1
 * 
 * Input: Ip and TCP Port of the created node 
 ******************************************************************************/
void initNode(char *ip, char *port, int flag) {

    /* inciamos o id connosco */
    strcpy(my_node.myself.ip, ip);
    strcpy(my_node.myself.tcp_port, port);
    my_node.myself.fd = -1;

    /* iniciamos o nosso vz_extr com o nosso id */
    strcpy(my_node.vz_extr.ip, ip);
    strcpy(my_node.vz_extr.tcp_port, port);
    my_node.vz_extr.fd = -1;

    strcpy(my_node.vz_safe.ip, "/0");
    strcpy(my_node.vz_safe.tcp_port, "/0");
    my_node.vz_safe.fd = -1;

    for(int i = 0; i <= MAX_INTR_SIZE; i++){
        strcpy(my_node.vz_intr[i].ip, "/0");
        strcpy(my_node.vz_intr[i].tcp_port, "/0");
        my_node.vz_intr[i].fd = -1;
    }
    
    my_node.fd_UDP = -1;
    my_node.n_intr = 0;

    
    for(int i = 0; i < my_node.CacheSize; i++){
        if(my_node.receivedObjs != NULL)
        memset(my_node.receivedObjs[i].objName, '\0', sizeof(my_node.receivedObjs[i].objName));
    }
    my_node.n_objs_inCache = 0;
    

    for(int i= 0; i < MAX_N_OBJECTS; i++){
        if(flag != 1)
            memset(my_node.inherentObjs[i].objName, '\0', sizeof(my_node.inherentObjs[i].objName));
        memset(my_node.intTab.objPending[i].objectName, '\0', sizeof(my_node.intTab.objPending[i].objectName));
        for(int j = 0; j < MAX_INTR_SIZE; j++){
            my_node.intTab.objPending[i].socketInfo[j].socket_id = -1;
            my_node.intTab.objPending[i].socketInfo[j].socketState = NUN;
        }
    }
    if(flag != 1)
        my_node.n_inherentObjs = 0;
    my_node.intTab.n_objects_iwant = 0;
    
    


}



/**********************************************************************
 * ReadFunction()
 * 
 * Funcao simples que le byte a byte da string enquanto nao encontra
 * o fim da linha. Tem o objetivo de ler apenas uma mensagem de cada 
 * vez e previne que um read leia mais que uma mensagem, pois todas 
 * as mensagens acabam em '\n'.
 * 
 * Inputs: 
 * -> fd - file descriptor de onde e suposto ler
 * -> buffer - buffer que passamos por referencia para guardar
 *             a mensagem que vamos ler
 *******************************************************************/
void ReadFunction(int fd, char *buffer){
    ssize_t n;
    char local = '\0';
    int char_counter = 0;

    while(local != '\n'){
        n = read(fd, &local, sizeof(local));
        if(n == -1){
            perror("Error in read in ReadFuncion - MESSAGE\n");
            exit(EXIT_FAILURE);
        }
        buffer[char_counter++] = local;
    }

}



/*********************************************************************
 * CloseAll()
 * 
 * Closes all the file descriptors opened.
 * 
 * File descriptors to be closed and reseted:
 * -> Listening socket fd
 * -> External Neighbour fd
 * -> All Internal Neighourbs fd's
 * -> UDP Socket for Server fd
 ******************************************************************/
void CloseAll(fd_set readfds){
    int flag = 0;
    if(my_node.vz_extr.fd != -1){
        FD_CLR(my_node.vz_extr.fd, &readfds);
        close(my_node.vz_extr.fd);
    }

    for(int i = 0; i < my_node.n_intr; i++){
        if(my_node.vz_intr[i].fd != -1){
            FD_CLR(my_node.vz_intr[i].fd, &readfds);
            close(my_node.vz_intr[i].fd);
        }
    }

    if(my_node.fd_UDP != -1){
        FD_CLR(my_node.fd_UDP, &readfds);
        close(my_node.fd_UDP);
    }
    if(my_node.myself.fd != -1){
        FD_CLR(my_node.myself.fd, &readfds);
        close(my_node.myself.fd);
    }

    // Salvar os objetos inherent
    flag = 1;
    initNode(my_node.myself.ip, my_node.myself.tcp_port, flag);

    if(my_node.receivedObjs != NULL){
        free(my_node.receivedObjs);
        my_node.receivedObjs = NULL;
    }
        
    

    return;
}