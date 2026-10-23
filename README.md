# Predictive Input Buffer — Ech8

Buffer adaptativo para Windows, GD 2.2081 y Geode 5.11.2.

## Que hace

Al pulsar o soltar el salto, simula un jugador oculto con ese input. Si la
trayectoria inmediata choca, prueba retrasos de 1 tick hasta el limite elegido.
Usa el menor retraso cuya trayectoria completa no choque dentro del horizonte.
Si el input inmediato es seguro, no encuentra una correccion o agota su tiempo
de calculo, el input pasa inmediatamente. No altera la muerte del jugador real.

F6 cambia el limite: 1, 2, 4, 8, 16, OFF. Comienza con 2 ticks; tambien se puede
cambiar en los ajustes de Geode. El limite no es una latencia fija. Los clics y
sueltas que llegan durante una correccion conservan su orden y duracion.

## Limites de la prediccion

La simulacion contempla geometria y peligros en su posicion actual. No simula
el nivel entero ni futuras acciones del usuario; no garantiza evitar la muerte.
Para evitar decisiones basadas en estados incompletos, el input pasa sin ayuda
en dual, platformer, spider, dash o si la trayectoria toca portales, orbes, pads
y otros objetos interactivos. Los obstaculos que se mueven pueden causar
predicciones incorrectas. No se ha validado la sincronizacion con CBF u otros
mods que cambian la fisica o el momento de los inputs.

La prediccion de suelo es de 120 ticks y la de vuelo de 32 por defecto. Se pueden
ajustar junto con el presupuesto de calculo. La consola de Geode muestra cada
correccion aplicada; ese mensaje indica una prediccion favorable, no prueba que
el jugador haya sobrevivido en la partida real.

## Compilacion

GitHub Actions > Build Geode Mod. Descargar el artefacto
Predictive-Input-Buffer-Windows y extraer `ech8.predictive-input-buffer.geode`.
Con el juego cerrado, reemplazar la version anterior en `geode/mods`.
Desactivar otras copias de este buffer para que dos mods no procesen el mismo
input. F6 muestra el limite activo. El juego debe probarse con un nivel y unas
pulsaciones concretos para evaluar la calidad de las predicciones.

## Cambios respecto a v4.7

- Se sustituye el retraso fijo por prediccion de trayectoria.
- Las sueltas tambien se evalúan.
- Las simulaciones usan un jugador separado y restauran GJGameState.
- Los reinicios descartan los inputs pendientes; desactivar reconcilia el boton.
- El identificador usa minusculas; se mantiene el autor Ech8.
- La compilacion usa la version estable declarada en mod.json.

Referencias tecnicas consultadas: bindings oficiales de geode-sdk/bindings
para 2.2081 y el enfoque de jugador fantasma mostrado en el proyecto publico
fraudian/Predictive-Input-Buffer. Esta implementacion reemplaza el main.cpp
simplificado que habia en awd.
