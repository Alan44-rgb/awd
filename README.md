# Predictive Input Buffer — Ech8 — v4.10.0

Windows, Geometry Dash 2.2081 y Geode 5.11.2.

## Uso

Shift+F12 abre o cierra el menu dentro de GD. Si lo abres durante una partida, el
juego se pausa; cierra el panel y pulsa Reanudar para seguir.

- Ajusta Pulsar y Soltar por separado, siempre en ticks (0 a 32).
- Los botones +/- cambian un tick; los presets permiten 1, 2, 4, 8, 16 o 32.
- 0 desactiva solo esa accion. Activar/Desactivar controla todo el buffer.
- Mostrar/Ocultar oculta el indicador del nivel. Shift+F12 permite volver al menu.
- Como funciona explica el comportamiento y sus limites.

Los ajustes se guardan. Al actualizar, el limite de clics que ya habias elegido
se conserva. El nuevo limite de sueltas empieza en 4 ticks. El ajuste antiguo
release-frames ya no se utiliza.

## Prediccion y mejoras de cube

Al pulsar o soltar, simula un jugador oculto. Si la trayectoria inmediata
colisiona, prueba retrasos de 1 tick hasta el limite y usa una correccion
favorable. Prueba despues un tick adicional para entregar el clic o la suelta
un tick mas tarde. Si esa prueba empeora, es incierta o agota el tiempo,
conserva la correccion ya validada. El total puede ser limite + 1 tick.
Si el input inmediato es seguro, no aplica retraso.

Cube, ball, UFO y robot reconocen el aterrizaje de la accion. La simulacion
suelta el boton al aterrizar para no inventar un segundo salto al mantenerlo
pulsado para siempre. Comprueba unos ticks sobre la superficie antes de darlo
por bueno. Spider tambien puede usar la prediccion basica; ship, wave y swing
usan horizontes adaptados al modo.

Una colision posterior contra el mismo obstaculo no cuenta como mejora. Si un
retraso supera por completo el obstaculo original y ofrece suficiente margen
hasta otro peligro diferente, puede elegirse aunque ese peligro posterior
requiera tu siguiente input. Un candidato no simulable no cancela la busqueda
de los otros retrasos.

El estado del mundo se captura una vez por busqueda y se restaura tras cada
prueba. El tiempo de busqueda es configurable. El indicador y el log muestran
si el input era seguro, el peligro detectado, la correccion, una zona que no se
puede simular o el agotamiento del tiempo de calculo.

## Limites

No es noclip: las muertes del jugador real siguen funcionando. No garantiza
salvar un input ni representa una simulacion completa del nivel. Dual,
platformer, dash y encuentros con portales, orbes y pads pueden pasar sin ayuda.
Los objetos que se mueven, triggers y otros mods que cambian la fisica pueden
reducir la precision. Los horizontes se expresan en pasos de la fisica normal;
CBF se integra conservando la fase de sus subpasos; requiere comprobarse
en niveles reales ademas de las pruebas automaticas.

El comportamiento dentro de un nivel debe probarse con inputs concretos.
Compilar y pasar las pruebas de decisiones no valida la fisica de todos los
niveles ni el aspecto del menu en todas las resoluciones.

## Compilacion y pruebas

GitHub Actions ejecuta primero buffer_core_tests.cpp: escenarios de cube,
sueltas, cambio de obstaculo, presupuesto agotado y 2112 combinaciones de
ventanas de ticks. Usa el mismo selector que el mod. Despues compila el paquete
.geode para Windows. El artefacto se llama Predictive-Input-Buffer-Windows.

Reemplaza la version anterior del mismo mod y desactiva otras recreaciones
para evitar procesar cada input dos veces.

## Registro y destello

Shift+F12 > Registro muestra los ultimos 8 eventos, los totales de clics y
sueltas y el porcentaje de la ultima correccion aplicada. El indicador del
nivel tambien muestra ese porcentaje y el total de correcciones.

El archivo input-log.csv, en la carpeta de datos guardados del mod, conserva
todas las sesiones: sesion, ID de nivel, intento, ID de input, fase, tick,
CLICK/RELEASE, modo, X, Y, porcentaje y retraso en ticks. RECIBIDO identifica
el input fisico; DIRECTO pasa inmediatamente; EN_COLA espera; APLICADO indica
la entrega; CANCELADO explica un input pendiente descartado; MUERTE registra
la muerte real e INICIO un nuevo intento. Los IDs relacionan origen y entrega.
Los contadores y el historial de pantalla abarcan la sesion del nivel; el CSV
se conserva entre partidas. Se puede desactivar el archivo desde los ajustes
Geode del mod. Los inputs del jugador simulado nunca entran en este registro.

Shift+F12 > Destello activa o desactiva el destello blanco. Empieza desactivado,
dura 0.15 segundos y limita los destellos a uno cada 0.25 segundos. Solo aparece
al entregar una correccion predicha independiente, no al detectarla ni si se
cancela. El indicador puede ocultarse y el destello funcionar por separado.
Ni el contador ni el destello certifican haber evitado una muerte: indican una
correccion aplicada basada en la simulacion. No se implementan ticks negativos.

## Integracion con CBF 1.5.0

Se captura el paso nativo antes del hook de CBF y se integra despues de su
division. El reloj suma fracciones de tick, no llamadas a update. Los retrasos
conservan la fase del input; si la entrega cae dentro de un paso, se divide
ese paso y se comprueban las colisiones intermedias. La simulacion usa las
mismas fronteras y el mismo instante de entrega. El aterrizaje estable tambien
se mide en ticks completos. No cambia los ajustes de CBF.

El CSV incluye tick_fraction, native_dt, held, on_ground, jump_buffered y
age_since_correction, y BOTON_REAL al entrar en pushButton/releaseButton.
Estos datos permiten comparar recepcion, entrega y muerte por un solo tick.
Las pruebas cubren 231 combinaciones de fases CBF y retrasos, ademas de las
2112 combinaciones de decisiones. Esto verifica el reloj; la fisica y las
interacciones con otros mods requieren una prueba dentro del nivel.
