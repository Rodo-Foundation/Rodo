# Rodo

**Rodo — dados separados do código, armazenados de forma compacta e acessíveis universalmente.**

Rodo é um formato universal de armazenamento de dados baseado em arquivos binários `.rd`. Ele não é um banco de dados, não possui interface gráfica, painel, servidor ou sistema próprio de gerenciamento. Funciona diretamente por meio de SDKs para diferentes linguagens de programação.

O objetivo do Rodo é permitir que aplicações armazenem grandes quantidades de dados ocupando pouco espaço, reduzindo repetição de nomes de campos, informações redundantes, representação textual desnecessária e tamanho de números.

---

## Características principais

- **Formato binário compacto** com identificadores numéricos, varints e dicionário de strings.
- **Separação total entre código e dados**: remover uma linha de código não apaga os dados do arquivo `.rd`.
- **Remoção explícita**: dados só são apagados quando o programa chama `delete()`.
- **Universalidade**: SDKs oficiais para JavaScript, Python, C, C++, Rust, Java, Kotlin, C#, Go, Swift, Dart, PHP e Ruby.
- **Sem dependências externas**: funciona localmente, sem servidor, SQL ou qualquer banco de dados.
- **Arquivo único e portável**: o `.rd` pode existir independentemente da aplicação que o utiliza.
- **Compatibilidade entre versões**: o formato mantém compatibilidade e permite migração de tipos.
- **Alta velocidade e baixo consumo de armazenamento**.

---

## Exemplo conceitual

```javascript
const rodo = require('rodo-sdk');

const dados = rodo.link('dados.rd');

dados.set('nome', 'Davi');
dados.set('level', 87);
dados.set('coins', 1500);

console.log(dados.get('nome')); // "Davi"

dados.delete('coins');

dados.close();
```

O Rodo transforma esses dados em uma representação extremamente compacta, usando identificadores numéricos para os nomes dos campos:

```
nome  → 1
level → 2
coins → 3
```

Internamente, os dados são armazenados de forma binária, sem repetir os nomes dos campos.

---

Arquitetura

```
Aplicação
    ↓
SDK do Rodo
    ↓
Formato Rodo
    ↓
arquivo .rd
```

---

Estrutura do repositório

```
rodo/
├── spec/               # Especificação oficial do formato .rd
├── core/C/             # Implementação de referência em C
├── sdks/               # SDKs oficiais para cada linguagem
├── tests/              # Testes de conformidade e integração
├── tools/              # Ferramentas auxiliares (inspetor, conversor)
├── examples/           # Exemplos de uso
├── docs/               # Documentação
└── benchmarks/         # Comparações de tamanho e velocidade
```

---

SDKs disponíveis

Linguagem Diretório Status
C sdks/c Implementação de referência
JavaScript sdks/javascript Disponível
Python sdks/python Disponível
C++ sdks/cpp Disponível
Rust sdks/rust Disponível
Java sdks/java Disponível
Kotlin sdks/kotlin Disponível
C# sdks/csharp Disponível
Go sdks/go Disponível
Swift sdks/swift Disponível
Dart sdks/dart Disponível
PHP sdks/php Disponível
Ruby sdks/ruby Disponível

---

Instalação rápida

JavaScript (Node.js)

```bash
npm install rodo-sdk
```

```javascript
const rodo = require('rodo-sdk');
const dados = rodo.link('dados.rd');
dados.set('chave', 'valor');
console.log(dados.get('chave'));
dados.close();
```

Python

```bash
pip install rodo
```

```python
import rodo
dados = rodo.link('dados.rd')
dados.set('chave', 'valor')
print(dados.get('chave'))
dados.close()
```

Rust

```toml
[dependencies]
rodo = "0.1"
```

```rust
use rodo::Rodo;

let mut dados = Rodo::link("dados.rd")?;
dados.set("chave", "valor");
println!("{:?}", dados.get("chave"));
dados.close()?;
```

Consulte a documentação de cada SDK para exemplos completos.

---

Documentação

· Especificação do formato .rd
· API comum entre SDKs
· Tipos suportados
· Guia de versionamento
· Começando
· Referência da API
· Arquitetura

---

Testes de conformidade

O diretório tests/conformance contém casos de teste independentes de linguagem que garantem que todos os SDKs produzam e leiam arquivos .rd compatíveis entre si.

---

Repositório

O código-fonte está disponível em:
https://github.com/rodo-foundation/rodo

---

Licença

Este projeto está licenciado sob a licença MIT. Consulte o arquivo LICENSE para mais detalhes.

---

Contribuindo

Contribuições são bem-vindas! Leia o CONTRIBUTING.md para saber como participar.

---

Rodo — dados separados do código, armazenados de forma compacta e acessíveis universalmente.