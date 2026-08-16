# Contribuindo com o Rodo

Obrigado pelo interesse em contribuir com o Rodo! Este documento descreve as diretrizes para contribuir com o projeto, seja reportando problemas, propondo melhorias ou enviando código.

## Como Contribuir

Existem várias formas de contribuir:

- **Reportando bugs**: encontrou um problema? Abra uma issue descrevendo o comportamento esperado e o observado.
- **Sugerindo melhorias**: tem uma ideia para aprimorar o formato ou os SDKs? Compartilhe conosco.
- **Escrevendo código**: corrija bugs, implemente novas funcionalidades ou melhore a documentação.
- **Melhorando a documentação**: revise e amplie os arquivos em `docs/` e `spec/`.

## Reportando Problemas

Ao abrir uma issue, inclua:

- Descrição clara do problema.
- Passos para reproduzir.
- Comportamento esperado vs. comportamento observado.
- Ambiente (sistema operacional, linguagem, versão do SDK).
- Se possível, um trecho de código mínimo que reproduza o bug.

## Propondo Mudanças

Para alterações significativas, abra primeiro uma issue para discussão. Isso evita retrabalho e garante que a mudança esteja alinhada com os objetivos do projeto.

## Configuração do Ambiente de Desenvolvimento

O repositório é um monorepo contendo o núcleo em C e SDKs para várias linguagens. Você precisará das ferramentas básicas de compilação para a linguagem em que deseja trabalhar.

### Dependências Gerais

- Git
- Compilador C (GCC, Clang ou MSVC) para o core em C
- CMake (para construir o core e ferramentas)
- Linguagens específicas e seus gerenciadores de pacotes (npm, pip, cargo, etc.) conforme o SDK que você for modificar

### Clonando o Repositório

```bash
git clone https://github.com/rodo-foundation/rodo.git
cd rodo
```

Construindo o Core em C

```bash
cd core/C
mkdir build && cd build
cmake ..
cmake --build .
```

Executando Testes

Cada SDK possui seus próprios testes. Consulte o README.md dentro de cada diretório em sdks/ para instruções específicas.

Além disso, o diretório tests/ contém:

· Testes de conformidade: garantem que todos os SDKs leiam e escrevam arquivos .rd compatíveis.
· Testes de integração: verificam a interoperabilidade entre diferentes linguagens.
· Fixtures: arquivos .rd de exemplo usados nos testes.

Para rodar os testes de conformidade, utilize o script tests/conformance/runner/run_all.sh (requer que as SDKs relevantes estejam instaladas).

Estrutura do Projeto

· spec/ – Especificação oficial do formato .rd. Alterações no formato devem começar aqui.
· core/C/ – Implementação de referência em C.
· sdks/ – SDKs para cada linguagem.
· tests/ – Testes de conformidade e integração.
· tools/ – Utilitários auxiliares.
· docs/ – Documentação para usuários e desenvolvedores.
· benchmarks/ – Scripts de avaliação de desempenho e tamanho.

Padrões de Código

Core em C

· Siga o padrão C99.
· Use nomes de funções com prefixo rodo_.
· Mantenha o código portável (evite extensões específicas de compilador, quando possível).
· Documente funções públicas com comentários no estilo Doxygen.

SDKs

Cada SDK deve seguir as convenções idiomáticas da linguagem:

· JavaScript: padrão do ESLint recomendado, ES6+.
· Python: PEP 8.
· Rust: rustfmt e clippy.
· Go: gofmt e golint.
· Java/Kotlin: convenções oficiais da Oracle/JetBrains.
· C#: convenções da Microsoft.
· C++: C++17, estilo Google ou similar.
· Swift: API Design Guidelines da Apple.
· Dart: Effective Dart.
· PHP: PSR-12.
· Ruby: Ruby Style Guide.

Independentemente da linguagem, todo código deve ser limpo, legível e bem documentado.

Testes

· Todos os novos recursos devem incluir testes.
· Testes unitários devem cobrir casos básicos, tipos de dados, persistência e remoção.
· Testes de conformidade devem ser atualizados se o formato .rd mudar.
· Verifique se os testes existentes passam antes de enviar um pull request.

Convenção de Commits

Use mensagens de commit claras e descritivas. Sugerimos o seguinte formato:

```
<tipo>: <descrição curta no imperativo>

[corpo opcional com detalhes]
```

Tipos comuns:

· feat: nova funcionalidade
· fix: correção de bug
· docs: alterações na documentação
· test: adição ou modificação de testes
· refactor: refatoração de código sem mudança de comportamento
· chore: tarefas de manutenção, build, etc.

Exemplo:

```
feat: adicionar suporte a compressão LZ4 no formato .rd
```

Pull Requests

· Crie um branch a partir da branch principal (main).
· Faça commits pequenos e focados.
· Inclua uma descrição clara do que foi feito e por quê.
· Garanta que todos os testes passem.
· Atualize a documentação, se necessário.
· Se a mudança alterar o formato .rd, atualize a especificação em spec/ e os testes de conformidade.

Licença

Ao contribuir, você concorda que seu código será licenciado sob a licença MIT do projeto. Consulte o arquivo LICENSE para mais detalhes.

Agradecemos sua contribuição!