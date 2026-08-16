package rodo

import (
	"testing"
	"os"
)

func TestSetGet(t *testing.T) {
	path := "test_go.rd"
	r, err := Link(path)
	if err != nil {
		t.Fatal(err)
	}
	defer os.Remove(path)

	val := Value{Type: TypeString, StringVal: "Davi"}
	if err := r.Set("nome", val); err != nil {
		t.Fatal(err)
	}
	got, ok := r.Get("nome")
	if !ok || got.StringVal != "Davi" {
		t.Fatal("get failed")
	}
	if !r.Has("nome") {
		t.Fatal("has failed")
	}
	if err := r.Close(); err != nil {
		t.Fatal(err)
	}
}

func TestPersistence(t *testing.T) {
	path := "test_persist_go.rd"
	{
		r, err := Link(path)
		if err != nil {
			t.Fatal(err)
		}
		r.Set("level", Value{Type: TypeInt, IntVal: 87})
		r.Close()
	}
	{
		r, err := Link(path)
		if err != nil {
			t.Fatal(err)
		}
		got, ok := r.Get("level")
		if !ok || got.IntVal != 87 {
			t.Fatal("data did not persist")
		}
		r.Close()
	}
	os.Remove(path)
}

func TestDelete(t *testing.T) {
	path := "test_delete_go.rd"
	r, _ := Link(path)
	r.Set("coins", Value{Type: TypeInt, IntVal: 1500})
	if err := r.Delete("coins"); err != nil {
		t.Fatal(err)
	}
	if r.Has("coins") {
		t.Fatal("key not deleted")
	}
	r.Close()
	os.Remove(path)
}