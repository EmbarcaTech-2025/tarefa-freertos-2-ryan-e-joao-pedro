
# Tarefa: Roteiro de FreeRTOS #2 - EmbarcaTech 2025

Autor: **João Pedro Lacerda e Ryan Micael**

Curso: Residência Tecnológica em Sistemas Embarcados

Instituição: EmbarcaTech - HBr

Brasília, 25/06 de 2025

---


# Projeto RTOS: Jogo de Reflexo e Sincronização na BitDogLab/Pico

## 1. Resumo do Projeto

Este repositório contém o código-fonte de um jogo de reflexo desenvolvido para a placa BitDogLab (baseada no Raspberry Pi Pico). O projeto demonstra conceitos avançados de sistemas de tempo real (RTOS) utilizando **FreeRTOS** para gerenciar tarefas concorrentes e sincronizar o acesso a recursos compartilhados de forma segura e eficiente.

## 2. A Ideia do Jogo

O objetivo do jogador é testar seu tempo de reação e sua capacidade de concentração. Ele deve apertar um botão no momento exato em que um padrão específico (um 'X') aparece na matriz de LED, enquanto ignora uma série de distrações visuais e sonoras que rodam em paralelo para tentar enganá-lo.

O projeto foi desenhado para ser uma demonstração prática de problemas de concorrência e como um RTOS pode resolvê-los de forma elegante.

## 3. Periféricos Utilizados

O projeto faz uso dos seguintes componentes de hardware da placa:

* **Matriz de LED 5x5 (WS2812B):** É o periférico central. Usada tanto para exibir o sinal "VAI!" (o padrão 'X') quanto para exibir os padrões de distração visual.
* **Display OLED 128x64 (SSD1306):** Exibe as instruções do jogo, o estado atual ("Prepare-se...") e o resultado final de cada rodada (o tempo de reação do jogador).
* **Buzzer Passivo:** Utilizado pela tarefa de distração sonora para emitir bipes e notas aleatórias, aumentando o desafio.
* **Botão de Usuário (Push-button):** É o meio de entrada do jogador para registrar sua reação.

## 4. Dinâmica de Funcionamento

Uma rodada do jogo segue os seguintes passos:

1.  **Preparação:** O jogo começa exibindo a mensagem "Prepare-se!" no display OLED.
2.  **Distrações:** Imediatamente, tarefas de distração são ativadas em paralelo. A matriz de LED começa a piscar padrões visuais aleatórios (quadrados, círculos, etc.) e o buzzer emite sons em intervalos irregulares.
3.  **O Sinal:** Em um momento aleatório (entre 2 a 7 segundos), as distrações visuais param instantaneamente e o padrão "VAI!" (um 'X' verde) é exibido na matriz de LED.
4.  **Reação:** O jogador deve pressionar o botão o mais rápido possível ao reconhecer o padrão 'X'.
5.  **Resultado:** O tempo de reação (em milissegundos) é calculado e exibido no display. Se o jogador for muito lento ou não reagir, uma mensagem correspondente é mostrada.
6.  **Reinício:** Após uma breve pausa, o ciclo recomeça para uma nova rodada.

## 5. O Papel do FreeRTOS (O Coração do Projeto)

A utilização de um RTOS não é apenas um detalhe, mas sim a base para que o projeto funcione de forma confiável. A complexidade reside em fazer várias coisas "ao mesmo tempo" de forma organizada, especialmente quando múltiplas tarefas querem usar o mesmo recurso.

#### Tarefas Concorrentes

O sistema é dividido em 4 tarefas independentes, cada uma com uma responsabilidade:

* `game_logic_task`: A tarefa "cérebro" do projeto. Controla o fluxo principal do jogo, decide quando mostrar o sinal "VAI!" e calcula o tempo.
* `input_task`: Uma tarefa de alta prioridade dedicada exclusivamente a monitorar o botão. Isso garante que a reação do jogador seja capturada com o mínimo de latência.
* `visual_distraction_task`: Roda em segundo plano, tentando constantemente exibir padrões aleatórios na matriz de LED para confundir o jogador.
* `audio_distraction_task`: Também em segundo plano, toca sons aleatórios no buzzer.

#### Sincronização e Acesso a Recursos

O maior desafio é que a `game_logic_task` e a `visual_distraction_task` competem pela matriz de LED. Para resolver isso, usamos dois mecanismos de sincronização do FreeRTOS:

* **Mutex (`g_criticalMomentMutex`):** Funciona como uma "chave de acesso" para a matriz de LED.
    * A tarefa do jogo **pega a chave** antes de mostrar o padrão 'X'.
    * A tarefa de distração visual só pode desenhar na matriz **se a chave estiver livre**.
    * Isso garante que uma distração nunca irá sobrescrever ou aparecer ao mesmo tempo que o sinal principal, resolvendo o problema de condição de corrida.

* **Semáforo Binário (`g_reactionSignalSemaphore`):** Funciona como um sinalizador ultraeficiente.
    * A `game_logic_task` pausa e fica esperando por este sinal após mostrar o 'X'.
    * A `input_task`, ao detectar o toque no botão, simplesmente "levanta a bandeira" (libera o semáforo).
    * A tarefa do jogo, que estava bloqueada, acorda instantaneamente para registrar o tempo. Isso é muito mais eficiente e seguro do que usar variáveis globais voláteis.

---

## 📜 Licença
GNU GPL-3.0.
