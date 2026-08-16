'use strict';

const binding = require('bindings')('rodo_binding');

function convertValue(value) {
    // Convert JS value to a representation the native binding understands
    // The native binding expects a specific structure.
    // For simplicity, we'll create a plain object with type and value.
    if (value === null || value === undefined) {
        return { type: 0 };
    }
    if (typeof value === 'boolean') {
        return { type: 1, boolVal: value ? 1 : 0 };
    }
    if (typeof value === 'number') {
        if (Number.isInteger(value) && Number.isSafeInteger(value)) {
            return { type: 2, intVal: value };
        }
        return { type: 3, floatVal: value };
    }
    if (typeof value === 'string') {
        return { type: 4, stringVal: value };
    }
    if (value instanceof Uint8Array) {
        return { type: 5, bytes: Array.from(value) };
    }
    if (Array.isArray(value)) {
        return { type: 6, array: value.map(convertValue) };
    }
    if (value instanceof Date) {
        return { type: 8, dateVal: value.getTime() };
    }
    // Treat as plain object map
    return {
        type: 7,
        map: Object.entries(value).map(([k, v]) => ({ key: k, value: convertValue(v) }))
    };
}

function convertFromNative(nativeValue) {
    if (!nativeValue) return null;
    switch (nativeValue.type) {
        case 0: return null;
        case 1: return nativeValue.boolVal === 1;
        case 2: return nativeValue.intVal;
        case 3: return nativeValue.floatVal;
        case 4: return nativeValue.stringVal;
        case 5: return new Uint8Array(nativeValue.bytes);
        case 6: return nativeValue.array.map(convertFromNative);
        case 7: {
            const obj = {};
            for (const pair of nativeValue.map) {
                obj[pair.key] = convertFromNative(pair.value);
            }
            return obj;
        }
        case 8: return new Date(nativeValue.dateVal);
        case 9: return nativeValue.uuid; // assume hex string
        default: return null;
    }
}

class Rodo {
    constructor(path) {
        this._handle = binding.rodoLink(path);
        if (!this._handle) {
            throw new Error('Failed to open or create .rd file: ' + path);
        }
    }

    set(key, value) {
        const converted = convertValue(value);
        const result = binding.rodoSet(this._handle, key, converted);
        if (result !== 0) {
            throw new Error('Failed to set value for key: ' + key);
        }
        return this;
    }

    get(key) {
        const nativeVal = binding.rodoGet(this._handle, key);
        return convertFromNative(nativeVal);
    }

    has(key) {
        return binding.rodoHas(this._handle, key) === 1;
    }

    delete(key) {
        return binding.rodoDelete(this._handle, key) === 0;
    }

    keys() {
        return binding.rodoKeys(this._handle);
    }

    all() {
        // Not yet implemented in native binding; can use keys + get
        const keys = this.keys();
        const result = {};
        for (const key of keys) {
            result[key] = this.get(key);
        }
        return result;
    }

    close() {
        if (this._handle) {
            binding.rodoClose(this._handle);
            this._handle = null;
        }
    }
}

function link(path) {
    return new Rodo(path);
}

module.exports = { link };