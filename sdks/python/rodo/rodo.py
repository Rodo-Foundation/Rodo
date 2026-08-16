import rodo_native
from typing import Any, Optional, List, Dict

class Rodo:
    def __init__(self, path: str):
        self._handle = rodo_native.rodo_link(path.encode('utf-8'))
        if not self._handle:
            raise IOError(f"Não foi possível abrir/criar o arquivo: {path}")

    def set(self, key: str, value: Any) -> 'Rodo':
        key_bytes = key.encode('utf-8')
        converted = self._value_to_py(self._convert_value(value))
        result = rodo_native.rodo_set(self._handle, key_bytes, converted)
        if result != 0:
            raise ValueError(f"Falha ao definir valor para a chave: {key}")
        return self

    def get(self, key: str) -> Any:
        key_bytes = key.encode('utf-8')
        native_val = rodo_native.rodo_get(self._handle, key_bytes)
        if native_val is None:
            return None
        return self._convert_from_native(native_val)

    def has(self, key: str) -> bool:
        return rodo_native.rodo_has(self._handle, key.encode('utf-8')) == 1

    def delete(self, key: str) -> bool:
        return rodo_native.rodo_delete(self._handle, key.encode('utf-8')) == 0

    def keys(self) -> List[str]:
        raw_keys = rodo_native.rodo_keys(self._handle)
        if raw_keys is None:
            return []
        return [k.decode('utf-8') for k in raw_keys]

    def all(self) -> Dict[str, Any]:
        return {k: self.get(k) for k in self.keys()}

    def close(self) -> None:
        if self._handle:
            rodo_native.rodo_close(self._handle)
            self._handle = None

    # Helpers for value conversion
    @staticmethod
    def _convert_value(value: Any) -> Any:
        """Convert Python value to a tuple (type_code, value) that the C extension understands."""
        if value is None:
            return (0, None)
        if isinstance(value, bool):
            return (1, 1 if value else 0)
        if isinstance(value, int):
            return (2, value)
        if isinstance(value, float):
            return (3, value)
        if isinstance(value, str):
            return (4, value)
        if isinstance(value, bytes):
            return (5, value)
        if isinstance(value, list):
            return (6, [Rodo._convert_value(v) for v in value])
        if isinstance(value, dict):
            return (7, [(k, Rodo._convert_value(v)) for k, v in value.items()])
        # Date is not directly handled; could use timestamp
        # We'll just treat everything else as error
        raise TypeError(f"Tipo não suportado: {type(value)}")

    @staticmethod
    def _value_to_py(value: Any) -> Any:
        """Convert from the Python representation used in _convert_value to the format expected by the C extension.
        The C extension expects a specific structure (a PyObject with attributes).
        We'll define a simple protocol: the C extension receives a PyObject that is a tuple or list.
        In the C code, we'll parse it accordingly.
        """
        return value

    @staticmethod
    def _convert_from_native(value: Any) -> Any:
        """Convert native C extension return value to Python."""
        if value is None:
            return None
        if isinstance(value, dict):
            # Native returns a dict with keys: type, value
            t = value['type']
            v = value['value']
            if t == 0:
                return None
            elif t == 1:
                return bool(v)
            elif t == 2:
                return int(v)
            elif t == 3:
                return float(v)
            elif t == 4:
                return str(v)
            elif t == 5:
                return bytes(v)
            elif t == 6:
                return [Rodo._convert_from_native(item) for item in v]
            elif t == 7:
                return {k: Rodo._convert_from_native(val) for k, val in v}
            elif t == 8:
                # Date: return timestamp (ms)
                return int(v)
            elif t == 9:
                return str(v)  # UUID as hex string
        # Fallback
        return value


def link(path: str) -> Rodo:
    return Rodo(path)