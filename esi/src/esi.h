#ifndef ESI_H_
#define ESI_H_

#include <biblioteca/sockets.h>
#include <commons/log.h>
#include <commons/config.h>
#include <parsi/parser.h>
#include <stdio.h>
#include <stdlib.h>

#define RUTA_CONFIG "/home/utnso/workspace/tp-2018-1c--0/configuraciones/esi.cfg"

/*------------------------Variables globales-------------------------*/
t_log * logESI;

int socketCoordinador;
int socketPlanificador;

bool recibirSolicitudes;

void * archivo;
int ip;

/*-------------------------Conexion-------------------------*/
void 		conectarEsi					(void);
t_config* 	leerConfiguracion			(void);

/*-------------------------Procesamiento paquetes-------------------------*/
void 		procesarPaquete				(t_paquete * unPaquete, int * client_socket);
void 		procesarSolicitudEjecucion	(void);
void 		procesarError				(void);
void 		procesarRespuestaSolicitud	(void);

/*-------------------------Funciones auxiliares-------------------------*/
void * 		abrirArchivo				(char * rutaArchivo, size_t * tamArc, FILE ** archivo);
char * 		proximaSentencia			(char * archivo, int * ip);

#endif /* ESI_H_ */
