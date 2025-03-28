/**********************************************************************************************
 * Filename:    ndn_objects.c
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
void createObject(char *name){
    int position = my_node.n_inherentObjs;

    strcpy(my_node.inherentObjs[position].objName, name);
    my_node.n_inherentObjs++;

    printf("Object %s successfully added to node, in position %d\n", name, position);
}



/**
 * 
 */
void deleteObject(char *name){
    int flag = 0;
    //char *temp;

    // Delete from inherent object vector 
    for(int i = 0; i < my_node.n_inherentObjs; i++){
        if(strcmp(my_node.inherentObjs[i].objName, name) == 0){
            int j = i;
            // Reordena primeiro e depois apaga
            while(j+1 < my_node.n_inherentObjs){
                strcpy(my_node.inherentObjs[j].objName, my_node.inherentObjs[j+1].objName);
            }
            memset(my_node.inherentObjs[j].objName, '\0', sizeof(my_node.inherentObjs[j].objName));
            my_node.n_inherentObjs--;
            flag = 1;
            printf("Object %s deleted from node inherent objects list.\n", name);
        }
    }
    if(flag == 0){
        printf("Object not found in node inherent objects list.\n");
        
    } 
    flag = 0;
    // Delete from cache object vector 
    for(int i = 0; i < my_node.n_objs_inCache; i++){
        if(strcmp(my_node.receivedObjs[i].objName, name) == 0){
            int j = i;
            // Reordena primeiro e depois apaga
            while(j+1 < my_node.n_objs_inCache){
                strcpy(my_node.receivedObjs[j].objName, my_node.receivedObjs[j+1].objName);
                j++;
            }
            memset(my_node.receivedObjs[j].objName, '\0', sizeof(my_node.receivedObjs[j].objName));
            my_node.n_objs_inCache--;
            flag = 1;
            printf("Object %s deleted from cache.\n", name);
        }
    }
    if(flag == 0){
        printf("Object not found in node cache.\n");
        
    } 
    flag = 0;

    return;
}



/**
 * 
 */
void processINTEREST_received(int fd, char *targetName){
    int flag = -1;
    int position_inherentObj, position_intTabObj, position_cacheObj;

    // Verifica se o objecto esta no no
    for(int i = 0; i < my_node.n_inherentObjs; i++){
        if(strcmp(my_node.inherentObjs[i].objName, targetName) == 0){
            OBJECT_SEND_MESSAGE(fd, targetName);
            flag = i;
            break;
        }
    }
    position_inherentObj = flag;

    flag = -1;
    // Verifica se o objecto esta na cache do no
    for(int i = 0; i < my_node.n_objs_inCache; i++){
        if(strcmp(my_node.receivedObjs[i].objName, targetName) == 0){
            OBJECT_SEND_MESSAGE(fd, targetName);
            flag = i;
            break;
        }
    }
    position_cacheObj = flag;
    
    // Object nao esta no no
    if(position_inherentObj == -1 && position_cacheObj == -1){
        // Verifica se esta na tabela de interesses
        for(int i = 0; i < my_node.intTab.n_objects_iwant; i++){
            if(strcmp(my_node.intTab.objPending[i].objectName, targetName) == 0){
                flag = i;
                break;
            }
        }
        position_intTabObj = flag;
        flag = -1;
        // O objeto nao esta na tabela de interesses
        if(position_intTabObj == -1){
            // Verifica se esta sozinho 
            if(my_node.vz_extr.fd != fd && my_node.vz_extr.fd != -1){
                flag = fd;
            }
            for(int i = 0; i < my_node.n_intr; i++){
                if(my_node.vz_intr[i].fd != fd && my_node.vz_intr[i].fd != -1){
                    flag = i;
                }   
            }

            // No nao tem mais conexoes
            if(flag == -1){
                NOOBJECT_SEND_MESSAGE(fd, targetName);
            } else {
                /* Create entrada na tabela */
                int obj = my_node.intTab.n_objects_iwant;
                int interface = 0;

                strcpy(my_node.intTab.objPending[obj].objectName, targetName);

                // Encontra o primeiro socket que possa ser usado
                while(my_node.intTab.objPending[obj].socketInfo[interface].socket_id != -1 || interface == MAX_INTR_SIZE){
                    interface++;
                }
                if(interface == MAX_INTR_SIZE){
                    fprintf(stderr, "Limite máximo de vizinhos alcançado");
                    exit(EXIT_FAILURE);
                }
                my_node.intTab.objPending[obj].socketInfo[interface].socket_id = fd;
                my_node.intTab.objPending[obj].socketInfo[interface].socketState = ANSWER;
                flag = -1;
                for(int i = 0; i < my_node.n_intr; i++){
                    if(my_node.vz_intr[i].fd != -1){
                        if(my_node.vz_intr[i].fd != fd){
                            INTEREST_SEND_MESSAGE(my_node.vz_intr[i].fd, targetName);
                        }
                        if(my_node.vz_extr.fd == my_node.vz_intr[i].fd || my_node.vz_extr.fd == fd){
                            flag = i;
                        } 
                    }
                }
                // se nao for igual a nenhum vizinho interno nem a fd
                if(flag == -1 && my_node.vz_extr.fd != -1){
                    INTEREST_SEND_MESSAGE(my_node.vz_extr.fd, targetName);
                }
                my_node.intTab.n_objects_iwant++;
            }
        } else { // Objeto nao esta no no mas esta na tabela
            int obj = 0;
            int interface = 0;
            for(int i = 0; i < MAX_N_OBJECTS; i++){
                if(strcmp(my_node.intTab.objPending[i].objectName, targetName) == 0){
                    obj = i;
                    break;
                }
            }

            while(my_node.intTab.objPending[obj].socketInfo[interface].socket_id != fd){
                interface++;
            }
            my_node.intTab.objPending[obj].socketInfo[interface].socketState = ANSWER;

            interface = 0;
            flag = -1;
            while(my_node.intTab.objPending[obj].socketInfo[interface].socket_id != -1 || interface == MAX_INTR_SIZE){
                if(my_node.intTab.objPending[obj].socketInfo[interface].socketState == WAITING){
                    flag++;
                }
                interface++;
            }
            if(interface == MAX_INTR_SIZE){
                fprintf(stderr, "Limite máximo de vizinhos alcançado");
                exit(EXIT_FAILURE);
            }
            if(flag == -1 && fd != -1){
                NOOBJECT_SEND_MESSAGE(fd, targetName);
            }
        }
    }
    showInterestTable();
    return;
}



/***
 * 
 */
void retrieveObject(char *targetName){
    int flag = -1;
    int obj = my_node.intTab.n_objects_iwant;
    
    // verifica se o objeto ja se encontra no objeto
    for(int i = 0; i < my_node.n_inherentObjs; i++){
        if(strcmp(my_node.inherentObjs[i].objName, targetName) == 0){
            flag = i;
            printf("Object %s is in local memory of the node, no need to retrieve it\n", targetName);
            printf("You can use the command \"sn\" to see what objects are in the local memory.\n");
            return;
        }
    }
    // se nao encontrar na memoria local, vai ainda procurar a cache
    if(flag == -1){
        for(int i = 0; i < my_node.n_objs_inCache; i++){
            if(strcmp(my_node.receivedObjs[i].objName, targetName) == 0){
                flag = i;
                printf("Object %s is inside of cache of the node, no need to retrieve it\n", targetName);
                printf("You can use the command \"sc\" to see what objects are in the cache.\n");
                return;
            }
        }
    }

    // So se nao encontrar o objeto e que ele faz retrieve
    strcpy(my_node.intTab.objPending[obj].objectName, targetName);
    

    for(int i = 0; i < my_node.n_intr; i++){
        if(my_node.vz_intr[i].fd != -1){
            INTEREST_SEND_MESSAGE(my_node.vz_intr[i].fd, targetName);
        }
        if(my_node.vz_extr.fd == my_node.vz_intr[i].fd && my_node.vz_extr.fd != -1){
            flag = i;
        } 
    }
    // Vizinho externo não é igual a nenhum filho
    if(flag == -1 && my_node.vz_extr.fd != -1){
        INTEREST_SEND_MESSAGE(my_node.vz_extr.fd, targetName);
    }
    my_node.intTab.n_objects_iwant++;
    showInterestTable();
}



/***
 * 
 */
void processOBJECT_received(char *name){

    int interface = 0;
    int obj_position = -1;

    // Encontra objeto na lista
    for(int i = 0; i < my_node.intTab.n_objects_iwant; i++){
        if(strcmp(my_node.intTab.objPending[i].objectName, name) == 0){
            obj_position = i;
        }
    }

    // Para as sockets em estado de resposta envia mensagem de objeto
    while(my_node.intTab.objPending[obj_position].socketInfo[interface].socket_id != -1){
        if(my_node.intTab.objPending[obj_position].socketInfo[interface].socketState == ANSWER){
            OBJECT_SEND_MESSAGE(my_node.intTab.objPending[obj_position].socketInfo[interface].socket_id, name);
        }
        interface++;
    }
    // Como mandamos mensagem de objeto, ja n precisamos de tabela de interesses
    deleteEntry(obj_position);

    // if cache is full
    if(my_node.n_objs_inCache + 1 > my_node.CacheSize){
        accessCache(name);
    } else {
        strcpy(my_node.receivedObjs[my_node.n_objs_inCache].objName, name);
        my_node.n_objs_inCache++;
        accessCache(name);
    }
    showInterestTable();
    
}



/**
 * 
 */
void processNOOBJECT_received(int fd, char *name){

    int obj_position = -1;
    int interface = 0;
    int flag = -1;

    // Encontra o objeto na lista
    for(int i = 0; i < my_node.intTab.n_objects_iwant; i++){
        if(strcmp(my_node.intTab.objPending[i].objectName, name) == 0){
            obj_position = i;
        }
    }
    // Encontra o socket na lista
    for(interface = 0; interface < MAX_INTR_SIZE; interface++){
        if(my_node.intTab.objPending[obj_position].socketInfo[interface].socket_id == fd){
            break;
        }
    }
    if(interface == MAX_INTR_SIZE){
        printf("Como já recebi objeto, não faço nada com NOOBJECT\n");
        return;
    }
    my_node.intTab.objPending[obj_position].socketInfo[interface].socketState = CLOSED;

    // Verifica se ainda ha sockets em espera
    interface = 0;
    while(my_node.intTab.objPending[obj_position].socketInfo[interface].socket_id != -1){
        if(my_node.intTab.objPending[obj_position].socketInfo[interface].socketState == WAITING){
            flag = interface;
            printf("There is stil interface %d in WAITING state\n", my_node.intTab.objPending[obj_position].socketInfo[flag].socket_id);
        }
        interface++;
    }

    // se nao houver sockets em espera
    if(flag == -1){
        printf("There are no more interfaces WAITING\n");
        interface = 0;
        while(my_node.intTab.objPending[obj_position].socketInfo[interface].socket_id != -1){
            if(my_node.intTab.objPending[obj_position].socketInfo[interface].socketState == ANSWER){
                NOOBJECT_SEND_MESSAGE(my_node.intTab.objPending[obj_position].socketInfo[interface].socket_id, name);
            }
            interface++;
        }
        deleteEntry(obj_position);

    }
    showInterestTable();

    return;
}



/***
 * 
 */
void deleteEntry(int obj_position){
    int n_objs = my_node.intTab.n_objects_iwant;
    int counter = obj_position;
    int flag = -1;

    while(counter + 1 < n_objs){
        flag = 0;
        strcpy(my_node.intTab.objPending[counter].objectName, my_node.intTab.objPending[counter+1].objectName);
        while(my_node.intTab.objPending[counter].socketInfo[flag+1].socket_id != -1){
            my_node.intTab.objPending[counter].socketInfo[flag].socketState = my_node.intTab.objPending[counter].socketInfo[flag+1].socketState;
            my_node.intTab.objPending[counter].socketInfo[flag].socket_id = my_node.intTab.objPending[counter].socketInfo[flag+1].socket_id;
            flag++;
        }
        counter++;
    }
    // Object was the only one in the table
    memset(my_node.intTab.objPending[counter].objectName, '\0', sizeof(my_node.intTab.objPending[counter].objectName));
    flag = 0;
    while(my_node.intTab.objPending[counter].socketInfo[flag].socket_id != -1){
        my_node.intTab.objPending[counter].socketInfo[flag].socketState = NUN;
        my_node.intTab.objPending[counter].socketInfo[flag].socket_id = -1;
        flag++;
    }
    
    my_node.intTab.n_objects_iwant--;

    return;
}



/**
 * 
 */
void accessCache(const char *object) {
    int i, pos = -1;

    // Check if the object is already in cache
    for (i = 0; i < my_node.CacheSize; i++) {
        if (strcmp(my_node.receivedObjs[i].objName, object) == 0) {
            pos = i;  // Found object position
            break;
        }
    }

    if (pos == -1) {  // Object NOT found → Evict if cache is full
        // Find the first empty slot (if available)
        for (i = 0; i < my_node.CacheSize; i++) {
            if (my_node.receivedObjs[i].objName[0] == '\0') {
                break; // Found an empty slot, insert here
            }
        }

        if (i == my_node.CacheSize) {  // Cache is FULL, evict LRU (first element)
            // Shift all elements to the left (remove LRU)
            for (i = 1; i < my_node.CacheSize; i++) {
                strcpy(my_node.receivedObjs[i - 1].objName, my_node.receivedObjs[i].objName);
            }
            i--; // Adjust `i` to last position
        }

        // Insert new object at the last position (Most Recently Used)
        strncpy(my_node.receivedObjs[i].objName, object, 100);
        my_node.receivedObjs[i].objName[100] = '\0';  // Ensure null-termination

    } else {  // Object FOUND → Move it to the end (Most Recently Used)
        Obj temp;
        strcpy(temp.objName, my_node.receivedObjs[pos].objName);

        for (i = pos; i < my_node.n_objs_inCache - 1; i++) {
            strcpy(my_node.receivedObjs[i].objName, my_node.receivedObjs[i + 1].objName);
        }

        // Move accessed object to the end if there is more than one 
        if(i != pos)
            strcpy(my_node.receivedObjs[my_node.n_objs_inCache - 1].objName, temp.objName);
    }

    return;
}




 /**
 * 
 */
void INTEREST_SEND_MESSAGE(int fd, char *name){
    ssize_t n;
    int interface = 0;
    int obj = my_node.intTab.n_objects_iwant; 

    // encontra o primeiro socket que nao estiver ocupado
    while(my_node.intTab.objPending[obj].socketInfo[interface].socket_id != -1 || interface == MAX_INTR_SIZE){
        interface++;
    }
    if(interface == MAX_INTR_SIZE){
        fprintf(stderr, "Limite máximo de vizinhos alcançado");
        exit(EXIT_FAILURE);
    }
    my_node.intTab.objPending[obj].socketInfo[interface].socket_id = fd;
    my_node.intTab.objPending[obj].socketInfo[interface].socketState = WAITING;
    

    char buffer[64];
    memset(buffer, 0, sizeof(buffer));

    if (fcntl(fd, F_GETFD) == -1) {
        perror("Socket is invalid");
    }

    snprintf(buffer, sizeof(buffer), "INTEREST %s\n", name);
    n = write(fd, buffer, strlen(buffer));
    if(n == -1){
        perror("Error in write - INTEREST");
        exit(EXIT_FAILURE);
    }
    write(1, "echo: ", 6);
    write(1, buffer, n);

    return;

}



/**
 * 
 */
void OBJECT_SEND_MESSAGE(int fd, char *name){
    ssize_t n;

    char buffer[64];
    memset(buffer, 0, sizeof(buffer));

    if (fcntl(fd, F_GETFD) == -1) {
        perror("Socket is invalid");
    }

    snprintf(buffer, sizeof(buffer), "OBJECT %s\n", name);
    n = write(fd, buffer, strlen(buffer));
    if(n == -1){
        perror("Error in write - OBJECT");
        exit(EXIT_FAILURE);
    }
    write(1, "echo: ", 6);
    write(1, buffer, n);

    return;

}



/**
 * 
 */
void NOOBJECT_SEND_MESSAGE(int fd, char *name){
    ssize_t n;

    char buffer[64];
    memset(buffer, 0, sizeof(buffer));

    if (fcntl(fd, F_GETFD) == -1) {
        perror("Socket is invalid");
    }

    snprintf(buffer, sizeof(buffer), "NOOBJECT %s\n", name);
    n = write(fd, buffer, strlen(buffer));
    if(n == -1){
        perror("Error in write - NOOBJECT");
        exit(EXIT_FAILURE);
    }
    write(1, "echo: ", 6);
    write(1, buffer, n);

    return;

}



/**
 * 
 */
void showInterestTable(){
    char *closed = "fechado";
    char *waiting = "espera";
    char *answer = "resposta";
    char *state = NULL;


    printf("Lista de pedidos pendentes:\n");
    if(my_node.intTab.n_objects_iwant == 0){
        printf("No objects requested\n");
    } else {
        for(int i = 0; i < my_node.intTab.n_objects_iwant; i++){
            int j = 0;
            printf("%s ", my_node.intTab.objPending[i].objectName);
            while(my_node.intTab.objPending[i].socketInfo[j].socket_id != -1){
                if(my_node.intTab.objPending[i].socketInfo[j].socketState == CLOSED){
                    state = closed; 
                } else if(my_node.intTab.objPending[i].socketInfo[j].socketState == WAITING){
                    state = waiting;
                } else if(my_node.intTab.objPending[i].socketInfo[j].socketState == ANSWER){
                    state = answer;
                }
                printf("%d:%s ", my_node.intTab.objPending[i].socketInfo[j].socket_id, state);
                j++;
            }
            printf("\n");
        }
    }
}



/**
 * 
 */
void showNames(){
    printf("------------------ Objects in node ------------------\n");
    for(int i = 0; i < my_node.n_inherentObjs; i++){
        printf("Obj[%d]: %s\n", i, my_node.inherentObjs[i].objName);
    }
    printf("-----------------------------------------------------\n");
}



/**
 * 
 */
void showCache(){
    printf("------------------ Objects in cache ------------------\n");
    for(int i = 0; i < my_node.n_objs_inCache; i++){
        printf("Obj[%d]: %s\n", i, my_node.receivedObjs[i].objName);
    }
    printf("-----------------------------------------------------\n");
}