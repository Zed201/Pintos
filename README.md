# pintOS
<details>
<summary>Mudanças no ./src </summary>

- Para facilitar o export no src/utils depois de usar `make` usar `export PATH=$PATH:$(pwd)`, se nao quiser colocar no .bashrc/zshrc
Mas no src/threads(adicionado no src/threads/Makefile os comandos para executar o pintos mais facil. com GUI ou sem)
não precisa nem no `make check` do src/threads/build (se der algum erro oque foi modificado ta no src/tests/Make.tests:58)

- Para funcionar no Arch Linux modifiquei o src/Makefile.build:93 para ele reduzir o tamanho do loader.bin

- Adicionado logica para ir executando os testes em especifico, no caso do threads, basicamente usa `make ngui/gui TEST=<nome_do_test>`
</details>


### Objetivos:
- [ ] Alarm Clock
- [ ] Advanced Scheduler
### Detalhamentos:
<details>
    <summary>Alarm</summary>
    Reimplementar ` timer_sleep()` no `device/time.c` que ta originalmente implementado com 'busy wait',
    que fica chamando `thread_yiel()` enquanto o tempo não tiver passado
    ideia:
    Adicionar a verificação ao scheduler, adicionando um campo na struct de threads para indicar o tempo que ela deve ficar parada se tiver com status de blocking
</details>
<details>
    <summary>Scheduler</summary>
    Implementar uma mlfqs, na documentação oficial ele diz para dar opção de ter o mlfqs ou o por prioridade, então deveria implementar os dois(verificar!!); Com o mlfqs as prioridades definidas pelas threads devem ser ignoradas e controladas pelo escalonador
    
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
O kernel não suporta float nem double, então a doc recomenda usar o formato de 17.14, 17 bits para a parte inteira e 14 para a fracionária; Para transformar reais nesses tipos é só multiplicar por 2^Q, onde Q é o numero de bits separado para a parte fracionária, e truncar para int, a documentação recomenda usar isso no recent cpu time e no avg, nesse caso então vai basicamente simular operações em float usando inteiros(ver [aqui](https://www.scs.stanford.edu/23wi-cs212/pintos/pintos_7.html) como as operações podem ser feitas
</details>

<details>
    <summary>Tests</summary>

    
- [X] PASS tests/threads/alarm-single
- [X] PASS tests/threads/alarm-multiple
- [X] PASS tests/threads/alarm-simultaneous
- [ ] FAIL tests/threads/alarm-priority
- [X] PASS tests/threads/alarm-zero
- [X] PASS tests/threads/alarm-negative
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
- [X] PASS tests/threads/mlfqs-load-1
- [X] PASS tests/threads/mlfqs-load-60
- [X] PASS tests/threads/mlfqs-load-avg
- [X] PASS tests/threads/mlfqs-recent-1
- [X] PASS tests/threads/mlfqs-fair-2
- [X] PASS tests/threads/mlfqs-fair-20
- [ ] FAIL tests/threads/mlfqs-nice-2
- [ ] FAIL tests/threads/mlfqs-nice-10
- [ ] FAIL tests/threads/mlfqs-block


</details>


Equipe:
| [<img src="https://avatars.githubusercontent.com/u/96800329?v=4" width=115><br><sub>Luiz Gustavo</sub>](https://github.com/Zed201) |  [<img src="https://avatars.githubusercontent.com/u/101292201?v=4" width=115><br><sub>Heitor Melo</sub>](https://github.com/HeitorMelo)  | [<img src="https://avatars.githubusercontent.com/u/129231720?v=4" width=115><br><sub>Henrique César</sub>](https://github.com/SapoSopa) | [<img src="https://avatars.githubusercontent.com/u/136932932?v=4" width=115><br><sub>Emanuelle Giovanna</sub>](https://github.com/manugio3)
| :---: | :---: | :--:| :--:|


