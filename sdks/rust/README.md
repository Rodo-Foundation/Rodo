# Rodo SDK para Rust

SDK oficial do Rodo para Rust, permitindo manipular arquivos `.rd` de forma simples e eficiente.

## Uso

Adicione `rodo` ao seu `Cargo.toml`:

```toml
[dependencies]
rodo = "0.1.0"
```

Exemplo:

```rust
use rodo::{link, Value};

let mut dados = link("dados.rd").unwrap();
dados.set("nome", Value::String("Davi".to_string())).unwrap();
dados.set("level", Value::Int(87)).unwrap();

if let Some(Value::String(nome)) = dados.get("nome") {
    println!("Nome: {}", nome);
}

dados.close().unwrap();
```

Documentação

· Documentação principal
· Especificação do formato
· API comum

Testes

```bash
cargo test
```