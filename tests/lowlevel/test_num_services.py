# coding: utf-8

import pytest as _pytest

import ipvs as _ipvs

pytestmark = _pytest.mark.usefixtures('ipvs')


def test_ipvs_num_services(service, dest):
    _ipvs.add_service(service)
    _ipvs.add_dest(service, dest)
    result = _ipvs.num_services()
    assert result == 1
