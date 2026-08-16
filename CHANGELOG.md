# Changelog

Todas as mudanças notáveis neste projeto serão documentadas neste arquivo.

O formato é baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.0.0/),
e este projeto adere ao [Versionamento Semântico](https://semver.org/lang/pt-BR/).

## [Não publicado]

### Adicionado
- Nada por enquanto.

## [0.1.0] - 2026-08-16

### Adicionado
- Formato binário `.rd` inicial com cabeçalho, tabela de símbolos, dicionário de strings e blocos de dados.
- Codificação de números inteiros usando varints (LEB128) com zig-zag para números negativos.
- Suporte aos tipos de dados: Null, Bool, Int, Float, String, Bytes, Array, Map, Date e UUID.
- Tabela de símbolos para mapear nomes de campos a identificadores numéricos, reduzindo repetição.
- Dicionário de strings para deduplicação automática de valores repetidos.
- API padrão definida: `link`, `close`, `set`, `get`, `has`, `delete`, `keys`, `all` e `clear`.
- Implementação de referência em C (`core/C`).
- SDKs oficiais para JavaScript, Python, Rust, Go, Java, Kotlin, C#, C++, Swift, Dart, PHP e Ruby.
- Testes de conformidade para garantir compatibilidade entre SDKs.
- Documentação da especificação do formato, API e guia de uso.
- Ferramenta `rodo-inspector` para inspecionar arquivos `.rd`.
- Ferramenta `rodo-converter` para converter entre `.rd` e JSON/CSV.

### Alterado
- Nada.

### Corrigido
- Nada.

### Removido
- Nada.