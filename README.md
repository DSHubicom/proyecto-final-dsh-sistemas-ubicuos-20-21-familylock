# Proyecto final DSH sistemas ubicuos 2020/21 - FamilyLock

## Estructura de carpetas
Esquemas hardware -> Contiene los archivos fritzing de las conexiones y las imágenes de dichos esquemas. Solo hay dos esquemas porque una de las placas no contiene ningún componente.
buttonBoard -> Contiene el código del dispositivo que corresponde a la pulsera del usuario. Su función es permitir o denegar el acceso a la puerta, además de notificar al usuario de la llegada de una visita.
doorManager -> Contiene el código del dispositivo que se encuentra en la puerta. Su función es detectar al visitante y abrir la puerta en caso de que se permita el acceso.
locationDetector -> Contiene el código del dispositivo que localiza al usuario dentro del hogar. Su función es localizar al usuario e interpertar la intensidad de la señal para determinar la habitación en la que se encuentra.
