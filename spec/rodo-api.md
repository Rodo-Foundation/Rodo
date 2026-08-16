API Comum do Rodo

Este documento define a interface pública que todos os SDKs oficiais devem implementar. A API é intencionalmente simples e simétrica entre linguagens, mantendo o mesmo comportamento.

1. Abertura e Fechamento

link(path)

Abre ou cria um arquivo .rd no caminho especificado.

Parâmetros:

· path (string): caminho para o arquivo .rd.

Retorno:

· Um objeto/instância representando a conexão com o arquivo.

Comportamento:

· Se o arquivo não existir, um novo arquivo vazio é criado.
· Se o arquivo existir, ele é aberto e seu conteúdo é carregado (ou mapeado) para acesso.
· Se o arquivo estiver corrompido, deve lançar um erro/exception.

close()

Fecha a conexão com o arquivo, garantindo que todas as alterações sejam gravadas.

Comportamento:

· Após close(), qualquer operação de leitura/escrita deve resultar em erro.
· É seguro chamar close() múltiplas vezes.

2. Operações de Escrita

set(key, value)

Define um valor para uma chave. Se a chave já existir, o valor é substituído; se não existir, uma nova chave é criada.

Parâmetros:

· key (string): nome do campo.
· value (qualquer tipo suportado): valor a ser armazenado.

Comportamento:

· A chave é registrada na tabela de símbolos (se ainda não estiver).
· O valor é convertido para a representação binária do Rodo e armazenado no bloco de dados corrente.
· Retorna a instância para permitir encadeamento (em linguagens que suportam).

delete(key)

Remove explicitamente a chave e seu valor do arquivo. Se a chave não existir, nenhuma ação é tomada.

Parâmetros:

· key (string): nome do campo a remover.

Comportamento:

· O par chave-valor é removido do bloco de dados.
· O símbolo na tabela de símbolos é marcado como removido (mas seu ID não é reutilizado).
· O valor associado no dicionário de strings pode permanecer (a menos que seja feita uma compactação posterior).

clear()

Remove todos os dados do arquivo, deixando-o vazio. Deve ser usado com cautela, pois é irreversível.

Comportamento:

· Todos os blocos de dados são removidos.
· A tabela de símbolos e o dicionário podem ser mantidos (para reutilização) ou zerados, dependendo da implementação. O recomendado é zerar para liberar espaço, mas isso pode afetar compatibilidade com versões antigas do código. Padrão: zerar.

3. Operações de Leitura

get(key)

Retorna o valor associado à chave, ou null/None/nil se a chave não existir.

Parâmetros:

· key (string): nome do campo.

Retorno:

· O valor armazenado, convertido para o tipo nativo da linguagem.
· Se a chave não existir, retorna null (ou equivalente).

has(key)

Verifica se a chave existe no arquivo.

Retorno:

· Booleano true se a chave existe, false caso contrário.

keys()

Retorna uma lista com todas as chaves presentes no arquivo.

Retorno:

· Array/lista de strings com os nomes das chaves.

all()

Retorna um mapa/objeto com todas as chaves e seus valores.

Retorno:

· Um objeto/mapa onde as chaves são os nomes dos campos e os valores são os dados armazenados.

4. Propriedades Adicionais (opcionais)

Alguns SDKs podem fornecer métodos extras, mas a API mínima é a definida acima. Métodos opcionais comuns:

· size(): número de pares chave-valor.
· sync(): força a gravação das alterações no disco imediatamente.
· compact(): reescreve o arquivo removendo espaços não utilizados (como strings órfãs).
· transaction(fn): executa uma função dentro de uma transação (garantindo atomicidade).

5. Tratamento de Erros

Cada SDK deve lançar exceções ou retornar códigos de erro apropriados para os seguintes casos:

· Arquivo não encontrado ou sem permissão de leitura/escrita.
· Arquivo corrompido (checksum inválido, estrutura malformada).
· Chave inválida (não string, string vazia).
· Valor de tipo não suportado pela linguagem.
· Operação após close().

6. Exemplo de Uso

JavaScript:

```javascript
const rodo = require('rodo-sdk');
const dados = rodo.link('dados.rd');

dados.set('nome', 'Davi');
dados.set('level', 87);

console.log(dados.get('nome')); // "Davi"
console.log(dados.has('coins')); // false

dados.delete('level');
console.log(dados.keys()); // ["nome"]

dados.close();
```