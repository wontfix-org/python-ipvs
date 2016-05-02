# coding: utf-8

import pytest as _pytest

import ipvs as _ipvs

from conftest import make_service


pytestmark = _pytest.mark.usefixtures('ipvs')


def test_ipvs_get_services(service):
    _ipvs.add_service(service)
    result = _ipvs.get_services()
    assert result == (service,)
