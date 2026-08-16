# Rodo SDK para Java

SDK oficial do Rodo para Java, permitindo manipular arquivos `.rd`.

## Uso

```java
import com.rodo.Rodo;

public class Main {
    public static void main(String[] args) throws Exception {
        Rodo dados = Rodo.link("dados.rd");

        Rodo.Value nome = new Rodo.Value();
        nome.type = Rodo.TYPE_STRING;
        nome.stringVal = "Davi";
        dados.set("nome", nome);

        Rodo.Value level = new Rodo.Value();
        level.type = Rodo.TYPE_INT;
        level.intVal = 87;
        dados.set("level", level);

        Rodo.Value getNome = dados.get("nome");
        System.out.println(getNome.stringVal); // Davi

        dados.delete("level");
        dados.close();
    }
}
```

Documentação

· Documentação principal
· Especificação do formato
· API comum

```
