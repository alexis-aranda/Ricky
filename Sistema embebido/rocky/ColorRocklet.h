/*
    ColorRocklet.h - Biblioteca para identificar el color de un rocklet
*/

#ifndef ColorRocklet_h
#define ColorRocklet_h

class ColorRocklet{
	public:
	    /* Definición de id para los Colores */
		static const int NO_IDENTIFICADO = 0; //para cuando no reconoce ningún color
		static const int VERDE = 1;
		static const int AZUL = 2;
		static const int ROJO = 3;
		static const int NARANJA = 4;
		static const int AMARILLO = 5;
		static const int MARRON = 6;

		/* Defino las medias de valores para cada color */
		/* MEDIA_COLOR[] = {rojo, verde, azul};*/
		static const int M_VERDE[] = {155,142,154};
		static const int M_AZUL[] = {169,156,111};
		static const int M_ROJO[] = {135,231,183};
		static const int M_NARANJA[] = {100,183,163};
		static const int M_AMARILLO[] = {98,138,158};
		static const int M_MARRON[] = {182,233,188};

		ColorRocklet(); //Inicializo como NO_IDENTIFICADO por defecto
		ColorRocklet(const int rojo, const int verde, const int azul); //Inicializo con parametros recibidos
		void setColor(const int idColor); //Seteo el id del color
		int getColor(); //Devuelve el id del color
		void identificarColor();

	private:
		int idColor; //id actual
		int rojo; //rojo medido actual
		int verde; //verde medido actual
		int azul; //azul medido actual

    bool enRango(const int vecColor[]); /*verifica que los valores esten dentro de un
                                         *rango indicado (equivalente a un color)*/
};

#endif
