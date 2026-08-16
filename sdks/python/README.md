# Rodo SDK para Python

SDK oficial do Rodo para Python, permitindo manipular arquivos `.rd` de forma simples e eficiente.

## Instalação

### Dependências

- Python 3.6+
- Compilador C (para compilar a extensão nativa)
- CMake (opcional, para compilar o núcleo separadamente)

### Compilação e instalação

```bash
cd sdks/python
pip install -e .
```

Isso compilará a extensão nativa rodo_native a partir do núcleo C.

Uso

```python
import rodo

dados = rodo.link('dados.rd')

dados.set('nome', 'Davi')
dados.set('level', 87)
dados.set('coins', 1500)

print(dados.get('nome'))  # Davi

dados.delete('coins')

dados.close()
```

Documentação

· Documentação principal
· Especificação do formato
· API comum

Testes

```bash
python -m unittest tests/tes