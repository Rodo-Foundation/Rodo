Versionamento e Compatibilidade do Rodo

Este documento define as regras de versionamento do formato .rd e como os SDKs devem lidar com mudanças entre versões.

1. Versionamento do Formato

O formato .rd possui um número de versão composto por major e minor, ambos armazenados no cabeçalho (bytes 4 e 5). A versão atual é 1.0.

· Versão Major: incrementada quando ocorrem mudanças incompatíveis que impedem que um leitor antigo leia corretamente um arquivo novo. Exemplos: alteração na estrutura do cabeçalho, remoção de um tipo de dado, mudança na codificação fundamental.
· Versão Minor: incrementada para adições compatíveis, como novos tipos de dados, novas flags ou seções opcionais. Um leitor antigo pode ignorar essas adições sem corromper os dados.

2. Compatibilidade Entre Versões

2.1 Regras de Leitura

· Um SDK da versão X.Y deve ser capaz de ler qualquer arquivo com versão major igual a X e versão minor menor ou igual a Y.
· Se a versão major for menor (arquivo mais antigo), o SDK deve fazer a migração apropriada, se possível.
· Se a versão major for maior (arquivo de um formato mais novo), o SDK deve recusar a leitura e informar que é necessária uma atualização do SDK.

2.2 Regras de Escrita

· Um SDK sempre grava no formato mais recente que suporta.
· Ao abrir um arquivo de versão mais antiga, o SDK pode optar por manter a versão original ou atualizar o arquivo para a versão atual. Recomenda-se manter a versão original para maximizar compatibilidade, a menos que o usuário solicite explicitamente a migração.

2.3 Migração de Tipos

Se um campo tem seu tipo alterado entre versões do programa (ex.: de int para string), o Rodo não impede a mudança. O símbolo na tabela de símbolos pode ter um type_hint que sugere o tipo, mas não é vinculativo. Um valor armazenado com tipo antigo ainda pode ser lido pelo tipo novo se o SDK conseguir converter (por exemplo, int para string). Caso contrário, o SDK deve lançar erro.

3. Compatibilidade entre SDKs

Todos os SDKs oficiais são testados em conjunto por meio de testes de conformidade. A compatibilidade é garantida nos seguintes níveis:

· Nível de arquivo: arquivos .rd criados por um SDK devem ser lidos corretamente por todos os outros SDKs.
· Nível de API: a semântica das operações (set, get, delete, etc.) deve ser idêntica entre linguagens.
· Nível de tipos: a correspondência de tipos deve ser consistente, evitando perda de dados em conversões.

4. Adoção de Novos Recursos

Novos recursos (como novos tipos, flags ou seções) devem ser introduzidos de forma incremental, mantendo compatibilidade retroativa. O processo recomendado:

1. Adicionar a nova funcionalidade no spec/rodo-format-v1.md com uma nota de versão minor.
2. Implementar no core em C e nos SDKs.
3. Adicionar testes de conformidade.
4. Incrementar a versão minor no formato e atualizar os SDKs para reconhecer a nova versão minor.

5. Descontinuação de Recursos

Recursos raramente são removidos. Se for necessário remover um tipo ou seção, deve-se:

· Incrementar a versão major.
· Fornecer um período de transição onde os SDKs ainda conseguem ler arquivos antigos, mas não criam mais o recurso.
· Documentar claramente a mudança no changelog e na especificação.

6. Exemplo de Evolução

Versão 1.0: tipos básicos (Null, Bool, Int, Float, String, Bytes, Array, Map, Date, UUID).

Versão 1.1 (futura): adiciona tipo Decimal (código 0x0A) para números decimais de precisão arbitrária. SDKs antigos, ao encontrarem esse tipo, gerarão erro (ou poderão ignorar se configurados). SDKs novos leem e escrevem normalmente.

Versão 2.0 (hipotética): muda a codificação de strings para um novo esquema de compressão. Arquivos 2.0 não são legíveis por SDKs 1.x. SDKs 2.0 podem ler arquivos 1.x convertendo automaticamente.