#include "instancia.h"

int main(void) {
 //Creo archivo de log
 logInstancia = log_create("log_Instancia.log", "instancia", true,
 LOG_LEVEL_TRACE);
 log_trace(logInstancia, "Inicio el proceso instancia \n");

 //Conecto instancia con coordinador
 conectarInstancia();

 //Quedo a la espera de solicitudes
 recibirSolicitudes = true;
 while (recibirSolicitudes) {
 gestionarSolicitudes(socketCoordinador, (void*) procesarPaquete,
 logInstancia);
 }

 //Termina esi
 log_trace(logInstancia, "Termino el proceso instancia \n");

 //Destruyo archivo de log
 log_destroy(logInstancia);

 return EXIT_SUCCESS;
 }

/*-------------------------Conexion-------------------------*/
void conectarInstancia() {
	//Leo la configuracion del esi
	t_config* configInstancia = leerConfiguracion();

	//Setteo las variables de configuracion
	char * coordinadorIP = config_get_string_value(configInstancia,
			"COORDINADOR_IP");
	int coordinadorPuerto = config_get_int_value(configInstancia,
			"COORDINADOR_PUERTO");
	algoritmoReemplazo = config_get_string_value(configInstancia,
			"ALGORITMO_REEMPLAZO");
	puntoMontaje = config_get_string_value(configInstancia, "PUNTO_MONTAJE");
	char * nombreInstancia = config_get_string_value(configInstancia,
			"NOMBRE_INSTANCIA");
	intervaloDump = config_get_int_value(configInstancia, "INTERVALO_DUMP");

	//Conecto al coordinador
	socketCoordinador = conectarCliente(coordinadorIP, coordinadorPuerto,
			INSTANCIA);

	enviarNombreInstancia(socketCoordinador, nombreInstancia);

	//Destruyo la configuracion
	config_destroy(configInstancia);
}

t_config* leerConfiguracion() {
	t_config* configInstancia = config_create(RUTA_CONFIG);
	if (!configInstancia) {
		log_error(logInstancia, "Error al abrir la configuracion \n");
		exit(EXIT_FAILURE);
	}
	return configInstancia;
}

/*-------------------------Procesamiento paquetes-------------------------*/
void procesarPaquete(t_paquete * unPaquete, int * client_socket) {
	switch (unPaquete->codigoOperacion) {
	case ENVIAR_INFO_INSTANCIA:
		procesarEnviarInfoInstancia(unPaquete);
		break;
	default:
		break;
	}
	destruirPaquete(unPaquete);
}

void procesarEnviarInfoInstancia(t_paquete * unPaquete) {
	t_infoInstancia * info = recibirInfoInstancia(unPaquete);

	//Setteo tam de entrada y cantidad
	info->cantEntradas = cantEntradas;
	info->tamanioEntrada = tamanioEntrada;

	//Creo el espacio de almacenamiento
	crearStorage();

	//Creo bitMap del storage
	crearBitMap();

	//Libero memoria
	free(info);

}

/*-------------------------Tabla de entradas-------------------------*/
void crearTablaEntradas(void) {
	tablaEntradas = list_create();
}

void destruirTabla(void) {
	void eliminarEntrada(t_tabla_entradas * entrada) {
		free(entrada->clave);
		free(entrada);
	}

	list_destroy_and_destroy_elements(tablaEntradas, (void *) eliminarEntrada);
}

t_tabla_entradas * buscarEntrada(char * clave) {
	bool esEntradaBuscada(t_tabla_entradas * entrada) {
		return string_equals_ignore_case(entrada->clave, clave);
	}

	t_tabla_entradas * registroEntrada = list_find(tablaEntradas,
			(void*) esEntradaBuscada);

	return registroEntrada;
}

void agregarClave(char * clave) {
	t_tabla_entradas * registroEntrada = malloc(sizeof(t_tabla_entradas));

	registroEntrada->clave = strdup(clave);

	registroEntrada->tamanio = 0;

	registroEntrada->inexComienzo = -1;

	list_add(tablaEntradas, registroEntrada);
}

void eliminarClave(char * clave) {
	bool esEntradaBuscada(t_tabla_entradas * entrada) {
		return string_equals_ignore_case(entrada->clave, clave);
	}

	t_tabla_entradas * entradaBuscada = list_remove_by_condition(tablaEntradas,
			(void*) esEntradaBuscada);

	if (entradaBuscada != NULL) {
		if (entradaBuscada->tamanio != 0) {
			int i;
			int cantidadEntradasABorar = entradaBuscada->tamanio
					/ tamanioEntrada;

			if (entradaBuscada->tamanio % tamanioEntrada != 0)
				cantidadEntradasABorar++;

			for (i = 0; i < entradaBuscada->tamanio; i++)
				liberarIndex(entradaBuscada->inexComienzo + i);
		}
		free(entradaBuscada->clave);
		free(entradaBuscada);
	}
}

void mostrarTabla(void) {
	int i;

	printf("Clave			Tamanio			1°Entrada \n");
	printf("----------------------------------------------------------\n");

	for (i = 0; i < tablaEntradas->elements_count; i++) {
		t_tabla_entradas * entrada = list_get(tablaEntradas, i);
		printf("%s			%d			%d \n", entrada->clave, entrada->tamanio,
				entrada->inexComienzo);
	}
	printf("\n");
}

int agregarClaveValor(char * clave, void * valor) {
	int tamValor = string_length(valor);

	int index = -1;

	void * respuesta = guardarEnStorage(valor, &index);

	if (respuesta == NULL) {
		return -1;
	} else {
		t_tabla_entradas * registroEntrada = malloc(sizeof(t_tabla_entradas));

		registroEntrada->clave = strdup(clave);

		registroEntrada->entrada = respuesta;

		registroEntrada->tamanio = tamValor;

		registroEntrada->inexComienzo = index;

		list_add(tablaEntradas, registroEntrada);

		return 0;
	}
}

void * buscarValorSegunClave(char * clave) {
	t_tabla_entradas * entrada = buscarEntrada(clave);

	char * respuesta = malloc(entrada->tamanio + 1);

	memcpy(respuesta, entrada->entrada, entrada->tamanio);

	respuesta[entrada->tamanio] = '\0';

	return respuesta;
}

t_tabla_entradas * buscarEntradaSegunIndex(int index) {
	bool esEntradaBuscada(t_tabla_entradas * entrada) {
		return (entrada->inexComienzo == index);
	}

	t_tabla_entradas * registroEntrada = list_find(tablaEntradas,
			(void*) esEntradaBuscada);

	return registroEntrada;
}

void mostrarEntrada(char * clave) {
	t_tabla_entradas * entrada = buscarEntrada(clave);

	printf("Clave: %s \n", entrada->clave);

	char * respuesta = malloc(entrada->tamanio + 1);

	memcpy(respuesta, entrada->entrada, entrada->tamanio);

	respuesta[entrada->tamanio] = '\0';

	printf("Valor: %s \n", respuesta);

	printf("Tamanio: %d \n\n", entrada->tamanio);

	free(respuesta);
}


/*-------------------------BitMap del Storage-------------------------*/
void crearBitMap(void) {
	bitMap = malloc(sizeof(bool) * cantEntradas);

	liberarBitMap();
}

void destruirBitMap(void) {
	free(bitMap);
}

void liberarBitMap(void) {
	for (int i = 0; i < cantEntradas; i++)
		liberarIndex(i);
}

void llenarBitMap(void) {
	for (int i = 0; i < cantEntradas; i++)
		ocuparIndex(i);
}

void liberarIndex(int index) {
	if (index + 1 <= cantEntradas) {
		bitMap[index] = false;
	} else {
		log_error(logInstancia,
				"No se puede liberar el index %d ya que no existe \n", index);
	}
}

void ocuparIndex(int index) {
	if (index + 1 <= cantEntradas) {
		bitMap[index] = true;
	} else {
		log_error(logInstancia,
				"No se puede ocuapar el index %d ya que no existe \n", index);
	}
}

int buscarIndexLibre(void) {
	int index = 0;

	while (bitMap[index]) {
		index++;
	}

	if (index < 99)
		bitMap[index] = false;

	return index;
}

void mostrarBitmap(void) {
	printf("Index			Ocupado \n");
	printf("-------------------------------\n");
	int i;
	for (i = 0; i < cantEntradas; i++)
		printf("%d			%d \n", i, bitMap[i]);
	printf("\n");
}

int buscarCantidadIndexLibres(int cantidad) {
	bool loEncontre = false;
	int candidato;
	int contador;
	int i;

	for (i = 0; !loEncontre && i < cantEntradas; i++) {
		if (!bitMap[i]) {
			candidato = i;
			//printf("El candidato es: %d \n", candidato);
			contador = 1;
			//printf("El contador es: %d \n", contador);

			while (contador < cantidad && (i + 1) < cantEntradas
					&& !bitMap[i + 1]) {
				i++;
				contador++;
				//printf("El contador es: %d \n", contador);
			}

			//printf("El contador definitivo es: %d \n",contador);

			if (contador == cantidad)
				loEncontre = true;
		}
	}

	if (!loEncontre)
		candidato = -1;

	return candidato;
}

/*-------------------------Storage-------------------------*/
void crearStorage(void) {
	storage = malloc(cantEntradas * tamanioEntrada);
}

void destruirStorage(void) {
	free(storage);
}

void * guardarEnStorage(void * valor, int * index) {
	int tamValor = strlen(valor);

	int entradasNecesaria = tamValor / tamanioEntrada;

	int resto = tamValor % tamanioEntrada;

	if (resto != 0)
		entradasNecesaria++;

	*index = buscarCantidadIndexLibres(entradasNecesaria);

	if ((*index) != -1) {

		return guardarEnStorageEnIndex(valor, *index);

	} else {
		return NULL;
	}
}

void * guardarEnStorageEnIndex(void * valor, int index) {
	int tamValor = strlen(valor);

	int entradasNecesaria = tamValor / tamanioEntrada;

	int resto = tamValor % tamanioEntrada;

	if (resto != 0)
		entradasNecesaria++;

	int i;

	for (i = 0; i < entradasNecesaria; i++)
		ocuparIndex(index + i);

	memcpy(storage + index, valor, tamValor);

	return storage + index;
}

void compactar(void) {
	int i;
	int primeraEntradaLibre;

	for (i = 0; i < cantEntradas; i++) {
		if (!bitMap[i]) {

			primeraEntradaLibre = i;

			for (; i < cantEntradas && !bitMap[i]; i++)
				;

			if (i == cantEntradas)
				break;

			int indexOcupado = i;

			t_tabla_entradas * entrada = buscarEntradaSegunIndex(i);

			int cantidadEntradas = entrada->tamanio / tamanioEntrada;

			if (entrada->tamanio % tamanioEntrada != 0)
				cantidadEntradas++;

			entrada->inexComienzo = primeraEntradaLibre;

			void * valor = buscarValorSegunClave(entrada->clave);

			entrada->entrada = guardarEnStorageEnIndex(valor,
					primeraEntradaLibre);

			free(valor);

			for (int i2 = 0; i2 < cantidadEntradas; i2++) {
				ocuparIndex(primeraEntradaLibre);
				liberarIndex(indexOcupado);
				primeraEntradaLibre++;
				indexOcupado++;
			}
		}

	}
}
