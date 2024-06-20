# FileSend
Estas aplicaciónes fueron creadas principalmente para uso personal, por lo que su uso puede no ser muy intuitivo de cara al usuario.
<br>El servidor envia los archivos que se encuentren en la carpeta definida por el usuario, la cual se escaneara cada cierto tiempo para identificar cambios en los archivos.
<br>Por su parte, el cliente se conectará al servidor y esperará a recibir datos. Los archivos recibidos se guardan con la misma estructura de carpetas que se tiene en el servidor.
<br>En conjunto, estas aplicaciones permiten transferir archivos de una computadora a otra.

# Intrucciones de uso

En la seccion Releases se encuentran 2 archivos, el ejecutable Servidor y el archivo de Python Cliente. La aplicación servidor será<br>la que enviará los archivos a la computadora que este ejecutando el script cliente. Por el momento, el servidor solo es compatible con el sistema operativo Windows.
<br>Para usar el servidor, es necesario:
* Descargar el ejecutable Cliente-Windows.exe
* Ubicarlo fuera de la carpeta que se desea transferir (Por ejemplo, si se desea escanear la carpeta F:\Videos, el ejecutable debera estar ubicado en el disco F:\)
* En el Firewall, permitir la aplicación manualmente (De no hacerlo, el servidor no se podra comunicar con otra computadora)
* Hacer doble clic en el archivo e ingresar los datos que se solicitan (Carpeta a escanear e intervalo entre cada escaneo)

Para el cliente, se deberá tener instalado Python 3, una vez instalado:<br>
* Descargar el archivo Cliente.py
* Ubicarlo DENTRO de la carpeta en donde se guardarán los archivos
* Abrir la terminal en esta misma carpeta
* Dependiendo de la instalación de python, se ejecutará con: <i>py Cliente.py</i> ó <i>python3 Cliente.py</i>
* Introducir la dirección IP del servidor.

# Planes de actualización

Por el momento solo esta planeado agregar la aplicación servidor en Linux. Probablemente, una opción de descarga para la aplicación cliente que no requiera Python.
