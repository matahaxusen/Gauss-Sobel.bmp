/**
 * CODIGO FINAL DE LA PARTE DE PARALELO
 */

#include <iostream> //Importante para poder imprimir por pantalla entre otras cosas
#include <fstream>  //Manejo de ficheros de entrada y salida
#include <cstring>  //Utilizado para comparar Strings
#include <dirent.h> //Manejo entre directorios
#include <omp.h>
#include <chrono>

int NUM_THREADS = 4;

char origen[256];  //El path origen que me han pasado
char destino[256]; //El path origen que me han pasado
int op = -1;
int mGauss[5][5] = {{1, 4, 7, 4, 1},
                    {4, 16, 26, 16, 4},
                    {7, 26, 41, 26, 7},
                    {4, 16, 26, 16, 4},
                    {1, 4, 7, 4, 1}};

int mxSobel[3][3] = {{1, 2, 1},
                     {0, 0, 0},
                     {-1, -2, -1}};

int mySobel[3][3] = {{-1, 0, 1},
                     {-2, 0, 2},
                     {-1, 0, 1}};
typedef struct infoImagen
{
    char B = 'B';            // Array de chars "BM"
    char M = 'M';            // Array de chars "BM"
    int sFile;               // Tamaño del fichero
    int reservado = 0;       //Espacio reservado
    int offsetImagen = 54;   //Inicio del contenido de los pixeles de la imagen
    int sCabecera = 40;      // Tamaño de la cabecera
    int anchura;             // Anchura de la imagen
    int altura;              // Altura de la imagen
    short nPlanos = 1;       // Numero de planos de la imagen
    short bitPorPixel = 24;  //Bits por pixeles de la imange
    int compresion = 0;      // Compresion de la imagen
    int sImagen;             // Tamaño total solo de la imagen (altura*anchura*3)
    int rX = 2835;           // Resolucion horizontal
    int rY = 2835;           // Resolucion vertical
    int sColor = 0;          // Tamaño de la tabla de color
    int colorImportante = 0; // Colores Importantes
    unsigned char *imagen;   // Datos de la imange BMP
} infoImagen;

typedef struct tiempo
{
    int total = 0;
    long loadTime = 0;
    int gaussTime = 0;
    int sobelTime = 0;
    int storeTime = 0;

} tiempo;
using namespace std;

char *obtenerFilePath(char *path, char *fichero);
int operacion(char *fichero, tiempo *time);
infoImagen leerImagen(const char *fileName, short *error);
int escribirImagen(const char *filePathDestino, infoImagen imagen);
unsigned char *gauss(infoImagen datos);
unsigned char *sobel(infoImagen datos, unsigned char *imagen);
int comprobarBMP(infoImagen datos);
void printError(int tipo, char **argv)
{
    switch (tipo)
    {
    case 0:
        cout << "Wrong format:\n";
        break;
    case 1:
        cout << "Unexpected operation: " << argv[1] << "\n";
        break;
    case 2:
        cout << "Cannot open directory [" << argv[2] << "]\n";
        break;
    case 3:
        cout << "Output directory [" << argv[3] << "] does not exist\n";
        break;
    }
    cout << "  image-seq operation in_path out_path\n    operation: copy, gauss, sobel\n$\n";
}
int main(int argc, char *argv[])
{
    cout << "$image-seq ";
    //Si el número de argumentos que me pasa el programa es menor que 4 es que está mal
    if (argc != 4)
    {
        printError(0, argv);
        return -1;
    }
    cout << argv[1] << " " << argv[2] << " " << argv[3] << "\n";
    //Obtener la operación pasada por argumento
    if (strcmp(argv[1], "copy") == 0)
    {
        op = 1; //En el caso de que el trabajo sea copy
    }
    else if (strcmp(argv[1], "gauss") == 0)
    {
        op = 2; //En el caso de que el trabajo sea gauss
    }
    else if (strcmp(argv[1], "sobel") == 0)
    {
        op = 3; //En el caso de que el trabajo sea sobel
    }
    else
    {
        printError(1, argv);
        return -1;
    }
    memcpy(origen, argv[2], strlen(argv[2]));
    memcpy(destino, argv[3], strlen(argv[3]));
    if (origen[strlen(origen) - 1] != '/')
        strcat(origen, "/"); // En el caso de que no exista la barra en dir origen
    if (destino[strlen(destino) - 1] != '/')
        strcat(destino, "/"); // En el caso de que no exista la barra en dir origen
    cout << "Input path: " << origen << "\n"
         << "Output path: " << destino << "\n";
    struct dirent *eDirOrigen;          // Lee los ficheros que hay en el directorio de origen
    DIR *dirOrigen = opendir(origen);   // Obtengo todos los ficheros del origen
    DIR *dirDestino = opendir(destino); // Obtengo todos los ficheros del destino
    // Debe de existir los dos directorios
    if (dirOrigen == NULL)
    {
        printError(2, argv);
        return -1;
    }
    if (dirDestino == NULL)
    {
        printError(3, argv);
        return -1;
    }

    while ((eDirOrigen = readdir(dirOrigen)) != NULL) // Mientras el elemento que me pase el directorio no sea nulo
    {
        if (strcmp(eDirOrigen->d_name, ".") && strcmp(eDirOrigen->d_name, "..")) // Evito que utilicen como fichero el . y ..
        {
            tiempo time;
            auto start_time = chrono::high_resolution_clock::now();
            if (operacion(eDirOrigen->d_name, &time) == -1) // Tareas de la imagen
                return -1;
            auto end_time = chrono::high_resolution_clock::now();
            time.total = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
            cout << "File: \"" << origen << eDirOrigen->d_name << "\" (time: " << time.total << ")\n"
                 << "  Load time: " << time.loadTime << "\n";
            if (op == 2 || op == 3)
            {
                cout << "  Gauss time: " << time.gaussTime << "\n";
                if (op == 3)
                    cout << "  Sobel time: " << time.sobelTime << "\n";
            }
            cout << "  Store time: " << time.storeTime << "\n";
        }
    }
    closedir(dirOrigen);  //Cierro el directorio de origen
    closedir(dirDestino); //Cierro el directorio de destino
    cout << "$\n";
    return 0;
}

/* Esta función realiza la acción indicada por el usuario a cada uno de los archivos */
int operacion(char *fichero, tiempo *time)
{
    char *filePathOrigen = obtenerFilePath(origen, fichero);
    short error = 0;
    /*---------------- Leer Imagen -------------------*/
    auto start_time = chrono::high_resolution_clock::now();
    infoImagen imagenOrigen = leerImagen(filePathOrigen, &error);
    auto end_time = chrono::high_resolution_clock::now();
    time->loadTime = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
    free(filePathOrigen);
    if (error != 0)
        return -1;
    infoImagen imagenDestino;
    imagenDestino.altura = imagenOrigen.altura;
    imagenDestino.anchura = imagenOrigen.anchura;
    imagenDestino.sFile = imagenOrigen.sImagen + 54;
    imagenDestino.sImagen = imagenOrigen.sImagen;
    if (op == 2 || op == 3)
    {
        /*---------------- Gauss -------------------*/
        start_time = chrono::high_resolution_clock::now();
        unsigned char *imagenDestinoChar = gauss(imagenOrigen);
        end_time = chrono::high_resolution_clock::now();
        time->gaussTime = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
        if (op == 3)
        {
            /*---------------- Sobel -------------------*/
            start_time = chrono::high_resolution_clock::now();
            imagenDestinoChar = sobel(imagenOrigen, imagenDestinoChar);
            end_time = chrono::high_resolution_clock::now();
            time->sobelTime = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
        }
        imagenDestino.imagen = imagenDestinoChar;
    }
    else
    {
        imagenDestino.imagen = imagenOrigen.imagen;
    }

    /* Elegir la imagen que se va a Escribir */

    char *filePathDestino = obtenerFilePath(destino, fichero);

    /*---------------- Escribir el nuevo fichero -------------------*/
    start_time = chrono::high_resolution_clock::now();
    int errorEscribir = escribirImagen(filePathDestino, imagenDestino);
    end_time = chrono::high_resolution_clock::now();
    time->storeTime = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
    free(imagenDestino.imagen);
    free(filePathDestino); //Liberar el path de destino
    if (errorEscribir == -1)
        return -1;
    return 0;
}

/* Esta funcion obtiene el path donde se encuentra el fichero juntando la carpeta origen y el nombre del archivo */
char *obtenerFilePath(char *path, char *fichero)
{
    char *filePath = new char[256]; // Creo un espacio donde guardar los paths a los archivos
    strcat(filePath, path);               // Copio la carpeta
    strcat(filePath, fichero);            // Copio el nombre del fichero
    return filePath;                      // Devuelvo el puntero al path completo hacia el archivo
}

/* Esta función lee la imagen que ha recibido por parámetro y comprueba que todos los parámetros necesarios 
   son correctos. También actualiza los valores anchura y altura pasados por parámetro*/
infoImagen leerImagen(const char *fileName, short *error)
{
    FILE *leerDF = fopen(fileName, "rb"); // Descriptor de fichero de la imagen
    infoImagen tmp;
    if(leerDF == NULL){ //En caso de que el archivo no pueda ser leído o no exista
        perror("Error al intentar leer el fichero");
        *error = -1;
        return tmp;
    }
    if ((fread(&tmp, 1, 2, leerDF)) == 0)
    { //Leo los dos primeros bytes
        perror("Error de escritura");
        *error = -1;
        return tmp;
    }
    if ((fread(&tmp.sFile, sizeof(int), 6, leerDF)) == 0)
    { //Leo los siguientes enteros de la cabecera
        perror("Error de escritura");
        *error = -1;
        return tmp;
    }
    if ((fread(&tmp.nPlanos, sizeof(short), 2, leerDF)) == 0)
    { //Leo los dos shorts de la cabecera
        perror("Error de escritura");
        *error = -1;
        return tmp;
    }
    if ((fread(&tmp.compresion, sizeof(int), 6, leerDF)) == 0)
    { //Leo los ultimo enteros de la cabecera
        perror("Error de escritura");
        *error = -1;
        return tmp;
    }
    if (comprobarBMP(tmp) != 0)
    { //En caso de que no sea un fichero valido
        *error = 1;
        return tmp;
    }
    int unpaddedRowSize = tmp.anchura * 3; // Consigo el tamaño de una linea sin padding
    int paddedRowSize;
    if (unpaddedRowSize % 4 != 0) //Para averiguar si el fichero tiene padding o no
        paddedRowSize = unpaddedRowSize + (4 - (unpaddedRowSize % 4));
    else
        paddedRowSize = unpaddedRowSize;

    int totalSize = unpaddedRowSize * tmp.altura; //Tamaño total del archivo sin padding
    tmp.imagen = (unsigned char *)malloc(totalSize); //Lugar donde vamos a guardar los archivos
    unsigned char *currentRowPointer = tmp.imagen + ((tmp.altura - 1) * unpaddedRowSize); //Puntero al lugar donde se guarda la imagen
                                                                                          //Para guardar cada linea en su lugar correspondiente
    for (int i = 0; i < tmp.altura; i++) //Guarado el fichero por lineas
    {
        fseek(leerDF, tmp.offsetImagen + (i * paddedRowSize), SEEK_SET); //posiciono el offset
        if ((fread(currentRowPointer, 1, unpaddedRowSize, leerDF)) == 0)
        { //En el caso de que una linea no tenga informacion
            perror("Error de escritura");
            *error = -1;
            return tmp;
        }
        currentRowPointer -= unpaddedRowSize;
    }
    fclose(leerDF); // Cierro el descriptor de fichero
    return tmp;
}


/*Esta funcion obtiene todos los parametros necesarios para leer una imagen y escribe la imagen pasada por argumento
   ademas del lugar donde guardarla, la altura y la anchura*/
int escribirImagen(const char *fileName, infoImagen imagen)
{
    FILE *escribirDF = fopen(fileName, "wb");
    if (escribirDF == NULL)
    {
        perror("Error al intentar crear el archivo de destino");
        return -1;
    }
    // Escribir cada uno de los parámetros de la cabecera
    if (fwrite(&imagen, 1, 2, escribirDF) == 0)
    { // Escribo BM
        perror("Error de escritura al escribir los datos de la imagen");
        return -1;
    }
    if (fwrite(&imagen.sFile, sizeof(int), 6, escribirDF) == 0)
    { // Escribo los siguientes enteros de la cabecera
        perror("Error de escritura al escribir los datos de la imagen");
        return -1;
    }
    if (fwrite(&imagen.nPlanos, sizeof(short), 2, escribirDF) == 0)
    { // Escribo los shorts de la cabecera
        perror("Error de escritura al escribir los datos de la imagen");
        return -1;
    }
    if (fwrite(&imagen.compresion, sizeof(int), 6, escribirDF) == 0)
    { // Escribo los últimos enteros de la cabecera
        perror("Error de escritura al escribir los datos de la imagen");
        return -1;
    }
    fseek(escribirDF, imagen.offsetImagen, SEEK_SET); // Establezco la posición donde se escribe la imagen
    int unpaddedRowSize = imagen.anchura * 3;
    int paddedRowSize;
    if (unpaddedRowSize % 4 != 0)
        paddedRowSize = unpaddedRowSize + (4 - (unpaddedRowSize % 4));
    else
        paddedRowSize = unpaddedRowSize;
    for (int i = 0; i < imagen.altura; i++)
    {
        int pixelOffset = ((imagen.altura - i) - 1) * unpaddedRowSize;
        if (fwrite(&imagen.imagen[pixelOffset], 1, paddedRowSize, escribirDF) == 0)
        {
            perror("Error de escritura al escribir los pixeles");
            return -1;
        }
    }
    fclose(escribirDF); // Cierro el descriptor de fichero de escribir
    return 0;
}

int comprobarBMP(infoImagen datos)
{

    if (datos.B != 'B' || datos.M != 'M')
    {
        perror("El archivo no es BMP");
        return -1;
    }
    else if (datos.nPlanos != 1)
    {
        perror("El número de planos es distinto de 1");
        return -1;
    }
    else if (datos.bitPorPixel != 24)
    {
        perror("El número de bits por pixel no es 24");
        return -1;
    }
    else if (datos.compresion != 0)
    {
        perror("La compresion del archivo no es 0");
        return -1;
    }
    return 0;
}

unsigned char *sobel(infoImagen datos, unsigned char *imagen)
{
    //Obtengo la anchura y altura del fichero
    int width = datos.anchura; 
    int height = datos.altura;
    int w = 8; //Peso designado para realizar la mascara
    unsigned char *pixels = imagen; // la imagen origen
    int linea = width * 3;  //Tamaño de una linea
    int tmpBx, tmpBy, tmpRx, tmpRy, tmpGx, tmpGy;   //Valores temporales de un byte de un pixel
    int size = height * linea;  //El tamaño del fichero 
    int columnaByte;    //La columna donde esta el byte, se usa para comparar si esta dentro del fichero o fuera
    int columnaSobel;
    int filaSobel;
    int byte;
    unsigned char *pixelsN = (unsigned char *)malloc(size); //Imagen nueva
    size -=1; //Para evitar que se realice la operacion en cada if
    omp_set_num_threads(NUM_THREADS);
    #pragma omp parallel for private (tmpBx, tmpBy, tmpRx, tmpRy, tmpGx, tmpGy) schedule(dynamic)
    for (int i = 0; i <= height - 1; i += 1)    //Empiezo en 0, por lo tanto tengo que llegar hasta altura -1
        for (int j = 0; j <= linea - 1; j += 3) //Empiezo en 0, por lo tanto tengo que llegar hasta linea -1
        {                                       //Como realizamos la operación del los tres bytes desde j, entonces
                                                //Se le debe sumar 3 a j para los 3 bytes siguientes
            {
                //Reset valores
                tmpBx = 0;
                tmpGx = 0;
                tmpRx = 0;
                tmpBy = 0;
                tmpGy = 0;
                tmpRy = 0;
                //Mascara de Sobel
                for (int s = -1; s <= 1; s++)
                {
                    for (int t = -1; t <= 1; t++)
                    {
                        //Obtengo los valores que se van a usar en el proceso
                        filaSobel = s + 1;  
                        columnaSobel = t + 1;
                        columnaByte = t * 3 + j;
                        byte = (i + s) * linea + columnaByte ;
                        // Todos los parametros son para comprobar  si el byte escogido está dentro de la imagen
                        if (byte >= 0 && byte <= size  && 0 <= j + t * 3 && columnaByte  <= linea - 1)
                        {
                            tmpBx += mxSobel[filaSobel][columnaSobel] * pixels[byte];
                            tmpBy += mySobel[filaSobel][columnaSobel] * pixels[byte];
                        }
                        byte += 1;
                        columnaByte +=1;
                        if (byte >= 0 && byte <= size  && 0 <= columnaByte && columnaByte <= linea - 1)
                        {
                            tmpGx += mxSobel[filaSobel][columnaSobel] * pixels[byte];
                            tmpGy += mySobel[filaSobel][columnaSobel] * pixels[byte];
                        }
                        byte += 1;
                        columnaByte +=1;
                        if (byte >= 0 && byte <= size  && 0 <= columnaByte  && columnaByte  <= linea - 1)
                        {
                            tmpRx += mxSobel[filaSobel][columnaSobel] * pixels[byte];
                            tmpRy += mySobel[filaSobel][columnaSobel] * pixels[byte];
                        }
                    }
                }
                // Guardo los valores en la nueva imagen
                pixelsN[(i * linea) + j] = (unsigned char) ((abs(tmpBx)+ abs(tmpBy)) /w);
                pixelsN[(i * linea) + j + 1] = (unsigned char) ((abs(tmpGx) + abs(tmpGy)) /w);
                pixelsN[(i * linea) + j + 2] = (unsigned char) ((abs(tmpRx) + abs(tmpRy))/w);
            }
        }
    free(pixels); // Libero la memoria de la antigua imagen
    return pixelsN; //Devuelvo la nueva
}

unsigned char *gauss(infoImagen datos)
{
    //Obtengo los valores que me han pasado
    int width = datos.anchura;
    int height = datos.altura;
    unsigned char *pixels = datos.imagen;
    int w = 273; //El peso de la mascara
    int linea = width * 3; //Tamaño de la fila
    int tmpB, tmpR, tmpG; //Declarar valores temporales
    int size = height * linea; //Tamaño de la imagen
    unsigned char *pixelsN = (unsigned char *)malloc(size); //Nuevo imagen
    size -= 1; //Para que no se tenga que hacer la operacion en cada linea
    //Declaro variables
    int columnaByte;
    int columnaGauss;
    int filaGauss;
    int byte;
    omp_set_num_threads(NUM_THREADS);
    #pragma omp parallel for private(tmpB, tmpR, tmpG)schedule(dynamic)
    for (int i = 0; i <= height - 1; i += 1) //Altura
        for (int j = 0; j <= linea - 1; j += 3) //Anchura
        {
            {
                //Reseto las variables
                tmpB = 0;
                tmpG = 0;
                tmpR = 0;
                //Mascara de Gauss
                for (int s = -2; s <= 2; s++)
                {
                    for (int t = -2; t <= 2; t++)
                    {
                        //Inicializo las varibles
                        columnaByte = j + t * 3; 
                        filaGauss = s + 2;
                        columnaGauss = t + 2;
                        byte = (i + s) * linea + columnaByte;
                        //Parametros para conocer si el byte esta dentro de la imagen
                        if (byte >= 0 && byte <= size && 0 <= columnaByte && columnaByte <= linea - 1)
                            tmpB += mGauss[filaGauss][columnaGauss] * pixels[byte];
                        byte += 1;
                        columnaByte +=1;
                        if (byte >= 0 && byte <= size && 0 <= columnaByte && columnaByte <= linea - 1)
                            tmpG += mGauss[filaGauss][columnaGauss] * pixels[byte];
                        byte += 1;
                        columnaByte +=1;
                        if (byte >= 0 && byte <= size && 0 <= columnaByte && columnaByte <= linea - 1)
                            tmpR += mGauss[filaGauss][columnaGauss] * pixels[byte];
                    }
                }
                //Guardo los valores en la imagen nueva
                tmpB /=w;
                tmpG /=w;
                tmpR /=w;
                pixelsN[(i * linea) + j] = (unsigned char)(tmpB);
                pixelsN[(i * linea) + j + 1] = (unsigned char)(tmpG);
                pixelsN[(i * linea) + j + 2] = (unsigned char)(tmpR);
            }
        }
    free(pixels); //Libero memoria de la antigua imagen
    return pixelsN; //Devuelvo la nueva
}