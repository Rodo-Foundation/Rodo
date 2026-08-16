Tipos de Dados Suportados pelo Rodo

O Rodo reconhece um conjunto de tipos de dados primitivos e compostos. Cada tipo possui um código binário usado na codificação. Os SDKs devem converter os tipos nativos da linguagem para esses tipos universais e vice-versa.

1. Tabela de Tipos

Tipo Código Descrição
Null 0x00 Ausência de valor
Bool 0x01 Booleano (true/false)
Int 0x02 Inteiro com sinal (até 64 bits)
Float 0x03 Ponto flutuante (32 ou 64 bits)
String 0x04 Texto UTF-8
Bytes 0x05 Sequência binária
Array 0x06 Lista ordenada de valores
Map 0x07 Mapa de chave-valor (chaves string)
Date 0x08 Data/hora (timestamp Unix em ms)
UUID 0x09 Identificador universalmente único

2. Correspondência entre Linguagens

Tipo Rodo JavaScript Python Rust Java C++
Null null None None null nullptr
Bool boolean bool bool boolean bool
Int number int i64 long int64_t
Float number float f64 double double
String string str String String std::string
Bytes Uint8Array bytes Vec<u8> byte[] std::vector<uint8_t>
Array Array list Vec<RodoValue> List<Object> std::vector<RodoValue>
Map Object dict HashMap<String, RodoValue> Map<String,Object> std::map<std::string, RodoValue>
Date Date datetime SystemTime java.time.Instant std::chrono::time_point
UUID string (UUID) uuid.UUID Uuid java.util.UUID std::array<uint8_t,16>

3. Codificação Binária

3.1 Null

Nenhum dado é armazenado. Apenas o código do tipo (0x00) aparece.

3.2 Bool

1 byte: 0x00 para false, 0x01 para true.

3.3 Int

Inteiros são codificados em varint com sinal usando zig-zag. O tamanho é variável, de 1 a 10 bytes para inteiros de 64 bits.

Exemplos:

· 0 → 0x00
· -1 → 0x01
· 1 → 0x02
· 127 → 0xFE 0x01
· -128 → 0xFF 0x01

3.4 Float

Ponto flutuante é armazenado em IEEE 754. O SDK deve decidir se usa 32 bits (float) ou 64 bits (double) com base no valor e na configuração. Por padrão, usa-se 64 bits para maior precisão. A codificação é big-endian.

· float (32 bits): 4 bytes.
· double (64 bits): 8 bytes.

3.5 String

Strings são armazenadas no dicionário global. O valor codificado é um varint com o ID da string no dicionário.

3.6 Bytes

Bytes são armazenados diretamente no valor: primeiro um varint com o comprimento, depois a sequência de bytes.

3.7 Array

Um array é codificado como: número de elementos (varint) + cada elemento codificado como tipo (1 byte) + valor (codificado conforme o tipo). Os elementos podem ser de tipos heterogêneos.

3.8 Map

Um map é codificado como: número de pares (varint) + para cada par: key (string codificada com ID do dicionário) + value (tipo + valor). As chaves são strings e podem repetir, mas o SDK deve tratar como um mapa normal (chaves únicas, última sobrescreve).

3.9 Date

Date é um inteiro com sinal representando o timestamp Unix em milissegundos, codificado como varint com zig-zag. Fuso horário é sempre UTC.

3.10 UUID

UUID é armazenado como 16 bytes brutos (RFC 4122). Nenhuma codificação adicional.

4. Extensibilidade

Novos tipos podem ser adicionados em versões futuras do formato, recebendo novos códigos de tipo. SDKs antigos, ao encontrarem um tipo desconhecido, devem lançar erro ou ignorar o valor, dependendo da configuração. Recomenda-se que a leitura de um tipo desconhecido resulte em erro para evitar perda silenciosa de dados.