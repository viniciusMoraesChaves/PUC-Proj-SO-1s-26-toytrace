#include "student_api.h"

int student_pair_syscall(struct syscall_pairer *pairer,
                         const struct syscall_event *ev,
                         struct syscall_event *out)
{
    if(ev->entering == 1)
/* esse bagui é dificil de entender, basicamente tenho 3 estrutura, -> pairer que possui tanto o flag de entrada(ou seja, se tem entrada ou nao para o evento e )Aqui está o texto ajustado:

A função recebe três parâmetros:

*pairer — uma estrutura que serve como memória entre as duas chamadas. Ela contém has_entry, um flag booleano que indica se já existe um evento de entrada salvo, e entry,
* que armazena o próprio evento de entrada com todos os seus campos: pid, syscall_no e args.
*ev — o evento atual que chegou, podendo ser de entrada ou de saída. Na entrada, carrega pid, syscall_no e args. Na saída, carrega o valor de retorno em ev->ret e entering igual a 0.
*out — o evento de saída completo que a função deve montar quando as duas metades estiverem disponíveis.
*O fluxo funciona assim: na primeira chamada, quando ev->entering é 1, os dados do evento de entrada são salvos em pairer->entry e has_entry é marcado como 1.
*Na segunda chamada, quando ev->entering é 0, a função verifica se has_entry é 1, copia o evento de entrada salvo para out — trazendo pid, syscall_no e args — 
*e então sobrescreve out->ret e out->entering com os valores vindos do ev atual, completando o par.
*/
  {
        pairer-> entry = *ev; // passo tudo que esta armazenado no evento para a estrutura de entrada
        pairer->has_entry = 1; // ajusto o flag de possuir entrada para 1, ja que mandei os valores para entry
        return 0; // retornando 0 pois apenas estou passando valores para evento apenas de entrada, ou seja, falta os de saida   
    }

    if(ev -> entering == 0)
    {
        if(pairer->has_entry == 1) // quero verificar se já houve inserção no evento 
        {
            *out = pairer->entry ; // to passando os valores tanto de pid, syscallno e de argcs para terem o valor de se tem entrada ou nao pela variavel has->entry(que é booleano pelo que entendi)
            out -> ret = ev ->  ret ; // sobrescrevendo as variaveis de entrada que nesse caso é 0 por conta de eu estar retornando um valor, e tambem a variavel de retorno de out, para ele conseguir retornar algo a partir do proprio evento;
            out -> entering =  ev-> entering ;
            pairer -> has_entry = 0; // volto essa varivel para 0 pois o evento de entrada e saida aconteceram, entao é como se tivesse reiniciado os valores
            return 1;

        }
    
    {
        perror("Erro no tratamento de eventos de retorno ou de inserção ");
        return -1;

    }

// FEITO -> Não retirei os comentarios para registrar a semana 
    /*
     * TODO Semana 2:
     *
     * O runtime chama esta funcao duas vezes para cada syscall:
     *
     *   1. uma vez antes da syscall executar
     *   2. uma vez depois da syscall terminar
     *
     * Na primeira parada, os argumentos estao disponiveis.
     * Na segunda parada, o retorno esta disponivel.
     *
     * Seu trabalho e produzir um evento completo apenas quando ja existirem
     * as duas metades da syscall.
     *
     * Dicas:
     * - ev->entering == 1 indica entrada de syscall.
     * - ev->entering == 0 indica saida de syscall.
     * - para comecar, assuma apenas um processo monitorado.
     *
     * Retorne:
     *   1 se out contem uma syscall completa
     *   0 se ainda nao ha syscall completa
     *  -1 se a sequencia de eventos parece invalida
     */
}
}
