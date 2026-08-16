package com.rodo;

import org.junit.Test;
import java.io.File;
import static org.junit.Assert.*;

public class RodoTest {
    @Test
    public void testSetGet() throws Exception {
        String path = "test_java.rd";
        Rodo r = Rodo.link(path);
        Rodo.Value v = new Rodo.Value();
        v.type = Rodo.TYPE_STRING;
        v.stringVal = "Davi";
        r.set("nome", v);
        Rodo.Value got = r.get("nome");
        assertNotNull(got);
        assertEquals("Davi", got.stringVal);
        assertTrue(r.has("nome"));
        r.close();
        new File(path).delete();
    }

    @Test
    public void testPersistence() throws Exception {
        String path = "test_persist_java.rd";
        {
            Rodo r = Rodo.link(path);
            Rodo.Value v = new Rodo.Value();
            v.type = Rodo.TYPE_INT;
            v.intVal = 87;
            r.set("level", v);
            r.close();
        }
        {
            Rodo r = Rodo.link(path);
            Rodo.Value got = r.get("level");
            assertNotNull(got);
            assertEquals(87, got.intVal);
            r.close();
        }
        new File(path).delete();
    }

    @Test
    public void testDelete() throws Exception {
        String path = "test_delete_java.rd";
        Rodo r = Rodo.link(path);
        Rodo.Value v = new Rodo.Value();
        v.type = Rodo.TYPE_INT;
        v.intVal = 1500;
        r.set("coins", v);
        r.delete("coins");
        assertFalse(r.has("coins"));
        r.close();
        new File(path).delete();
    }
}