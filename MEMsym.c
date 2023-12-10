#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#define TAM_LINEA 16
#define NUM_LINEAS_CACHE 8
#define TAM_RAM 4096

typedef  struct
{
    unsigned char ETQ;
    unsigned char Data[TAM_LINEA+1];
}T_CACHE_LINE;

typedef  struct
{
	unsigned char **direcciones_memoria_hex;
	int num_direcciones_memoria;
}STRUCT_DIRECCIONES;

int existeFichero(char* str);
void LimpiarCACHE(T_CACHE_LINE tbl[NUM_LINEAS_CACHE]);
void leerFichero(FILE* descriptor, char linea[]);
char *leelineaDinamicaFichero (FILE *fd);
unsigned char** leerLineasDinamicasFicheo(FILE* descriptor, int *i);
void conversorHexadecimalDecimal(unsigned char* hexa, unsigned int* addr);
unsigned int* conversorDecimalBinario(unsigned int* decimal);
void ParsearDireccion(unsigned int addr, int *ETQ, int*palabra, int *linea, int *bloque);
int conversorBinarioADecimal(int* binario, int inicio, int fin) ;
void TratarFallo(T_CACHE_LINE *tbl, char *MRAM, int ETQ,int linea, int bloque);

int globaltime = 0;
int numfallos = 0;
char texto[100];

int main(int argc, char* argv[]){

	
    //INICIALIZAMOS TODAS LAS VARIABLES
    T_CACHE_LINE cache[NUM_LINEAS_CACHE];
    LimpiarCACHE(cache);
	FILE* fichero;
    unsigned char Simul_RAM[TAM_RAM];
	STRUCT_DIRECCIONES struct_direcciones;
	struct_direcciones.num_direcciones_memoria=0;
    int ETQ, palabra, linea, bloque;
    unsigned int addr;
	int aciertos=0;
	
	//COMPROBAMOS SI LOS FICHEROS EXISTEN
	if(existeFichero("accesos_memoria.txt")==0&&existeFichero("CONTENTS_RAM.bin")==0){
		
		//LEEMOS LOS FICHEROS Y GUARDAMOS LAS DIRECCIONES DE MEMORIA EN DIRECCIONES_MEMORIA_HEX
    	fichero = fopen( "CONTENTS_RAM.bin" , "r");
		leerFichero(fichero,Simul_RAM);
		fclose(fichero);
    	fichero = fopen( "accesos_memoria.txt" , "r");
        struct_direcciones.direcciones_memoria_hex = leerLineasDinamicasFicheo(fichero, &struct_direcciones.num_direcciones_memoria);
		fclose(fichero);
	
	}else{
		return -1;
	}
	
	
    for(int i=0; i<struct_direcciones.num_direcciones_memoria;i++){
        conversorHexadecimalDecimal(struct_direcciones.direcciones_memoria_hex[i],&addr);
		ParsearDireccion(addr, &ETQ, &palabra, &linea, &bloque);
		if(ETQ!=cache[linea].ETQ){
			//TRATAMOS EL FALLO
			TratarFallo(cache, Simul_RAM, ETQ,linea,  bloque);
			numfallos++;
			//AUMENTAMOS EL TIEMPO GLOBAL EN 20
			globaltime=globaltime+20;
			//ENSEÑAMOS EL FALLO
			printf("T: %d, Fallo de CACHE %d, ADDR %04X, Label %X, linea %02X, palabra %02X, bloque %02X\n",i+1,numfallos,addr,ETQ,linea, palabra, bloque);
		}
			//ENSEÑAMOS DONDE SE CARGAN LOS DATOS
			printf("Se ha cargado en el bloque %02X, en la linea %02X.\n", bloque, linea);
			//GUARDAMOS EL ACIERTO
			texto[aciertos]=cache[linea].Data[15-palabra];
			printf("T: %d, Acierto de CACHE, ADDR %04X, Label%X, linea %02X, palabra %02X, DATO %c\n\n",i+1,addr,ETQ,linea, palabra, texto[aciertos]);
			aciertos++;
			//AUMENTAMOS EL TIEMPO GLOBAL EN 1 Y HACEMOS UN SLEEP DE 1S
			globaltime++;
			sleep(1);
		
		//ENSEÑAMOS LOS DATOS DE LA ACHE
		for(int j=0; j<NUM_LINEAS_CACHE;j++){
			printf("%x\t", cache[j].ETQ);
        	for(int i=0; i<TAM_LINEA;i++){
				printf("%x ", cache[j].Data[i]);
        	}
			printf("\n");
    	}
		printf("\n");
        
    }
	//PRINTEAMOS LOS DATOS
	printf("Accesos totales: %d; fallos %d; tiempo medio  %f \n",struct_direcciones.num_direcciones_memoria, numfallos, (float)((float)globaltime/14));
	printf("El texto es %s", texto);
	
	
}

//COMPRUEBA LOS FICHEROS
int existeFichero(char* str){
	FILE* fichero;
	int i=0;
	fichero = fopen( str , "r");
	if(fichero==NULL){
		printf("el fichero %s no existe",str);
		i=-1;
	}
	return i;
}

//INICIALIZA EL CACHE
void LimpiarCACHE(T_CACHE_LINE tbl[NUM_LINEAS_CACHE]){
    for(int j=0; j<NUM_LINEAS_CACHE;j++){
        tbl[j].ETQ=0xFF;
        for(int i=0; i<TAM_LINEA;i++){
            tbl[j].Data[i]=0x23;
        }
		tbl[j].Data[TAM_LINEA]='\0';
    }
}

//LEE UNA LINEA DEL FICHERO Y TE LO DEVUELVE EN ARRAY ESTATICO
void leerFichero(FILE* descriptor, char linea[]){
    int  t=0;
	char c;
	while( (((linea[t]=getc(descriptor))!=EOF)&&(linea[t]!='\n'))  && TAM_RAM>t ){
		t++;
    }
}

//LEE UNA LINEA DEL FICHERO Y TE LO VEVUELVE EN ARRAY DINAMICO
char *leelineaDinamicaFichero (FILE *fd){
    int  t=0;
	char* linea;
	char c;
	linea=(char*)malloc(sizeof(char));
	while(((c=getc(fd))!=EOF)&&(c!='\n')){
		linea[t]=c;
	    t++;
		linea=(char*)realloc(linea, sizeof(char)*(t+1));		
	}
	linea[t]='\0';
	return linea;
}

//DEVUELVE Y CUENTA LAS LINEAS DEL FICHERO
unsigned char** leerLineasDinamicasFicheo(FILE* descriptor, int *i){ 
	unsigned char** lineas;
    char c;
    lineas=(unsigned char**)malloc(sizeof(unsigned char*));
    do{
        lineas[*i]=leelineaDinamicaFichero(descriptor);
		*i=*i+1;
        lineas=(unsigned char**)realloc(lineas,sizeof(unsigned char*)*(*i+1));
    }while(!feof(descriptor));
	lineas[*i]='\0';
	return lineas;
}

//CONVERSOR DE HEXADECIMAL A DECIMAL
void conversorHexadecimalDecimal(unsigned char* hexa, unsigned int *addr){
    *addr = strtol(hexa, NULL, TAM_LINEA);
}

//CONVERSOR DE DECIMAL A BINARIO
unsigned int* conversorDecimalBinario(unsigned int* decimal){
	unsigned int* binario;
	binario=(unsigned int*)malloc(sizeof(unsigned int)*13);

	for(int i=11; i>=0;i--){
		binario[i]=*decimal%2;
		*decimal=*decimal/2;
	}
	binario[13]='\0';
	
	return binario;
	
}

//CONVERSOR BINARIO A DECIMAL
int conversorBinarioADecimal(int* binario, int inicio, int fin) {
    int decimal = 0, exponente = 0, digito;

    for (int i = inicio; i >= fin; i--) {
        decimal =decimal+ (binario[i] * (1 << exponente));
        exponente++;
    }
    return decimal;
}

//FUNCION PARSEAR DIRECCION
void ParsearDireccion(unsigned int addr, int *ETQ, int* palabra, int* linea, int* bloque){
	unsigned int *binario;
	int contador=0;
	binario=conversorDecimalBinario(&addr);
	*ETQ=conversorBinarioADecimal(binario,4 ,0);
	*linea=conversorBinarioADecimal(binario,7 ,5);
	*palabra=conversorBinarioADecimal(binario,11 ,8);
	*bloque=conversorBinarioADecimal(binario,7 ,0);

}

//FUNCION TRATAR FALLO
void TratarFallo(T_CACHE_LINE *tbl, char *MRAM, int ETQ, int linea, int bloque){
  	int cont=0;
    tbl[linea].ETQ=ETQ;
	for(int i= TAM_LINEA*bloque+15;i>=TAM_LINEA*bloque;i--){
		printf("%c", MRAM[i]);
		tbl[linea].Data[cont]=MRAM[i];
		cont++;
	}
}




