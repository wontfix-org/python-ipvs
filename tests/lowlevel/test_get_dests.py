# coding: utf-8

import pytest as _pytest

import ipvs as _ipvs

pytestmark = _pytest.mark.usefixtures('ipvs')


def test_ipvs_get_dests(service, dest):
    result = _ipvs.add_service(service)
    assert 0 == _ipvs.add_dest(service, dest)
    result = _ipvs.get_dests(service)
    print result
