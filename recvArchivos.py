import socket 
import time
import os

if __name__ == '__main__': 
	# Define la direccion ip del servidor al que se conectara
	# Es necesario modificarla para conectarse al servidor
	# Poner la direccion del servidor entre las comillas simples
	host = input()
	# Define el puerto en el que se conecta al servidor, debe de ser el mismo configurado en la aplicacion que envia los archivos
	# en este caso, el ejecutable se ejecuta por defecto en el puerto 8080
	port = 8080

	# La aplicacion esta pensada para mantenerse en ejecucion, por lo que todo se encuentra en un ciclo infinito
	# para cerrarla, el usuario debera cerrar la terminal o usar la combinacion Ctrl + Z
	while True:
		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		try:
			# El tiempo maximo, en segundos, para intentar conectarse al servidor
			sock.settimeout(15)
			# Se realiza la conexion, esto ocurre tambien cada que se recibe un archivo
			sock.connect((host, port))
			try:
				# Se recibe el nombre del directorio o los directorios
				folder = sock.recv(100).decode()

				# Se eliminan los caracteres que delimitan cada directorio, si solo es uno, lo guarda como un array
				if folder.find("\\") != -1:
					folder = folder.split("\\")
				elif folder.find("/") != -1:
					folder = folder.split("/")
				else:
					folder = [folder]

				#Si el directorio recibido es "root", quiere decir que no se tiene que crear, se guardara en la misma carpeta del ejecutable
				if not folder.__contains__("root"):
					# Si la ruta aun no existe, la crea
					if not os.path.exists(os.path.join(*folder)):
						os.makedirs(os.path.join(*folder))
				else:
					folder = [""]
				
				# Se recibe el nombre del archivo
				file_name = sock.recv(100).decode()

				# Se recibe el tamaño del archivo
				# Esto es necesario para saber la cantidad de datos a recibir hasta que el archivo
				# Este completo
				file_size = sock.recv(100).decode()

				# Se crea y abre el archivo.
				with open(os.path.join(*folder, file_name), "wb") as file:
					# Se le informa al usuario el nombre del archivo que se esta recibiendo
					print("Recibiendo archivo: " , file_name)
					# Se contabiliza la cantidad de bytes recibidos, para comparar con el total a recibir
					bytes_received = 0

					# Mientras los bytes recibidos sean menores a lo que se debe recibir
					# El programa espera recibir 1024 bytes de datos y los escribe en el archivo
					while bytes_received < int(file_size):
						try:
							data = sock.recv(1024)
							file.write(data)
							bytes_received += len(data)
						# Si llega a existir un error, por ejemplo, que el servidor deje de funcionar a mitad de una transferencia
						# Se le notifica al usuario que el archivo no se guardo correctamente
						except socket.timeout:
							print("Tiempo de espera alcanzado, el archivo no se ha guardado correctamente")
							break  # Salir del bucle en caso de error de socket
						except socket.error as e:
							print("Error de socket:", e , "\nEl archivo no se ha guardado correctamente")
							break  # Salir del bucle en caso de error de socket
					
					# Al recibir el archivo, se cierra el socket, se le informa al usuario el archivo que se guardo
					# Y se suspende el programa por 10 segundos
					# Esto ayuda a que no se reciban continuamente datos, que podria ocasionar, a su vez, que estos sean incorrectos
					sock.close()
					print(file_name , " guardado")
					time.sleep(10)
			except socket.error as e:
				print("Error de socket:", e)
				sock.close()
				continue  # Salir del bucle en caso de error de socket
			except:
				sock.close()
				continue
		except:
			continue