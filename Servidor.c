#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h> 

// definimos nuestro puerto
#define PORT 8080
//el tamaño máximo del buffer que utilizaremos para leer y escribir datos en el socket.
#define MAXBUF 1024
//Definir el numero maximo de preguntas
#define MAX_QUESTIONS 30

// Variables para los exámenes y puntajes
int puntaje_espanol = 0;
int puntaje_matematicas = 0;
int puntaje_computacion = 0;
int puntaje_psicometrico =0;

int has_passed_spanish = 0;
int has_passed_math = 0;
int has_passed_computer = 0;


// Estructura para representar una pregunta
typedef struct {
    char pregunta[256];
    char opciones[3][64];
    int opcion_correcta;
} Pregunta;

// Variables para los exámenes
Pregunta preguntas_espanol[MAX_QUESTIONS];
Pregunta preguntas_matematicas[MAX_QUESTIONS];
Pregunta preguntas_computacion[MAX_QUESTIONS];
Pregunta preguntas_psicometrico[MAX_QUESTIONS];

// FUNCION SERVIDOR-CLIENTE 
void handle_client(int client_socket);
// FUNCION PARA CARGAR LAS PREGUNTAS DESDE UN ARCHIVO 
void load_questions(Pregunta preguntas[], char *filename, const char *categoria);
// FUNCION PARA ENVIAR LAS PRGUNTAS RANDOMS AL CLIENTE 
void send_questions_one_by_one(int client_socket, Pregunta preguntas[], int num_preguntas, int *puntaje, const char *categoria);
// FUNCION PARA EXALUAR CADA EXAMEN
int evaluate_exam(int score);
// FUNCION PARA GENERAR UNA CODIGO
char *generate_random_key() ;
// FUNCION PARA REINICIAR LAS CALIFICACIONES
void reiniciar_puntajes() ;
//FUNCION PARA PASAR AL EXAMEN DE PSICOMETRICO
void ExamenPsicometrico(int client_socket, Pregunta preguntas_psicometrico[]);
//FUNCION PARA REINICAR LOS PUNTAJES
void reiniciar_puntajes() ;



void reiniciar_puntajes(){
    puntaje_espanol = 0;
    puntaje_matematicas = 0;
    puntaje_computacion = 0;
    puntaje_psicometrico = 0;
}

void send_questions_one_by_one(int client_socket, Pregunta preguntas[], int num_preguntas, int *puntaje, const char *categoria) {
    char buffer[MAXBUF];
    int indices[num_preguntas];
    int respuestas_correctas[num_preguntas];

    // Reiniciar los puntajes
    reiniciar_puntajes();

    // Envío del mensaje de bienvenida
    snprintf(buffer, sizeof(buffer), "¡Bienvenido al examen de %s!\n", categoria);
    send(client_socket, buffer, strlen(buffer), 0);

    // Inicializar el arreglo de índices
    for (int i = 0; i < num_preguntas; ++i) {
        indices[i] = i;
    }

    // Aleatorizar el arreglo de índices
    srand(time(NULL)); // Semilla para números aleatorios basada en el tiempo actual
    for (int i = num_preguntas - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }

    // Enviar solo las primeras 10 preguntas aleatorizadas
    for (int i = 0; i < 10; i++) {
        int index = indices[i];

        // Construir el mensaje de la pregunta
        snprintf(buffer, sizeof(buffer), "Pregunta %d: %s\n1. %s\n2. %s\n3. %s\n",
                 i + 1, preguntas[index].pregunta,
                 preguntas[index].opciones[0],
                 preguntas[index].opciones[1],
                 preguntas[index].opciones[2]);

        send(client_socket, buffer, strlen(buffer), 0);

        // Esperar la respuesta del cliente
        memset(buffer, 0, MAXBUF);
        ssize_t bytes_received = recv(client_socket, buffer, MAXBUF, 0);

        if (bytes_received <= 0) {
            perror("Error al recibir respuesta del cliente");
            return;
        }

        // Verificar la respuesta del cliente y enviar retroalimentación
        int respuesta_cliente;
        sscanf(buffer, "%d", &respuesta_cliente);


        // Dentro de la función send_questions_one_by_one

        // Verificar la respuesta del cliente y enviar retroalimentación
        if (respuesta_cliente == 1 || respuesta_cliente == 2 || respuesta_cliente == 3) {
            if (respuesta_cliente == preguntas[index].opcion_correcta) {
                send(client_socket, "RESPUESTA CORRECTA\n", strlen("Respuesta correcta\n"), 0);
                (*puntaje)++;
            } else {
                send(client_socket, "RESPUESTA INCORRECTA\n", strlen("Respuesta incorrecta\n"), 0);
            }
        } else {
            send(client_socket, "Respuesta no válida. Por favor ingrese 1, 2 o 3.\n", strlen("Respuesta no válida. Por favor ingrese 1, 2 o 3.\n"), 0);
            i --; // Para que la pregunta se repita ya que la respuesta fue inválida 
        }

           // Enviar el puntaje final al cliente
            char mensaje_puntaje[50];
            snprintf(mensaje_puntaje, sizeof(mensaje_puntaje), "PUNTAJE ACTUAL: %d\n", *puntaje);
            send(client_socket, mensaje_puntaje, strlen(mensaje_puntaje), 0);
    }
    // Enviar el puntaje final al cliente
    char mensaje_puntaje[50];
    snprintf(mensaje_puntaje, sizeof(mensaje_puntaje), "\nPUNTAJE FINAL: %d\n\n", *puntaje);
    send(client_socket, mensaje_puntaje, strlen(mensaje_puntaje), 0);

}

// Función para cargar las preguntas desde un archivo
void load_questions(Pregunta preguntas[], char *filename, const char *categoria) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error al abrir el archivo de preguntas");
        exit(EXIT_FAILURE);
    }

    printf("Cargando preguntas para la categoría: %s\n", categoria);

    for (int i = 0; i < MAX_QUESTIONS; i++) {
        if (feof(file)) {
            break;  // Fin del archivo
        }
        // Leer la pregunta
        fgets(preguntas[i].pregunta, sizeof(preguntas[i].pregunta), file);
        preguntas[i].pregunta[strcspn(preguntas[i].pregunta, "\n")] = '\0';

        // Leer opciones de respuesta
        for (int j = 0; j < 3; j++) {
            fgets(preguntas[i].opciones[j], sizeof(preguntas[i].opciones[j]), file);
            preguntas[i].opciones[j][strcspn(preguntas[i].opciones[j], "\n")] = '\0';
        }
        // Leer la opción correcta
        fscanf(file, "%d\n", &preguntas[i].opcion_correcta);
    }
    fclose(file);
}

int evaluate_exam(int score) {
    if (score >= 6) { //Piuntaje mínimo para aprobar
        return 1; // Devuelve 1 si el puntaje es suficiente para pasar
    } else {
        return 0; // Devuelve 0 si el puntaje no es suficiente
    }
}

// Generar clave aleatoria de 5 caracteres alfanuméricos
char *generate_random_key() {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    char *key = malloc(7 * sizeof(char)); // Reserva espacio para 6 caracteres + 1 para el terminador nulo
    if (key == NULL) {
        perror("Error en la asignación de memoria");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL)); // Semilla para números aleatorios basada en el tiempo actual
    for (int i = 0; i < 5; ++i) {
        key[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    key[6] = '\0'; // Agrega el terminador nulo al final de la cadena
    return key;
}

void ExamenPsicometrico(int client_socket, Pregunta preguntas_psicometrico[]) {
    int puntaje_psicometrico = 0;

    // Evaluar si el cliente ha pasado los exámenes anteriores
    int passed_exams = evaluate_exam(puntaje_espanol);

    if (passed_exams) {

        // Enviar la clave para el examen psicométrico al cliente
        char *clave = generate_random_key(); // Generar la clave aleatoria
        send(client_socket, clave, strlen(clave), 0);
        free(clave); // Liberar la memoria asignada para la clave aleatoria

        // Enviar mensaje al cliente sobre la clave y el inicio del examen psicométrico
        send(client_socket, "¡Felicidades! Has pasado el Examen.\n Ingresa la clave para continuar con el examen psicométrico.\n\n", strlen("¡Felicidades! Has pasado el Examen.\n Ingresa la clave para continuar con el examen psicométrico.\n\n"), 0);

        // Esperar la respuesta del cliente
        char buffer[MAXBUF];
        memset(buffer, 0, MAXBUF);
        ssize_t bytes_received = recv(client_socket, buffer, MAXBUF, 0);

        // Verificar si la clave ingresada es correcta
        if (bytes_received > 0 && strncmp(buffer, clave, strlen(clave)) == 0) {
            send_questions_one_by_one(client_socket, preguntas_psicometrico, MAX_QUESTIONS, &puntaje_psicometrico, "Psicométrico");
            // Aquí puedes hacer más cosas después del examen psicométrico si lo deseas
        } else {
            send(client_socket, "Clave incorrecta. El examen psicométrico no se puede iniciar.\n", strlen("Clave incorrecta. El examen psicométrico no se puede iniciar.\n"), 0);
        }
    } else {
        send(client_socket, "Lo sentimos, no has pasado los exámenes anteriores. No puedes iniciar el examen psicométrico.\n", strlen("Lo sentimos, no has pasado los exámenes anteriores. No puedes iniciar el examen psicométrico.\n"), 0);
    }
}

//funciones para manejar solicitudes del cliente
void handle_client(int client_socket) {
    char buffer[MAXBUF];
    char *psicometric_key = NULL; // Variable para almacenar la clave del examen psicométrico
    int opcion;
    

    while (1) {
        memset(buffer, 0, MAXBUF);
        snprintf(buffer, sizeof(buffer),
                 "------------------------------------------------------------------\n"
                 "|                               EXAMEN                           |\n"
                 "------------------------------------------------------------------\n"
                 "Opciones a realizar: \n"
                 "1.Iniciar Evaluación de Español \n"
                 "2.Iniciar Evaluación de Matematicas \n"
                 "3.Iniciar Evaluación de Computación\n" 
                 "4.Cancelar Evaluación \n"
                 "-------------------------------------------------------------------\n");
        send(client_socket, buffer, strlen(buffer), 0);

        memset(buffer, 0, MAXBUF);
        if (recv(client_socket, buffer, MAXBUF, 0) <= 0) {
            break;
        }
        sscanf(buffer, "%d", &opcion);

        if (opcion < 1 || opcion > 4) {
            snprintf(buffer, sizeof(buffer), "Opcion invalida, vuelva intentar\n");
            send(client_socket, buffer, strlen(buffer), 0);
            continue;
        }

        switch (opcion) {
            case 1:
                printf("> El usuario ha seleccionado la opción 1 para empezar la evaluación de Español\n");
                send_questions_one_by_one(client_socket, preguntas_espanol, MAX_QUESTIONS, &puntaje_espanol, "Español");
                has_passed_spanish = evaluate_exam(puntaje_espanol);
                printf("> El usuario termino exitosamente la Exaluación de Español\n");
                if (has_passed_spanish == 1) {
                    printf("> El usuario APROBÓ la evaluacion de Español\n");
                    ExamenPsicometrico(client_socket, preguntas_psicometrico);
                }
                reiniciar_puntajes();
            break;
            case 2:
                printf("> El usuario ha seleccionado la opción 2 para empezar la evaluación de Matematicas\n");
                send_questions_one_by_one(client_socket, preguntas_matematicas, MAX_QUESTIONS, &puntaje_matematicas, "Matemáticas");
                has_passed_math = evaluate_exam(puntaje_matematicas);
                if (has_passed_math) {
                    ExamenPsicometrico(client_socket, preguntas_psicometrico);
                }
            break;
            case 3:
                printf("> El usuario ha seleccionado la opción 3 para empezar la evaluación de Computación\n");
                send_questions_one_by_one(client_socket, preguntas_computacion, MAX_QUESTIONS, &puntaje_computacion, "Computación");
                has_passed_computer = evaluate_exam(puntaje_computacion);
                if (has_passed_computer) {
                    ExamenPsicometrico(client_socket, preguntas_psicometrico);
                }
                break;
            case 4:
                printf(">Evaluacion Cancelado\n");
                snprintf(buffer, sizeof(buffer), "--------------------------------\nBye Bye!\n--------------------------------\n");
                send(client_socket, buffer, strlen(buffer), 0);
                close(client_socket); // Cierra el socket del cliente
                return;
        }
    }
}

//funcion principal
int main() {

    // Cargar preguntas para cada categoría
    load_questions(preguntas_espanol, "espanol.txt", "Español");
    load_questions(preguntas_matematicas, "matematicas.txt", "Matematicas");
    load_questions(preguntas_computacion, "computacion.txt", "Computacion ");
    load_questions(preguntas_psicometrico, "psicometrico.txt", "Psicometrico");



    // Declara variables para el socket del servidor y del cliente
    int server_socket, client_socket;
    // Declara las estructuras para las direcciones del servidor y del cliente
    struct sockaddr_in server_addr, client_addr;
    // Declara una variable para almacenar la longitud de las direcciones
    socklen_t addr_len;

    // Crea el socket del servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creando socket");
        return 1;
    }

    // Configura la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Asocia el socket del servidor a la dirección configurada
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        return 1;
    }

    // Pone al servidor en modo de escucha
    if (listen(server_socket, 5) < 0) {
        perror("Error en listen");
        return 1;
    }

    // Informa que el servidor está escuchando en el puerto especificado
    printf("Servidor escuchando en el puerto %d\n", PORT);

    // Bucle infinito para aceptar conexiones de clientes
    while (1) {
        addr_len = sizeof(client_addr);
        // Acepta una conexión de cliente
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

        if (client_socket < 0) {
            perror("Error en accept");
            continue;
        }
        
        // Informa que un cliente se ha conectado
        printf("Cliente conectado: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Maneja la comunicación con el cliente
        handle_client(client_socket);
    }

    // Cierra el socket del servidor
    close(server_socket);

    return 0;
}