# Introduzindo PintOS
<details>
<summary>Mudanças no ./src </summary>

- Para facilitar o export no src/utils depois de usar `make` usar `export PATH=$PATH:$(pwd)`, se nao quiser colocar no .bashrc/zshrc
Mas no src/threads(adicionado no src/threads/Makefile os comandos para executar o pintos mais facil. com GUI ou sem)
não precisa nem no `make check` do src/threads/build (se der algum erro oque foi modificado ta no src/tests/Make.tests:58);

- Para funcionar no Arch Linux modifiquei o src/Makefile.build:93 para ele reduzir o tamanho do loader.bin;

- Adicionado logica para ir executando os testes em especifico, no caso do threads, basicamente usa `make ngui/gui TEST=<nome_do_test>`;
</details>

<details>
<summary>Recursos necessários </summary>

1. 
   <details>
   <summary>Make </summary>
   
   - O make serve para ajudar nossa vida. Não há uma obrigatoriedade de usá-lo, mas iremos;
   - Caso necessite instalá-lo, utilize o comando `sudo apt install make`;
   </details>

2.
   <details>
   <summary>GCC e GDB </summary>
   
   - Compilador e Depurador para C;
   - Caso necessite instalá-los, utilize os comandos `sudo apt install gcc` e `sudo apt install gdb`;
   </details>

3. 
   <details>
   <summary>Qemu </summary>
   
   - Recurso necessário para executar o sistema nos casos testes;
   - Caso necessite instalá-lo, utilize o comando `sudo apt install qemu-system-i386`;
   </details>

4. 
   <details>
   <summary>Boch </summary>
   
   - Alternativa mais rápida ao Qemu, entretanto não utilizaremos ele;
   - Se desejar saber como fazê-lo funcionar, acesse `https://web.stanford.edu/class/cs140/projects/pintos/pintos_12.html#SEC167`;
   </details>

</details>


<details>
<summary>WSL </summary>

- Caso opte por não fazer em sua máquina com Linux ou não possua permissão para baixar algum recurso no computador, siga o tutorial oficial oficial `https://learn.microsoft.com/pt-br/windows/wsl/install`;
</details>

<details>
<summary>Tutorial de comandos </summary>

- Uma vez que todos os recursos já estiverem instalados e sua branch desse repositório devidamente clonado, basta executar alguns `make`antes de começar a modificar o arquivo;
- Primeiramente, pelo terminal, vá até a pasta `src/utils` do repositório clonado e execute `make`;
- Em seguida, vá para a pasta `src/threads` e execute `make`;
- Então, na pasta `src/threads/build` você já pode executar `make check`, que executará, por padrão das modificações, o qemu de todos os testes;
- Caso deseje executar novamente, lembre-se de dar `make clean` antes de usar o proximo `make check`;
- Existe o comando `make check VERBOSE=1` fará com que tudo seja executado de maneira mais limpa em que cada teste aparecerá no terminal apenas durante sua execução;
- Você também pode executar o `make ngui TEST=nome_do_teste` na pasta `src/threads` após o make para executar apenas um teste específico;
- Todos os testes executados geram alguns arquivos relatando a saida e se passou ou não, que ficam na pasta `src/threads/build/tests`;
</details>

<details>
<summary>Observações </summary>

- Todas as operações foram verificadas também no WSL;
- É possivel executar os comandos do Boch normalmente, porém se você não tiver ajustado ele, os resultados dos testes ficarão 27 de 27 falhas (testado no WSL);
</details>

### Objetivos:
- [ ] Alarm Clock;
- [ ] Advanced Scheduler;
### Detalhamentos:
<details>
    <summary>Alarm</summary>
    Reimplementar ` timer_sleep()` no `device/time.c` que ta originalmente implementado com 'busy wait',
    que fica chamando `thread_yield()` enquanto o tempo não tiver passado
    ideia:
    Adicionar a verificação ao scheduler, adicionando um campo na struct de threads para indicar o tempo que ela deve ficar parada se tiver com status de blocking;
</details>
<details>
    <summary>Scheduler</summary>
    Implementar uma mlfqs, na documentação oficial ele diz para dar opção de ter o mlfqs ou o por prioridade, então deveria implementar os dois(verificar!!); Com o mlfqs as prioridades definidas pelas threads devem ser ignoradas e controladas pelo escalonador;
    
   [Fila esquema](https://www.google.com/url?sa=i&url=https://medium.com/@francescofranco_39234/multilevel-feedback-queue-3ae862436a95&psig=AOvVaw0uPvTNvKvDx0bKwYGvKyn_&ust=1718223750727000&source=images&cd=vfe&opi=89978449&ved=0CBIQjRxqFwoTCLD727Sw1IYDFQAAAAAdAAAAABAI)

Segundo o apêndice que fala do scheduler basicamente temos de implementar o conceito de avg_load, thread_nice e o cpu_recent_time;
O avg_load basicamente é a carga média do sistema levando em conta a quantidade de threads em ready_list, sem incluir a thread ociosa:

```math
avg = (\frac{59}{60}) * avg + (\frac{1}{60}) * (tamanho-da-ready-list)
```
O cpu recent time basicamente é uma média móvel exponencial, específica de cada thread e que começa em 0, que serve como peso na hora de calcular a prioridade, basicamente ele vai considerar uma função exponencial em que quanto mais o tempo passa os cpu time antigos tenham pesos menores e os mais recentes os pesos maiores, todas as threads devem ter seu recent time recalculados 1 vez por segundo(timer_ticks() % TIMER_FREQ == 0) usando:
```math
CpuTime = ( \frac{2 * avg}{2 * avg + 1} * CpuTime + nice) * 100
```

O nice é específico de cada thread, tem funções para implementar e ele funcionar direito com o resto, e ele deve estar entre -20 e 20, ele vai servir para calcular a prioridade, quanto mais positivo, menor a prioridade, que vai ser calculada usando o recent_time(apenas se ele mudar) para mudar a thread de fila na mlfq, usando a formula:
```math
p = floor(PriMax - (\frac{RecentCpuTime}{4}) - (nice * 2))
```
##### Pontos Flutuantes
O kernel não suporta float nem double, então a doc recomenda usar o formato de 17.14, 17 bits para a parte inteira e 14 para a fracionária; Para transformar reais nesses tipos é só multiplicar por 2^Q, onde Q é o numero de bits separado para a parte fracionária, e truncar para int, a documentação recomenda usar isso no recent cpu time e no avg, nesse caso então vai basicamente simular operações em float usando inteiros(ver [aqui](https://www.scs.stanford.edu/23wi-cs212/pintos/pintos_7.html) como as operações podem ser feitas;
</details>

<details>
    <summary>Tests</summary>


- [ ] FAIL tests/threads/alarm-single
- [ ] FAIL tests/threads/alarm-multiple
- [ ] FAIL tests/threads/alarm-simultaneous
- [ ] FAIL tests/threads/alarm-priority
- [ ] pass tests/threads/alarm-zero
- [ ] pass tests/threads/alarm-negative
- [ ] FAIL tests/threads/priority-change
- [ ] FAIL tests/threads/priority-donate-one
- [ ] FAIL tests/threads/priority-donate-multiple
- [ ] FAIL tests/threads/priority-donate-multiple2
- [ ] FAIL tests/threads/priority-donate-nest
- [ ] FAIL tests/threads/priority-donate-sema
- [ ] FAIL tests/threads/priority-donate-lower
- [ ] FAIL tests/threads/priority-fifo
- [ ] FAIL tests/threads/priority-preempt
- [ ] FAIL tests/threads/priority-sema
- [ ] FAIL tests/threads/priority-condvar
- [ ] FAIL tests/threads/priority-donate-chain
- [ ] FAIL tests/threads/mlfqs-load-1
- [ ] FAIL tests/threads/mlfqs-load-60
- [ ] FAIL tests/threads/mlfqs-load-avg
- [ ] FAIL tests/threads/mlfqs-recent-1
- [ ] FAIL tests/threads/mlfqs-fair-2
- [ ] FAIL tests/threads/mlfqs-fair-20
- [ ] FAIL tests/threads/mlfqs-nice-2
- [ ] FAIL tests/threads/mlfqs-nice-10
- [ ] FAIL tests/threads/mlfqs-block


</details>
<details>

