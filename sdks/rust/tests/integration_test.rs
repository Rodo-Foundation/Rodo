use rodo::Rodo;
use rodo::Value;
use std::fs;

#[test]
fn test_set_get() {
    let path = "test_rust.rd";
    let mut dados = Rodo::link(path).unwrap();
    dados.set("nome", Value::String("Davi".to_string())).unwrap();
    assert_eq!(dados.get("nome"), Some(&Value::String("Davi".to_string())));
    dados.close().unwrap();
    fs::remove_file(path).unwrap();
}

#[test]
fn test_persistence() {
    let path = "test_persist_rust.rd";
    {
        let mut dados = Rodo::link(path).unwrap();
        dados.set("level", Value::Int(87)).unwrap();
        dados.close().unwrap();
    }
    {
        let dados = Rodo::link(path).unwrap();
        assert_eq!(dados.get("level"), Some(&Value::Int(87)));
    }
    fs::remove_file(path).unwrap();
}

#[test]
fn test_delete() {
    let path = "test_delete_rust.rd";
    let mut dados = Rodo::link(path).unwrap();
    dados.set("coins", Value::Int(1500)).unwrap();
    assert!(dados.delete("coins").unwrap());
    assert!(!dados.has("coins"));
    dados.close().unwrap();
    fs::remove_file(path).unwrap();
}