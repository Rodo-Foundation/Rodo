Especificação do Formato Rodo (.rd) — Versão 1

Este documento descreve a estrutura binária do arquivo .rd utilizado pelo Rodo. O formato foi projetado para máxima compactação, velocidade de leitura/escrita e simplicidade de implementação.

1. Visão Geral

Um arquivo .rd é um único arquivo binário autocontido, composto por seções sequenciais:

1. Cabeçalho (Header)
2. Tabela de Símbolos (Symbol Table)
3. Dicionário de Strings (String Dictionary)
4. Blocos de Dados (Data Blocks)
5. Índice de Blocos (Block Index) — opcional
6. Metadados (Metadata)

A ordem das seções é fixa. O arquivo começa com um número mágico para identificação e termina com um checksum para verificação de integridade.

2. Cabeçalho (Header)

O cabeçalho possui tamanho fixo de 18 bytes:

Offset Tamanho (bytes) Campo Descrição
0 4 magic 'R' 'D' '0' '1' (0x52 0x44 0x30 0x31)
4 1 version_major Versão principal do formato (atual: 1)
5 1 version_minor Versão secundária do formato (atual: 0)
6 1 flags Bit flags (ver abaixo)
7 8 timestamp Timestamp Unix em milissegundos da última modificação (big endian)
15 4 header_crc32 CRC32 dos primeiros 14 bytes do cabeçalho (verificação)

2.1 Flags

Bit Nome Descrição
0 COMPRESSED Blocos de dados estão comprimidos (LZ4 ou Zstandard)
1 ENCRYPTED Dados estão criptografados (AES-256-GCM)
2 HAS_BLOCK_INDEX Índice de blocos está presente
3 USE_ZSTD Usa Zstandard em vez de LZ4 para compressão
4-7 RESERVED Reservado para uso futuro

3. Tabela de Símbolos (Symbol Table)

A tabela de símbolos mapeia nomes de campos (strings) para identificadores numéricos (IDs) inteiros positivos. Cada nome de campo recebe um ID único, começando em 1 e incrementando conforme novos símbolos são adicionados. IDs nunca são reutilizados, mesmo se o símbolo for removido (para evitar referências quebradas).

3.1 Estrutura da Seção

A seção começa com um inteiro count (varint) indicando o número de símbolos. Em seguida, count entradas de símbolos são armazenadas sequencialmente.

3.2 Entrada de Símbolo

Cada entrada contém:

Campo Tipo Descrição
symbol_id varint ID numérico do símbolo (1 a N)
name_length varint Comprimento em bytes do nome (UTF-8)
name bytes String UTF-8 do nome do campo
type_hint byte Tipo de dados sugerido (opcional, 0 = sem tipo fixo)
flags byte Flags específicos do símbolo (ex: removido, etc.)

type_hint usa os mesmos códigos de tipo definidos na seção 5.

3.3 Exemplo

Se o programa chamar:

```
dados.set("nome", "Davi")
dados.set("level", 87)
dados.set("coins", 1500)
```

A tabela de símbolos terá:

symbol_id name type_hint
1 "nome" 4 (string)
2 "level" 2 (int)
3 "coins" 2 (int)

4. Dicionário de Strings (String Dictionary)

O dicionário é usado para deduplicar strings armazenadas nos valores. Cada string única é armazenada uma vez, e os valores fazem referência ao ID da string no dicionário.

4.1 Estrutura da Seção

Começa com count (varint) indicando o número de strings no dicionário. Em seguida, cada string é codificada como:

Campo Tipo Descrição
string_id varint ID da string (1 a N)
length varint Comprimento em bytes (UTF-8)
bytes bytes Conteúdo da string

4.2 Deduplicação

Strings idênticas compartilham o mesmo string_id. Ao armazenar um valor do tipo string, o SDK verifica se a string já existe no dicionário; se sim, usa o ID existente; caso contrário, adiciona uma nova entrada.

5. Blocos de Dados (Data Blocks)

Os dados reais são armazenados em blocos. Cada bloco representa um conjunto de pares chave-valor que correspondem a um "registro" lógico (equivalente a um documento ou linha). O formato não impõe esquema: cada bloco pode ter qualquer combinação de chaves.

5.1 Estrutura da Seção

A seção inicia com block_count (varint) informando quantos blocos existem. Em seguida, os blocos são armazenados sequencialmente.

5.2 Estrutura de um Bloco

Campo Tipo Descrição
block_id varint Identificador único do bloco (começando em 1)
pair_count varint Número de pares chave-valor neste bloco
pairs repetido pair_count pares, cada um com a estrutura abaixo

5.3 Par Chave-Valor

Campo Tipo Descrição
symbol_id varint ID do símbolo (chave) na tabela de símbolos
value_type byte Código do tipo do valor (ver seção 6)
value codificado Valor codificado conforme o tipo (ver seção 6)

6. Codificação de Tipos de Dados

Tipo Código Codificação
Null 0x00 Nenhum dado
Bool 0x01 1 byte: 0 (false) ou 1 (true)
Int 0x02 Varint com sinal usando codificação zig-zag
Float 0x03 IEEE 754: 4 bytes (float) ou 8 bytes (double)
String 0x04 ID da string no dicionário (varint)
Bytes 0x05 Comprimento (varint) + sequência de bytes
Array 0x06 Número de elementos (varint) + elementos codificados
Map 0x07 Número de pares (varint) + pares chave-valor
Date 0x08 Timestamp Unix em ms (varint, zig-zag)
UUID 0x09 16 bytes (RFC 4122)

6.1 Varints com Zig-Zag

Inteiros são codificados em formato LEB128 sem sinal. Para valores com sinal, aplica-se a codificação zig-zag: mapeia o inteiro n para (n << 1) ^ (n >> 31) (para 32 bits) ou (n << 1) ^ (n >> 63) (para 64 bits). Isso faz com que números pequenos, positivos ou negativos, ocupem poucos bytes.

6.2 Arrays e Maps

· Array: armazena elementos de tipos potencialmente heterogêneos. Cada elemento é codificado como value_type + value.
· Map: semelhante a um bloco, mas as chaves são strings livres (não necessariamente símbolos). O Map armazena pares key (string codificada com ID do dicionário) e value (codificado). É útil para estruturas aninhadas.

7. Índice de Blocos (Block Index) — Opcional

Se a flag HAS_BLOCK_INDEX estiver ativa, uma seção adicional é incluída após os blocos de dados. O índice mapeia hashes de chaves (symbol_id) para IDs de blocos, acelerando buscas.

7.1 Estrutura

Campo Tipo Descrição
entry_count varint Número de entradas no índice
entries repetido Cada entrada: hash (4 bytes) + block_id (varint)

O hash é calculado sobre symbol_id usando um algoritmo simples (ex.: FNV-1a de 32 bits). Colisões são resolvidas armazenando múltiplas entradas com o mesmo hash e verificando o symbol_id durante a busca.

8. Metadados (Metadata)

Seção final do arquivo, com tamanho variável. Contém informações adicionais e checksum geral.

Campo Tipo Descrição
total_blocks varint Número total de blocos no arquivo
total_symbols varint Número total de símbolos na tabela
total_strings varint Número total de strings no dicionário
file_size varint Tamanho total do arquivo em bytes (sem metadata)
crc32 4 bytes CRC32 de todo o arquivo até este ponto

9. Compressão e Criptografia

Quando a flag COMPRESSED está ativa, os blocos de dados são comprimidos usando LZ4 (padrão) ou Zstandard. A compressão é aplicada somente à seção de blocos de dados, não às demais. A descompressão é transparente para o SDK.

Se ENCRYPTED estiver ativa, os dados (blocos) são criptografados com AES-256-GCM. O SDK deve fornecer a chave ao abrir o arquivo. O cabeçalho e a tabela de símbolos permanecem em texto claro para permitir a leitura da estrutura.

10. Exemplo de Arquivo

Suponha um arquivo .rd com os dados:

```
nome = "Davi"
level = 87
coins = 1500
```

Uma possível representação binária (sem compressão, sem índice) seria:

```
Magic: 52 44 30 31
Versão: 01 00
Flags: 00
Timestamp: ...
Header CRC: ...
--- Tabela de Símbolos ---
count: 03
Entrada 1: id=1, len=4, "nome", type=04 (string)
Entrada 2: id=2, len=5, "level", type=02 (int)
Entrada 3: id=3, len=5, "coins", type=02 (int)
--- Dicionário de Strings ---
count: 01
Entrada 1: id=1, len=4, "Davi"
--- Bloco de Dados ---
block_count: 01
Bloco 1:
  block_id: 01
  pair_count: 03
  Par 1: symbol_id=1, type=04 (string), value=1 (ID da string "Davi")
  Par 2: symbol_id=2, type=02 (int), value=87 (varint 0x57)
  Par 3: symbol_id=3, type=02 (int), value=1500 (varint 0xDC 0x0B)
--- Metadados ---
total_blocks: 01
total_symbols: 03
total_strings: 01
file_size: ...
crc32: ...
```

11. Considerações de Implementação

· IDs de símbolos e strings são sempre varints (não fixos em 4 bytes) para economia.
· O arquivo pode ser editado in-place para atualizações rápidas, mas em caso de remoção de muitos dados, uma compactação (rewrite) pode ser feita pelo SDK.
· Toda leitura deve validar o CRC32 do cabeçalho e, se presente, o CRC32 geral, para detectar corrupção.