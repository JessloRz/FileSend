#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <map>
#include <winsock2.h>
#include <chrono>
#include <thread>
#include <ws2tcpip.h>

using namespace std;

// Variables que se utilizan dentro de las funciones
// la aplicacion esta principalmente pensada para enviar videos,
// por ello el nombre de la variable en este momento
ifstream video;
// Mediante sockets es necesario enviar datos poco a poco
// en esta variable se guardaran los datos de los archivos en partes de 1024 bytes
char fileData[1024];

// "Crea" las funciones que se utilizaran, estan definidas hasta el final
int sendFiles(SOCKET* client, const filesystem::__cxx11::directory_entry& file, string* folder);

chrono::seconds lastModified(const filesystem::__cxx11::directory_entry& filePath);

int main()
{
    // Inicializa los socket
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2,2);
    WSAStartup(wVersionRequested, &wsaData);
    SOCKET client;
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;

    // Crea un map en donde se guardara la ruta y el tamaño de los archivos ya transferidos
    // estos se encuentran en el archivo ignore.txt
    map<string, string> alreadyTransfered;
    string line;
    
    int sendedFiles;
    string fileFolder;

    // La ubicacion del archivo ignore debera ser la misma ubicacion del ejecutable
    // Si no existe el archivo, se da por hecho que no hay archivos a ignorar, es decir
    // No se han transferido archivos hasta el momento
    fstream ignoreFile;
    string ignoreFile_Path = ".\\ignore.txt";

    // Define el protocolo, el puerto y las redes a utilizar para el socket
    // En este caso se usa el protocolo TCP, el puerto 8080 y se permiten conexiones en todas las redes
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(listenSocket,(SOCKADDR*)&server, sizeof(server));

    // Abre el archivo ignore solo como lectura
    ignoreFile.open(ignoreFile_Path, ios::in);
    // Se posiciona al inicio
    ignoreFile.seekg(0);

    // Recorre el archivo mientras haya lineas
    while (getline(ignoreFile, line))
    {
        // Guarda en el map la ruta del archivo como clave y el tamaño como valor
        // En el archivo txt estos valores se encuentran separados por coma
        alreadyTransfered[line.substr(0, line.find(","))] = line.substr(line.find(",") + 1, line.size());
    }
    // Cierra el archivo
    ignoreFile.close();


    unsigned int minutes;
    string inputFolder;

    // Solicita el nombre del directorio que se va a escanear
    // Debe ser un directorio que se encuentre en la misma carpeta que el ejecutable
    cout << "Escribe el nombre del directorio que deseas escanear (debe estar en la misma ubicacion del ejecutable): " << endl;
    // El valor que se ingrese se guarda en la variable input folder
    cin >> inputFolder;
    filesystem::path scanFolder = filesystem::current_path() / inputFolder / "";

    // Se revisa si el directorio ingresado es valido, es decir, existe
    while ((inputFolder.find("\\") != -1 or inputFolder.find("/") != -1) or !filesystem::is_directory(inputFolder))
    {
        // Si no es valido, continua solicitando un directorio valido
        cout << "Directorio no valido, ingrese el nombre de un directorio existente en la carpeta actual:" << endl;
        cin >> inputFolder;
    }
    cout << endl;

    // Solicita el intervalo en el que se volveran a revisar cambios en la carpeta
    cout << "Cada cuantos minutos desea que se revise la carpeta?" << endl;
    cin >> minutes;
    while (cin.fail())
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Favor de introducir solo numeros" << endl;
        cout << "Cada cuantos minutos desea que se revise la carpeta?" << endl;
        cin >> minutes;
    }
    cout << endl;
    

    // Determina cuantas conexiones simultaneas se aceptan
    // En este caso solo se acepta 1 conexion
    listen(listenSocket, 1);

    // Todo el programa se ejecuta en un bucle.
    // Actualmente la unica forma de detenerlo es cerrar el programa
    // Una vez finaliza el escaneo, hay un sleep dependiendo del valor en minutos ingresado por el usuario
    // El programa esta pensado para que se ejecute en un segundo plano
    while (true)
    {
        sendedFiles = 0;
        cout << "Iniciando escaneo" << endl << endl;
        // Itera en los archivos y carpetas ubicados en el folder que se ingreso
        // Este ciclo no "entra" en los demas directorios que se encuentren dentro
        for (const auto& entry : filesystem::directory_iterator(scanFolder))
        {
            // Revisa si entry es un directorio
            if (entry.is_directory())
            {
                // Si entry es un directorio, itera en este. El siguiente for recorre todos los archivos y carpetas
                // Revisando de manera "profunda" el directorio
                for (const auto& file : filesystem::recursive_directory_iterator(entry.path()))
                {
                    // Si la variable file no indica un directorio, procede a transferirlo
                    if (!file.is_directory())
                    {
                        // Se obtiene el "path" en el que se encuentra el archivo
                        // Mantiene solo los directorios dentro del cual se escanea, por ejemplo:
                        // Si el archivo se encuentra en F:\\videos\\Peliculas\\Drama y folder a escanear es videos, en la variable fileFolder se guarda Peliculas\\Drama
                        fileFolder = file.path().parent_path().string().replace(0, scanFolder.string().size(), "");

                        // Revisa si el archivo no se ha transferido, si ya se transfirio revisa que el tamaño del archivo sea diferente al guardado en ignore.txt, que el tamaño del archivo sea mayor a 0
                        // y que el archivo no haya sido modificado en los ultimos 10 segundos, esto ultimo para evitar transferir archivos que no han sido completamente creados
                        if (alreadyTransfered[file.path().string().c_str()] != to_string(file.file_size()) and file.file_size() > 0 and lastModified(file) > chrono::seconds(10))
                        {
                            // En este punto, el programa espera una conexion por parte del programa cliente, mientras esta conexion no se realice, el programa no avanza.
                            // Esto se pide cada que se envie un archivo, de esta manera, el cliente recibe los datos correctos de cada archivo
                            cout << "Esperando solicitud de conexion" << endl;
                            client = accept(listenSocket, NULL, NULL);
                            cout << "Conexion establecida" << endl << endl;
                            try
                            {
                                // Se pasa el control a la funcion sendFiles, pasando el cliente, el archivo y las carpetas
                                // Si se regresa un -1, quiere decir que ocurrio un error durante la funcion
                                if (sendFiles(&client, file, &fileFolder) == -1)
                                {
                                    cerr << "Ocurrio un error al enviar el archivo: " << file.path().filename().string() << endl << endl;
                                }
                                // En caso de que el archivo se haya enviado correctamente, se guarda la informacion del archivo en el map
                                // y en el archivo ignore
                                else
                                {
                                    alreadyTransfered[file.path().string().c_str()] = to_string(file.file_size());

                                    ignoreFile.open(ignoreFile_Path, ios::out | ios::trunc);
                                    for (const auto& pair : alreadyTransfered)
                                    {
                                        ignoreFile << pair.first << "," << pair.second << endl;
                                    }
                                    ignoreFile.flush();
                                    ignoreFile.close();

                                    cout << "Archivo enviado." << endl << endl;
                                    sendedFiles++;
                                }
                                
                            }
                            catch(const std::exception& e)
                            {
                                std::cerr << e.what() << endl << endl;
                                continue;
                            }
                            // Cierra la conexion con el cliente para espera una nueva
                            closesocket(client);
                        }
                        else
                        {
                            continue;
                        }
                    }
                    // Despues de cada procesamiento de archivo, se detiene el programa por 5 milisegundos para evitar un uso contante de CPU al escanear archivos ya enviados
                    this_thread::sleep_for(chrono::milliseconds(5));
                }
            }
            // Este else solo maneja archivos fuera de cualquier carpeta, dentro de la carpeta escaneada, su funcionamiento es el mismo
            else
            {
                // en este caso, el directorio en el que se encuentra el archivo se guarda como root
                // esto se controla desde el cliente para determinar que no se tiene que crear una carpeta para este archivo
                fileFolder = "root";
                if (alreadyTransfered[entry.path().string().c_str()] != to_string(entry.file_size()) and entry.file_size() > 0 and lastModified(entry) > chrono::seconds(10))
                {
                    cout << "Esperando solicitud de conexion" << endl;
                    client = accept(listenSocket, NULL, NULL);
                    cout << "Conexion establecida" << endl << endl;
                    try
                    {
                        if (sendFiles(&client, entry, &fileFolder) == -1)
                        {
                            cerr << "Ocurrio un error al enviar el archivo: " << entry.path().filename().string() << endl << endl;
                        }
                        else
                        {
                            alreadyTransfered[entry.path().string().c_str()] = to_string(entry.file_size());

                            ignoreFile.open(ignoreFile_Path, ios::out | ios::trunc);
                            for (const auto& pair : alreadyTransfered)
                            {
                                ignoreFile << pair.first << "," << pair.second << endl;
                            }
                            ignoreFile.flush();
                            ignoreFile.close();

                            cout << "Archivo enviado." << endl << endl;
                            sendedFiles++;
                        }
                        
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << endl << endl;
                        continue;
                    }
                    closesocket(client);
                }
                else
                {
                    continue;
                }
            }
            this_thread::sleep_for(chrono::milliseconds(5));
        }
        
        // Al finalizar de escanear el directorio se le informa al usuario
        // Se muestra la cantidad de archivos enviados exitosamente y el tiempo en el que se volvera a escanear en busca de cambios
        cout << "Escaneo finalizado" << endl;
        cout << "Archivos enviados: " << sendedFiles << endl;
        cout << "Se escaneara de nuevo en " << minutes << " minutos" << endl << endl;

        // El programa se suspende por la cantidad de tiempo que ingreso el usuario
        this_thread::sleep_for(chrono::minutes(minutes));
    }
}

int sendFiles(SOCKET* client, const filesystem::__cxx11::directory_entry& file, string* folder)
{
    try
    {
        // se intenta enviar el directorio en el que se encuentra el archivo
        // si falla se regresa un -1 y se cierra la conexion con el cliente
        // Enviar el directorio sirve para que, en el cliente, la estructura de la carpeta quede de la misma manera
        // que en el servidor
        if (send(*client, (*folder).c_str(), (*folder).size(), 0) == -1)
        {
            closesocket(*client);
            return -1;
        }
        // se suspende el programa por 2 segundos, esto ayuda a que el cliente reciba cada dato correctamente
        // sin el sleep, pueden haber ocasiones en que se juntan los datos enviados
        this_thread::sleep_for(chrono::seconds(2));
        

        // Se le notifica al usuario que archivo se esta enviando
        cout << "Enviando archivo: " << file.path().filename().string() << endl;

        // Se intenta enviar el nombre del archivo
        if (send(*client, file.path().filename().string().c_str(), file.path().filename().string().size(), 0) == -1)
        {
            closesocket(*client);
            return -1;
        }
        this_thread::sleep_for(chrono::seconds(2));
        
        // Se intenta enviar el peso del archivo
        // esto es necesario para que tanto el cliente como el servidor sepan la cantidad total de datos que se deben enviar
        if (send(*client, to_string(file.file_size()).c_str(), to_string(file.file_size()).size(), 0) == -1)
        {
            closesocket(*client);
            return -1;
        }
        this_thread::sleep_for(chrono::seconds(2));

        // en la variable c se lleva el conteo de la cantidad de datos enviados
        int c = 0;
        // Se abre el archivo en modo binario, esto es importante para la correcta lectura y envio de los datos
        video.open(file.path(), std::ios::binary);
        // Se posiciona la lectura del archivo al inicio
        video.seekg(0, ios::beg);

        // Se lee del archivo 1024bytes cada vez, hasta leerlo por completo
        while (c < file.file_size())
        {
            // Utilizando memset, se eliminan los datos ya existentes en memoria
            // Es necesario debido a que, al final de la lectura, no necesariamente se llenaran los 1024 bytes, por lo que
            // si no se eliminan, el final del archivo se enviara "corrompido"
            memset(fileData, 0, sizeof(fileData));
            video.read(fileData, sizeof(fileData));
            // Solo cuando la lectura del archivo sea mayor a 0 se intentara enviar
            if (video.gcount() > 0)
            {
                if (send(*client, fileData, video.gcount(), 0) == -1)
                {
                    closesocket(*client);
                    return -1;
                }
                c += video.gcount();
            }
        }
        video.close();
        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << endl << endl;
        return -1;
    }
}

// Esta funcion regresa el tiempo en segundos desde que el archivo fue modificado
// con el fin de evitar enviar archivos que aun no se terminan de crear, por ejemplo, debido a una descarga en curso o a una conversion de video sin finalizar
chrono::seconds lastModified(const filesystem::__cxx11::directory_entry& file)
{
    // Se obtiene la hora actual
    auto now = chrono::system_clock::now();
    // Se obtiene la hora de la ultima modificacion al archivo
    auto lastWriteTime = filesystem::last_write_time(file.path());
    // Se convierte la hora de la modificacion para poder obtener la diferencia de tiempo
    auto cast = chrono::time_point_cast<chrono::system_clock::duration>(lastWriteTime - filesystem::file_time_type::clock::now() + now);
    auto difference = now - cast;
    // se regresa convertido a segundos la diferencia
    return chrono::duration_cast<chrono::seconds>(difference);
}