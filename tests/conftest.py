# coding: utf-8

import pytest as _pytest

import ipvs as _ipvs

_ipvs.init()

def make_service(**kwargs):
    defaults = {
        'port': 90,
        'address': '192.168.53.1',
        'protocol': 'TCP',
        'scheduler': 'wlc',
        'u_threshold': 10,
        'l_threshold': 20,
    }
    return dict(defaults, **kwargs)


def make_dest(**kwargs):
    defaults = {
        'address': '192.168.53.2',
        'port': 90,
        'weight': 25,
    }
    return dict(defaults, **kwargs)


@_pytest.yield_fixture(scope='session')
def ipvs_init():
    # Since the ipvs config is global to the system, we could even get
    # residue from our last aborted test run, we really need to make
    # sure to cleanup at least once before we start for consistent tests
    for svc in _ipvs.get_services():
        result = _ipvs.del_service(svc)
        if result != 0:
            print "COULD NOT REMOVE SERVICE %r" % (svc,)
    yield


@_pytest.yield_fixture
def ipvs(ipvs_init):
    yield
    for svc in _ipvs.get_services():
        result = _ipvs.del_service(svc)
        if result != 0:
            print "COULD NOT REMOVE SERVICE %r" % (svc,)


@_pytest.fixture
def service(ipvs):
    return make_service()


@_pytest.fixture
def dest(ipvs):
    return make_dest()
