#!/usr/bin/python

import setuptools as _st

ipvs = _st.Extension(
    '_ipvs',
    sources=['libipvs.c', 'pyipvs.c'],
)

_st.setup(
    name='ipvs',
    version='0.0.1',
    description='Python bindings for the ipvs userspace-API',
    url='https://github.com/wontfix-org/python-ipvs/',
    author='Michael van Bracht',
    author_email='michael@wontfix.org',
    ext_modules=[ipvs],
    modules=['ipvs'],
    tests_require=[
        'pytest',
    ],
    setup_requires=[
        'setuptools',
    ],
)
