# coding: utf-8

import pytest as _pytest

import ipvs as _ipvs

from conftest import make_service


pytestmark = _pytest.mark.usefixtures('ipvs')


def test_ipvs_del_service(service):
    _ipvs.add_service(service)
    result = _ipvs.del_service(service)
    print result
    print _ipvs.get_services()
