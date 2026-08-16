'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const rodo = require('../src/index.js');

const TEST_FILE = path.join(__dirname, 'test_data.rd');

function cleanUp() {
    try {
        if (fs.existsSync(TEST_FILE)) {
            fs.unlinkSync(TEST_FILE);
        }
    } catch (e) {
        // ignore
    }
}

function runTests() {
    cleanUp();

    // Test: link creates new file
    const dados = rodo.link(TEST_FILE);
    assert.ok(fs.existsSync(TEST_FILE), 'Arquivo .rd deve ser criado');

    // Test: set e get de string
    dados.set('nome', 'Davi');
    assert.strictEqual(dados.get('nome'), 'Davi', 'String deve ser recuperada corretamente');

    // Test: set e get de inteiro
    dados.set('level', 87);
    assert.strictEqual(dados.get('level'), 87, 'Inteiro deve ser recuperado corretamente');

    // Test: set e get de float
    dados.set('pi', 3.14159);
    assert.ok(Math.abs(dados.get('pi') - 3.14159) < 0.00001, 'Float deve ser recuperado corretamente');

    // Test: set e get de booleano
    dados.set('ativo', true);
    assert.strictEqual(dados.get('ativo'), true, 'Booleano deve ser recuperado corretamente');

    // Test: set e get de null
    dados.set('nulo', null);
    assert.strictEqual(dados.get('nulo'), null, 'Null deve ser recuperado corretamente');

    // Test: has
    assert.strictEqual(dados.has('nome'), true, 'has deve retornar true para chave existente');
    assert.strictEqual(dados.has('inexistente'), false, 'has deve retornar false para chave ausente');

    // Test: keys
    const keys = dados.keys();
    assert.ok(Array.isArray(keys), 'keys deve retornar um array');
    assert.ok(keys.includes('nome'), 'keys deve conter nome');
    assert.ok(keys.includes('level'), 'keys deve conter level');
    assert.ok(keys.includes('pi'), 'keys deve conter pi');
    assert.ok(keys.includes('ativo'), 'keys deve conter ativo');
    assert.ok(keys.includes('nulo'), 'keys deve conter nulo');

    // Test: all
    const all = dados.all();
    assert.strictEqual(all.nome, 'Davi', 'all deve conter nome');
    assert.strictEqual(all.level, 87, 'all deve conter level');

    // Test: delete
    dados.delete('level');
    assert.strictEqual(dados.has('level'), false, 'Chave deletada não deve existir');
    assert.strictEqual(dados.get('level'), null, 'get de chave deletada deve retornar null');

    // Fechar e reabrir para testar persistência
    dados.close();

    // Reabrir o arquivo
    const dados2 = rodo.link(TEST_FILE);

    // Dados devem persistir
    assert.strictEqual(dados2.get('nome'), 'Davi', 'Dado deve persistir após reabrir');
    assert.strictEqual(dados2.get('pi') > 3.14 && dados2.get('pi') < 3.15, true, 'Float deve persistir');
    assert.strictEqual(dados2.has('level'), false, 'Chave deletada não deve reaparecer');
    assert.strictEqual(dados2.get('level'), null, 'Chave deletada deve retornar null após reabrir');

    // Simular remoção de código: não chamar set novamente, apenas verificar se o dado ainda existe
    // (Não fazemos nada, apenas verificamos que 'nome' ainda existe)
    assert.strictEqual(dados2.get('nome'), 'Davi', 'Dado não deve ser apagado por falta de set');

    // Fechar
    dados2.close();

    cleanUp();
    console.log('Todos os testes JavaScript passaram!');
}

try {
    runTests();
} catch (error) {
    console.error('Teste falhou:', error);
    cleanUp();
    process.exit(1);
}