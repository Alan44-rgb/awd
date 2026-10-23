# Predictive Input Buffer — Ech8 — v4.9.0

Windows, Geometry Dash 2.2081 y Geode 5.11.2.

## Uso

F6 abre o cierra el menu dentro de GD. Si lo abres durante una partida, el
juego se pausa; cierra el panel y pulsa Reanudar para seguir.

- Ajusta Pulsar y Soltar por separado, siempre en ticks (0 a 32).
- Los botones +/- cambian un tick; los presets permiten 1, 2, 4, 8, 16 o 32.
- 0 desactiva solo esa accion. Activar/Desactivar controla todo el buffer.
- Mostrar/Ocultar oculta el indicador del nivel. F6 permite volver al menu.
- Como funciona explica el comportamiento y sus limites.

Los ajustes se guardan. Al actualizar, el limite de clics que ya habias elegido
se conserva. El nuevo limite de sueltas empieza en 4 ticks. El ajuste antiguo
release-frames ya no se utiliza.

## Prediccion y mejoras de cube

Al pulsar o soltar, simula un jugador oculto. Si la trayectoria inmediata
colisiona, prueba retrasos de 1 tick hasta el limite y usa una correccion
favorable. Si el input inmediato es seguro, no aplica retraso.

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
la sincronizacion con CBF requiere una comprobacion dentro del juego.

El comportamiento dentro de un nivel debe probarse con inputs concretos.
Compilar y pasar las pruebas de decisiones no valida la fisica de todos los
niveles ni el aspecto del menu en todas las resoluciones.

## Compilacion y pruebas

GitHub Actions ejecuta primero buffer_core_tests.cpp: escenarios de cube,
sueltas, cambio de obstaculo, presupuesto agotado y 1056 combinaciones de
ventanas de ticks. Usa el mismo selector que el mod. Despues compila el paquete
.geode para Windows. El artefacto se llama Predictive-Input-Buffer-Windows.

Reemplaza la version anterior del mismo mod y desactiva otras recreaciones
para evitar procesar cada input dos veces.
