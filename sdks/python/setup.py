from setuptools import setup, Extension
import os

core_dir = os.path.join('..', '..', 'core', 'C')

sources = [
    'rodo/_native/rodo_bind.c',
    os.path.join(core_dir, 'src', 'rodo.c'),
    os.path.join(core_dir, 'src', 'rodo_format.c'),
    os.path.join(core_dir, 'src', 'rodo_varint.c'),
    os.path.join(core_dir, 'src', 'rodo_symbol.c'),
    os.path.join(core_dir, 'src', 'rodo_dict.c'),
    os.path.join(core_dir, 'src', 'rodo_platform.c'),
]

include_dirs = [
    os.path.join(core_dir, 'include'),
]

ext = Extension(
    'rodo_native',
    sources=sources,
    include_dirs=include_dirs,
    language='c',
    extra_compile_args=['-std=c99'],
)

setup(
    name='rodo-sdk',
    version='0.1.0',
    description='Rodo SDK para Python - formato universal de armazenamento compacto .rd',
    author='Equipe Rodo',
    license='MIT',
    packages=['rodo'],
    ext_modules=[ext],
)