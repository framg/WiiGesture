#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <time.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;


//Variables del algoritmo
int centroEncontrado=0;
int xCentro, yCentro;
int yMotion;



void Init_Video(void);
//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	/*****************************************************************************
	
	El siguiente codigo ha sido extraido de internet para el uso del mando wii en 
	la videoconsola. Utiliza las librerias wpad.h para el uso del wii mote y 
	gccore para el uso del video.
	
	*****************************************************************************/
	u32 expansion_type, pad;
	WPADData * wmote_data;
	
	Init_Video();

	// Esta función inicializa los PADs conectados
	WPAD_Init();
	
	// Busca el primer pad encendido y guarda el tipo de su expansión en expansion_type
	int ret = -1;
	while (ret < 0) {
		for(pad=0;(pad<4)&(ret<0);pad++) 
			ret=WPAD_Probe(pad, &expansion_type);
	}
	pad--;


	// Asigna la resolucion que nos devuelve el IR
	WPAD_SetVRes(pad, 640, 480);
	
	// ajusta el modo de operación de los PADS activando acelerometros y el IR
	WPAD_SetDataFormat(pad, WPAD_FMT_BTNS_ACC_IR);


	while(1) {
		
	
		// Esto lee el estado de los mandos a cada ciclo del bucle
		WPAD_ScanPads();
		
		// recoge los datos extra del wiimote
		wmote_data=WPAD_Data(pad);
		
		// WPAD_ButtonsHeld nos dice todos los botones pulsados tanto del wiimote como
		// de las expansiones
		// Si en su lugar usaramos WPAD_ButtonsDown solo se activaría una vez y no volveria
		// a activarse hasta que no soltaramos el botón y lo volvieramos a pulsar
		u32 pressed = WPAD_ButtonsHeld(pad);
		
		/*****************************************************************************
	
		Aqui se encuetra mi algoritmo, con el se puede detectar la posicion del wiimote
		con relacion al cuerpo humano. Hay 4 posibles posiciones:
		-Debajo de la cadera.
		-Por encima de la cadera.
		-Por encima del hombro.
		-Por encima de la cabeza.
		
		Para conseguir esto se han utilizado 3 variables proporcionadas por el wiiMote:
		-Pitch, la inclinacion del mando. Si el wii mote esta inclinado hacia arriba 
		significara que esta por encima del hombro o de la cabeza, si lo esta hacia abajo
		estara por debajo de la cadera y si no estuviera inclinado estara por encima de la
		cadera.
		-La posicion x del wiiMote. Como podra observarse el ir del mando wii no puede captar
		todo, hay muchos puntos ciegos. Para poder saber donde se encuentra se dara una posicion
		relativa, es decir obteniendo el punto medio del centro donde empezamos apuntando con el 
		wii mote comprobamos cual fue el ultimo punto captado y asi se sabra su posicion.
		-Los aceleromentros nos indicaran si se mueve hacia abajo o hacia arriba.
		
	
		*****************************************************************************/
		
		
		
		//Lo primero de todo es encontrar el centro, el punto donde empezamos apuntando.
		if(centroEncontrado){
			printf("\x1b[%d;%dH", 16, 10);
			printf("Se ha encontrado el centro");
		}else{
			printf("\x1b[%d;%dH", 16, 10);
			printf("Aun no se conoce el centro");
		}
		
		//Este punto debe estar situado con una inclinacion normal, es decir ni apuntando hacia arriba
		//ni hacia abajo. Ademas debera apuntar a la pantalla (barra de infrarrojos).
		if(wmote_data->orient.pitch>=-10 && wmote_data->orient.pitch<=10 && wmote_data->ir.x != 0){
			centroEncontrado=1;
			xCentro = wmote_data->ir.x;
			yMotion = wmote_data->accel.y;
		}
	
		//Una vez encontrado el centro se procedera al tracking.
		if(centroEncontrado){
			//La aceleracion actual sera la obtenida cuando el mando estaba en el centro, posicion natural
			//menos la actual. De esta forma obtenemos el cambio sufrido y se puede actuar en base a ello.
			//Lo mismo para la posicion actual x.
			int accelActual = yMotion - wmote_data->accel.y;
			int posActual = xCentro - wmote_data->ir.y; 
			
			//Con esta estructura se puede conocer la posicion del wiiMote, si la inclinacion es mayor de 30 
			//el wiiMote esta apuntando hacia abajo, si la acceleracion es menor de 0 significa que se mueve en
			//direccion descendente y por ultimo si la posicion actual es menor de 0 significa que se ha salido
			//de pantalla(barra de infrarrojos) por la parte inferior.
			//Para las demas posiciones se hace un estudio similar.
			
			if(wmote_data->orient.pitch >= 30 && accelActual < 0 && posActual < 0){
				printf("\x1b[%d;%dH", 17, 10);
				printf("Mano por debajo de la cadera");
			}else if((wmote_data->orient.pitch < 30) && (wmote_data->orient.pitch >= -20)){	
				printf("\x1b[%d;%dH", 17, 10);
				printf("Mano por encima de la cadera");
			}else if((wmote_data->orient.pitch < -20) && (wmote_data->orient.pitch >= -50) && accelActual > 0 && posActual > 0){
				printf("\x1b[%d;%dH", 17, 10);
				printf("Mano por encima del hombro  ");
			}else if((wmote_data->orient.pitch < -50) && accelActual > 0 && posActual > 0){
				printf("\x1b[%d;%dH", 17, 10);
				printf("Mano por encima de la cabeza");		
			}else{
				printf("\x1b[%d;%dH", 17, 10);
				printf("No se reconoce la posicion  ");
			}
		
		}
			
				
		// Volvemos al launcher cuando pulsamos el botón Home
		if ( (pressed & WPAD_BUTTON_HOME) | (ret < 0)) exit(0);

		// Esperamos al siguiente frame
		VIDEO_WaitVSync();
	}

	// Terminamos de trabajar con el wiimote
	WPAD_Shutdown();
	
	return 0;
}


void Init_Video(void) {
	// Initialise the video system
	VIDEO_Init();
	

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);
	
	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
}
