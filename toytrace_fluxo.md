# toytrace — Fluxo do Projeto

`toytrace` é um monitor didático de chamadas de sistema (syscalls) para Linux x86\_64, inspirado no `strace`. O projeto usa `ptrace` para interceptar e exibir as syscalls feitas por um processo monitorado.

---

## Estrutura de Arquivos

```
toytrace/
├── include/
│   ├── cli.h               # Estrutura trace_options e assinaturas da CLI
│   ├── syscall_event.h     # Definição de struct syscall_event
│   ├── syscall_names.h     # Tradução número → nome de syscall
│   ├── student_api.h       # API pública das funções dos estudantes
│   ├── trace_helpers.h     # Helper read_child_string()
│   └── trace_runtime.h     # Assinatura de trace_program()
├── src/
│   ├── main.c              # Ponto de entrada; define o observer callback
│   ├── cli.c               # Parse de argumentos (parse_args)
│   ├── syscall_names.c     # Tabela número → nome de syscall
│   ├── trace_helpers.c     # Implementação de read_child_string()
│   ├── trace_runtime.c     # Runtime ptrace (fornecido + TODOs dos estudantes)
│   └── student/
│       ├── pairer.c        # Pareamento entrada/saída de syscall (estudante)
│       └── formatter.c     # Formatação final da syscall (estudante)
├── tests/
│   ├── targets/            # Programas simples usados como alvos de teste
│   │   ├── hello_write.c
│   │   ├── open_hosts.c
│   │   ├── failed_open.c
│   │   └── exec_echo.c
│   ├── unit/
│   │   └── test_student.c  # Testes unitários das funções dos estudantes
│   └── test_integration.py # Testes de integração via pytest
├── examples/
│   └── sample_outputs/     # Saídas de referência esperadas
└── Makefile
```

---

## Fluxo de Execução

### 1. Entrada — `main()` (`src/main.c`)

O programa começa em `main()`, que:

1. Chama `parse_args()` para processar os argumentos da linha de comando.
2. Configura o `struct trace_state` com o modo escolhido (`raw_events` ou normal).
3. Chama `trace_program()` passando o vetor de argumentos do programa alvo e a função `trace_observer` como callback.

```
./toytrace trace [--raw-events] -- <programa> [args...]
```

---

### 2. Parse de Argumentos — `parse_args()` (`src/cli.c`)

Valida os argumentos e preenche `struct trace_options`:

- Exige o subcomando `trace`.
- Processa a flag opcional `--raw-events`.
- Localiza o separador `--` e extrai o vetor `target_argv` (programa a ser monitorado).

---

### 3. Runtime de Tracing — `trace_program()` (`src/trace_runtime.c`)

É o núcleo do projeto. Executa as seguintes etapas em sequência:

#### 3.1 Criação do processo monitorado — `launch_tracee()`

```
fork()
  └─ filho:
       ptrace(PTRACE_TRACEME)   ← autoriza o pai a rastrear
       raise(SIGSTOP)           ← pausa para sincronizar com o pai
       execvp(argv[0], argv)    ← substitui imagem pelo programa alvo
  └─ pai:
       retorna o PID do filho
```

#### 3.2 Espera pela parada inicial — `wait_for_initial_stop()`

O pai espera o `SIGSTOP` do filho com `waitpid()`. Garante que o filho está pronto antes de configurar as opções de trace.

#### 3.3 Configuração do trace — `configure_trace_options()`

```c
ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACESYSGOOD)
```

Ativa `PTRACE_O_TRACESYSGOOD`: paradas de syscall chegam com o bit `0x80` setado no sinal, permitindo diferenciá-las de outros `SIGTRAP`.

#### 3.4 Loop principal de tracing

Após configurar, o runtime entra em loop contínuo:

```
resume_until_next_syscall()   ← ptrace(PTRACE_SYSCALL, ...)
      │
      ▼
wait_for_syscall_stop()
      │
      ├─ filho terminou (WIFEXITED / WIFSIGNALED) → retorna código de saída
      │
      └─ parada de syscall (bit 0x80) →
              ptrace(PTRACE_GETREGS, ...)
              fill_event_from_regs()        ← preenche struct syscall_event
              observer(&ev, userdata)       ← chama o callback de main.c
              entering = !entering          ← alterna entrada/saída
              resume_until_next_syscall()   ← continua o ciclo
```

Cada syscall gera **dois eventos**: um na entrada (`entering=1`) e um na saída (`entering=0`).

---

### 4. Preenchimento do Evento — `fill_event_from_regs()` (`src/trace_runtime.c`)

Converte os registradores x86\_64 para `struct syscall_event`:

| Campo          | Registrador    | Observação                                 |
|----------------|----------------|--------------------------------------------|
| `syscall_no`   | `orig_rax`     | Número da syscall                          |
| `ret`          | `rax`          | Válido somente na saída                    |
| `args[0..5]`   | `rdi, rsi, rdx, r10, r8, r9` | Salvos na entrada; reutilizados na saída |
| `pid`          | parâmetro      | PID do processo monitorado                 |
| `entering`     | parâmetro      | 1 = entrada, 0 = saída                     |

Os argumentos são salvos globalmente na entrada pois, na saída, os registradores podem ter sido alterados pelo kernel.

---

### 5. Callback Observer — `trace_observer()` (`src/main.c`)

Chamado a cada evento. Dois modos de operação:

#### Modo `--raw-events`
Chama diretamente `student_debug_raw_event()` e imprime uma linha de depuração:
```
pid=1234 write entrada
pid=1234 write saida
```

#### Modo normal
1. Chama `student_pair_syscall()` para parear o evento de entrada com o de saída.
2. Quando o par está completo (`ready > 0`), chama `student_format_event()` para formatar e imprime a linha.

---

### 6. Pareamento — `student_pair_syscall()` (`src/student/pairer.c`)

Gerencia o `struct syscall_pairer` que armazena o evento de entrada pendente:

```
Evento de ENTRADA:
  ├─ Armazena em pairer->entry
  ├─ Caso especial execve: lê o caminho do processo filho via read_child_string()
  └─ Retorna 0 (par incompleto)

Evento de SAÍDA:
  ├─ Combina com pairer->entry
  ├─ Copia ret da saída para o evento completo
  └─ Retorna 1 (par completo → pronto para formatar)
```

---

### 7. Formatação — `student_format_event()` (`src/student/formatter.c`)

Formata o evento completo em uma linha legível. Syscalls com tratamento especial:

| Syscall       | Formato de saída                                        |
|---------------|---------------------------------------------------------|
| `read`        | `read(fd, "conteúdo", count) = ret`                     |
| `write`       | `write(fd, "conteúdo", count) = ret`                    |
| `openat`      | `openat(dirfd, "path", flags, mode) = ret`              |
| `execve`      | `execve("path", ...) = ret`                             |
| `exit_group`  | `exit_group(status) = ret`                              |
| outras        | `nome(arg0, arg1, arg2, arg3, arg4, arg5) = ret`        |

Para `read`, `write` e `openat`, usa `read_child_string()` para ler strings do espaço de memória do processo monitorado. Se a leitura falhar, imprime `<ilegivel>`.

---

### 8. Helper de Memória — `read_child_string()` (`src/trace_helpers.c`)

Lê uma string terminada em `\0` da memória do processo filho usando `ptrace(PTRACE_PEEKDATA, ...)`, palavra por palavra (8 bytes por vez em x86\_64). Necessário porque o processo monitorado tem seu próprio espaço de endereçamento.

---

## Diagrama Resumido do Fluxo

```
Linha de comando
      │
      ▼
parse_args()           [cli.c]
      │
      ▼
trace_program()        [trace_runtime.c]
      │
      ├─ launch_tracee()          fork + PTRACE_TRACEME + execvp
      ├─ wait_for_initial_stop()  aguarda SIGSTOP do filho
      ├─ configure_trace_options() PTRACE_O_TRACESYSGOOD
      │
      └─ loop:
           resume_until_next_syscall()  PTRACE_SYSCALL
           wait_for_syscall_stop()      waitpid()
           fill_event_from_regs()       registradores → syscall_event
           trace_observer()             [main.c]
                 │
                 ├─ (raw) student_debug_raw_event()   [formatter.c]
                 │
                 └─ (normal)
                       student_pair_syscall()          [pairer.c]
                             │
                             └─ (par completo)
                                   student_format_event()  [formatter.c]
                                         │
                                         └─ read_child_string()  [trace_helpers.c]
                                               │
                                               ▼
                                         puts(linha formatada)
```

---

## Semanas de Desenvolvimento

| Semana | Foco                                                                 |
|--------|----------------------------------------------------------------------|
| 1      | Exploração do código; leitura do caminho `main → CLI → runtime → student` |
| 2      | Implementação de `launch_tracee()` e `wait_for_initial_stop()`       |
| 3      | Implementação de `configure_trace_options()`, `resume_until_next_syscall()` e `wait_for_syscall_stop()` |
| 4      | Preenchimento de `struct syscall_event` via `PTRACE_GETREGS`; uso de `--raw-events` para validar |
| 5      | Implementação de `student_pair_syscall()` e `student_format_event()` com formatação específica por syscall |

---

## Compilação e Testes

```bash
make                  # compila toytrace e os binários de teste
make test             # roda testes unitários (C) + integração (pytest)
make test-unit        # apenas testes unitários
make test-integration # apenas testes de integração
make clean            # remove artefatos compilados
```

Exemplo de uso:

```bash
./toytrace trace -- /bin/echo oi
./toytrace trace --raw-events -- ./tests/targets/hello_write
```
